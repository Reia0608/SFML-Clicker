#include "stdafx.h"
#include "DPSTracker.h"
#include <algorithm>

void DPSTracker::recordDamage(BigNumber amount)
{
	events.push_back({ amount, 0.0f });
}

void DPSTracker::update(float dt)
{
	for (auto& e : events)
	{
		e.age += dt;
	}

	events.erase(
		std::remove_if(events.begin(), events.end(), [this](const DamageEvent& e) 
		{
			return e.age >= windowDuration;
		}),
		events.end()
	);
}

BigNumber DPSTracker::getDPS() const
{
	BigNumber total = BigNumber(0);
	for (const auto& e : events)
	{
		total = total + e.amount;
	}
	return total / windowDuration;
}