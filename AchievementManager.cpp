#include "stdafx.h"
#include "AchievementManager.h"

void AchievementManager::initAchievements()
{
	Statistics* stats = statistics;   // copy of the pointer to use in lambdas for clarity
	Familiar* fam = familiar;         // same pattern, for clarity in the lambda below
	ItemManager* items = itemManager;

	// --- existing achievements stay exactly as they are ---
	achievements.push_back(Achievement("First Blood", "Defeat 1 enemy",
		[stats]() { return stats->getEnemiesDefeated() >= 1; }, BigNumber(100)));

	// ... (rest of your existing block unchanged) ...

	// --- new tiered achievements ---
	addThresholdAchievement("Slayer Legend", "Defeat 1000 enemies",
		[stats]() { return stats->getEnemiesDefeated(); }, 1000, BigNumber(1000000));

	addThresholdAchievement("Nightmare", "Defeat 25 bosses",
		[stats]() { return stats->getBossesDefeated(); }, 25, BigNumber(5000));

	addThresholdAchievement("Stage Conqueror", "Reach Stage 10",
		[stats]() { return stats->getHighestStageReached(); }, 10, BigNumber(2000));

	addThresholdAchievement("Stage Master", "Reach Stage 20",
		[stats]() { return stats->getHighestStageReached(); }, 20, BigNumber(20000));

	addThresholdAchievement("Sharpshooter", "Land 1000 critical hits",
		[stats]() { return stats->getCriticalHitsLanded(); }, 1000, BigNumber(2000000));

	addThresholdAchievement("Big Spender", "Purchase 50 shop upgrades",
		[stats]() { return stats->getShopUpgradesPurchased(); }, 50, BigNumber(3000));

	addThresholdAchievement("Click Master", "Click 5000 times",
		[stats]() { return stats->getTotalClicks(); }, 5000, BigNumber(100000));

	addThresholdAchievement("Marathon Runner", "Play for 1 hour",
		[stats]() { return static_cast<int>(stats->getPlayTimeSeconds()); }, 3600, BigNumber(500000));

	addThresholdAchievement("Endurance", "Play for 5 hours",
		[stats]() { return static_cast<int>(stats->getPlayTimeSeconds()); }, 18000, BigNumber(10000000));

	achievements.push_back(Achievement("Gold Rush", "Earn 100,000 total gold",
		[stats]() { return stats->getTotalGoldEarned() >= BigNumber(100000); }, BigNumber(50000)));

	achievements.push_back(Achievement("Familiar Bond", "Summon your first familiar",
		[fam]() { return fam->isUnlocked(); }, BigNumber(2000)));

	achievements.push_back(Achievement("Speedrunner", "Defeat a boss in under 5 seconds",
		[stats]() {
			float t = stats->getFastestBossKillSeconds();
			return t >= 0.0f && t <= 5.0f;
		}, BigNumber(10000)));

	achievements.push_back(Achievement("Collector", "Collect your first item",
		[items]() {
			int total = 0;
			for (int i = 0; i < items->getItemCount(); i++) total += items->getInventoryCount(i);
			return total >= 1;
		}, BigNumber(300)));

	achievements.push_back(Achievement("Hoarder", "Collect 3 items",
		[items]() 
		{
			int total = 0;
			for (int i = 0; i < items->getItemCount(); i++) total += items->getInventoryCount(i);
			return total >= 3; 
		},
		BigNumber(1000), "1000 Gold + Gold Multiplier Gain x2 (retroactive)"));

	achievements.push_back(Achievement("Elite Spender", "Purchase 100 shop upgrades",
		[stats]() { return stats->getShopUpgradesPurchased() >= 100; }, BigNumber(10000),
		"", "EliteSpenderUnlocked")); // requiredFeature — stays hidden until that key is unlocked
}

void AchievementManager::addThresholdAchievement(const std::string& name, const std::string& description,
	std::function<int()> getter, int threshold, BigNumber reward, std::string requiredFeature)
{
	achievements.push_back(Achievement(name, description,
		[getter, threshold]() { return getter() >= threshold; }, reward, "", requiredFeature));
}

AchievementManager::AchievementManager(Hero* hero, Statistics* statistics, Familiar* familiar, ItemManager* itemManager, UnlockManager* unlockManager)
	: hero(hero), statistics(statistics), familiar(familiar), itemManager(itemManager), unlockManager(unlockManager)
{
	initAchievements();
}

void AchievementManager::update()
{
	for (auto& achievement : achievements)
	{
		if (!achievement.requiredFeature.empty() && !unlockManager->isUnlocked(achievement.requiredFeature))
			continue; // dormant until its prerequisite feature is unlocked — condition isn't even evaluated

		if (!achievement.unlocked && achievement.condition())
		{
			achievement.unlocked = true;

			if (onAchievementUnlocked)
			{
				onAchievementUnlocked(achievement);
			}
		}
	}
}

const std::vector<Achievement>& AchievementManager::getAchievements() const
{
	return achievements; // the full list, used by Game::saveToFile/loadFromFile
}

std::vector<const Achievement*> AchievementManager::getVisibleAchievements() const
{
	std::vector<const Achievement*> visible;
	for (const auto& achievement : achievements)
	{
		if (achievement.requiredFeature.empty() || unlockManager->isUnlocked(achievement.requiredFeature))
			visible.push_back(&achievement);
	}
	return visible;
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

			if (onAchievementClaimed)
			{
				onAchievementClaimed(achievement);
			}

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


