#pragma once
#include "stdafx.h"
#include "Familiar.h"
#include "UIButton.h"
#include "UnlockManager.h"
#include <vector>
#include <memory>
#include <functional>

// Owns HUD widgets that don't belong to any single game-area actor (Hero, Familiar, Enemy) -
// starting with manual attack buttons for whichever familiars are currently slotted. Other
// scattered HUD elements (DPS text, cooldown rings, hudStatPanel) stay where they are for now
// and get migrated in individually, later.
class HUD
{
private:
	struct ManualAttackEntry
	{
		Familiar* familiar; // non-owning
		size_t attackIndex;
		std::unique_ptr<UIButton> button;
	};
	std::vector<ManualAttackEntry> manualAttackEntries;
	std::function<sf::Vector2f()> getTargetPosition; // supplied by Game - resolves where a manual attack should aim, e.g. enemySpawner->getEnemyPosition()
	UnlockManager* unlockManager; // non-owning - used only to check a gated attack's FamiliarAttack::unlockFeatureKey

	bool isAttackAvailable(const FamiliarAttack& attack) const; // true if this attack isn't gated at all, or its unlockFeatureKey has been purchased
	bool matchesCurrentSlots(const std::vector<Familiar*>& familiarSlots) const;
	void rebuildManualAttackEntries(const std::vector<Familiar*>& familiarSlots);
	void layoutManualAttackEntries();

public:
	HUD(std::function<sf::Vector2f()> getTargetPosition, UnlockManager* unlockManager);

	// Rebuilds the button set only if it no longer matches what's slotted (evolution changed
	// the attack list, or a familiar was assigned/unassigned) - cheap to call every frame,
	// and avoids resetting each button's press/notify animation state on frames where nothing changed.
	void sync(const std::vector<Familiar*>& familiarSlots);
	void update(); // refreshes each button's label (attack name vs. remaining cooldown) and ready-pulse
	bool handleClick(sf::Vector2f pos); // true if a HUD widget consumed the click
	void render(sf::RenderWindow* window);
	void onResize(); // re-lays-out existing buttons against the new window size
};