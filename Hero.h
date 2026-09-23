#pragma once
#include "Sprites.h"
#include "Text.h"
#include "BigNumber.h"
#include "DPSTracker.h"
#include "PublicVariables.h"


class Hero
{
private:
	BigNumber attackPower;
	BigNumber currency = BigNumber(0);

	Sprites* sprites;
	Text* currencyText;
	Text* dpsText;
	Text* clickDamageText;
	DPSTracker heroDPSTracker;

	float currencyPulseElapsed = 0.0f;
	bool currencyPulseActive = false;
	double goldMultiplier = 1.0;
	const float currencyPulseDuration = 0.3f;
	const float currencyPulseMaxScale = 1.4f;
	bool critUnlocked = false;
	double critChance = 0.0;
	double critDamageMultiplier = 1.5;
	bool hasAttack1played = false; // to check when to insert attack2
	bool hasAttack2played = false; // to check when to insert attack1

	void initCurrencyText();
	void updateCurrencyText();
	void triggerCurrencyPulse();
	void initClickDamageText();
	void initDPSText();
	void updateDPSText();

public:
	Hero();
	~Hero();

	void initSprites();
	void gainCurrency(BigNumber amount);
	void recordDamageDealt(BigNumber amount);
	void increaseGoldMultiplier(double amount);

	void updateSprites(float dt);
	void updateCurrencyPulse(float dt);
	void onResize();
	void updateDPS(float dt);
	void draw(sf::RenderWindow* window);     // sprite only - depth-sorted with decorations
	void drawHUD(sf::RenderWindow* window);  // currency/DPS/click-damage text - HUD layer

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

	BigNumber rollDamage(bool& outWasCritical);

	void grantBonusGold(BigNumber amount);

	// Debug-only: absolute setters (bypass normal game rules) and a reset back to a fresh Hero's starting values.
	void setAttackPower(BigNumber value); // absolute set, bypasses the normal increase-only upgrade path (debug use)
	void setCurrency(BigNumber value); // absolute set (debug use)
	void setCritChance(double value); // also unlocks crit if not already unlocked (debug use)
	void setCritDamageMultiplier(double value); // absolute set (debug use)
	void resetToDefaults();

	// is public so ShopTab.cpp can access it to update the damage text when buying Attack+1
	void updateClickDamageText();
};