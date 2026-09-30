#pragma once
#include "stdafx.h"
#include "BigNumber.h"
#include "ElementalAffinity.h"

struct EnemyType
{
	std::string spriteAddress; // sprite address for idle animation
	sf::Vector2u spriteSize;
	BigNumber maxHealth;
	BigNumber gold;
	float frameTimerMax = 0.0f;
	std::string hurtSpriteAddress;
	sf::Vector2u hurtSpriteSize;
	float hurtFrameTimerMax = 0.0f;
	std::string deathSpriteAddress;
	sf::Vector2u deathSpriteSize;
	float deathFrameTimerMax = 0.0f;
	ElementalAffinity elementalAffinity; // neutral (1.0x every element) unless set via setElementalAffinity()
	std::string name = "Enemy"; // display name shown in the enemy info UI, unless set via setName()
	// can be extended later: attackpower...

	EnemyType(std::string spriteAddress, sf::Vector2u spriteSize, BigNumber maxHealth, float frameTimerMax, std::string hurtSpriteAddress, sf::Vector2u hurtSpriteSize, float hurtFrameTimerMax, std::string deathSpriteAddress, sf::Vector2u deathSpriteSize, float deathFrameTimerMax, BigNumber gold)
		: spriteAddress(spriteAddress), spriteSize(spriteSize), maxHealth(maxHealth), frameTimerMax(frameTimerMax), hurtSpriteAddress(hurtSpriteAddress), hurtSpriteSize(hurtSpriteSize), hurtFrameTimerMax(hurtFrameTimerMax),
		deathSpriteAddress(deathSpriteAddress), deathSpriteSize(deathSpriteSize), deathFrameTimerMax(deathFrameTimerMax), gold(gold)
	{

	}

	~EnemyType()
	{

	};

	// Chainable so weaknesses/resistances can be attached right where an EnemyType (or the
	// boss type) is defined, e.g.:
	//   EnemyType("Slime1/...", ...).setElementalAffinity(ElementalAffinity().setWeakness(Fire))
	EnemyType& setElementalAffinity(ElementalAffinity affinity)
	{
		elementalAffinity = affinity;
		return *this;
	}

	// Chainable so a display name can be attached right where an EnemyType (or the boss type)
	// is defined, e.g.:
	//   EnemyType("Slime1/...", ...).setName("Green Slime")
	EnemyType& setName(std::string newName)
	{
		name = std::move(newName);
		return *this;
	}
};