#pragma once
#include "stdafx.h"
#include <functional>

struct FamiliarAbility
{
	std::string name;
	std::string description;
	float cooldown;              // seconds between manual activations
	float cooldownRemaining = 0.0f;
	bool unlocked = false;       // unlockable in the Shop/Familiar-Tab, like in the achievements
	std::function<void()> effect; // z.B. Boss-Timer freeze, use DoT, ...

	FamiliarAbility(std::string name, std::string description, float cooldown, std::function<void()> effect)
		: name(name), description(description), cooldown(cooldown), effect(effect)
	{

	}

	bool tryActivate() // activated with the button of the later Familiar-Tab?
	{
		if (!unlocked || cooldownRemaining > 0.0f) return false;
		effect();
		cooldownRemaining = cooldown;
		return true;
	}

	void update(float dt)
	{
		if (cooldownRemaining > 0.0f) cooldownRemaining = std::max(0.0f, cooldownRemaining - dt);
	}
};