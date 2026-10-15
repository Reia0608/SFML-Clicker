#include "stdafx.h"
#include "Projectile.h"
#include <cmath>

Projectile::Projectile(sf::Vector2f startPosition, sf::Vector2f targetPosition, float travelDuration, BigNumber physicalDamage, ElementalPower elementalPower, const Textures& textureTemplate, float scale, ProjectileMotion motion, float motionAmplitude, float motionCycles)
	: texture(std::make_unique<Textures>(textureTemplate)), sprite(*texture),
	startPosition(startPosition), targetPosition(targetPosition), travelDuration(travelDuration),
	physicalDamage(physicalDamage), elementalPower(elementalPower),
	motion(motion), motionAmplitude(motionAmplitude), motionCycles(motionCycles)
{
	texture->resetAnimation(); // start this projectile's own copy at frame 0, independent of the template's playback position

	sf::Vector2u frameSize = texture->getFrameSize();
	sprite.setOrigin({ frameSize.x / 2.0f, frameSize.y / 2.0f });
	sprite.setScale({ scale, scale });
	sprite.setTextureRect(sf::IntRect(texture->getTextureCoord(), sf::Vector2i(frameSize)));

	// Tilt the sprite to face the target. Flight is along the overall start->target line, so
	// the angle is constant for the whole trip even with a Bobbing/GroundArc wobble on top -
	// computed once here rather than every update() call. Assumes the source artwork faces
	// right (0 degrees) at rest, matching fire_bolt_small.png.
	sf::Vector2f direction = targetPosition - startPosition;
	float angleDeg = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
	sprite.setRotation(sf::degrees(angleDeg));

	sprite.setPosition(startPosition);
}

Projectile::Projectile(const Projectile& other)
	: texture(std::make_unique<Textures>(*other.texture)), sprite(other.sprite),
	startPosition(other.startPosition), targetPosition(other.targetPosition),
	travelDuration(other.travelDuration), elapsed(other.elapsed),
	physicalDamage(other.physicalDamage), elementalPower(other.elementalPower),
	motion(other.motion), motionAmplitude(other.motionAmplitude), motionCycles(other.motionCycles)
{
	sprite.setTexture(*texture); // rebind the copied sprite to THIS projectile's own texture copy, not the source's
}

Projectile& Projectile::operator=(const Projectile& other)
{
	if (this != &other)
	{
		texture = std::make_unique<Textures>(*other.texture);
		sprite = other.sprite;
		sprite.setTexture(*texture);
		startPosition = other.startPosition;
		targetPosition = other.targetPosition;
		travelDuration = other.travelDuration;
		elapsed = other.elapsed;
		physicalDamage = other.physicalDamage;
		elementalPower = other.elementalPower;
		motion = other.motion;
		motionAmplitude = other.motionAmplitude;
		motionCycles = other.motionCycles;
	}
	return *this;
}

bool Projectile::update(float dt)
{
	elapsed += dt;
	float t = travelDuration > 0.0f ? elapsed / travelDuration : 1.0f;

	// Advance this projectile's own texture copy. Because it's a copy (not shared
	// with sibling projectiles), calling frameRight() here only ever affects this
	// one projectile's animation.
	texture->setFrameTimer(texture->getFrameTimer() - dt);
	if (texture->getFrameTimer() <= 0.0f)
	{
		texture->frameRight();
		sprite.setTextureRect(sf::IntRect(texture->getTextureCoord(), sf::Vector2i(texture->getFrameSize())));
		texture->setFrameTimer(texture->getFrameTimerMax());
	}

	if (t >= 1.0f)
	{
		sprite.setPosition(targetPosition); // lands exactly on target - no wobble offset on arrival
		return true;
	}

	sf::Vector2f position = startPosition + (targetPosition - startPosition) * t;

	if (motion != ProjectileMotion::Linear)
	{
		// A sideways offset, perpendicular to the overall start->target line, added on top of
		// the straight-line position above. Bobbing repeats this as a full sine wave (several
		// oscillations over the flight); GroundArc uses one half-wave bump instead - zero at
		// launch and at impact, peaking in the middle - so the curve reads as one deliberate
		// swerve rather than a repeating wiggle.
		sf::Vector2f pathVector = targetPosition - startPosition;
		float pathLength = std::sqrt(pathVector.x * pathVector.x + pathVector.y * pathVector.y);
		if (pathLength > 0.0001f)
		{
			sf::Vector2f perpendicular(-pathVector.y / pathLength, pathVector.x / pathLength);
			const float pi = 3.14159265f;
			float offset = motion == ProjectileMotion::Bobbing
				? motionAmplitude * std::sin(t * motionCycles * 2.0f * pi)
				: motionAmplitude * std::sin(t * pi);
			position += perpendicular * offset;
		}
	}

	sprite.setPosition(position);
	return false;
}

void Projectile::draw(sf::RenderWindow* window) const
{
	window->draw(sprite);
}

BigNumber Projectile::getPhysicalDamage() const
{
	return physicalDamage;
}

const ElementalPower& Projectile::getElementalPower() const
{
	return elementalPower;
}
