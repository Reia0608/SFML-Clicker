#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include "Hero.h"
#include "ItemBar.h"
#include "ItemManager.h"
#include <vector>
#include <functional>

// Handles the purely-visual "loot drop" juice: when EnemySpawner reports that an enemy dropped
// gold and/or an item, this class spawns a little sprite at the enemy's position that falls onto
// the lower half of the play area, lingers there for a bit (coins spin through their turning
// animation, items get a small shine pulse), then flies to its real HUD destination (the Gold
// counter or the matching ItemBar slot) and disappears.
//
// IMPORTANT: the actual reward (Hero::gainCurrency / ItemManager::addItem) is only granted at
// that final moment, via the onXCollectedCallback - Game wires those up. This mirrors
// EnemySpawner's existing onBossEncounterEndedCallback pattern, so nothing here reaches directly
// into Hero's currency or ItemManager's inventory except through those two callbacks.
class LootDropManager
{
public:
	enum class DropType { Gold, Item };

private:
	enum class DropPhase { Landing, Lingering, FlyingToTarget, Done };

	struct LootDrop
	{
		DropType type;
		DropPhase phase = DropPhase::Landing;
		float phaseElapsed = 0.0f;

		sf::Sprite sprite;

		sf::Vector2f startPos;    // where the enemy died
		sf::Vector2f landPos;     // random point in the lower half of the play area
		sf::Vector2f flyStartPos; // captured position when FlyingToTarget begins

		BigNumber goldAmount;     // used when type == Gold
		int itemIndex = -1;       // used when type == Item

		int coinFrame = 0;
		float coinFrameTimer = 0.0f;
		float shinePhase = 0.0f;  // randomized start offset so simultaneous items don't pulse in lockstep

		LootDrop(const sf::Texture& texture, DropType type, sf::Vector2f startPos)
			: type(type), sprite(texture), startPos(startPos)
		{
		}
	};

	Hero* hero;               // non-owning, part of Game - gives us the Gold counter's HUD position and is nudged via the collected callback in Game, never called directly here except getCurrencyTextPosition()
	ItemBar* itemBar;         // non-owning, part of Game - gives us each item slot's HUD position
	ItemManager* itemManager; // non-owning, part of Game - gives us the item icon sheet + item definitions

	sf::Texture coinTexture;
	sf::Vector2u coinFrameSize = { 16, 16 };
	int coinFrameCount = 1;

	std::vector<LootDrop> drops;

	// --- Tuning knobs, exposed as setters below so they can be wired to a Debug tab later for
	// live optimisation without touching any logic in this class. ---
	float landingDuration = 0.35f;     // seconds for the drop to fall from the enemy to its landing spot
	float landingHopHeight = 18.0f;    // how high the little "pop" arc is while landing
	float lingerDurationGold = 2.5f;   // seconds a coin sits on the ground spinning before flying off
	float lingerDurationItem = 2.5f;   // seconds an item sits on the ground shining before flying off
	float flyDuration = 0.45f;         // seconds for the flight from the ground to the HUD target
	float coinFrameTimerMax = 0.08f;   // seconds per coin-spin frame
	float coinScale = 1.5f;
	float itemScale = 3.0f;
	float shinePulseSpeed = 6.0f;      // radians/sec of the item's scale-pulse "shine"
	float shinePulseStrength = 0.15f;  // +/- scale fraction of the pulse
	size_t maxSimultaneousDrops = 60;  // safety net, mirrors Enemy::showDamage's damageNumbers cap -
	// if this is hit, the reward is still granted INSTANTLY (see
	// onGoldDropped/onItemDropped), only the visual is skipped

	std::function<void(BigNumber amount)> onGoldCollectedCallback;
	std::function<void(int itemIndex)> onItemCollectedCallback;

	sf::Vector2f randomLowerHalfLandingPoint() const;
	void updateDrop(LootDrop& drop, float dt);
	void drawItemShine(sf::RenderWindow* window, const LootDrop& drop) const;

public:
	LootDropManager(Hero* hero, ItemBar* itemBar, ItemManager* itemManager);
	~LootDropManager();

	// Called by Game, wired to EnemySpawner's onGoldDroppedCallback/onItemDroppedCallback -
	// spawns the falling/lingering/flying visual. position is where the enemy died (world space).
	void onGoldDropped(BigNumber amount, sf::Vector2f position);
	void onItemDropped(int itemIndex, sf::Vector2f position);

	void update(float dt);
	void render(sf::RenderWindow* window);

	// Fired once a drop's flight actually reaches its HUD target - Game wires these to
	// hero->gainCurrency()/statistics->recordGoldEarned() and itemManager->addItem() respectively,
	// so the reward is only granted at the moment it's visually collected.
	void setOnGoldCollectedCallback(std::function<void(BigNumber amount)> callback);
	void setOnItemCollectedCallback(std::function<void(int itemIndex)> callback);

	// Tuning setters, for optimisation/feel passes.
	void setLandingDuration(float seconds);
	void setLingerDurations(float goldSeconds, float itemSeconds);
	void setFlyDuration(float seconds);
	void setMaxSimultaneousDrops(size_t count);
};