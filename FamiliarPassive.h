#pragma once
#include "stdafx.h"
#include <string>

// One familiar-stage's passive ability data - e.g. Fire Dragon's Burn, Water Dragon's Soaked,
// Wind Dragon's Keen Gale. Set per stage inside FamiliarType::passive (via FamiliarType::setPassive()),
// and copied into Familiar on construction/evolveTo() exactly like FamiliarType::attacks already is
// (see Familiar::getPassive()) - so a passive's tuning always reflects whichever stage this
// familiar currently is, with no switch-on-Age needed anywhere that uses it.
//
// Every field stands alone - a passive that doesn't need a given field (e.g. Burn has no use for
// `magnitude`) just leaves it at its default, same philosophy FamiliarAttack's own per-attack
// fields already follow. Default-constructed (empty name) means this stage has no passive at all -
// not used by current content (every Fire/Water/Wind stage so far defines one), but a future
// lineage without a passive simply wouldn't call setPassive().
struct FamiliarPassive
{
	std::string name;             // e.g. "Burn" - display name, and (for Burn/Soaked) the status-effect "type" key Enemy's framework matches on (see Enemy::applyStatusEffect)
	std::string unlockFeatureKey; // e.g. "FireDragonBurnUnlocked" - gates whether this passive's effect applies at all; empty = always active once this stage is reached (no current passive leaves this empty, every one today is purchasable)

	// True for a passive that deals its OWN damage over time (currently just Burn) - false (the
	// default) for one that only modifies damage taken/dealt without landing a hit of its own
	// (Soaked, Keen Gale). Read by StatisticsTab to decide whether a familiar gets its own
	// "Passive Damage" stat line - kept as an explicit flag here, rather than inferred from other
	// fields, so a future passive just states what it does instead of StatisticsTab having to guess.
	bool dealsDamage = false;

	float duration = 0.0f;     // Burn/Soaked: how long the effect lasts, in seconds - ignored while permanent is true
	bool permanent = false;    // Burn/Soaked: true = lasts until the enemy dies (Ancient tier) - duration is ignored
	float tickInterval = 1.0f; // Burn: seconds between damage ticks - unused by Soaked/Keen Gale

	float magnitude = 0.0f; // Soaked: the damage-taken multiplier (e.g. 2.0 = double damage taken). Keen Gale: the crit-chance bonus (0.10 = +10%). Unused by Burn, which scales off the familiar's own current elemental power instead of a flat magnitude.
	bool forceNeutralAffinity = false;            // Soaked only: true forces the enemy's own weakness/resistance/immunity to the affected elements to neutral while this is active (Ancient Water Dragon)
	bool overflowConvertsToSecondaryStat = false; // Keen Gale only: true lets crit chance exceed 100%, converting the overflow into bonus crit damage (Ancient Wind Dragon)
};
