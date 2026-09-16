#pragma once
#include "stdafx.h"
#include <set>

// Central registry of "feature keys" unlocked through gameplay (currently: claiming achievements).
// Shop buttons and achievements both query this instead of knowing about each other directly.
class UnlockManager
{
	private:
		std::set<std::string> unlockedFeatures;

	public:
		bool isUnlocked(const std::string& featureKey) const; // empty key = always unlocked (no requirement)
		void unlock(const std::string& featureKey);

		// for save/load
		std::vector<std::string> getUnlockedFeatures() const;
		void restoreUnlocked(const std::string& featureKey); // used only during load, same "no side effects" pattern as Achievement's setUnlocked/setClaimed
};