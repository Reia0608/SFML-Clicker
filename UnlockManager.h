#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include <set>
#include <vector>
#include <string>

// Central registry of "feature keys" unlocked through gameplay (currently: claiming achievements).
// Shop buttons and achievements both query this instead of knowing about each other directly.
class UnlockManager
{
	private:
		std::set<std::string> unlockedFeatures;

		// Which familiars have been unlocked so far, in the order they were actually bought - index
		// 0 is whichever one was unlocked FIRST, regardless of species (normally Fire Dragon, since
		// its unlock button is the only one reachable before the Familiars tab itself reveals - see
		// hasAnyFamiliarUnlocked() below - but nothing here assumes that). A familiar's position in
		// this list, once set, never changes - new entries are only ever appended, never inserted or
		// reordered - which is what lets getElementalDamageThreshold() below treat a familiar's own
		// threshold as permanently fixed the moment it's unlocked.
		std::vector<std::string> familiarUnlockOrder;

		// Tunable starting numbers for whichever familiar ends up first in familiarUnlockOrder -
		// every later one (any species) costs/requires 10x the previous one. Single place to retune
		// both during playtesting; see getNextFamiliarUnlockCost()/getElementalDamageThreshold().
		static constexpr double familiarUnlockBaseCost = 5000.0;
		static constexpr double familiarAchievementBaseThreshold = 10000.0;

	public:
		bool isUnlocked(const std::string& featureKey) const; // empty key = always unlocked (no requirement)
		void unlock(const std::string& featureKey);

		// for save/load
		std::vector<std::string> getUnlockedFeatures() const;
		void restoreUnlocked(const std::string& featureKey); // used only during load, same "no side effects" pattern as Achievement's setUnlocked/setClaimed

		// --- Familiar unlock-order ladder ---
		// Call once, at the exact moment a familiar actually becomes unlocked (a ShopTab or
		// FamiliarTab purchase) - familiarKey is that species' save-string (e.g. "FireDragon", same
		// spelling Game::familiarSaveName() already uses). A repeat call for a key already recorded
		// is a no-op, so it can never shift the position of anything unlocked after it.
		void recordFamiliarUnlocked(const std::string& familiarKey);

		// True once at least one familiar has been unlocked, whichever species it was - this is what
		// reveals the Familiars tab (see Game::initUI()), replacing the old check that only ever
		// looked at Fire Dragon specifically.
		bool hasAnyFamiliarUnlocked() const;

		// What unlocking the NEXT (not-yet-unlocked) familiar currently costs, in gold -
		// familiarUnlockBaseCost * 10^(how many are already unlocked). Every still-locked familiar's
		// unlock button reads this live, so buying ANY of them immediately raises the price shown on
		// every other one still locked.
		BigNumber getNextFamiliarUnlockCost() const;

		// The frozen elemental-damage threshold for familiarKey's own passive-ability achievement -
		// familiarAchievementBaseThreshold * 10^(familiarKey's position in familiarUnlockOrder).
		// Stays fixed from the moment familiarKey is unlocked, since its position can only ever be
		// appended to, never shifted by a later, different familiar unlocking. Returns the unscaled
		// base threshold if familiarKey hasn't been recorded yet - callers shouldn't normally ask
		// before then, since that achievement itself is gated behind the familiar being unlocked.
		BigNumber getElementalDamageThreshold(const std::string& familiarKey) const;

		// for save/load - order matters here (unlike getUnlockedFeatures()'s unordered set), so
		// these mirror the save file being written/read one line at a time, in order.
		std::vector<std::string> getFamiliarUnlockOrder() const;
		void restoreFamiliarUnlockOrder(const std::string& familiarKey); // appends one entry - call once per saved entry, IN SAVED ORDER
};
