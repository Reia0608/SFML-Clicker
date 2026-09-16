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

	for (const Achievement* achievement : achievementManager->getVisibleAchievements())
	{
		AchievementLine line;
		line.nameText = new Text(achievement->name, "Fonts/arial.ttf");
		line.descriptionText = new Text(achievement->description + " (Reward: " + achievement->rewardLabel + ")", "Fonts/arial.ttf");

		std::string name = achievement->name;
		line.claimButton = new UIButton("Claim " + achievement->rewardLabel, [this, name]() { achievementManager->claimReward(name); });
		lines.push_back(line);
	}
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
	const float margin = 10.0f;
	const float entryHeight = 44.0f;
	const float buttonWidth = 160.0f; // should it not be a variable to better adapt the buttons while resizing the window?
	const float buttonHeight = 30.0f; // same as above.

	contentHeight = margin + lines.size() * entryHeight; // total height of the unscrolled layout, so TabbedPanel can enable scrolling if it overflows

	const auto& achievements = achievementManager->getVisibleAchievements();

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
	hasUnseenUnlock = false;

	auto visible = achievementManager->getVisibleAchievements(); // NEW
	if (visible.size() != lines.size())
	{
		rebuildLines(); // a previously-hidden achievement just became visible — resync line count
	}

	refreshClaimButtons();

	for (size_t i = 0; i < lines.size() && i < visible.size(); i++)
	{
		const Achievement& achievement = *visible[i]; // CHANGED
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