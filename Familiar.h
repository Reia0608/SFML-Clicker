#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include "Sprites.h"
#include "Textures.h"
#include "FamiliarType.h"
#include "Text.h"
#include "DPSTracker.h"
#include <string>
#include "Projectile.h"
#include <vector>
#include <functional>


// The first familiar: a baby fire dragon, specializing in physical + fire damage.
// Stays on the left side of the game area and auto-attacks the current enemy every
// "attackInterval" seconds. Manually-activated special abilities (freeze boss timer,
// fire DoT, etc.) come later via a separate FamiliarAbility class, once evolutions
// and the dedicated Familiar tab exist - this class intentionally stays minimal for now.
class Familiar
{
private:
	FamiliarName name = BabyFireDragon;
	std::string label = "no_name";
	std::string type = "no_type";
	BigNumber physicalAttackPower = BigNumber(5);
	BigNumber elementalAttackPower = BigNumber(3);
	float attackInterval = 3.0f;          // seconds between auto-attacks, upgradable later
	float attackCooldownRemaining = 0.0f;
	float AbilityCooldown = 10.0f;
	bool unlocked = false;

	Sprites* sprites;
	DPSTracker familiarDPSTracker;

	// "Template" texture for this familiar's projectile animation (fireball, ice shard, etc.),
	// loaded once from FamiliarType's projectile asset info. Never drawn or animated directly -
	// each Projectile copies it on creation so in-flight projectiles animate independently
	// (see Projectile.h for why a shared Textures* can't be animated by multiple owners at once).
	Textures* projectileTexture = nullptr;
	float projectileFrameTimerMax = 0.1f;
	float projectileScale = 1.0f;            // on-screen size multiplier for the projectile sprite
	float projectileTravelDuration = 1.0f;   // seconds for the projectile to cross the game area

	std::vector<Projectile> activeProjectiles; // fireballs currently in flight
	std::function<void(BigNumber damage)> onProjectileHit; // Game hooks this up to actually apply damage to the enemy

	// radial cooldown indicator (Donut, fills clockwise from the top; countdown text in the middle)
	Text* cooldownText;
	Text* familiarDPSText;
	sf::VertexArray cooldownRingTrack;    // full ring drawn underneath, always visible as a dim background track
	sf::VertexArray cooldownRingProgress; // partial ring on top, grows as the cooldown elapses
	static constexpr float cooldownRingInnerRadius = 20.0f; // the "hole" in the middle of the Donut
	static constexpr float cooldownRingOuterRadius = 28.0f;
	static constexpr int cooldownRingSegments = 48;          // resolution for a full 360 degree ring
	static constexpr float cooldownRingOffsetY = -80.0f;     // placed above the sprite

	void initSprites(std::string idleAnimationAddress, float idleFrameTimerMax, std::string attack1AnimationAddress, float attack1FrameTimerMax, sf::Vector2u spriteSize);
	void initFamiliarDPSText();
	void initProjectileAnimation(std::string projectileAnimationAddress, sf::Vector2u projectileSpriteSize, float projectileFrameTimerMax, int projectileFrameCount, float projectileScale, float projectileTravelDuration);
	void initCooldownIndicator();
	void updateCooldownIndicator();
	static void buildRingArc(sf::VertexArray& target, sf::Vector2f center, float innerRadius, float outerRadius, float startAngleDeg, float sweepAngleDeg, int maxSegments, sf::Color color);
	void attackAnimation(); // plays the attack animation and returns to idle once finished

public:
	Familiar(FamiliarType familiarType);
	~Familiar();

	void onResize();
	void update(float dt);
	void draw(sf::RenderWindow* window);

	float getGroundY() const { return sprites->getGroundY(); } // Y of the familiar's feet on the ground - Stage uses this to depth-sort it against decorations

	bool isUnlocked() const;
	void unlock(); // called once from ShopTab's purchase, or from Game::loadFromFile() to restore a saved unlock

	bool isAttackReady() const;   // true once every attackInterval seconds, consumed by Game::update()
	void triggerAttack(sf::Vector2f targetPosition); // launches a projectile at targetPosition, plays the attack animation, resets the cooldown
	void setOnProjectileHitCallback(std::function<void(BigNumber damage)> callback); // called once per projectile, on arrival
	BigNumber rollDamage() const; // physical + elemental combined - no elemental resistances on enemies yet

	BigNumber getPhysicalAttackPower() const;
	BigNumber getElementalAttackPower() const;
	void increasePhysicalAttack(BigNumber amount);
	void increaseElementalAttack(BigNumber amount);
	void reduceAttackCooldown(float amount);

	// for save/ load
	void setName(FamiliarName name);
	FamiliarName getName() const;

	void setPhysicalAttackPower(BigNumber value);
	void setElementalAttackPower(BigNumber value);

	float getAttackInterval() const;
	void setAttackInterval(float seconds); // for later upgrades ("attack interval" reduction) and save/load restore

	void updateFamiliarDPSText(); // public so ShopTab can access it to update
	BigNumber getFamiliarDPS();
};