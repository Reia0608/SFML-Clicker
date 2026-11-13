#include "stdafx.h"
#include "StatisticsTab.h"
#include "Familiar.h"
#include "PublicVariables.h" // for Age

namespace
{
	std::string ageToDisplayString(Age age)
	{
		switch (age)
		{
			case Baby: return "Baby";
			case Juvenile: return "Juvenile";
			case Adult: return "Adult";
			case Ancient: return "Ancient";
		}
		return "Unknown";
	}

	// True once `familiar`'s CURRENT stage's passive both deals damage (e.g. Fire Dragon's Burn -
	// see FamiliarPassive::dealsDamage) and has actually been purchased (passive.unlockFeatureKey).
	// A passive that only modifies incoming/outgoing damage (Soaked, Keen Gale) never qualifies,
	// and a damaging one that hasn't been bought yet doesn't either.
	bool hasActiveDamagingPassive(Familiar* familiar, UnlockManager* unlockManager)
	{
		const FamiliarPassive& passive = familiar->getPassive();
		return passive.dealsDamage && unlockManager->isUnlocked(passive.unlockFeatureKey);
	}
}

StatisticsTab::StatisticsTab(Hero* hero, Statistics* statistics, UnlockManager* unlockManager, std::vector<Familiar*> familiars)
	: hero(hero), statistics(statistics), unlockManager(unlockManager), familiars(std::move(familiars))
{
	const int lineCount = 10;
	for (int i = 0; i < lineCount; i++)
	{
		Text* line = new Text("", "Fonts/arial.ttf");
		line->setFillColor(sf::Color::White);
		line->setCharacterSize(14);
		lines.push_back(line);
	}
}

StatisticsTab::~StatisticsTab()
{
	for (auto* line : lines)
	{
		delete line;
	}

	for (auto& block : familiarBlocks)
	{
		delete block.headerText;
		delete block.physicalText;
		delete block.elementalText;
		delete block.dpsText;
		delete block.passiveText; // safe no-op if nullptr
	}
}

void StatisticsTab::refreshValues()
{
	lines[0]->setString("Attack Power: " + hero->getAttackPower().toString());
	lines[1]->setString("Enemies Defeated: " + std::to_string(statistics->getEnemiesDefeated()));
	lines[2]->setString("Bosses Defeated: " + std::to_string(statistics->getBossesDefeated()));
	lines[3]->setString("Highest Stage: " + std::to_string(statistics->getHighestStageReached()));
	lines[4]->setString("Upgrades Purchased: " + std::to_string(statistics->getShopUpgradesPurchased()));

	std::ostringstream multiplierOut;
	multiplierOut << std::fixed << std::setprecision(2) << hero->getGoldMultiplier();
	lines[5]->setString("Gold Multiplier: x" + multiplierOut.str());

	lines[6]->setString("Total Clicks: " + std::to_string(statistics->getTotalClicks()));
	lines[7]->setString("Play Time: " + statistics->getFormattedPlayTime());

	double critChanceBonus = getCritChanceBonus ? getCritChanceBonus() : 0.0;
	double critDamageBonus = getCritDamageBonus ? getCritDamageBonus() : 0.0;

	std::ostringstream critChanceOut;
	critChanceOut << std::fixed << std::setprecision(2) << ((hero->getCritChance() + critChanceBonus) * 100.0);
	lines[8]->setString("Crit Chance: " + critChanceOut.str() + "%");

	std::ostringstream critDamageOut;
	critDamageOut << std::fixed << std::setprecision(0) << ((hero->getCritDamageMultiplier() + critDamageBonus) * 100.0);
	lines[9]->setString("Crit Damage: " + critDamageOut.str() + "%");
}

void StatisticsTab::refreshFamiliarBlocks()
{
	std::vector<Familiar*> currentlyUnlocked;
	std::vector<bool> currentlyHasPassive;
	for (Familiar* f : familiars)
	{
		if (!f->isUnlocked()) continue;
		currentlyUnlocked.push_back(f);
		currentlyHasPassive.push_back(hasActiveDamagingPassive(f, unlockManager));
	}

	bool unchanged = currentlyUnlocked.size() == familiarBlocks.size();
	if (unchanged)
	{
		for (size_t i = 0; i < familiarBlocks.size(); i++)
		{
			bool blockHasPassive = familiarBlocks[i].passiveText != nullptr;
			if (familiarBlocks[i].familiar != currentlyUnlocked[i] || blockHasPassive != currentlyHasPassive[i])
			{
				unchanged = false;
				break;
			}
		}
	}
	if (unchanged) return; // nothing newly unlocked, and no passive line needs to appear/disappear

	for (auto& block : familiarBlocks)
	{
		delete block.headerText;
		delete block.physicalText;
		delete block.elementalText;
		delete block.dpsText;
		delete block.passiveText;
	}
	familiarBlocks.clear();

	for (size_t i = 0; i < currentlyUnlocked.size(); i++)
	{
		FamiliarStatBlock block;
		block.familiar = currentlyUnlocked[i];

		block.headerText = new Text("", "Fonts/arial.ttf");
		block.headerText->setCharacterSize(14);
		block.headerText->setFillColor(sf::Color(230, 210, 120)); // same gold AchievementsTab uses for an unlocked entry

		block.physicalText = new Text("", "Fonts/arial.ttf");
		block.physicalText->setCharacterSize(14);
		block.physicalText->setFillColor(sf::Color::White);

		block.elementalText = new Text("", "Fonts/arial.ttf");
		block.elementalText->setCharacterSize(14);
		block.elementalText->setFillColor(sf::Color::White);

		block.dpsText = new Text("", "Fonts/arial.ttf");
		block.dpsText->setCharacterSize(14);
		block.dpsText->setFillColor(sf::Color::White);

		if (currentlyHasPassive[i])
		{
			block.passiveText = new Text("", "Fonts/arial.ttf");
			block.passiveText->setCharacterSize(14);
			block.passiveText->setFillColor(sf::Color::White);
		}
		else
		{
			block.passiveText = nullptr;
		}

		familiarBlocks.push_back(block);
	}

	relayoutFamiliarBlocks(true); // force - these blocks' Text objects are brand new and have never had a real size set (see their setCharacterSize(14) placeholders above)
}

void StatisticsTab::relayoutFamiliarBlocks(bool forceFontSize)
{
	const float margin = 10.0f;
	const float fixedLineHeight = 24.0f; // matches onResize()'s own spacing for `lines` above
	const float statLineHeight = 20.0f;
	const float blockGap = 12.0f; // extra space between one familiar's block and the next
	const float indent = 16.0f;

	// TabbedPanel re-lays-out the active tab every single FRAME while it's scrolled (see
	// TabbedPanel::render()), not just on an actual resize - setCharacterSize() isn't free (it
	// forces SFML to rebuild that text's glyph layout), so skip re-setting it except when the
	// caller (onResize()) says tabBodyTextSize actually changed, or these blocks were just
	// (re)built and have never had their real size applied (see refreshFamiliarBlocks() below).
	bool applyFontSize = forceFontSize;

	float y = lastContentAreaOrigin.y + margin + lines.size() * fixedLineHeight + blockGap;

	for (auto& block : familiarBlocks)
	{
		if (applyFontSize) block.headerText->setCharacterSize(tabBodyTextSize + tabTitleTextSizeBonus); // this familiar's name/label line reads as a header, same as AchievementsTab's name line
		block.headerText->setPosition({ lastContentAreaOrigin.x + margin, y });
		y += statLineHeight;

		if (applyFontSize) block.physicalText->setCharacterSize(tabBodyTextSize);
		block.physicalText->setPosition({ lastContentAreaOrigin.x + margin + indent, y });
		y += statLineHeight;

		if (applyFontSize) block.elementalText->setCharacterSize(tabBodyTextSize);
		block.elementalText->setPosition({ lastContentAreaOrigin.x + margin + indent, y });
		y += statLineHeight;

		if (applyFontSize) block.dpsText->setCharacterSize(tabBodyTextSize);
		block.dpsText->setPosition({ lastContentAreaOrigin.x + margin + indent, y });
		y += statLineHeight;

		if (block.passiveText)
		{
			if (applyFontSize) block.passiveText->setCharacterSize(tabBodyTextSize);
			block.passiveText->setPosition({ lastContentAreaOrigin.x + margin + indent, y });
			y += statLineHeight;
		}

		y += blockGap;
	}

	contentHeight = y - lastContentAreaOrigin.y;
}

void StatisticsTab::refreshFamiliarValues()
{
	for (auto& block : familiarBlocks)
	{
		Familiar* f = block.familiar;
		block.headerText->setString(f->getLabel() + " (" + ageToDisplayString(f->getAge()) + ")");
		block.physicalText->setString("Physical Damage: " + f->getCumulativePhysicalDamage().toString());
		block.elementalText->setString("Elemental Damage: " + f->getCumulativeElementalDamage().toString());
		block.dpsText->setString("Current DPS: " + f->getFamiliarDPS().toString());
		if (block.passiveText)
		{
			block.passiveText->setString("Passive Damage: " + f->getCumulativePassiveDamage().toString());
		}
	}
}

void StatisticsTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;

	const float margin = 10.0f;
	const float lineHeight = 24.0f;

	// See relayoutFamiliarBlocks()'s own comment - TabbedPanel re-lays-out this tab every frame
	// while it's scrolled, not just on an actual resize, so only re-apply setCharacterSize() (not
	// free) when the tabBodyTextSize setting (Options tab) actually changed.
	bool applyFontSize = (tabBodyTextSize != lastAppliedFontSize);
	lastAppliedFontSize = tabBodyTextSize;

	for (size_t i = 0; i < lines.size(); i++)
	{
		if (applyFontSize) lines[i]->setCharacterSize(tabBodyTextSize);
		lines[i]->setPosition({ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + i * lineHeight });
	}

	relayoutFamiliarBlocks(applyFontSize);
}

void StatisticsTab::render(sf::RenderWindow* window)
{
	hasUnseenUnlock = false; // being viewed right now - stop pulsing, same as AchievementsTab::render()

	refreshValues();         // values will only be changed if the tab is active/visible
	refreshFamiliarBlocks(); // resync the block list if a familiar/passive just (un)locked
	refreshFamiliarValues();

	for (auto* line : lines)
	{
		window->draw(*line);
	}

	for (auto& block : familiarBlocks)
	{
		window->draw(*block.headerText);
		window->draw(*block.physicalText);
		window->draw(*block.elementalText);
		window->draw(*block.dpsText);
		if (block.passiveText) window->draw(*block.passiveText);
	}
}

bool StatisticsTab::handleClick(sf::Vector2f pos)
{
	return false;   // pure viewing tab, no active elements
}
