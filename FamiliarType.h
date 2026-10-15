#pragma once
#include "stdafx.h"
#include "PublicVariables.h"
#include "Elements.h"
#include "FamiliarAttack.h"
#include <vector>

struct FamiliarType
{
	FamiliarName name = FireDragon;
	std::string label;
	std::string type;
	Age age = Baby;
	// Note: no stage-level element here anymore - each FamiliarAttack in `attacks` carries its
	// own baseElementalPower, so different attacks of the same stage (or the same familiar
	// across future fusions) can use different elements, or several at once.

	std::string idleAnimationAddress;
	float idleFrameTimerMax;
	sf::Vector2u spriteSize;

	std::vector<FamiliarAttack> attacks; // every attack this stage can perform - auto and manual alike, each flagged via isAutoAttack

	FamiliarType(FamiliarName name, std::string label, std::string type, Age age,
		std::string idleAnimationAddress, float idleFrameTimerMax, sf::Vector2u spriteSize,
		std::vector<FamiliarAttack> attacks)
		: name(name), label(label), type(type), age(age),
		idleAnimationAddress(idleAnimationAddress), idleFrameTimerMax(idleFrameTimerMax), spriteSize(spriteSize),
		attacks(std::move(attacks))
	{

	}

	~FamiliarType()
	{

	};
};