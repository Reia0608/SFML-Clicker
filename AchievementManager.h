#pragma once
#include "stdafx.h"
#include "Achievement.h"
#include "Hero.h"
#include "Statistics.h"

class AchievementManager
{
	private:
		Hero* hero;             // non-owning
		Statistics* statistics; // non-owning
		std::vector<Achievement> achievements;

		void initAchievements();

	public:
		AchievementManager(Hero* hero, Statistics* statistics);

		void update();
		const std::vector<Achievement>& getAchievements() const;
		void setUnlocked(const std::string& name, bool unlocked); // restores saved state without re-granting the gold reward
	
		bool claimReward(const std::string& name); // grants the gold reward once, returns false if not unlocked yet or already claimed
		void setClaimed(const std::string& name, bool claimed); // restores saved claimed-state without granting gold again
		std::function<void(const Achievement&)> onAchievementUnlocked;
};