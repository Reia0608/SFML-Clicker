#pragma once
#include "Elements.h"
#include <array>

// Per-element damage multipliers for one enemy/boss type.
// 1.0 = neutral (default for every element), 2.0 = weak (takes double), 0.5 = resistant
// (takes half). Not limited to those two values - use whatever fits (0.0 = immune,
// 1.5 = minor weakness, 0.25 = heavily resistant, etc).
//
// Only the *elemental* portion of an attack is scaled. Physical damage always lands in full,
// so an attack's total damage against a target is:
//     physicalAttack + elementalAttack * affinity.getMultiplierForMask(attackElementMask)
//
// Usage (EnemyType.h / EnemySpawner.cpp), builder-style so existing EnemyType entries
// don't need to change unless they actually have a weakness/resistance to set:
//     EnemyType("Slime1/...", ...)
//         .setElementalAffinity(ElementalAffinity().setWeakness(Fire));
//
//     EnemyType("Slime3/...", ...) // lava slime
//         .setElementalAffinity(ElementalAffinity().setResistance(Fire).setWeakness(Water));
class ElementalAffinity
{
private:
	std::array<double, ElementCount> multipliers;

public:
	ElementalAffinity()
	{
		multipliers.fill(1.0); // neutral to everything by default
	}

	// Chainable, so a full affinity profile can be built inline where an EnemyType is defined.
	ElementalAffinity& setMultiplier(Elements element, double multiplier)
	{
		multipliers[static_cast<size_t>(element)] = multiplier;
		return *this;
	}

	ElementalAffinity& setWeakness(Elements element, double multiplier = 2.0)
	{
		return setMultiplier(element, multiplier);
	}

	ElementalAffinity& setResistance(Elements element, double multiplier = 0.5)
	{
		return setMultiplier(element, multiplier);
	}

	ElementalAffinity& setImmunity(Elements element)
	{
		return setMultiplier(element, 0.0);
	}

	double getMultiplier(Elements element) const
	{
		return multipliers[static_cast<size_t>(element)];
	}

	// Resolves the multiplier for an attack that may carry several elements at once
	// (e.g. a future "Steam" ability tagged Fire | Water) by averaging the per-element
	// multipliers. A single-element attack (the common case today) just returns that
	// element's multiplier unchanged. No element (mask == 0, pure physical) returns 1.0,
	// since there's no elemental portion to scale.
	double getMultiplierForMask(ElementMask mask) const
	{
		if (mask == 0) return 1.0;

		double sum = 0.0;
		int count = 0;
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			Elements element = static_cast<Elements>(i);
			if (mask & elementBit(element))
			{
				sum += multipliers[i];
				count++;
			}
		}
		return count > 0 ? sum / count : 1.0;
	}
};