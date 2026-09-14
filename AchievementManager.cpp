#include "stdafx.h"
#include "AchievementManager.h"

void AchievementManager::initAchievements()
{
	Statistics* stats = statistics;   // copy of the pointer to use in lambdas for clarity

	achievements.push_back(Achievement("First Blood", "Defeat 1 enemy",
		[stats]() { return stats->getEnemiesDefeated() >= 1; }, BigNumber(10)));

	achievements.push_back(Achievement("Getting Started", "Defeat 10 enemies",
		[stats]() { return stats->getEnemiesDefeated() >= 10; }, BigNumber(25)));

	achievements.push_back(Achievement("Veteran Slayer", "Defeat 100 enemies",
		[stats]() { return stats->getEnemiesDefeated() >= 100; }, BigNumber(100)));

	achievements.push_back(Achievement("Boss Down", "Defeat your first boss",
		[stats]() { return stats->getBossesDefeated() >= 1; }, BigNumber(200)));

	achievements.push_back(Achievement("Boss Hunter", "Defeat 5 bosses",
		[stats]() { return stats->getBossesDefeated() >= 5; }, BigNumber(500)));

	achievements.push_back(Achievement("Stage Climber", "Reach Stage 5",
		[stats]() { return stats->getHighestStageReached() >= 5; }, BigNumber(300)));

	achievements.push_back(Achievement("Critical Thinker", "Land 100 critical hits",
		[stats]() { return stats->getCriticalHitsLanded() >= 100; }, BigNumber(150)));

	achievements.push_back(Achievement("Shopaholic", "Purchase 10 shop upgrades",
		[stats]() { return stats->getShopUpgradesPurchased() >= 10; }, BigNumber(150)));

	achievements.push_back(Achievement("Clicker", "Click 500 times",
		[stats]() { return stats->getTotalClicks() >= 500; }, BigNumber(75)));

	achievements.push_back(Achievement("Dedicated", "Play for 10 minutes",
		[stats]() { return stats->getPlayTimeSeconds() >= 600.0f; }, BigNumber(100)));
}

AchievementManager::AchievementManager(Hero* hero, Statistics* statistics) : hero(hero), statistics(statistics)
{
	initAchievements();
}

void AchievementManager::update()
{
	for (auto& achievement : achievements)
	{
		if (!achievement.unlocked && achievement.condition())
		{
			achievement.unlocked = true;
			hero->grantBonusGold(achievement.goldReward);
			if (onAchievementUnlocked)
			{
				onAchievementUnlocked(achievement);
			}
			// TODO: as soon as Items are implemented, ItemTab-Callback should be called here
			// TODO: as soon as Shop unlocks are implemented, ShopTab-Callback should be called here
		}
	}
}

const std::vector<Achievement>& AchievementManager::getAchievements() const
{
	return achievements;
}

void AchievementManager::setUnlocked(const std::string& name, bool unlocked)
{
	for (auto& achievement : achievements)
	{
		if (achievement.name == name)
		{
			achievement.unlocked = unlocked; // no gold granted here, currency is restored separately from the save file
			return;
		}
	}
}

bool AchievementManager::claimReward(const std::string& name)
{
	for (auto& achievement : achievements)
	{
		if (achievement.name == name)
		{
			if (!achievement.unlocked || achievement.claimed) return false; // not achieved yet, or already collected

			achievement.claimed = true;

			hero->grantBonusGold(achievement.goldReward);
			// TODO: as soon as Items are implemented, ItemTab-Callback should be called here
			// TODO: as soon as Shop unlocks are implemented, ShopTab-Callback should be called here
			return true;
		}
	}
	return false;
}

void AchievementManager::setClaimed(const std::string& name, bool claimed) // restores saved state without granting gold again
{
	for (auto& achievement : achievements)
	{
		if (achievement.name == name)
		{
			achievement.claimed = claimed;
			return;
		}
	}
}


