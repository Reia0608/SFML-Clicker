#pragma once
#include "stdafx.h"
#include "Achievement.h"
#include "Hero.h"
#include "Statistics.h"
#include "Familiar.h"
#include "ItemManager.h"
#include "UnlockManager.h"

class AchievementManager
{
private:
	Hero* hero;						// non-owning
	Statistics* statistics;			// non-owning
	Familiar* familiar;				// non-owning - Fire, kept under its original name since every existing achievement already refers to it this way
	Familiar* waterFamiliar;		// non-owning
	Familiar* windFamiliar;			// non-owning
	ItemManager* itemManager;		// non-owning
	UnlockManager* unlockManager;	// non-owning
	std::vector<Achievement> achievements;

	void initAchievements();
	void addThresholdAchievement(const std::string& name, const std::string& description,
		std::function<int()> getter, int threshold, BigNumber reward, std::string requiredFeature = ""); // helper for simple ">= threshold" achievements
	void addItemRewardAchievement(const std::string& name, const std::string& description, std::function<bool()> condition,
		int itemIndex, int itemCount, BigNumber goldReward = BigNumber(0), std::string requiredFeature = ""); // helper for achievements that grant an item (optionally plus gold) - builds a readable rewardLabel from ItemManager's item name, e.g. "2x Ruby Emblem" or "500 Gold + 2x Ruby Emblem"

public:
	AchievementManager(Hero* hero, Statistics* statistics, Familiar* familiar, ItemManager* itemManager, UnlockManager* unlockManager,
		Familiar* waterFamiliar, Familiar* windFamiliar);

	void update();
	const std::vector<Achievement>& getAchievements() const;		// full list incl. locked ones � save/load still needs to restore their unlocked/claimed state even while hidden
	std::vector<const Achievement*> getVisibleAchievements() const; // only ones whose requiredFeature is met; AchievementsTab uses this

	void setUnlocked(const std::string& name, bool unlocked); // restores saved state without re-granting the gold/item reward
	bool claimReward(const std::string& name); // grants the gold and/or item reward once, returns false if not unlocked yet or already claimed
	void setClaimed(const std::string& name, bool claimed); // restores saved claimed-state without granting the reward again
	std::function<void(const Achievement&)> onAchievementUnlocked; // fired when the condition is first met (background auto-unlock)
	std::function<void(const Achievement&)> onAchievementClaimed;  // fired when the player manually claims the reward via the button
};