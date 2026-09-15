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
		[stats]() { return stats->getTotalClicks(); }, 5000, BigNumber(3000));

	addThresholdAchievement("Marathon Runner", "Play for 1 hour",
		[stats]() { return static_cast<int>(stats->getPlayTimeSeconds()); }, 3600, BigNumber(15000));

	addThresholdAchievement("Endurance", "Play for 5 hours",
		[stats]() { return static_cast<int>(stats->getPlayTimeSeconds()); }, 18000, BigNumber(100000));

	achievements.push_back(Achievement("Gold Rush", "Earn 100,000 total gold",
		[stats]() { return stats->getTotalGoldEarned() >= BigNumber(100000); }, BigNumber(1000)));

	achievements.push_back(Achievement("Familiar Bond", "Summon your first familiar",
		[fam]() { return fam->isUnlocked(); }, BigNumber(500)));

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

	achievements.push_back(Achievement("Hoarder", "Collect 3 items", // unlocking this also grants +1.0 gold multiplier, see update()
		[items]() {
			int total = 0;
			for (int i = 0; i < items->getItemCount(); i++) total += items->getInventoryCount(i);
			return total >= 3;
		}, BigNumber(1000)));
}

void AchievementManager::addThresholdAchievement(const std::string& name, const std::string& description,
	std::function<int()> getter, int threshold, BigNumber reward)
{
	achievements.push_back(Achievement(name, description,
		[getter, threshold]() { return getter() >= threshold; }, reward));
}

AchievementManager::AchievementManager(Hero* hero, Statistics* statistics, Familiar* familiar, ItemManager* itemManager)
	: hero(hero), statistics(statistics), familiar(familiar), itemManager(itemManager)
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

			if (achievement.name == "Hoarder") // special-cased bonus effect for this achievement
			{
				hero->increaseGoldMultiplier(1.0);
			}

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


