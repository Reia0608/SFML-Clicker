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
	std::string rewardLabel;			// human-readable reward text, shown in the description and on the claim button.
										// Defaults to "<goldReward> Gold" when left blank, so existing calls don't need to change.
	std::string requiredFeature;		// if non-empty, hidden from the tab and dormant until UnlockManager::isUnlocked(requiredFeature)
	bool unlocked = false;
	bool claimed = false; // reward has to be collected manually via a button click, separate from "unlocked"


	Achievement(std::string name, std::string description, std::function<bool()> condition, BigNumber goldReward,
		std::string rewardLabel = "", std::string requiredFeature = "")
		: name(name), description(description), condition(condition), goldReward(goldReward),
		rewardLabel(rewardLabel.empty() ? (goldReward.toString() + " Gold") : rewardLabel),
		requiredFeature(requiredFeature)
	{

	}
};