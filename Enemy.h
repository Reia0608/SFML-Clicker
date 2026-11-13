#pragma once
#include "stdafx.h"
#include "Sprites.h"
#include "EnemyType.h"
#include "Text.h"
#include "BigNumber.h"
#include "FloatingDamageText.h"
#include "Elements.h"
#include "Projectile.h"
#include <memory>
#include <functional>

class Enemy : public EnemyType
{
private:
	BigNumber health;
	float damageTextDuration = 2.0f; // Duration for which the damage text is displayed
	bool deathAnimationStarted = false;
	bool isBoss = false;
	float bossTimeRemaining = 0.0f;

	Sprites* sprites;
	Fonts* fonts;
	std::vector<std::unique_ptr<FloatingDamageText>> damageNumbers;

	sf::RectangleShape healthBarBackground;
	sf::RectangleShape healthBarFill;
	Text* healthText;

	sf::RectangleShape timerBarBackground;
	sf::RectangleShape timerBarFill;
	Text* timerText = nullptr;

	// One independently-running status effect currently active on this enemy (e.g. "Burn",
	// "Soaked" - see applyStatusEffect() below). Several different TYPES can be active at once
	// (e.g. burned AND poisoned), each running its own timer completely independently of the
	// others - but applying the SAME type again always refreshes this same entry in place rather
	// than adding a second one, since no status type stacks with itself today (a future stacking
	// type, if one's ever added, would need its own separate handling in applyStatusEffect()).
	struct ActiveStatusEffect
	{
		std::string type;      // e.g. "Burn" - what this status IS, and the no-stack key it's matched by
		std::string sourceKey; // which familiar applied this (its save-string, e.g. "FireDragon") - lets Game cancel it immediately the moment that familiar leaves its active slot (see clearStatusEffectsFrom())
		bool permanent = false;         // true = never expires on its own (e.g. Ancient Fire Dragon's burn) - remainingDuration is ignored while this is true
		float remainingDuration = 0.0f; // seconds left before this effect ends on its own - meaningless while permanent is true
		float tickInterval = 1.0f;      // seconds between each onTick() firing
		float tickCountdown = 0.0f;     // counts down to the next tick, independently of remainingDuration
		std::function<void()> onTick;   // whatever this status DOES each tick (damage, a visual, both) - fully defined by whoever applied it; this struct doesn't know or care what's inside

		// Damage-TAKEN modifier (e.g. Water Dragon's Soaked) - optional, only meaningful for a
		// status that changes how much damage THIS enemy takes from certain elements while it's
		// active, independent of whether it also ticks. damageMultiplierMask = 0 (the default)
		// means this status doesn't modify damage taken at all - every field below is then unused,
		// which is why Burn (a pure damage-OVER-TIME effect) never needs to touch these.
		ElementMask damageMultiplierMask = 0; // which element(s) this status boosts damage taken from
		double damageMultiplier = 1.0;        // the boost - multiplies ON TOP of the enemy's own weakness/resistance, never replaces it (unless forceNeutralAffinity below)
		bool forceNeutralAffinity = false;    // true: this enemy's own weakness/resistance/immunity to damageMultiplierMask's elements is ignored (treated as neutral 1.0x) while this is active, with damageMultiplier still applied on top - e.g. Ancient Water Dragon
	};
	std::vector<ActiveStatusEffect> activeStatusEffects;

	// Short-lived, non-traveling animated visuals currently playing on this enemy (e.g. a burning
	// overlay) - reuses Projectile purely as "an animated sprite that plays at a fixed point for a
	// duration", exactly the way Familiar::activeEffects already does for an attack's own
	// enemy-overlay effect (see FamiliarAttack::spawnsEnemyEffect). These never deal damage and are
	// purely cosmetic - playStatusEffectVisual() below is how a status effect's onTick spawns one.
	std::vector<Projectile> activeStatusVisuals;

	void initHealth(BigNumber maxHealth);
	void initFonts();
	void initHealthBar();

	void updateHealthBar();
	// baseColor is the source-identity color (Hero red / familiar blue / passive pink). For a
	// physical hit (elementMask == 0) it's the text's fill. For an elemental hit it becomes the
	// text's OUTLINE instead, while the fill switches to that element's gradient (see
	// Elements.h::getElementColorGradient) - so both "who hit" and "what element" stay visible
	// at once. Crit still overrides everything to solid orange fill / black outline.
	void showDamage(BigNumber damage, bool isCritical, sf::Color baseColor = sf::Color::Red, ElementMask elementMask = 0);
	void initBossTimer();
	void updateTimerBar();

public:
	Enemy(std::string spriteAddress, sf::Vector2u spriteSize, BigNumber maxHealth, float frameTimerMax,
		std::string hurtSpriteAddress, sf::Vector2u hurtSpriteSize, float hurtFrameTimerMax,
		std::string deathSpriteAddress, sf::Vector2u deathSpriteSize, float deathFrameTimerMax, BigNumber gold,
		bool isBoss = false, ElementalAffinity elementalAffinity = ElementalAffinity());
	virtual ~Enemy();

	BigNumber getHealth() const;
	const ElementalAffinity& getElementalAffinity() const; // weaknesses/resistances, inherited from EnemyType

	bool isClicked(sf::Vector2f mousePos) const;
	bool isDead() const;
	bool isDeathAnimationFinished() const;
	bool isShowingDamageText() const;

	Sprites* getSprites() const;
	bool getIsDeathAnimationStarted() const;

	// baseColor identifies the damage's source (Hero/familiar/passive) in its floating number if
	// elementMask is 0 (a purely physical hit); elementMask != 0 uses that element's gradient
	// instead (crit always overrides both to solid orange). Both default so existing non-familiar,
	// non-elemental call sites (Hero's own click) don't need to change.
	void takeDamage(BigNumber damage, bool isCritical, sf::Color baseColor = sf::Color::Red, ElementMask elementMask = 0);
	void setHealth(BigNumber newHealth); // debug only, clamps to >= 0
	void updateDamageText(float dt);
	void drawDamageText(sf::RenderWindow* window);
	void drawHealthBar(sf::RenderWindow* window);
	void refreshHealthBar();
	void loadDeathAnimation();

	bool getIsBoss() const;
	void updateBossTimer(float dt);
	bool isBossTimeUp() const;
	void drawBossTimer(sf::RenderWindow* window);

	float getBossTimeElapsed() const; // seconds since the boss encounter started (bossTimeLimit - bossTimeRemaining)

	// --- Status effects (Burn, Soaked, ...) ---
	// Starts (or refreshes) one status effect on this enemy. type identifies WHICH status this is
	// (e.g. "Burn") - applying the same type again while it's already active just resets its timer
	// and swaps in the new onTick/sourceKey, rather than stacking a second copy; a different type
	// is entirely independent and untouched by this. Pass permanent = true for an unlimited-
	// duration effect (duration is then ignored). onTick fires once every tickInterval seconds for
	// as long as this effect is active - it's the caller's own closure, so it decides everything
	// about what actually happens each tick (damage dealt, a visual played via
	// playStatusEffectVisual() below, or both) - this struct and its bookkeeping know nothing about
	// damage, elements, or textures.
	// damageMultiplierMask/damageMultiplier/forceNeutralAffinity are optional (default to "this
	// status doesn't modify damage taken") - see the ActiveStatusEffect member comments above for
	// what they do. Pass them for a status like Soaked; leave them at their defaults for a pure
	// tick effect like Burn.
	void applyStatusEffect(const std::string& type, const std::string& sourceKey, float duration, bool permanent, float tickInterval, std::function<void()> onTick,
		ElementMask damageMultiplierMask = 0, double damageMultiplier = 1.0, bool forceNeutralAffinity = false);

	// Immediately ends every active status effect that came from sourceKey (e.g. "FireDragon") -
	// call this the instant that familiar leaves its active slot, so a status effect never
	// outlives the familiar actually responsible for it, rather than quietly running out its own
	// timer after the familiar is gone.
	void clearStatusEffectsFrom(const std::string& sourceKey);

	bool hasStatusEffect(const std::string& type) const; // true while a status of this type is currently active, regardless of who applied it

	// This enemy's actual weakness/resistance multiplier for `element`, after folding in every
	// active status effect's damage-taken modifier (see ActiveStatusEffect's damageMultiplierMask
	// above) - this enemy's own base ElementalAffinity (getElementalAffinity()) multiplied by any
	// matching status effects' damageMultiplier, or forced to neutral (1.0x) first if any matching
	// status has forceNeutralAffinity set. This is what every damage calculation should actually
	// call (see EnemySpawner::getEnemyElementalMultiplierForElement()) - getElementalAffinity()
	// alone only ever reflects this enemy's permanent, built-in profile.
	double getEffectiveElementalMultiplier(Elements element) const;

	// Spawns a short, non-traveling visual at offset from this enemy's own position, playing once
	// before disappearing - the generic piece a status effect's onTick calls into to actually show
	// itself (e.g. Burn's lower-half flicker), so content code never has to touch
	// activeStatusVisuals directly.
	void playStatusEffectVisual(const Textures& textureTemplate, float scale, float duration, sf::Vector2f offset = { 0.0f, 0.0f });

	void updateStatusEffects(float dt); // ticks every active status effect's timer, firing onTick() whenever one's tickCountdown reaches 0, and expiring any whose remainingDuration has run out (never while permanent) - also advances activeStatusVisuals
	void drawStatusEffectVisuals(sf::RenderWindow* window) const; // world-layer draw for activeStatusVisuals - call alongside the enemy's own sprite draw

	// Everything about this enemy that can change frame-to-frame - used by frame-stepping (see
	// FrameStepHistory). Deliberately excludes damageNumbers (the floating "123!" popups) -
	// purely cosmetic, and FloatingDamageText has no capture/restore support of its own.
	struct State
	{
		BigNumber health;
		bool deathAnimationStarted = false;
		float bossTimeRemaining = 0.0f;
		std::vector<ActiveStatusEffect> activeStatusEffects;
		std::vector<Projectile> activeStatusVisuals;
		Sprites::State spritesState;
	};

	State captureState() const;
	void restoreState(const State& state);
};