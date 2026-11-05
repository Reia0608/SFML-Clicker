#pragma once
#include "stdafx.h"

// A simple repeating timer: once active, update(dt) returns true every time intervalSeconds has
// elapsed, so the caller can fire whatever "attack" actually means for it. Deliberately knows
// nothing about Hero, Familiar, damage, or animations - that's left entirely to whoever calls
// update() - so the same class could later drive a Familiar's own auto-attack timing too, without
// this class (or anything using it today) needing to change. Today it's only used by Hero.
class AutoAttack
{
private:
	bool active = false;
	float intervalSeconds = 0.5f;
	float timeAccumulated = 0.0f; // seconds banked toward the next shot while active

public:
	explicit AutoAttack(float intervalSeconds = 0.5f);

	void setActive(bool isActive); // turning ON restarts the countdown fresh - avoids an instant first shot from stale accumulation left over from before it was turned off
	bool isActive() const;
	void toggleActive();

	void setInterval(float seconds);
	float getInterval() const;

	// Advances the countdown by dt while active; returns true the instant intervalSeconds has been
	// reached, telling the caller "fire now" - and keeps whatever time is left over past that point
	// rather than discarding it, so the average rate stays correct even after an unusually slow frame.
	bool update(float dt);

	// Everything that changes frame-to-frame - same pattern as Hero::State/DPSTracker::State - so
	// frame-stepping (see FrameStepHistory) can save/restore it exactly.
	struct State
	{
		bool active = false;
		float timeAccumulated = 0.0f;
	};
	State captureState() const { return { active, timeAccumulated }; }
	void restoreState(const State& state) { active = state.active; timeAccumulated = state.timeAccumulated; }
};
