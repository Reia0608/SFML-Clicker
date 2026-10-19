#pragma once
#include "Hero.h"
#include "Familiar.h"
#include "EnemySpawner.h"
#include "PublicVariables.h"
#include <array>
#include <optional>
#include <vector>

// Exists only while the game is paused - Game creates one when pausing and destroys it on resume
// (see Game::pollEvents()), so stepping back can never reach further than the moment you paused.
// Holds one snapshot per frame stepped forward; stepBack() undoes exactly those, oldest-last.
//
// Deliberately doesn't own or know about Hero/Familiar/EnemySpawner beyond calling their own
// captureState()/restoreState() - those classes keep ownership of their own private fields, this
// class just remembers the plain-data snapshots they hand back.
class FrameStepHistory
{
private:
	struct Snapshot
	{
		Hero::State heroState;
		std::array<std::optional<Familiar::State>, static_cast<size_t>(FamiliarNameCount)> familiarStates; // empty for a species that isn't built yet - see Game::ownedFamiliars
		EnemySpawner::State enemySpawnerState;
	};
	std::vector<Snapshot> snapshots;

public:
	// Records the current state of everything passed in - call this right before advancing the
	// simulation by one tick, so stepBack() has something to return to afterward.
	void recordBeforeStep(Hero* hero, const std::array<Familiar*, static_cast<size_t>(FamiliarNameCount)>& ownedFamiliars, EnemySpawner* enemySpawner);

	bool canStepBack() const; // true if there's a recorded step left to undo

	// Restores the most recently recorded snapshot onto the live objects, and forgets it.
	void stepBack(Hero* hero, const std::array<Familiar*, static_cast<size_t>(FamiliarNameCount)>& ownedFamiliars, EnemySpawner* enemySpawner);
};
