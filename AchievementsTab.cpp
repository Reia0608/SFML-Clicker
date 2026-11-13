#include "stdafx.h"
#include "AchievementsTab.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include <sstream>
#include <algorithm>

namespace
{
	// 0 = can be claimed right now (top group), 1 = not yet reached (middle, the default for every
	// achievement), 2 = already claimed (bottom group) - buildGroupedAchievements() stable_sorts by
	// this, so achievements only ever move group on the two events that change unlocked/claimed:
	// claiming a reward, or a background auto-unlock reaching its condition.
	int achievementGroupRank(const Achievement& achievement)
	{
		if (achievement.unlocked && !achievement.claimed) return 0;
		if (!achievement.unlocked) return 1;
		return 2;
	}

	// The smallest rectangle containing every non-fully-transparent pixel in `image`. The group
	// header icons (padlock_locked/unlocked.png, success_check.png) ship on a padded 64x64 canvas
	// with the actual artwork only filling the center ~75-80% of it - scaling a sprite by the FULL
	// texture size would make the icon look noticeably smaller next to the header text than it
	// should. Cropping to this box instead means "icon height" means the actual drawn shape, same
	// as how a letter's own visible height is what you compare it to, not its font's full em-box.
	sf::IntRect opaqueBounds(const sf::Image& image)
	{
		sf::Vector2u size = image.getSize();
		int minX = static_cast<int>(size.x), minY = static_cast<int>(size.y), maxX = -1, maxY = -1;
		for (unsigned int y = 0; y < size.y; y++)
		{
			for (unsigned int x = 0; x < size.x; x++)
			{
				if (image.getPixel({ x, y }).a == 0) continue;
				minX = std::min(minX, static_cast<int>(x));
				minY = std::min(minY, static_cast<int>(y));
				maxX = std::max(maxX, static_cast<int>(x));
				maxY = std::max(maxY, static_cast<int>(y));
			}
		}

		if (maxX < minX) return sf::IntRect({ 0, 0 }, { static_cast<int>(size.x), static_cast<int>(size.y) }); // fully transparent image - fall back to the whole canvas rather than an empty rect
		return sf::IntRect({ minX, minY }, { maxX - minX + 1, maxY - minY + 1 });
	}

	// Same 3 colors render() already used per-achievement-row, factored out so a group's header and
	// every achievement name under it agree on what each state looks like.
	sf::Color achievementGroupColor(int rank)
	{
		switch (rank)
		{
		case 0: return sf::Color(230, 210, 120); // Unclaimed
		case 2: return sf::Color(120, 220, 120); // Claimed
		default: return sf::Color(150, 150, 150); // Locked
		}
	}

	// Reflows `str` into one or more lines (joined with '\n') so that none of them render wider
	// than maxWidth in `text`'s current font/character size. Measures by actually setting
	// candidate lines on `text` and reading back getLocalBounds() - simplest way to get an exact
	// answer without duplicating SFML's own glyph-advance math. Splitting on whitespace via the
	// stream extraction also treats any existing '\n' in `str` as a space, so calling this again
	// on an already-wrapped string (e.g. after a resize) just reflows it from scratch - safe to
	// call every relayout, not just once.
	//
	// maxWidth <= 0 (only happens on the very first layout, before this tab has ever been told its
	// real content area) returns `str` unchanged rather than wrapping against a nonsense width;
	// the startup onResize() that always follows calls relayoutLines() again with the real size.
	std::string wrapTextToWidth(Text* text, const std::string& str, float maxWidth)
	{
		if (maxWidth <= 0.0f) return str;

		std::istringstream words(str);
		std::string word;
		std::string result;
		std::string currentLine;

		while (words >> word)
		{
			std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
			text->setString(candidate);
			if (text->getLocalBounds().size.x > maxWidth && !currentLine.empty())
			{
				result += currentLine + "\n";
				currentLine = word;
			}
			else
			{
				currentLine = candidate;
			}
		}
		result += currentLine;
		return result;
	}
}

std::vector<const Achievement*> AchievementsTab::buildGroupedAchievements() const
{
	std::vector<const Achievement*> grouped = achievementManager->getVisibleAchievements();
	std::stable_sort(grouped.begin(), grouped.end(), [](const Achievement* a, const Achievement* b)
	{
		return achievementGroupRank(*a) < achievementGroupRank(*b);
	});
	return grouped;
}

void AchievementsTab::rebuildLines()
{
	for (auto& line : lines)
	{
		delete line.nameText;
		delete line.descriptionText;
		delete line.claimButton;
	}
	lines.clear();

	lastBuiltOrder = buildGroupedAchievements();

	for (const Achievement* achievement : lastBuiltOrder)
	{
		AchievementLine line;
		line.nameText = new Text(achievement->name, "Fonts/arial.ttf");
		line.nameText->setCharacterSize(tabBodyTextSize + tabTitleTextSizeBonus);

		line.rawDescription = achievementManager->describeWithLiveNumbers(*achievement) + " (Reward: " + achievement->rewardLabel + ")";
		line.descriptionText = new Text(line.rawDescription, "Fonts/arial.ttf");
		line.descriptionText->setCharacterSize(tabBodyTextSize);

		std::string name = achievement->name;
		line.claimButton = new UIButton("Claim " + achievement->rewardLabel, [this, name]() { achievementManager->claimReward(name); });
		lines.push_back(line);
	}

	// Every new line above starts with no position at all - only relayoutLines() sets that. The
	// very first call to rebuildLines() (from the constructor) runs before this tab has ever been
	// told its content area, so this uses whatever lastContentAreaOrigin/Size defaults to - that's
	// fine, the normal startup onResize() call that follows fixes it up, same as before this
	// method existed. What's NOT fine without this call is a LATER rebuild - triggered from
	// render() below, once a previously-hidden achievement becomes visible - which used to leave
	// every line (the whole tab, not just the new entry) sitting at that same stale default
	// position with no further onResize() ever coming to correct it.
	lastWrapWidth = -1.0f; // force the relayoutLines() call below to actually (re)wrap every line, including the brand new ones above - the width/font size itself probably hasn't changed, but their text has never been wrapped yet
	lastWrapFontSize = -1;
	relayoutLines();
}

void AchievementsTab::refreshClaimButtons()
{
	// Indexed by lastBuiltOrder, NOT achievementManager->getVisibleAchievements() - lines[i] was
	// built from lastBuiltOrder[i], which is the grouped (Unclaimed/Locked/Claimed) order, not the
	// manager's own order. Using the manager's order here would enable the wrong line's button
	// the moment grouping actually reorders anything.
	for (size_t i = 0; i < lines.size() && i < lastBuiltOrder.size(); i++)
	{
		lines[i].claimButton->setEnabled(lastBuiltOrder[i]->unlocked && !lastBuiltOrder[i]->claimed);
	}
}

AchievementsTab::AchievementsTab(AchievementManager* achievementManager) : achievementManager(achievementManager)
{
	const std::string groupTitleLabels[3] = { "Unclaimed", "Locked", "Claimed" };
	const std::string groupIconPaths[3] =
	{
		"Textures/UI/padlock_unlocked.png", // rank 0 (Unclaimed) - can be claimed right now
		"Textures/UI/padlock_locked.png",   // rank 1 (Locked) - not yet reached
		"Textures/UI/success_check.png"     // rank 2 (Claimed) - reward collected
	};

	for (int rank = 0; rank < 3; rank++)
	{
		groupTitleTexts[rank] = new Text(groupTitleLabels[rank], "Fonts/arial.ttf");
		groupTitleTexts[rank]->setStyle(sf::Text::Bold);
		groupTitleTexts[rank]->setFillColor(achievementGroupColor(rank));

		sf::Image iconImage;
		if (!iconImage.loadFromFile(groupIconPaths[rank]))
		{
			std::cerr << "ERROR::ACHIEVEMENTSTAB::COULD NOT LOAD GROUP ICON: " << groupIconPaths[rank] << std::endl;
		}

		groupIconTextures[rank] = new sf::Texture();
		if (!groupIconTextures[rank]->loadFromImage(iconImage))
		{
			std::cerr << "ERROR::ACHIEVEMENTSTAB::COULD NOT CREATE GROUP ICON TEXTURE: " << groupIconPaths[rank] << std::endl;
		}
		groupIconTextures[rank]->setSmooth(false); // keeps the pixel-art icon crisp instead of blurring it when scaled down

		groupIconSprites[rank] = new sf::Sprite(*groupIconTextures[rank]);
		groupIconSprites[rank]->setTextureRect(opaqueBounds(iconImage)); // crop out the transparent padding - see opaqueBounds()'s own comment
	}

	rebuildLines();
}

AchievementsTab::~AchievementsTab()
{
	for (auto& line : lines)
	{
		delete line.nameText;
		delete line.descriptionText;
		delete line.claimButton;
	}

	for (int rank = 0; rank < 3; rank++)
	{
		delete groupTitleTexts[rank];
		delete groupIconSprites[rank];
		delete groupIconTextures[rank];
	}
}

void AchievementsTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;
	relayoutLines();
}

void AchievementsTab::relayoutLines()
{
	const float margin = 10.0f;
	const float nameToDescriptionGap = 20.0f; // vertical offset from the name line down to the description's first line - independent of how many lines the description wraps to
	const float entryBottomPadding = 10.0f; // vertical gap after a description's last wrapped line, before the next entry starts
	const float buttonWidth = 160.0f; // should it not be a variable to better adapt the buttons while resizing the window?
	const float buttonHeight = 30.0f; // same as above.
	const float descriptionToButtonGap = 10.0f; // keeps wrapped description text from running into the claim button's column
	const float groupTitleGapAbove = 20.0f; // extra breathing room before a NEW group's header - skipped before the very first header, which already gets `margin`
	const float groupTitleGapBelow = 14.0f; // space between a group's header and the name line of its first achievement
	const float groupIconGap = 8.0f; // space between a group's icon and its header text

	// Width available for the description before it would run into the claim button column.
	// Only <= 0 on the very first rebuildLines() call from the constructor (see wrapTextToWidth).
	const float descriptionWidth = lastContentAreaSize.x - margin * 2.0f - buttonWidth - descriptionToButtonGap;

	// TabbedPanel re-lays-out the active tab every single FRAME while it's scrolled (see
	// TabbedPanel::render()), not just on an actual resize - so without this guard,
	// wrapTextToWidth()'s per-word setString()/getLocalBounds() measuring loop below re-ran for
	// every achievement, every frame, purely to reposition text that was already correctly
	// wrapped. That's what was tanking the framerate on this tab specifically (confirmed via
	// FrameProfiler, 2026-11-04) - only actually re-wrap when the available width or the
	// tabBodyTextSize setting (Options tab) changed.
	bool needsRewrap = (descriptionWidth != lastWrapWidth) || (tabBodyTextSize != lastWrapFontSize);
	lastWrapWidth = descriptionWidth;
	lastWrapFontSize = tabBodyTextSize;

	groupTitleVisible = { false, false, false };

	float y = lastContentAreaOrigin.y + margin;
	int previousRank = -1;
	for (size_t i = 0; i < lines.size() && i < lastBuiltOrder.size(); i++)
	{
		int rank = achievementGroupRank(*lastBuiltOrder[i]);
		if (rank != previousRank)
		{
			if (previousRank != -1) y += groupTitleGapAbove; // not before the very first header - `margin` above already covers that gap

			Text* title = groupTitleTexts[rank];
			// Noticeably bigger than an achievement's own name text (tabTitleTextSizeBonus * 2, not
			// * 1) plus Bold (set once in the constructor - style, unlike size, never needs redoing
			// here) so a group header reads as its own tier, not just another entry in the list.
			if (needsRewrap) title->setCharacterSize(tabBodyTextSize + tabTitleTextSizeBonus * 2);

			// Sized off the font's actual character size, NOT title->getLocalBounds() - ink bounds
			// only cover the glyphs actually present, so an all-caps-height string with no
			// descenders (e.g. "Unclaimed" has no g/p/y/j/q) measures far shorter than the text
			// visually reads, which is what made the icon end up tiny.
			float rowHeight = static_cast<float>(title->getCharacterSize());

			sf::Sprite* icon = groupIconSprites[rank];
			sf::Vector2i iconFrameSize = icon->getTextureRect().size; // the cropped (opaque-only) art size set in the constructor, not the padded full texture - same idea UIButton::layoutIcon() already uses for a cropped sprite-sheet frame
			float iconScale = (iconFrameSize.y > 0) ? (rowHeight / static_cast<float>(iconFrameSize.y)) : 1.0f;
			icon->setScale({ iconScale, iconScale });

			// Both anchored MiddleLeft within the same zero-width/rowHeight-tall box, so their
			// visual centers line up exactly regardless of the icon's own aspect ratio or the
			// text's ascender/descender quirks - same anchor-based centering UIButton/TabbedPanel
			// already use elsewhere, just applied to a label pair instead of inside one button.
			sf::Vector2f rowOrigin = { lastContentAreaOrigin.x + margin, y };
			LayoutManager::applyAnchor(*icon, Anchor::MiddleLeft, { 0.0f, 0.0f }, rowOrigin, { 0.0f, rowHeight });
			LayoutManager::applyAnchor(*title, Anchor::MiddleLeft, { iconFrameSize.x * iconScale + groupIconGap, 0.0f }, rowOrigin, { 0.0f, rowHeight });
			groupTitleVisible[rank] = true;

			y += rowHeight + groupTitleGapBelow;
			previousRank = rank;
		}

		if (needsRewrap)
		{
			lines[i].nameText->setCharacterSize(tabBodyTextSize + tabTitleTextSizeBonus);
			lines[i].descriptionText->setCharacterSize(tabBodyTextSize); // must be set before wrapTextToWidth measures it below
			lines[i].descriptionText->setString(wrapTextToWidth(lines[i].descriptionText, lines[i].rawDescription, descriptionWidth));
		}

		lines[i].nameText->setPosition({ lastContentAreaOrigin.x + margin, y });
		lines[i].descriptionText->setPosition({ lastContentAreaOrigin.x + margin, y + nameToDescriptionGap });

		lines[i].claimButton->setLayout( // anchored to the right edge of the content area, level with the name line
			{ lastContentAreaOrigin.x + lastContentAreaSize.x - margin - buttonWidth, y + 2.0f },
			{ buttonWidth, buttonHeight }
		);

		float descriptionHeight = lines[i].descriptionText->getLocalBounds().size.y;
		y += nameToDescriptionGap + descriptionHeight + entryBottomPadding; // next entry starts right after however tall this one turned out to be
	}

	contentHeight = y - lastContentAreaOrigin.y; // total height of the unscrolled layout, so TabbedPanel can enable scrolling if it overflows
}

void AchievementsTab::render(sf::RenderWindow* window)
{
	hasUnseenUnlock = false;

	std::vector<const Achievement*> grouped = buildGroupedAchievements();
	if (grouped != lastBuiltOrder) // a previously-hidden achievement just became visible, OR claiming one / reaching one moved it into a different group - either way `lines` needs rebuilding to match
	{
		rebuildLines();
	}

	refreshClaimButtons();

	for (int rank = 0; rank < 3; rank++)
	{
		if (!groupTitleVisible[rank]) continue;
		window->draw(*groupIconSprites[rank]);
		window->draw(*groupTitleTexts[rank]);
	}

	for (size_t i = 0; i < lines.size() && i < lastBuiltOrder.size(); i++)
	{
		const Achievement& achievement = *lastBuiltOrder[i];
		lines[i].nameText->setFillColor(achievementGroupColor(achievementGroupRank(achievement)));
		window->draw(*lines[i].nameText);
		window->draw(*lines[i].descriptionText);
		lines[i].claimButton->render(window);
	}
}

bool AchievementsTab::handleClick(sf::Vector2f pos)
{
	for (auto& line : lines) // routes clicks to the claim buttons.
	{
		if (line.claimButton->handleClick(pos)) return true;
	}

	return false;   // pure viewing tab, no click to unlock necessary (Auto-Unlock)
}
