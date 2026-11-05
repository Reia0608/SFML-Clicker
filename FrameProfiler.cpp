#include "stdafx.h"
#include "FrameProfiler.h"
#include <algorithm>

void FrameProfiler::begin()
{
	laps.clear();
	frameClock.restart();
	lapClock.restart();
}

void FrameProfiler::mark(const std::string& label)
{
	laps.push_back({ label, lapClock.restart().asSeconds() });
}

void FrameProfiler::end(float thresholdSeconds)
{
	if (!loggingEnabled) return;

	float total = frameClock.getElapsedTime().asSeconds();
	if (total < thresholdSeconds) return;

	std::vector<Lap> sorted = laps;
	std::sort(sorted.begin(), sorted.end(), [](const Lap& a, const Lap& b) { return a.seconds > b.seconds; });

	std::cout << "[FrameProfiler] Frame took " << (total * 1000.0f) << "ms:" << std::endl;
	for (const auto& lap : sorted)
	{
		float percent = (lap.seconds / total) * 100.0f;
		std::cout << "    " << lap.label << ": " << (lap.seconds * 1000.0f) << "ms (" << percent << "%)" << std::endl;
	}
}

void FrameProfiler::setLoggingEnabled(bool enabled)
{
	loggingEnabled = enabled;
}

bool FrameProfiler::isLoggingEnabled() const
{
	return loggingEnabled;
}
