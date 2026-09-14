#include "stdafx.h"
#include "AchievementsTab.h"


void AchievementsTab::rebuildLines()
{
	for (auto& line : lines)
	{
		delete line.nameText;
		delete line.descriptionText;
		delete line.claimButton;
	}
	lines.clear();

	for (const auto& achievement : achievementManager->getAchievements())
	{
		AchievementLine line;

		line.nameText = new Text(achievement.name, "Fonts/arial.ttf");
		line.nameText->setCharacterSize(16);
		line.nameText->setOutlineColor(sf::Color::Black);
		line.nameText->setOutlineThickness(1.0f);

		line.descriptionText = new Text(achievement.description + " (Reward: " + achievement.goldReward.toString() + " Gold)", "Fonts/arial.ttf");
		line.descriptionText->setCharacterSize(12);
		line.descriptionText->setFillColor(sf::Color(180, 180, 180));

		std::string name = achievement.name; // captured by value for the lambda below
		line.claimButton = new UIButton("Claim " + achievement.goldReward.toString() + " Gold", [this, name]() { achievementManager->claimReward(name); });

		lines.push_back(line);
	}
}

void AchievementsTab::refreshClaimButtons()
{
	const auto& achievements = achievementManager->getAchievements();
	for (size_t i = 0; i < lines.size() && i < achievements.size(); i++)
	{
		lines[i].claimButton->setEnabled(achievements[i].unlocked && !achievements[i].claimed); // hidden until earned, hidden again once claimed
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
	const float margin = 10.0f;
	const float entryHeight = 44.0f;
	const float buttonWidth = 160.0f; // should it not be a variable to better adapt the buttons while resizing the window?
	const float buttonHeight = 30.0f; // same as above.

	const auto& achievements = achievementManager->getAchievements();

	for (size_t i = 0; i < lines.size() && i < achievements.size(); i++)
	{
		float y = contentAreaOrigin.y + margin + i * entryHeight;
		lines[i].nameText->setPosition({ contentAreaOrigin.x + margin, y });
		lines[i].descriptionText->setPosition({ contentAreaOrigin.x + margin, y + 20.0f });

		lines[i].claimButton->setLayout( // anchored to the right edge of the content area
			{ contentAreaOrigin.x + contentAreaSize.x - margin - buttonWidth, y + 2.0f },
			{ buttonWidth, buttonHeight }
		);
	}
}

void AchievementsTab::render(sf::RenderWindow* window)
{
	hasUnseenUnlock = false; // this tab is only rendered while active, so being drawn means it's being seen

	refreshClaimButtons();

	const auto& achievements = achievementManager->getAchievements();

	for (size_t i = 0; i < lines.size() && i < achievements.size(); i++)
	{
		const Achievement& achievement = achievements[i];
		sf::Color color = achievement.claimed ? sf::Color(120, 220, 120) // green = claimed
			: achievement.unlocked ? sf::Color(230, 210, 120) // gold = unlocked, waiting to be claimed
			: sf::Color(150, 150, 150); // grey = locked

		lines[i].nameText->setFillColor(color);

		window->draw(*lines[i].nameText);
		window->draw(*lines[i].descriptionText);
		lines[i].claimButton->render(window); // no-op while hidden/disabled (see UIButton::render)
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