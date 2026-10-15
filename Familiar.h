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
	FamiliarName name = FireDragon; // which SPECIES this is - stays constant across this object's whole lifetime, even through evolveTo()
	Age currentAge = Age::Baby;     // which evolution STAGE this species is currently at - this is what evolveTo() actually advances
	std::string label = "no_name";
	std::string type = "no_type";
	bool unlocked = false;

	// Purchased upgrades (ShopTab), familiar-wide and persistent across evolutions/fusions -
	// deliberately NOT part of `attacks` (see FamiliarAttack.h), so evolveTo() rebuilding that
	// vector from the new stage's base stats never loses purchased progress. Per current design,
	// these only ever apply to auto-attacks (see getEffectiveCooldown / resolveAttack) - ShopTab
	// upgrades are for a familiar's auto-attack, not its manually-activated abilities.
	BigNumber bonusPhysicalPower = BigNumber(0);
	ElementalPower bonusElementalPower;
	float bonusCooldownReduction = 0.0f;

	Sprites* sprites;
	DPSTracker familiarDPSTracker;

	std::vector<FamiliarAttack> attacks; // runtime instances, built from FamiliarType::attacks at construction/evolveTo

	std::vector<Projectile> activeProjectiles; // projectiles currently in flight, from any ranged attack
	std::vector<Projectile> activeEffects; // non-traveling visual effects currently playing (e.g. a wind-slice overlay) - see FamiliarAttack::spawnsEnemyEffect. Reuses Projectile purely as "an animated sprite that plays at a fixed point for a duration" - these never deal damage and never trigger onDamageDealt
	std::function<void(BigNumber damage, ElementMask mask)> onDamageDealt; // fired once per landed hit - melee-instant or projectile-arrival alike - mask is 0 for a purely physical hit
	std::function<double(Elements)> elementalMultiplierResolver; // resolves ONE element's own weakness/resistance multiplier against whatever enemy is currently on the field. Falls back to 1.0 (no bonus/penalty) if never set or no enemy is present.

	// True while the sprite's current one-shot animation belongs to a MANUAL (active) attack
	// rather than an auto-attack - see startAttack()'s preemption rule just below. Stays false
	// whenever the sprite is idle/free, so it never needs resetting on its own: the next
	// successful playAnimation() call (auto or manual) always sets it fresh.
	bool currentOneShotIsManual = false;

	// radial cooldown indicator (Donut, fills clockwise from the top; countdown text in the middle)
	// visualizes the first isAutoAttack entry's cooldown, if this familiar has one
	Text* cooldownText;
	sf::VertexArray cooldownRingTrack;    // full ring drawn underneath, always visible as a dim background track
	sf::VertexArray cooldownRingProgress; // partial ring on top, grows as the cooldown elapses
	static constexpr float cooldownRingInnerRadius = 20.0f; // the "hole" in the middle of the Donut
	static constexpr float cooldownRingOuterRadius = 28.0f;
	static constexpr int cooldownRingSegments = 48;          // resolution for a full 360 degree ring
	static constexpr float cooldownRingOffsetY = -80.0f;     // placed above the sprite

	// Which row this familiar currently occupies on screen (0-based) and how many familiars are
	// currently sharing the stack with it - defaults to "alone in a stack of 1" (today's single-
	// familiar position) until Game actually assigns this familiar to a slot and calls
	// setSlotPosition() with its real values. Stored rather than passed around each time because
	// onResize() needs to re-apply the SAME position after a window resize, without Game having to
	// hand it back in every time.
	int currentSlotIndex = 0;
	int currentTotalSlots = 1;
	void applyStackedPosition(); // positions sprites (and the cooldown ring center) from currentSlotIndex/currentTotalSlots - shared by setSlotPosition() and onResize()

	sf::Vector2f restPosition = { 60.0f, 0.0f }; // this familiar's normal on-screen spot, as last set by applyStackedPosition() - self-movement (below) temporarily offsets from this rather than overwriting it outright
	void updateSelfMovement(); // each frame, lifts the sprite off restPosition if some currently-playing attack calls for it (see FamiliarAttack::riseStartFrame etc.) - otherwise keeps it at restPosition

	void initSprites(std::string idleAnimationAddress, float idleFrameTimerMax, sf::Vector2u spriteSize, const std::vector<FamiliarAttack>& attackList);
	void setupAttacks(std::vector<FamiliarAttack> attackList); // allocates projectile textures for ranged attacks, takes ownership into `attacks`
	void initCooldownIndicator();
	void updateCooldownIndicator();
	static void buildRingArc(sf::VertexArray& target, sf::Vector2f center, float innerRadius, float outerRadius, float startAngleDeg, float sweepAngleDeg, int maxSegments, sf::Color color);

	// Triggers one activation of `attack`: resets its cooldown and begins its countdown to the
	// first shot (see FamiliarAttack::beginActivation) immediately, and - separately - decides
	// whether `attack` gets to show its own one-shot animation right now:
	//   - A MANUAL (active) attack's animation is never interrupted by anything, including
	//     another manual attack - once one is playing, it always finishes.
	//   - An AUTO-attack's animation is never interrupted by anything either, BUT it CAN be
	//     preempted (cut short) by a manual attack activating - a manual ability is the
	//     "gameplay highlight" and always gets to be seen, even if a swipe was mid-flight.
	//   - An AUTO-attack never preempts anything; it only gets to show its animation when the
	//     sprite is completely free.
	// If `attack` has a loop (FamiliarAttack::hasLoop()) and gets to show its animation, that
	// animation's repeating frame range is armed here too, so it's ready the moment playback
	// reaches it - see FamiliarAttack.h's comment on loopStartFrame/loopEndFrame.
	// Either way, `attack`'s shot timing runs exactly on schedule regardless of whether it
	// actually gets to show its animation this time - its damage/projectile is never delayed or
	// lost, just not visible if its animation got skipped or preempted.
	void startAttack(FamiliarAttack& attack, sf::Vector2f targetPosition);

	// Resolves one shot of an attack's actual gameplay effect - melee damage lands, or a
	// projectile is spawned from attack.projectileSpawnOffset - called from update() each time
	// attack.tickActiveFrameCountdown() or attack.tickLoopFireCountdown() reports a shot is due.
	// For a multi-shot attack (FamiliarAttack::projectileCount > 1) this fires once per shot,
	// and turns off the attack's animation loop once attack.registerShotFired() reports none are
	// left, so its current pass finishes naturally instead of repeating again.
	void resolveAttack(FamiliarAttack& attack, sf::Vector2f targetPosition);

	void syncDPSTrackerWindow(); // keeps familiarDPSTracker's averaging window matched to the primary auto-attack's cooldown
	float getEffectiveCooldown(const FamiliarAttack& attack) const; // attack.baseCooldown minus bonusCooldownReduction (auto-attacks only), floored at attack.minCooldownLimit

public:
	Familiar(FamiliarType familiarType);
	~Familiar();

	void onResize();
	void setSlotPosition(int slotIndex, int totalActiveSlots); // tells this familiar which row (0-based) it occupies out of how many are currently stacked together on the left side - call whenever Game's slot assignment changes (a familiar is assigned/unassigned/evolved-in-place doesn't need this, but another familiar joining or leaving the stack does, since everyone's row count shifts)
	void update(float dt, sf::Vector2f targetPosition); // ticks all attack cooldowns, auto-fires any ready auto-attacks at targetPosition, advances projectiles/animations
	void draw(sf::RenderWindow* window);     // sprite + projectiles - depth-sorted with decorations
	void drawHUD(sf::RenderWindow* window);  // cooldown ring/text - HUD layer (familiar DPS is now a single combined line Game owns - see Game::activeFamiliarDPSText)

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
	FamiliarName getName() const; // which SPECIES - constant across evolutions, see the member comment above
	void setAge(Age age);
	Age getAge() const;           // which evolution STAGE - this is what changes when this familiar evolves

	void setBonusPhysicalPower(BigNumber value);
	void setBonusElementalPower(Elements element, BigNumber value); // restores one element's saved bonus at a time
	void setBonusCooldownReduction(float value);
	float getBonusCooldownReduction() const;

	float getAttackCooldown(size_t attackIndex) const; // returns that attack's current EFFECTIVE cooldown (base - bonus, floored) - for HUD display

	void evolveTo(FamiliarType newType); // swaps sprites/attacks to a later evolution stage in place; bonusPhysicalPower/bonusElementalPower/bonusCooldownReduction/unlocked are preserved

	BigNumber getFamiliarDPS() const; // this familiar's own rolling DPS - Game sums this across every slotted familiar into one combined HUD line (see Game::updateActiveFamiliarDPSText())

	float getAttackCooldownRemaining(size_t attackIndex) const; // used in HUD

	// Pale silver-blue - this familiar's source-identity color. Used both for Game's combined
	// activeFamiliarDPSText (see Game::initActiveFamiliarDPSText()) and passed to
	// EnemySpawner::handleClick() by Game whenever this familiar lands a hit, so its floating
	// damage numbers on the enemy match that HUD line.
	static constexpr sf::Color dpsColor = sf::Color(190, 210, 235);
};
