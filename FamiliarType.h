#pragma once
#include "stdafx.h"
#include "PublicVariables.h"


struct FamiliarType
{
	FamiliarName name = BabyFireDragon;
	std::string label;
	std::string type;
	Age age = Baby;

	std::string idleAnimationAddress;
	float idleFrameTimerMax;
	std::string attack1AnimationAddress;
	float attack1FrameTimerMax;
	std::string attack2AnimationAddress;
	float attack2FrameTimerMax;
	sf::Vector2u spriteSize;

	// Projectile visuals for this familiar's auto-attack (fireball, ice shard, etc.)
	// Kept here, alongside the idle/attack sprite info, so adding a new familiar means
	// filling in one FamiliarType entry rather than touching Familiar/Projectile code.
	std::string projectileAnimationAddress;
	float projectileFrameTimerMax;
	sf::Vector2u projectileSpriteSize;
	int projectileFrameCount; // how many frames the projectile spritesheet has - passed explicitly to Textures (see Familiar::initProjectileAnimation)
	float projectileScale;    // on-screen size multiplier for the projectile sprite (spriteSize is the source frame in pixels, this scales it up/down)
	float projectileTravelDuration; // seconds for the projectile to cross the game area, from familiar to enemy

	FamiliarType(FamiliarName name, std::string label, std::string type, Age age, std::string idleAnimationAddress, float idleFrameTimerMax, std::string attack1AnimationAddress, float attack1FrameTimerMax, std::string attack2AnimationAddress, float attack2FrameTimerMax,
		sf::Vector2u spriteSize,std::string projectileAnimationAddress, float projectileFrameTimerMax, sf::Vector2u projectileSpriteSize, int projectileFrameCount, float projectileScale, float projectileTravelDuration)
		: name(name), label(label), type(type), age(age), idleAnimationAddress(idleAnimationAddress), idleFrameTimerMax(idleFrameTimerMax), attack1AnimationAddress(attack1AnimationAddress), attack1FrameTimerMax(attack1FrameTimerMax),
		attack2AnimationAddress(attack2AnimationAddress), attack2FrameTimerMax(attack2FrameTimerMax), spriteSize(spriteSize),projectileAnimationAddress(projectileAnimationAddress), projectileFrameTimerMax(projectileFrameTimerMax),
		projectileSpriteSize(projectileSpriteSize), projectileFrameCount(projectileFrameCount), projectileScale(projectileScale), projectileTravelDuration(projectileTravelDuration)
	{

	}

	~FamiliarType()
	{

	};
};