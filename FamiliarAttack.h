#pragma once
#include "stdafx.h"
#include "Textures.h"
#include "Projectile.h"
#include "BigNumber.h"
#include "ElementalPower.h"
#include <string>
#include <algorithm>
#include <functional>

class Familiar; // forward declaration only - customEffect below just needs the type name, and Familiar.h already includes this header so a back-include would be circular

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
// Familiar::resolveAttack / getEffectiveCooldown). That split is what lets evolveTo() safely
// replace this whole struct wholesale without losing anything the player paid for.
//
// Built from FamiliarType's attack lists (see Game.h) using C++20 designated initializers
// (e.g. FamiliarAttack{ .name = "Claw Swipe", .baseCooldown = 7.0f, ... }) - every field not
// named in a given entry just keeps its default below. That's why every default here has to
// make sense on its own: a melee attack can skip every ranged-only field and still be valid,
// and nobody has to restate fields that don't apply to their attack.
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

	bool isRanged = false;             // true: spawns a projectile, damage lands on arrival. false: melee, damage lands on this attack's activeFrame below

	// only used when isRanged is true:
	std::string projectileAnimationAddress;
	sf::Vector2u projectileSpriteSize;
	float projectileFrameTimerMax = 0.1f;
	int projectileFrameCount = 0;
	float projectileScale = 1.0f;
	float projectileTravelDuration = 1.0f;

	// How this attack's projectile travels - see ProjectileMotion in Projectile.h. Linear (the
	// default) is every attack's behavior before this existed: a straight line at constant speed.
	ProjectileMotion projectileMotion = ProjectileMotion::Linear;
	float projectileMotionAmplitude = 0.0f; // Bobbing: how far side-to-side, in pixels. GroundArc: how far the curve bulges. Unused for Linear.
	float projectileMotionCycles = 3.0f;    // Bobbing only: how many oscillations over the flight. Unused for Linear/GroundArc.

	// Which frame of THIS attack's own animation (animationKey/animationAddress above),
	// counting from 1, is the moment the attack's FIRST shot actually happens: for melee, the
	// frame damage lands on; for ranged, the frame the first projectile is spawned on.
	// Everything before it is windup with no gameplay effect yet. Defaults to 1 - "the effect
	// happens immediately on activation" - which is every attack's behavior before this field
	// existed, so any attack that doesn't set this is unaffected. See
	// Familiar::startAttack/resolveAttack for how this is actually used - the short version is
	// that it's a plain timer (activeFrame-1)*animationFrameTimerMax seconds long, counted
	// independently of whatever the sprite is actually showing at the time, so this attack's
	// damage/projectile still lands exactly on schedule even on a frame where some OTHER
	// attack's animation is the one actually visible (see the no-interrupt rule in
	// Familiar::startAttack).
	unsigned int activeFrame = 1;

	// Where a ranged attack's projectile actually appears, as an offset in pixels from this
	// familiar's own on-screen position (which sits at the sprite's center - see
	// Sprites::addAnimation). Needed because spawning dead-center looks like the projectile is
	// firing out of the familiar's chest rather than its mouth/claws - and because a bigger
	// evolution stage's sprite needs a bigger offset than its baby stage's. Ignored for melee
	// attacks. Defaults to (0,0) - today's old center-of-sprite behavior - until tuned per attack.
	sf::Vector2f projectileSpawnOffset = { 0.0f, 0.0f };

	// How many times this attack's effect fires per activation. 1 (the default) is every
	// attack's behavior before this field existed: exactly one melee hit or one projectile from
	// activeFrame above. A number greater than 1 makes this a multi-shot attack like Water
	// Bubbles' five projectiles - the first shot still fires on activeFrame exactly as always;
	// every shot after that comes from the repeating loop below, which is required whenever
	// this is greater than 1 (see hasLoop()).
	unsigned int projectileCount = 1;

	// The repeating frame range (1-based, inclusive, same counting as activeFrame) this
	// attack's animation plays over and over - firing one more shot every time it comes back
	// around - until every one of projectileCount's shots has fired. Playback then continues on
	// past loopEndFrame (into an outro, or straight to the end of the sheet) and returns to idle
	// exactly like any other one-shot attack. loopStartFrame = 0 (the default) means "no loop" -
	// this attack is a single pass, exactly as before these fields existed. loopActiveFrame is
	// which frame WITHIN that range fires the shot each time around - defaults to
	// loopStartFrame itself (the very start of each pass) if left at 0 while a loop is configured.
	unsigned int loopStartFrame = 0;
	unsigned int loopEndFrame = 0;
	unsigned int loopActiveFrame = 0;

	// Lets this attack temporarily lift the familiar's own sprite off the ground while it's
	// playing - e.g. Bubble Stream's leap into the air. All four frame markers are 1-based,
	// same counting as activeFrame. riseEndFrame = 0 (the default) means "no self-movement" -
	// the familiar just stays put, exactly as before these fields existed. The sprite rises
	// smoothly from riseStartFrame to riseEndFrame, holds at liftHeight through
	// descendStartFrame (covering a firing loop in between, if this attack has one), then
	// smoothly descends back to the ground by descendEndFrame. Only moves the familiar up/down
	// (no horizontal shift) - see Familiar::updateSelfMovement for exactly how this is read.
	unsigned int riseStartFrame = 0;
	unsigned int riseEndFrame = 0;
	unsigned int descendStartFrame = 0;
	unsigned int descendEndFrame = 0;
	float liftHeight = 0.0f; // pixels risen at the peak - written as a positive "how high up" number; Familiar flips the sign since screen Y grows downward

	// Whether this attack's shot(s) also spawn a short-lived, non-traveling animated visual
	// effect AT the target - e.g. a wind-slice overlay appearing on the enemy - independent of
	// isRanged/melee above, which still governs where the actual DAMAGE comes from. Only used
	// when true; every attack that doesn't set this is unaffected.
	bool spawnsEnemyEffect = false;
	std::string enemyEffectAnimationAddress;
	sf::Vector2u enemyEffectSpriteSize;
	float enemyEffectFrameTimerMax = 0.1f;
	int enemyEffectFrameCount = 0;
	float enemyEffectScale = 1.0f;
	float enemyEffectDuration = 0.3f; // how long the overlay plays before disappearing
	sf::Vector2f enemyEffectSpawnOffset = { 0.0f, 0.0f }; // offset from the target position - e.g. to center it on the enemy's sprite rather than its exact click-point

	// Optional escape hatch for an attack whose effect doesn't fit melee/ranged/enemy-overlay
	// above at all - e.g. a future familiar ramming its own sprite into the enemy and back, or
	// one that shakes the whole screen for an earthquake. Left unset (nullptr) by every attack
	// today, including Wind Swipes, which uses the plain data-driven fields above instead. When
	// set, Familiar::resolveAttack() calls it once per shot, IN ADDITION TO the normal
	// damage/projectile/enemy-overlay handling above - so even a fully custom effect still gets
	// this struct's ordinary damage numbers, cooldown, and multi-shot timing for free.
	std::function<void(Familiar& familiar, const FamiliarAttack& attack, sf::Vector2f targetPosition)> customEffect;

	// Empty (the default) means this attack is available as soon as its familiar/stage has it -
	// true for every auto-attack, and for any manual attack that isn't meant to be gated.
	// Non-empty names an UnlockManager feature key: HUD will not build a button for this attack
	// (and treats it as if it didn't exist yet, same as an auto-attack) until
	// UnlockManager::isUnlocked(unlockFeatureKey) is true. The key is normally flipped on by a
	// FamiliarTab purchase button rather than directly by an achievement claim - see FamiliarTab
	// for the achievement-unlocks-the-shop-button / purchase-unlocks-the-attack two-step pattern.
	std::string unlockFeatureKey;

	// runtime-only state - always left at their defaults in FamiliarType's static attack lists;
	// only Familiar::setupAttacks() / startAttack() ever set these, on its own live copy
	float cooldownRemaining = 0.0f;
	Textures* projectileTexture = nullptr; // owning; allocated by Familiar when this attack is instantiated on a live familiar
	Textures* enemyEffectTexture = nullptr; // owning; allocated by Familiar when this attack is instantiated, only if spawnsEnemyEffect
	float activeFrameCountdown = -1.0f;    // seconds left until this attack's activeFrame fires; negative = nothing pending
	int shotsFiredThisActivation = 0;      // how many of projectileCount's shots have fired since the last beginActivation()
	float loopFireCountdown = -1.0f;       // seconds until the next loop-driven shot fires; negative = not currently mid-loop
	bool windupStarted = false;            // true once this cycle's auto-attack windup has begun (see Familiar::update) - reset false each time its cooldown resets, so the next cycle can start its own windup in turn

	bool isReady() const { return cooldownRemaining <= 0.0f; }
	// effectiveCooldown: this attack's actual current cooldown (baseCooldown minus any purchased
	// bonus reduction, floored at minCooldownLimit) - computed by Familiar::getEffectiveCooldown(),
	// since that requires the familiar-wide bonus this struct deliberately has no knowledge of.
	void resetCooldown(float effectiveCooldown) { cooldownRemaining = effectiveCooldown; }
	void tick(float dt) { if (cooldownRemaining > 0.0f) cooldownRemaining = std::max(0.0f, cooldownRemaining - dt); }

	// Seconds from this attack's activation to its activeFrame - 0 if activeFrame is 1 (or 0,
	// which would otherwise underflow since activeFrame is unsigned).
	float getActiveFrameDelay() const
	{
		return activeFrame > 1 ? static_cast<float>(activeFrame - 1) * animationFrameTimerMax : 0.0f;
	}

	void beginActiveFrameCountdown() { activeFrameCountdown = getActiveFrameDelay(); }

	// Ticks the countdown toward this attack's active frame. Returns true exactly once - the
	// frame it crosses zero - telling the caller (Familiar::update) to resolve this attack's
	// actual effect right now. Runs purely on time; never looks at the sprite.
	bool tickActiveFrameCountdown(float dt)
	{
		if (activeFrameCountdown < 0.0f) return false; // nothing pending
		activeFrameCountdown -= dt;
		if (activeFrameCountdown <= 0.0f)
		{
			activeFrameCountdown = -1.0f; // consumed - back to "nothing pending"
			return true;
		}
		return false;
	}

	bool hasLoop() const { return loopStartFrame > 0 && loopEndFrame >= loopStartFrame; }

	unsigned int getLoopActiveFrame() const { return loopActiveFrame > 0 ? loopActiveFrame : loopStartFrame; }

	// Seconds for one full pass through the loop range - how often a repeat shot comes due once looping.
	float getLoopPeriod() const
	{
		return hasLoop() ? static_cast<float>(loopEndFrame - loopStartFrame + 1) * animationFrameTimerMax : 0.0f;
	}

	// Starts a fresh activation of this attack: resets the shot counter and begins the
	// countdown to its first shot (fired from activeFrame, exactly as every attack has always
	// worked). Call this instead of beginActiveFrameCountdown() directly.
	void beginActivation()
	{
		shotsFiredThisActivation = 0;
		loopFireCountdown = -1.0f;
		beginActiveFrameCountdown();
	}

	// Call once a shot has just fired (from activeFrame or from the loop). Returns true if this
	// attack still owes more shots this activation, and arms loopFireCountdown for the next
	// one; false once projectileCount has been fully reached (or this attack has no loop to
	// repeat into), telling the caller the animation should stop re-wrapping its loop and
	// continue on instead.
	bool registerShotFired()
	{
		shotsFiredThisActivation++;
		if (!hasLoop() || shotsFiredThisActivation >= static_cast<int>(projectileCount))
		{
			loopFireCountdown = -1.0f;
			return false;
		}

		if (shotsFiredThisActivation == 1)
		{
			// From the single activeFrame shot just fired to the loop's own active frame being
			// reached for the first time - less than a full pass, unless they're the same frame
			// (e.g. Bubble Stream, where activeFrame IS the loop's start), in which case this
			// shot already WAS that frame, so the next one is a full pass away.
			unsigned int loopFrame = getLoopActiveFrame();
			loopFireCountdown = loopFrame > activeFrame
				? static_cast<float>(loopFrame - activeFrame) * animationFrameTimerMax
				: getLoopPeriod();
		}
		else
		{
			loopFireCountdown = getLoopPeriod(); // every shot after the second is exactly one full pass later
		}
		return true;
	}

	// Ticks the countdown to the next loop-driven shot. Returns true exactly once per shot that
	// comes due - the caller (Familiar::update) should resolve this attack's effect again right
	// now, same as tickActiveFrameCountdown() does for the first shot.
	bool tickLoopFireCountdown(float dt)
	{
		if (loopFireCountdown < 0.0f) return false;
		loopFireCountdown -= dt;
		if (loopFireCountdown <= 0.0f)
		{
			loopFireCountdown = -1.0f; // consumed - registerShotFired() (called right after) re-arms it if more shots remain
			return true;
		}
		return false;
	}

	bool hasSelfMovement() const { return riseEndFrame > 0; }

	// This attack's current vertical lift (positive = up on screen), given which frame its
	// animation is currently sitting on. 0 outside the rise/hold/descend window, or if this
	// attack has no self-movement at all.
	float getSelfMovementLift(unsigned int currentFrame) const
	{
		if (!hasSelfMovement() || currentFrame <= riseStartFrame) return 0.0f;
		if (currentFrame < riseEndFrame)
		{
			float t = static_cast<float>(currentFrame - riseStartFrame) / static_cast<float>(riseEndFrame - riseStartFrame);
			return liftHeight * t;
		}
		if (currentFrame <= descendStartFrame) return liftHeight; // holding airborne
		if (currentFrame < descendEndFrame)
		{
			float t = static_cast<float>(currentFrame - descendStartFrame) / static_cast<float>(descendEndFrame - descendStartFrame);
			return liftHeight * (1.0f - t);
		}
		return 0.0f; // landed
	}
};
