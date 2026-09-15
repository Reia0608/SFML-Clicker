#include "stdafx.h"
#include "Familiar.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>


void Familiar::initSprites(std::string idleAnimationAddress, float idleFrameTimerMax, std::string attack1AnimationAddress, float attack1FrameTimerMax, sf::Vector2u spriteSize)
{
	sprites = new Sprites();
	sprites->addAnimation("idle", idleAnimationAddress, spriteSize, idleFrameTimerMax);
	sprites->addAnimation("attack", attack1AnimationAddress, spriteSize, attack1FrameTimerMax);
	sprites->setPosition({ 60.0f, windowSize.y / 2.0f }); // left side of the game area, facing the enemy. y position depends on unlocked amount of familiars, but for now we only have one familiar, so it's centered vertically
	sprites->playAnimation("idle");
}

void Familiar::initFamiliarDPSText()
{
	familiarDPSText = new Text("click damage: " + getFamiliarDPS().toString(), "Fonts/arial.ttf");
	familiarDPSText->setFillColor(sf::Color::Green);
	familiarDPSText->setOutlineColor(sf::Color::Black);
	familiarDPSText->setOutlineThickness(2.0f);
	updateFamiliarDPSText();
}

void Familiar::initProjectileAnimation(std::string projectileAnimationAddress, sf::Vector2u projectileSpriteSize, float projectileFrameTimerMax, int projectileFrameCount, float projectileScale, float projectileTravelDuration)
{
	this->projectileFrameTimerMax = projectileFrameTimerMax;
	this->projectileScale = projectileScale;
	this->projectileTravelDuration = projectileTravelDuration;

	// frameCount passed explicitly for clarity/robustness - Textures::Textures() auto-detects it
	// from sheet width / frame width when 0 is passed, but being explicit avoids relying on that
	// for sheets that might later include padding frames not meant for this animation.
	projectileTexture = new Textures(projectileAnimationAddress, projectileSpriteSize, projectileFrameCount);
	projectileTexture->setFrameTimerMax(projectileFrameTimerMax);
	projectileTexture->setFrameTimer(projectileFrameTimerMax);
	projectileTexture->setSmooth(false);
}

void Familiar::buildRingArc(sf::VertexArray& target, sf::Vector2f center, float innerRadius, float outerRadius, float startAngleDeg, float sweepAngleDeg, int maxSegments, sf::Color color) // New
{
	target.clear();
	target.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
	if (sweepAngleDeg <= 0.0f) return;

	int segments = std::max(1, static_cast<int>(maxSegments * (sweepAngleDeg / 360.0f))); // fewer segments for a smaller slice - cheap and still smooth
	const float pi = 3.14159265f;

	for (int i = 0; i <= segments; i++)
	{
		float t = static_cast<float>(i) / static_cast<float>(segments);
		float angleRad = (startAngleDeg + sweepAngleDeg * t) * pi / 180.0f;
		sf::Vector2f dir(std::cos(angleRad), std::sin(angleRad)); // screen Y is down, so increasing angle sweeps clockwise

		sf::Vertex outer;
		outer.position = center + dir * outerRadius;
		outer.color = color;

		sf::Vertex inner;
		inner.position = center + dir * innerRadius;
		inner.color = color;

		target.append(outer);
		target.append(inner);
	}
}

void Familiar::initCooldownIndicator()
{
	cooldownText = new Text("", "Fonts/arial.ttf");
	cooldownText->setCharacterSize(14);
	cooldownText->setFillColor(sf::Color::White);
	cooldownText->setOutlineColor(sf::Color::Black);
	cooldownText->setOutlineThickness(1.0f);

	sf::Vector2f center = sprites->getPosition() + sf::Vector2f(0.0f, cooldownRingOffsetY);
	buildRingArc(cooldownRingTrack, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f, cooldownRingSegments, sf::Color(50, 50, 50, 200));
}

void Familiar::updateCooldownIndicator()
{
	sf::Vector2f center = sprites->getPosition() + sf::Vector2f(0.0f, cooldownRingOffsetY);

	float progress = attackInterval > 0.0f ? 1.0f - (attackCooldownRemaining / attackInterval) : 1.0f; // 0 = just attacked, 1 = ready
	progress = std::clamp(progress, 0.0f, 1.0f);

	buildRingArc(cooldownRingProgress, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f * progress, cooldownRingSegments, sf::Color(240, 140, 50));

	std::ostringstream out;
	out << std::fixed << std::setprecision(1) << std::max(0.0f, attackCooldownRemaining);
	cooldownText->setString(out.str());

	sf::FloatRect textBounds = cooldownText->getLocalBounds();
	cooldownText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f });
	cooldownText->setPosition(center);
}

void Familiar::attackAnimation()
{
	sprites->playAnimation("attack", true, "idle");   // plays once then returns to the idle animation
}

void Familiar::syncDPSTrackerWindow()
{
	familiarDPSTracker.setWindowDuration(attackInterval);
}

Familiar::Familiar(FamiliarType familiarType)
{
	name = familiarType.name;
	std::string label = familiarType.label;
	std::string type = familiarType.type;
	// TODO needs to be extend later on with more Attributes
	initSprites(familiarType.idleAnimationAddress, familiarType.idleFrameTimerMax, familiarType.attack1AnimationAddress, familiarType.attack1FrameTimerMax, familiarType.spriteSize);
	initProjectileAnimation(familiarType.projectileAnimationAddress, familiarType.projectileSpriteSize, familiarType.projectileFrameTimerMax, familiarType.projectileFrameCount, familiarType.projectileScale, familiarType.projectileTravelDuration);
	initCooldownIndicator();
	initFamiliarDPSText();
	syncDPSTrackerWindow();
}

Familiar::~Familiar()
{
	delete sprites;
	delete cooldownText;
	delete projectileTexture;
	delete familiarDPSText;
}

void Familiar::onResize()
{
	sprites->setPosition({ 60.0f, windowSize.y / 2.0f });

	sf::Vector2f center = sprites->getPosition() + sf::Vector2f(0.0f, cooldownRingOffsetY);
	buildRingArc(cooldownRingTrack, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f, cooldownRingSegments, sf::Color(50, 50, 50, 200));

	updateFamiliarDPSText();
}

void Familiar::update(float dt)
{
	if (!unlocked) return;
	if (attackCooldownRemaining > 0.0f) attackCooldownRemaining -= dt;
	sprites->animate(dt);
	familiarDPSTracker.update(dt);
	updateFamiliarDPSText();

	for (auto it = activeProjectiles.begin(); it != activeProjectiles.end(); )
	{
		bool arrived = it->update(dt);
		if (arrived)
		{
			familiarDPSTracker.recordDamage(it->getDamage());
			if (onProjectileHit) onProjectileHit(it->getDamage()); // Game applies the damage to whatever is currently at that spot
			it = activeProjectiles.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void Familiar::draw(sf::RenderWindow* window)
{
	if (!unlocked) return;
	window->draw(*sprites);

	for (const auto& projectile : activeProjectiles)
	{
		projectile.draw(window);
	}
}

void Familiar::drawHUD(sf::RenderWindow* window)
{
	if (!unlocked) return;

	updateCooldownIndicator();
	window->draw(cooldownRingTrack);
	window->draw(cooldownRingProgress);
	window->draw(*cooldownText);
	window->draw(*familiarDPSText);
}

bool Familiar::isUnlocked() const
{
	return unlocked;
}

void Familiar::unlock()
{
	unlocked = true;
}

bool Familiar::isAttackReady() const
{
	return unlocked && attackCooldownRemaining <= 0.0f;
}

void Familiar::triggerAttack(sf::Vector2f targetPosition) // replaces the old resetAttackCooldown()
{
	attackCooldownRemaining = attackInterval;
	sprites->playAnimation("attack", true, "idle");

	activeProjectiles.emplace_back(sprites->getPosition(), targetPosition, projectileTravelDuration, rollDamage(), *projectileTexture, projectileScale);
}

void Familiar::setOnProjectileHitCallback(std::function<void(BigNumber damage)> callback)
{
	onProjectileHit = std::move(callback);
}

BigNumber Familiar::rollDamage() const
{
	return physicalAttackPower + elementalAttackPower;
}

BigNumber Familiar::getPhysicalAttackPower() const
{
	return physicalAttackPower;
}

BigNumber Familiar::getElementalAttackPower() const
{
	return elementalAttackPower;
}

void Familiar::increasePhysicalAttack(BigNumber amount)
{
	physicalAttackPower = physicalAttackPower + amount;
}

void Familiar::increaseElementalAttack(BigNumber amount)
{
	elementalAttackPower = elementalAttackPower + amount;
}

void Familiar::reduceAttackCooldown(float amount)
{
	attackInterval = std::max(minAttackInterval, attackInterval - amount);
	syncDPSTrackerWindow();
}

bool Familiar::isAttackIntervalAtMinimum() const
{
	return attackInterval <= minAttackInterval;
}

void Familiar::setName(FamiliarName name)
{
	this->name = name;
}

FamiliarName Familiar::getName() const
{
	return name;
}

void Familiar::setPhysicalAttackPower(BigNumber value)
{
	physicalAttackPower = value;
}

void Familiar::setElementalAttackPower(BigNumber value)
{
	elementalAttackPower = value;
}

float Familiar::getAttackInterval() const
{
	return attackInterval;
}

void Familiar::setAttackInterval(float seconds)
{
	attackInterval = seconds;
	syncDPSTrackerWindow();
}


void Familiar::updateFamiliarDPSText()
{
	familiarDPSText->setString("Familiar DPS: " + getFamiliarDPS().toString());
	LayoutManager::applyAnchor(*familiarDPSText, Anchor::BottomRight, { -10.0f, -120.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

BigNumber Familiar::getFamiliarDPS() const
{
	return familiarDPSTracker.getDPS();
}
