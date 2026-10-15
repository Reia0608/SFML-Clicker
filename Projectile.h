#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include "Elements.h"
#include "ElementalPower.h"
#include "Textures.h"
#include <memory>

// How a ranged attack's projectile travels from its spawn point to the target - see
// Projectile::update() for exactly how each one is computed.
enum class ProjectileMotion
{
	Linear,    // straight line, constant speed - every attack's behavior before this existed
	Bobbing,   // travels toward the target in a straight line overall, but oscillates side to
	           // side across its own path as it goes, like Water Bubbles' projectiles
	GroundArc  // travels toward the target along one smooth curve off the straight line,
	           // rather than flying straight at it - e.g. a tornado skittering sideways
};

// A simple traveling attack effect (fireball, arrow, etc.) that moves from its start position
// to a target position and delivers its damage on arrival. See ProjectileMotion above for the
// shape of that movement - every motion still takes exactly travelDuration seconds end to end.
//
// IMPORTANT: Textures::frameRight()/getTextureCoord() store the current frame as
// mutable state ON THE TEXTURES OBJECT ITSELF (see Textures.h). That's fine for a
// single animated entity like a Familiar's idle sprite, but a familiar can have many
// projectiles in flight at once, all animating independently. If they shared one
// Textures* and each called frameRight(), they'd stomp on each other's frame index.
//
// So each Projectile owns a small COPY of the Textures object (constructed from the
// Familiar's already-loaded "template" Textures - see Familiar::projectileTexture).
// This copies GPU texture data, not the file, so there's no disk I/O per shot, but
// each projectile still gets its own independent, uncorrupted frame timer/index.
class Projectile
{
private:
	std::unique_ptr<Textures> texture; // owned copy, independent frame state per projectile - MUST stay declared before sprite (init order)
	sf::Sprite sprite;

	sf::Vector2f startPosition;
	sf::Vector2f targetPosition;
	float travelDuration; // seconds to cross from start to target
	float elapsed = 0.0f;
	BigNumber physicalDamage;
	ElementalPower elementalPower; // this projectile's elemental power, broken down per element - 0 active elements = physical only

	ProjectileMotion motion = ProjectileMotion::Linear;
	float motionAmplitude = 0.0f; // Bobbing: how far side-to-side it oscillates, in pixels. GroundArc: how far the curve bulges off the straight line, in pixels. Unused for Linear.
	float motionCycles = 0.0f;    // Bobbing only: how many full back-and-forth oscillations over the whole flight. Unused for Linear/GroundArc - GroundArc is always a single smooth bulge, not a repeating wave.

public:
	// textureTemplate: the Familiar's shared, already-loaded projectile animation asset.
	// Copied (not referenced) so this projectile can animate independently of its siblings.
	// scale: on-screen size multiplier applied to the sprite (source frame stays projectileSpriteSize pixels).
	// physicalDamage/elementalPower are kept separate (rather than pre-combined into one final
	// damage number) because each active element's own weakness/resistance multiplier is only
	// resolved on arrival, against whatever enemy is actually on the field at that moment - see
	// Familiar::update(). Keeping the full per-element breakdown (rather than a single pre-summed
	// elemental total) means a projectile carrying several elements at once still gets each one
	// scaled by its own multiplier on arrival, with no averaging.
	Projectile(sf::Vector2f startPosition, sf::Vector2f targetPosition, float travelDuration, BigNumber physicalDamage, ElementalPower elementalPower, const Textures& textureTemplate, float scale = 1.0f, ProjectileMotion motion = ProjectileMotion::Linear, float motionAmplitude = 0.0f, float motionCycles = 0.0f);
	Projectile(const Projectile& other);
	Projectile& operator=(const Projectile& other);
	Projectile(Projectile&&) noexcept = default;
	Projectile& operator=(Projectile&&) noexcept = default;

	bool update(float dt); // returns true the frame it reaches its target
	void draw(sf::RenderWindow* window) const;
	BigNumber getPhysicalDamage() const;
	const ElementalPower& getElementalPower() const;
};
