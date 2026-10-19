#include "stdafx.h"
#include "FrameStepHistory.h"

void FrameStepHistory::recordBeforeStep(Hero* hero, const std::array<Familiar*, static_cast<size_t>(FamiliarNameCount)>& ownedFamiliars, EnemySpawner* enemySpawner)
{
	Snapshot snapshot;
	snapshot.heroState = hero->captureState();
	for (size_t i = 0; i < ownedFamiliars.size(); i++)
	{
		if (ownedFamiliars[i]) snapshot.familiarStates[i] = ownedFamiliars[i]->captureState();
	}
	snapshot.enemySpawnerState = enemySpawner->captureState();
	snapshots.push_back(std::move(snapshot));
}

bool FrameStepHistory::canStepBack() const
{
	return !snapshots.empty();
}

void FrameStepHistory::stepBack(Hero* hero, const std::array<Familiar*, static_cast<size_t>(FamiliarNameCount)>& ownedFamiliars, EnemySpawner* enemySpawner)
{
	if (snapshots.empty()) return;

	const Snapshot& snapshot = snapshots.back();
	hero->restoreState(snapshot.heroState);
	for (size_t i = 0; i < ownedFamiliars.size(); i++)
	{
		if (ownedFamiliars[i] && snapshot.familiarStates[i]) ownedFamiliars[i]->restoreState(*snapshot.familiarStates[i]);
	}
	enemySpawner->restoreState(snapshot.enemySpawnerState);

	snapshots.pop_back();
}
