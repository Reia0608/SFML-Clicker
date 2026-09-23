#pragma once
#include "stdafx.h"
#include "Textures.h"
#include "BigNumber.h"
#include "ElementalPower.h"
#include <string>
#include <algorithm>

// One attack a familiar can perform - either fired automatically whenever its cooldown
// allows, or manually activated by the player via a HUD button. Every familiar/evolution
// stage defines its own independent list of these (see FamiliarType::attacks), so no two
// familiars - or even two stages of the same familiar - need behave alike: one might have
// a melee physical auto-attack and no ability at all; another a ranged mixed-damage
// auto-attack; another a melee auto-attack plus a ranged pure-elemental ability. Every
// combination of isAutoAttack / usesPhysicalDamage / usesElementalDamage / isRanged is valid.
//
// Every stat here is this attack's BASE value for its evolution stage - set once, in
// FamiliarType::attacks, and otherwise immutable at runtime. Purchased upgrades never touch
// these; they accumulate separately as Familiar::bonusPhysicalPower / bonusElementalPower /
// bonusCooldownReduction, which persist across evolutions and are combined with whichever
// attack is currently active at the moment damage/cooldown is resolved (see
// Familiar::executeAttack / getEffectiveCooldown). That split is what lets evolveTo() safely
// replace this whole struct wholesale without losing anything the player paid for.
//
// Field order matters here: all config fields come first, and the two runtime-only fields
// (cooldownRemaining, projectileTexture) are last. That's deliberate - this project targets
// C++17, so FamiliarType's attack lists (see Game.h) use plain positional aggregate
// initialization, not C++20 designated initializers. Keeping the runtime fields last means a
// melee attack's braced-init list can simply stop after isRanged and let every projectile
// field default, and every attack's list can omit the two runtime fields entirely.
struct FamiliarAttack
{
	std::string name;                  // e.g. "Claw Swipe", "Fireball"
	std::string animationKey;          // registered with Sprites (e.g. "attack1", "attack2") - must be unique per familiar stage
	std::string animationAddress;
	float animationFrameTimerMax = 0.1f;

	float baseCooldown = 1.0f;         // seconds between uses, before any purchased cooldown reduction
	float minCooldownLimit = 0.5f;     // hard floor for THIS attack's effective cooldown - purchased reduction can't push it below this.
	// Deliberately per-attack rather than a single game-wide constant: a later, harder-hitting
	// evolution stage's auto-attack might reasonably have both a higher base cooldown AND a
	// higher floor than an earlier stage's.

	bool isAutoAttack = true;          // true: fires automatically off cooldown every frame. false: only fires when the player activates it - what used to be called an "ability"

	bool usesPhysicalDamage = true;    // whether this attack deals any physical damage at all
	BigNumber basePhysicalPower = BigNumber(0); // this attack's own physical power at this evolution stage - ignored if usesPhysicalDamage is false

	bool usesElementalDamage = false;  // whether this attack deals any elemental damage at all
	ElementalPower baseElementalPower; // this attack's own per-element power at this evolution stage - e.g. ElementalPower().set(Fire, BigNumber(3)).
	// Which element(s) this attack is "tagged" with (for weakness/resistance) comes from
	// this ElementalPower's own mask - ignored if usesElementalDamage is false

	bool isRanged = false;             // true: spawns a projectile, damage lands on arrival. false: melee, damage lands instantly on activation

	// only used when isRanged is true:
	std::string projectileAnimationAddress;
	sf::Vector2u projectileSpriteSize;
	float projectileFrameTimerMax = 0.1f;
	int projectileFrameCount = 0;
	float projectileScale = 1.0f;
	float projectileTravelDuration = 1.0f;

	// runtime-only state - always left at their defaults in FamiliarType's static attack lists;
	// only Familiar::setupAttacks() ever sets these, on its own live copy
	float cooldownRemaining = 0.0f;
	Textures* projectileTexture = nullptr; // owning; allocated by Familiar when this attack is instantiated on a live familiar

	bool isReady() const { return cooldownRemaining <= 0.0f; }
	// effectiveCooldown: this attack's actual current cooldown (baseCooldown minus any purchased
	// bonus reduction, floored at minCooldownLimit) - computed by Familiar::getEffectiveCooldown(),
	// since that requires the familiar-wide bonus this struct deliberately has no knowledge of.
	void resetCooldown(float effectiveCooldown) { cooldownRemaining = effectiveCooldown; }
	void tick(float dt) { if (cooldownRemaining > 0.0f) cooldownRemaining = std::max(0.0f, cooldownRemaining - dt); }
};