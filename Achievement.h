#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include <functional>

struct Achievement
{
	std::string name;
	std::string description;
	std::function<bool()> condition;	// checks if the condition is met
	BigNumber goldReward;
	int itemRewardIndex = -1;			// -1 = no item reward; otherwise an index into ItemManager's item definitions (see AchievementManager::addItemRewardAchievement)
	int itemRewardCount = 0;			// how many of that item to grant on claim - only used if itemRewardIndex >= 0
	std::string rewardLabel;			// human-readable reward text, shown in the description and on the claim button.
	// Defaults to "<goldReward> Gold" when left blank, so existing gold-only calls don't need to change.
	// NOTE: this default does NOT know about item names (Achievement.h has no ItemManager access) -
	// any achievement granting an item should always pass an explicit rewardLabel; addItemRewardAchievement() does this for you.
	std::string requiredFeature;		// if non-empty, hidden from the tab and dormant until UnlockManager::isUnlocked(requiredFeature)
	bool unlocked = false;
	bool claimed = false; // reward has to be collected manually via a button click, separate from "unlocked"


	Achievement(std::string name, std::string description, std::function<bool()> condition, BigNumber goldReward,
		std::string rewardLabel = "", std::string requiredFeature = "",
		int itemRewardIndex = -1, int itemRewardCount = 0)
		: name(name), description(description), condition(condition), goldReward(goldReward),
		itemRewardIndex(itemRewardIndex), itemRewardCount(itemRewardCount),
		rewardLabel(rewardLabel.empty() ? (goldReward.toString() + " Gold") : rewardLabel),
		requiredFeature(requiredFeature)
	{

	}
};