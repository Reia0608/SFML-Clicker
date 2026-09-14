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
		const float windowDuration = 1.0f;   // length of the sliding time window in seconds

	public:
		void recordDamage(BigNumber amount);
		void update(float dt);
		BigNumber getDPS() const;
};