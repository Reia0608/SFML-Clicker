#include "stdafx.h"
#include "Projectile.h"
#include <cmath>

Projectile::Projectile(sf::Vector2f startPosition, sf::Vector2f targetPosition, float travelDuration, BigNumber physicalDamage, ElementalPower elementalPower, const Textures& textureTemplate, float scale)
	: texture(std::make_unique<Textures>(textureTemplate)), sprite(*texture),
	startPosition(startPosition), targetPosition(targetPosition), travelDuration(travelDuration),
	physicalDamage(physicalDamage), elementalPower(elementalPower)
{
	texture->resetAnimation(); // start this projectile's own copy at frame 0, independent of the template's playback position

	sf::Vector2u frameSize = texture->getFrameSize();
	sprite.setOrigin({ frameSize.x / 2.0f, frameSize.y / 2.0f });
	sprite.setScale({ scale, scale });
	sprite.setTextureRect(sf::IntRect(texture->getTextureCoord(), sf::Vector2i(frameSize)));

	// Tilt the sprite to face the target. Flight is a straight line, so the angle is
	// constant for the whole trip - computed once here rather than every update() call.
	// Assumes the source artwork faces right (0 degrees) at rest, matching fire_bolt_small.png.
	sf::Vector2f direction = targetPosition - startPosition;
	float angleDeg = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265f;
	sprite.setRotation(sf::degrees(angleDeg));

	sprite.setPosition(startPosition);
}

Projectile::Projectile(const Projectile& other)
	: texture(std::make_unique<Textures>(*other.texture)), sprite(other.sprite),
	startPosition(other.startPosition), targetPosition(other.targetPosition),
	travelDuration(other.travelDuration), elapsed(other.elapsed),
	physicalDamage(other.physicalDamage), elementalPower(other.elementalPower)
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
		sprite.setPosition(targetPosition);
		return true;
	}

	sprite.setPosition(startPosition + (targetPosition - startPosition) * t);
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