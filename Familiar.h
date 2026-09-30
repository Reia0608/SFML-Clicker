#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include "Sprites.h"
#include "Textures.h"
#include "FamiliarType.h"
#include "FamiliarAttack.h"
#include "ElementalPower.h"
#include "Text.h"
#include "DPSTracker.h"
#include "Projectile.h"
#include <string>
#include <vector>
#include <functional>


// A familiar auto-attacks the current enemy and, once evolved, may also gain a manually
// activated attack (see FamiliarAttack.h). Every stage defines its own independent list of
// attacks via FamiliarType::attacks, so no two familiars need behave the same way.
class Familiar
{
private:
	FamiliarName name = BabyFireDragon;
	std::string label = "no_name";
	std::string type = "no_type";
	bool unlocked = false;

	// Purchased upgrades (ShopTab), familiar-wide and persistent across evolutions/fusions -
	// deliberately NOT part of `attacks` (see FamiliarAttack.h), so evolveTo() rebuilding that
	// vector from the new stage's base stats never loses purchased progress. Per current design,
	// these only ever apply to auto-attacks (see getEffectiveCooldown / executeAttack) - ShopTab
	// upgrades are for a familiar's auto-attack, not its manually-activated abilities.
	BigNumber bonusPhysicalPower = BigNumber(0);
	ElementalPower bonusElementalPower;
	float bonusCooldownReduction = 0.0f;

	Sprites* sprites;
	DPSTracker familiarDPSTracker;

	std::vector<FamiliarAttack> attacks; // runtime instances, built from FamiliarType::attacks at construction/evolveTo

	std::vector<Projectile> activeProjectiles; // projectiles currently in flight, from any ranged attack
	std::function<void(BigNumber damage, ElementMask mask)> onDamageDealt; // fired once per landed hit - melee-instant or projectile-arrival alike - mask is 0 for a purely physical hit
	std::function<double(Elements)> elementalMultiplierResolver; // resolves ONE element's own weakness/resistance multiplier against whatever enemy is currently on the field. Falls back to 1.0 (no bonus/penalty) if never set or no enemy is present.

	// radial cooldown indicator (Donut, fills clockwise from the top; countdown text in the middle)
	// visualizes the first isAutoAttack entry's cooldown, if this familiar has one
	Text* cooldownText;
	Text* familiarDPSText;
	sf::VertexArray cooldownRingTrack;    // full ring drawn underneath, always visible as a dim background track
	sf::VertexArray cooldownRingProgress; // partial ring on top, grows as the cooldown elapses
	static constexpr float cooldownRingInnerRadius = 20.0f; // the "hole" in the middle of the Donut
	static constexpr float cooldownRingOuterRadius = 28.0f;
	static constexpr int cooldownRingSegments = 48;          // resolution for a full 360 degree ring
	static constexpr float cooldownRingOffsetY = -80.0f;     // placed above the sprite

	void initSprites(std::string idleAnimationAddress, float idleFrameTimerMax, sf::Vector2u spriteSize, const std::vector<FamiliarAttack>& attackList);
	void initFamiliarDPSText();
	void setupAttacks(std::vector<FamiliarAttack> attackList); // allocates projectile textures for ranged attacks, takes ownership into `attacks`
	void initCooldownIndicator();
	void updateCooldownIndicator();
	static void buildRingArc(sf::VertexArray& target, sf::Vector2f center, float innerRadius, float outerRadius, float startAngleDeg, float sweepAngleDeg, int maxSegments, sf::Color color);
	void executeAttack(FamiliarAttack& attack, sf::Vector2f targetPosition); // plays the attack's animation and applies/launches its damage
	void syncDPSTrackerWindow(); // keeps familiarDPSTracker's averaging window matched to the primary auto-attack's cooldown
	float getEffectiveCooldown(const FamiliarAttack& attack) const; // attack.baseCooldown minus bonusCooldownReduction (auto-attacks only), floored at attack.minCooldownLimit

public:
	Familiar(FamiliarType familiarType);
	~Familiar();

	void onResize();
	void update(float dt, sf::Vector2f targetPosition); // ticks all attack cooldowns, auto-fires any ready auto-attacks at targetPosition, advances projectiles/animations
	void draw(sf::RenderWindow* window);     // sprite + projectiles - depth-sorted with decorations
	void drawHUD(sf::RenderWindow* window);  // cooldown ring/text + familiarDPSText - HUD layer

	float getGroundY() const { return sprites->getGroundY(); } // Y of the familiar's feet on the ground - Stage uses this to depth-sort it against decorations

	bool isUnlocked() const;
	void unlock(); // called once from ShopTab's purchase, or from Game::loadFromFile() to restore a saved unlock

	size_t getAttackCount() const;
	const FamiliarAttack& getAttack(size_t index) const; // for HUD: name/cooldown/isAutoAttack, to build buttons and labels
	bool isManualAttackReady(size_t attackIndex) const;
	bool activateManualAttack(size_t attackIndex, sf::Vector2f targetPosition); // false (no-op) if not ready or not manual

	void setOnDamageDealtCallback(std::function<void(BigNumber damage, ElementMask mask)> callback); // called once per landed hit
	void setElementalMultiplierResolver(std::function<double(Elements)> resolver); // wired by Game to EnemySpawner::getEnemyElementalMultiplierForElement - resolves one element at a time, so a multi-element attack scales each element by its own multiplier rather than an average
	BigNumber rollDamage() const; // physical + elemental of the primary auto-attack (base + bonus) - a rough "power level" preview, no resistances applied

	// Purchased upgrades - ShopTab calls these; they only ever affect auto-attacks (see class comment above).
	// Method names/signatures kept stable from before this refactor so ShopTab.cpp didn't need to change.
	BigNumber getBonusPhysicalPower() const;
	void increasePhysicalAttack(BigNumber amount); // adds to bonusPhysicalPower

	BigNumber getBonusElementalPower(Elements element) const;
	void increaseElementalAttack(BigNumber amount); // adds `amount` to bonusElementalPower, for every element the primary auto-attack's baseElementalPower uses

	void increaseCooldownReduction(float amount); // adds to bonusCooldownReduction - applies to every auto-attack this familiar has
	bool isAutoAttackCooldownAtMinimum() const; // true once every auto-attack's effective cooldown has hit its own minCooldownLimit - ShopTab uses this to disable the reduction button. Re-evaluates fresh after evolveTo(), since a new stage's floor/base may differ.

	// for save/ load
	void setName(FamiliarName name);
	FamiliarName getName() const;

	void setBonusPhysicalPower(BigNumber value);
	void setBonusElementalPower(Elements element, BigNumber value); // restores one element's saved bonus at a time
	void setBonusCooldownReduction(float value);
	float getBonusCooldownReduction() const;

	float getAttackCooldown(size_t attackIndex) const; // returns that attack's current EFFECTIVE cooldown (base - bonus, floored) - for HUD display

	void evolveTo(FamiliarType newType); // swaps sprites/attacks to a later evolution stage in place; bonusPhysicalPower/bonusElementalPower/bonusCooldownReduction/unlocked are preserved

	void updateFamiliarDPSText(); // public so ShopTab can access it to update
	BigNumber getFamiliarDPS() const;

	sf::FloatRect getDPSTextBounds() const; // familiarDPSText's current on-screen bounds - used by Game to size its HUD backing panel. Only meaningful while this familiar is unlocked and actively slotted, both of which Game already checks before ever calling this (drawHUD()'s own callers are gated the same way)

	float getAttackCooldownRemaining(size_t attackIndex) const; // used in HUD

	// Pale silver-blue - this familiar's source-identity color. Used both for familiarDPSText
	// (see initFamiliarDPSText()) and passed to EnemySpawner::handleClick() by Game whenever this
	// familiar lands a hit, so its floating damage numbers on the enemy match its own HUD line.
	static constexpr sf::Color dpsColor = sf::Color(190, 210, 235);
};