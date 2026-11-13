#pragma once
#include "stdafx.h"
#include "PublicVariables.h"
#include "Elements.h"
#include "FamiliarAttack.h"
#include "FamiliarPassive.h"
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

	// This stage's passive ability (Burn, Soaked, Keen Gale, ...) - default-constructed (empty
	// name) means this stage has no passive at all. Set via setPassive() below, chainable the same
	// way EnemyType's own setElementalAffinity()/setName() are, so existing FamiliarType(...) call
	// sites that don't need one don't have to change.
	FamiliarPassive passive;

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

	// Chainable so a passive can be attached right where a FamiliarType is defined, e.g.:
	//   FamiliarType(FireDragon, "Baby Fire Dragon", ...).setPassive({ .name = "Burn", ... })
	FamiliarType& setPassive(FamiliarPassive newPassive)
	{
		passive = std::move(newPassive);
		return *this;
	}
};