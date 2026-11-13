#pragma once
#include "stdafx.h"
#include "Familiar.h"
#include "UIButton.h"
#include "UnlockManager.h"
#include "Text.h"
#include "BigNumber.h"
#include <vector>
#include <memory>
#include <functional>

// Owns HUD widgets that don't belong to any single game-area actor (Hero, Familiar, Enemy) -
// manual attack buttons, the stage name/enemy-info labels, the pause overlay, the familiar DPS
// stat lines + backing panel, and the hero's own currency/DPS/click-damage stat lines. Each
// actor still owns its own GAME state (Hero's currency/attackPower/DPS tracker, etc.) and just
// notifies HUD when it changes - see Hero::onCurrencyChanged/onAttackPowerChanged/onDPSChanged.
// Familiar/Enemy still draw their own name/health-bar HUD elements directly - not migrated here.
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

	// --- Pause overlay ---
	bool paused = false;
	Text* pausedText;
	void layoutPausedText(); // centers pausedText over the play area - called once on construction and again on every resize

	// --- Stage name / enemy info (moved from Stage) ---
	Text* stageText;
	Text* enemyInfoText; // current enemy's name + progress, or "Boss: <name>" - empty when no enemy is on the field
	int stageNumber = 1; // cached only so stageText's string can be rebuilt on resize without Game having to pass it again
	void initStageText();
	void initEnemyInfoText();
	void layoutStageText();
	void layoutEnemyInfoText();

	// --- Familiar DPS stat lines + backing panel (moved from Game) ---
	Text* passiveFamiliarDPSText;
	Text* activeFamiliarDPSText;
	sf::RectangleShape hudStatPanel;
	static constexpr float hudStatPanelPadding = 8.0f; // gap between the panel's edge and the text bounds it wraps
	void initHUDStatPanel();
	void initPassiveFamiliarDPSText();
	void initActiveFamiliarDPSText();
	void layoutPassiveFamiliarDPSText();
	void layoutActiveFamiliarDPSText();

	// --- Hero stat text (moved from Hero) ---
	Text* currencyText;
	Text* dpsText;
	Text* clickDamageText;
	float currencyPulseElapsed = 0.0f;
	bool currencyPulseActive = false;
	static constexpr float currencyPulseDuration = 0.3f;
	static constexpr float currencyPulseMaxScale = 1.4f;
	void initCurrencyText();
	void initHeroDPSText();
	void initClickDamageText();
	void layoutCurrencyText();
	void layoutHeroDPSText();
	void layoutClickDamageText();

public:
	HUD(std::function<sf::Vector2f()> getTargetPosition, UnlockManager* unlockManager);
	~HUD();

	// Rebuilds the button set only if it no longer matches what's slotted (evolution changed
	// the attack list, or a familiar was assigned/unassigned) - cheap to call every frame,
	// and avoids resetting each button's press/notify animation state on frames where nothing changed.
	void sync(const std::vector<Familiar*>& familiarSlots);
	void update(); // refreshes each button's label (attack name vs. remaining cooldown) and ready-pulse
	bool handleClick(sf::Vector2f pos); // true if a HUD widget consumed the click
	void render(sf::RenderWindow* window);
	void onResize(); // re-lays-out existing buttons against the new window size

	// --- Pause overlay ---
	void togglePause(); // flips the paused state - Game calls this when the pause key is pressed
	bool isPaused() const;

	// --- Stage name / enemy info ---
	void setStageNumber(int newStageNumber); // updates the "Stage: N" label - called every frame from Game::runSimulationTick(), and once on save-load
	void setEnemyInfoText(const std::string& text); // updates the enemy/boss label, or clears it when no enemy is on the field
	void renderStageInfo(sf::RenderWindow* window); // draws stageText/enemyInfoText - called first among the HUD elements pushed in Game::render() so everything else paints on top of it

	// --- Familiar DPS stat lines + backing panel ---
	// Dusty rose/pink - passive familiar damage's source-identity color. See Game.cpp's old comment
	// (now here) for why this was picked over lavender: Familiar::dpsColor's silver-blue sat too
	// close in hue at small sizes with a black outline. Public so Game/EnemySpawner can still color
	// a passive-damage floating number to match this HUD line.
	static constexpr sf::Color passiveDPSColor = sf::Color(230, 165, 195);

	void setPassiveFamiliarDPS(const BigNumber& dps); // updates passiveFamiliarDPSText's string + position - called every frame from Game::updateFamiliarPassiveDamage()
	void setActiveFamiliarDPS(const BigNumber& dps);  // updates activeFamiliarDPSText's string + position - called every frame from Game::update()

	// Resizes/repositions hudStatPanel to wrap the hero's own stat text (always shown) plus
	// whichever familiar DPS line(s) are currently visible - call after every stat text above has
	// refreshed for the frame, same ordering requirement updateHUDStatPanelLayout() used to have
	// in Game.cpp.
	void updateStatPanel(bool includePassiveFamiliarDPS, bool includeActiveFamiliarDPS);

	void renderStatPanel(sf::RenderWindow* window);        // draws the backing panel - call before hero's/familiars' own stat text so they paint on top of it
	void renderPassiveFamiliarDPS(sf::RenderWindow* window);
	void renderActiveFamiliarDPS(sf::RenderWindow* window);

	// --- Hero stat text (currency/DPS/click damage) ---
	// onCurrencyChanged's triggerPulse param (see Hero.h) maps straight to triggerCurrencyPulse()
	// below - Game wires that up, HUD never calls it on its own.
	void setCurrency(const BigNumber& currency); // updates currencyText's string + position
	void triggerCurrencyPulse(); // starts the currency-pulse scale animation (a brief "pop" on a gold gain)
	void updateCurrencyPulse(float dt); // advances the pulse animation - call every frame from Game::update()
	void setHeroDPS(const BigNumber& dps);
	void setClickDamage(const BigNumber& attackPower);
	sf::Vector2f getCurrencyTextPosition() const; // used by LootDropManager as the fly-to target for gold drops
	void renderHeroStats(sf::RenderWindow* window); // draws currencyText/dpsText/clickDamageText
};