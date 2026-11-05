#include "stdafx.h"
#include "AutoAttack.h"

AutoAttack::AutoAttack(float intervalSeconds)
	: intervalSeconds(intervalSeconds)
{
}

void AutoAttack::setActive(bool isActive)
{
	active = isActive;
	timeAccumulated = 0.0f; // fresh countdown either way, so re-enabling never fires early off old progress
}

bool AutoAttack::isActive() const
{
	return active;
}

void AutoAttack::toggleActive()
{
	setActive(!active);
}

void AutoAttack::setInterval(float seconds)
{
	intervalSeconds = seconds;
}

float AutoAttack::getInterval() const
{
	return intervalSeconds;
}

bool AutoAttack::update(float dt)
{
	if (!active) return false;

	timeAccumulated += dt;
	if (timeAccumulated >= intervalSeconds)
	{
		timeAccumulated -= intervalSeconds; // keep the remainder rather than resetting to 0 - a late/slow frame doesn't push the next shot's timing back
		return true;
	}
	return false;
}
