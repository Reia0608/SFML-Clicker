#pragma once
#include "Elements.h"
#include "BigNumber.h"
#include <array>
#include <functional>

// Per-element attack power carried by a single FamiliarAttack (as its "base" power for that
// evolution stage), or accumulated as a familiar-wide "bonus" from ShopTab purchases (see
// Familiar::bonusElementalPower). Mirrors ElementalAffinity's sparse, array-backed shape so
// adding a new element (Elements.h) never requires touching every attack definition - an
// attack simply "uses" whichever elements it has been given power in via set()/add().
//
// Supports attacks that carry several elements at once (e.g. a future "Steam" ability =
// Fire + Water) without needing a separate bool flag per element.
//
// Usage (Game.h, inside a FamiliarAttack's brace-init list):
//     ElementalPower().set(Fire, BigNumber(3))
//     ElementalPower().set(Fire, BigNumber(2)).set(Water, BigNumber(2)) // e.g. a Steam ability
class ElementalPower
{
private:
	std::array<BigNumber, ElementCount> power;
	ElementMask activeMask = 0; // which elements this ElementalPower actually carries - tracked
	// explicitly (not derived from the amounts) so it can never
	// accidentally "activate" an element just because a bonus
	// happened to be added to it - see addMatching() below

public:
	ElementalPower()
	{
		power.fill(BigNumber(0));
	}

	// Chainable, so an attack's full elemental profile can be built inline where it's defined.
	ElementalPower& set(Elements element, BigNumber amount)
	{
		power[static_cast<size_t>(element)] = amount;
		activeMask |= elementBit(element);
		return *this;
	}

	ElementalPower& add(Elements element, BigNumber amount)
	{
		power[static_cast<size_t>(element)] = power[static_cast<size_t>(element)] + amount;
		activeMask |= elementBit(element);
		return *this;
	}

	BigNumber get(Elements element) const
	{
		return power[static_cast<size_t>(element)];
	}

	// The elements this ElementalPower actually carries - e.g. an attack's baseElementalPower's
	// mask defines which element(s) that attack is tagged with for weakness/resistance purposes.
	ElementMask getMask() const
	{
		return activeMask;
	}

	// Sum of every element's power, regardless of which elements are active - used once the
	// weakness/resistance multiplier has already been resolved (elsewhere) against getMask().
	BigNumber getTotal() const
	{
		BigNumber total(0);
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			total = total + power[i];
		}
		return total;
	}

	// Elementwise sum, e.g. an attack's baseElementalPower + a familiar-wide bonus. The result
	// "uses" every element either side used.
	ElementalPower operator+(const ElementalPower& other) const
	{
		ElementalPower result;
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			result.power[i] = power[i] + other.power[i];
		}
		result.activeMask = activeMask | other.activeMask;
		return result;
	}

	// Adds `bonus`'s power only for elements THIS ElementalPower already carries - this
	// ElementalPower's own mask is unchanged. Used to apply a familiar-wide per-element bonus
	// (which may hold power for elements no attack of this familiar even uses yet) to one
	// specific attack, without leaking bonus power into an element that attack doesn't carry.
	ElementalPower addMatching(const ElementalPower& bonus) const
	{
		ElementalPower result = *this;
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			Elements element = static_cast<Elements>(i);
			if (activeMask & elementBit(element))
			{
				result.power[i] = result.power[i] + bonus.get(element);
			}
		}
		return result;
	}

	// Resolves each active element's own weakness/resistance multiplier independently and sums
	// the results - e.g. a Fire+Water attack applies the target's Fire multiplier to just the
	// fire portion and its Water multiplier to just the water portion, rather than blending the
	// two into one averaged multiplier the way a single flat elemental-damage-plus-mask
	// representation would have to. `multiplierForElement` is typically a small wrapper around
	// the target's ElementalAffinity::getMultiplier(); a null/unset resolver treats every active
	// element as neutral (multiplier 1.0).
	BigNumber computeDamage(const std::function<double(Elements)>& multiplierForElement) const
	{
		BigNumber total(0);
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			Elements element = static_cast<Elements>(i);
			if (!(activeMask & elementBit(element))) continue;
			double multiplier = multiplierForElement ? multiplierForElement(element) : 1.0;
			total = total + power[i] * BigNumber(multiplier);
		}
		return total;
	}
};