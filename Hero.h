#pragma once
#include "Sprites.h"
#include "BigNumber.h"
#include "DPSTracker.h"
#include "AutoAttack.h"
#include "PublicVariables.h"
#include <functional>


class Hero
{
private:
	BigNumber attackPower;
	BigNumber currency = BigNumber(0);

	Sprites* sprites;
	DPSTracker heroDPSTracker;
	AutoAttack autoAttack{ 0.5f }; // default repeat interval - see setAutoAttackInterval() for retuning this later (e.g. a future upgrade); gated entirely by UnlockManager's "HeroAutoAttackUnlocked" key outside this class (see Game.cpp)

	double goldMultiplier = 1.0;
	bool critUnlocked = false;
	double critChance = 0.0;
	double critDamageMultiplier = 1.5;
	bool hasAttack1played = false; // to check when to insert attack2
	bool hasAttack2played = false; // to check when to insert attack1

public:
	Hero();
	~Hero();

	void initSprites();
	void gainCurrency(BigNumber amount);
	void recordDamageDealt(BigNumber amount);
	void increaseGoldMultiplier(double amount);

	void updateSprites(float dt);
	void onResize();
	void updateDPS(float dt);
	void draw(sf::RenderWindow* window);     // sprite only - depth-sorted with decorations

	float getGroundY() const { return sprites->getGroundY(); } // Y of the hero's feet on the ground - Stage uses this to depth-sort him against decorations
	void playAttack1Animation();
	void playAttack2Animation();

	BigNumber getAttackPower() const;
	BigNumber getHeroDPS() const;
	BigNumber getCurrency() const;
	double getGoldMultiplier() const;
	bool trySpendCurrency(BigNumber amount);
	void increaseAttackPower(BigNumber amount);

	void setGoldMultiplier(double value); // absolute set � used to rebalance the multiplier when the per-purchase increase amount changes (e.g. Hoarder achievement)

	bool isCritUnlocked() const;
	double getCritChance() const;
	double getCritDamageMultiplier() const;

	// Attack animations
	bool getHasAttack1played() const;
	bool getHasAttack2played() const;
	void setHasAttack1played(bool value);
	void setHasAttack2played(bool value);

	void unlockCriticalHits(double initialChance);
	void increaseCritChance(double amount);
	void increaseCritDamage(double amount);

	// externalCritChanceBonus/externalCritDamageBonus let a passive (e.g. Wind Dragon's Keen Gale)
	// contribute to THIS roll without ever being added to critChance/critDamageMultiplier
	// themselves - those stay purely what's been purchased. The caller recomputes these fresh
	// every roll (see Game::computeKeenGaleCritBonus()), so a passive's bonus is only ever "live"
	// for as long as its source actually applies (e.g. Wind Dragon stays slotted) - nothing here
	// needs to add/remove anything when that changes.
	BigNumber rollDamage(bool& outWasCritical, double externalCritChanceBonus = 0.0, double externalCritDamageBonus = 0.0);

	void grantBonusGold(BigNumber amount);

	// --- Auto-attack ---
	// Hero owns the timer and its on/off state; it has no idea what firing it actually DOES
	// (dealing damage, playing an animation) - that stays Game's job, same division of
	// responsibility as the manual click already has (see Game::performHeroAttack()). Whether any
	// of this is even reachable by the player is gated entirely outside Hero, via UnlockManager's
	// "HeroAutoAttackUnlocked" key (see Game.cpp) - Hero itself will happily tick/fire regardless,
	// exactly like critChance would do nothing useful if nothing ever unlocked crits either.
	bool isAutoAttackActive() const;
	void setAutoAttackActive(bool active);
	void toggleAutoAttack();
	bool tickAutoAttack(float dt); // advances the timer; returns true exactly on the frame another shot is due - call performHeroAttack() (or equivalent) when it does
	float getAutoAttackInterval() const;
	void setAutoAttackInterval(float seconds); // not wired to any purchase yet - exists so a future upgrade can retune "x" without touching AutoAttack itself

	// Debug-only: absolute setters (bypass normal game rules) and a reset back to a fresh Hero's starting values.
	void setAttackPower(BigNumber value); // absolute set, bypasses the normal increase-only upgrade path (debug use) - does NOT fire onAttackPowerChanged, matching its pre-existing behavior (flagged, not fixed - see resetToDefaults() below too)
	void setCurrency(BigNumber value); // absolute set (debug use)
	void setCritChance(double value); // also unlocks crit if not already unlocked (debug use)
	void setCritDamageMultiplier(double value); // absolute set (debug use)
	void resetToDefaults();

	// Fired whenever currency/attack power/DPS change, so whoever owns the on-screen stat text
	// (HUD) can refresh it immediately - same callback pattern as OptionsTab::onTabTextSizeChanged,
	// so Hero doesn't need to know anything about HUD itself. Game wires all three once at startup.
	// onCurrencyChanged's triggerPulse is true only for a currency GAIN (gainCurrency/grantBonusGold),
	// never for a spend/debug-set/reset/restore.
	std::function<void(const BigNumber& currency, bool triggerPulse)> onCurrencyChanged;
	std::function<void(const BigNumber& attackPower)> onAttackPowerChanged;
	std::function<void(const BigNumber& dps)> onDPSChanged;

	// Everything about the Hero that can change frame-to-frame - used by frame-stepping (see
	// FrameStepHistory) to save/restore the Hero exactly as it was before a manual step.
	struct State
	{
		BigNumber attackPower;
		BigNumber currency;
		double goldMultiplier = 1.0;
		bool critUnlocked = false;
		double critChance = 0.0;
		double critDamageMultiplier = 1.5;
		bool hasAttack1played = false;
		bool hasAttack2played = false;
		DPSTracker::State dpsTrackerState;
		AutoAttack::State autoAttackState;
		Sprites::State spritesState;
	};

	State captureState() const;
	void restoreState(const State& state);
};
