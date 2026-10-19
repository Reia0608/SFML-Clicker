#include "stdafx.h"
#include "AchievementsTab.h"
#include <sstream>

namespace
{
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

void AchievementsTab::rebuildLines()
{
	for (auto& line : lines)
	{
		delete line.nameText;
		delete line.descriptionText;
		delete line.claimButton;
	}
	lines.clear();

	for (const Achievement* achievement : achievementManager->getVisibleAchievements())
	{
		AchievementLine line;
		line.nameText = new Text(achievement->name, "Fonts/arial.ttf");
		line.nameText->setCharacterSize(14);

		line.rawDescription = achievement->description + " (Reward: " + achievement->rewardLabel + ")";
		line.descriptionText = new Text(line.rawDescription, "Fonts/arial.ttf");
		line.descriptionText->setCharacterSize(14);

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
	relayoutLines();
}

void AchievementsTab::refreshClaimButtons()
{
	auto visible = achievementManager->getVisibleAchievements();
	for (size_t i = 0; i < lines.size() && i < visible.size(); i++)
	{
		lines[i].claimButton->setEnabled(visible[i]->unlocked && !visible[i]->claimed);
	}
}

AchievementsTab::AchievementsTab(AchievementManager* achievementManager) : achievementManager(achievementManager)
{
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

	// Width available for the description before it would run into the claim button column.
	// Only <= 0 on the very first rebuildLines() call from the constructor (see wrapTextToWidth).
	const float descriptionWidth = lastContentAreaSize.x - margin * 2.0f - buttonWidth - descriptionToButtonGap;

	const auto& achievements = achievementManager->getVisibleAchievements();

	float y = lastContentAreaOrigin.y + margin;
	for (size_t i = 0; i < lines.size() && i < achievements.size(); i++)
	{
		lines[i].descriptionText->setString(wrapTextToWidth(lines[i].descriptionText, lines[i].rawDescription, descriptionWidth));

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

	auto visible = achievementManager->getVisibleAchievements();
	if (visible.size() != lines.size())
	{
		rebuildLines(); // a previously-hidden achievement just became visible resync line count
	}

	refreshClaimButtons();

	for (size_t i = 0; i < lines.size() && i < visible.size(); i++)
	{
		const Achievement& achievement = *visible[i];
		sf::Color color = achievement.claimed ? sf::Color(120, 220, 120)
			: achievement.unlocked ? sf::Color(230, 210, 120)
			: sf::Color(150, 150, 150);

		lines[i].nameText->setFillColor(color);
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
