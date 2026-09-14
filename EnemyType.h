#pragma once
#include "stdafx.h"
#include "BigNumber.h"

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
	// can be extended later: attackpower, Name...

	EnemyType(std::string spriteAddress, sf::Vector2u spriteSize, BigNumber maxHealth, float frameTimerMax, std::string hurtSpriteAddress, sf::Vector2u hurtSpriteSize, float hurtFrameTimerMax, std::string deathSpriteAddress, sf::Vector2u deathSpriteSize, float deathFrameTimerMax, BigNumber gold)
		: spriteAddress(spriteAddress), spriteSize(spriteSize), maxHealth(maxHealth), frameTimerMax(frameTimerMax), hurtSpriteAddress(hurtSpriteAddress), hurtSpriteSize(hurtSpriteSize), hurtFrameTimerMax(hurtFrameTimerMax), 
		deathSpriteAddress(deathSpriteAddress), deathSpriteSize(deathSpriteSize), deathFrameTimerMax(deathFrameTimerMax), gold(gold)
	{

	}

	~EnemyType()
	{

	};
};
