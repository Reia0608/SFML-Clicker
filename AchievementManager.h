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
		Familiar* familiar;				// non-owning
		ItemManager* itemManager;		// non-owning
		UnlockManager* unlockManager;	// non-owning
		std::vector<Achievement> achievements;

		void initAchievements();
		void addThresholdAchievement(const std::string& name, const std::string& description,
			std::function<int()> getter, int threshold, BigNumber reward, std::string requiredFeature = ""); // helper for simple ">= threshold" achievements

	public:
		AchievementManager(Hero* hero, Statistics* statistics, Familiar* familiar, ItemManager* itemManager, UnlockManager* unlockManager);

		void update();
		const std::vector<Achievement>& getAchievements() const;		// full list incl. locked ones — save/load still needs to restore their unlocked/claimed state even while hidden
		std::vector<const Achievement*> getVisibleAchievements() const; // only ones whose requiredFeature is met; AchievementsTab uses this

		void setUnlocked(const std::string& name, bool unlocked); // restores saved state without re-granting the gold reward
		bool claimReward(const std::string& name); // grants the gold reward once, returns false if not unlocked yet or already claimed
		void setClaimed(const std::string& name, bool claimed); // restores saved claimed-state without granting gold again
		std::function<void(const Achievement&)> onAchievementUnlocked; // fired when the condition is first met (background auto-unlock)
		std::function<void(const Achievement&)> onAchievementClaimed;  // fired when the player manually claims the reward via the button
};