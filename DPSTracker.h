#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include <vector>

class DPSTracker
{
	private:
		struct DamageEvent
		{
			BigNumber amount;
			float age;
		};

		std::vector<DamageEvent> events;
		float windowDuration = 1.0f;   // length of the sliding time window in seconds - widened to match
		// a slow attacker's cooldown so a single hit every N seconds still
		// averages correctly instead of spiking then dropping to 0

	public:
		void recordDamage(BigNumber amount);
		void update(float dt);
		BigNumber getDPS() const;
		void setWindowDuration(float seconds); // e.g. Familiar keeps this synced to its attackInterval
		float getWindowDuration() const;
};