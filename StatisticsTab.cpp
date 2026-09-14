#include "stdafx.h"
#include "StatisticsTab.h"

StatisticsTab::StatisticsTab(Hero* hero, Statistics* statistics) : hero(hero), statistics(statistics)
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

	std::ostringstream critChanceOut;
	critChanceOut << std::fixed << std::setprecision(2) << (hero->getCritChance() * 100.0);
	lines[8]->setString("Crit Chance: " + critChanceOut.str() + "%");

	std::ostringstream critDamageOut;
	critDamageOut << std::fixed << std::setprecision(0) << (hero->getCritDamageMultiplier() * 100.0);
	lines[9]->setString("Crit Damage: " + critDamageOut.str() + "%");
}

void StatisticsTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	const float margin = 10.0f;
	const float lineHeight = 24.0f;

	for (size_t i = 0; i < lines.size(); i++)
	{
		lines[i]->setPosition({ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + i * lineHeight });
	}
}

void StatisticsTab::render(sf::RenderWindow* window)
{
	refreshValues();   // values will only be changed if the tab is active/visible

	for (auto* line : lines)
	{
		window->draw(*line);
	}
}

bool StatisticsTab::handleClick(sf::Vector2f pos)
{
	return false;   // pure viewing tab, no active elements
}