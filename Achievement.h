#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include <functional>

struct Achievement
{
	std::string name;
	std::string description;
	std::function<bool()> condition;   // checks if the condition is met
	BigNumber goldReward;
	bool unlocked = false;
	bool claimed = false; // reward has to be collected manually via a button click, separate from "unlocked"


	Achievement(std::string name, std::string description, std::function<bool()> condition, BigNumber goldReward)
		: name(name), description(description), condition(condition), goldReward(goldReward)
	{
	}
};