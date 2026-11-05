#pragma once
#include "stdafx.h"
#include <string>
#include <vector>

// Kept as a permanent dev tool (started as a one-off for the 2026-11-04 FPS investigation, proved
// useful enough to keep). Times labeled chunks of a frame and, once the frame's total time crosses
// thresholdSeconds, prints a sorted breakdown to stdout so the slow chunk(s) are visible without
// attaching a real profiler. begin()/mark() always run every frame (negligible cost - a handful of
// sf::Clock reads), so toggling loggingEnabled takes effect immediately with no warm-up; DebugTab
// has a button for it (see IDebugController::debugSetFrameProfilerLoggingEnabled).
class FrameProfiler
{
private:
	struct Lap
	{
		std::string label;
		float seconds;
	};

	sf::Clock frameClock;
	sf::Clock lapClock;
	std::vector<Lap> laps;
	bool loggingEnabled = true; // only gates end()'s stdout output - it only ever prints when a frame actually exceeds thresholdSeconds, so leaving this on costs nothing during normal play

public:
	void begin(); // call once at the very start of the frame
	void mark(const std::string& label); // records time since the previous mark()/begin() under this label
	void end(float thresholdSeconds); // call once at the very end of the frame - logs a breakdown if the frame ran longer than thresholdSeconds and loggingEnabled is true

	void setLoggingEnabled(bool enabled);
	bool isLoggingEnabled() const;
};
