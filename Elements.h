#pragma once
#include <cstdint>
#include <string>

enum Elements
{
	Fire,
	Water,
	Earth,
	Wind,
	Ice,
	Metal,
	Light,
	Dark,
	Chaos,
	Poison,
	ElementCount // keep last - not a real element, used to size arrays / loop bounds
};

// A bitmask of Elements, so a single attack (autoattack or ability) can carry
// zero, one, or several elements at once (e.g. a "Steam" ability = Fire | Water).
// Zero means "no element" - a purely physical attack. Physical power is always added at
// full value with no weakness/resistance applied to it (see FamiliarAttack::basePhysicalPower).
using ElementMask = std::uint32_t;

inline ElementMask elementBit(Elements element)
{
	return 1u << static_cast<unsigned>(element);
}

inline ElementMask elementMaskOf(std::initializer_list<Elements> elements)
{
	ElementMask mask = 0;
	for (Elements element : elements) mask |= elementBit(element);
	return mask;
}

inline std::string elementToString(Elements element)
{
	switch (element)
	{
	case Fire: return "Fire";
	case Water: return "Water";
	case Earth: return "Earth";
	case Wind: return "Wind";
	case Ice: return "Ice";
	case Metal: return "Metal";
	case Light: return "Light";
	case Dark: return "Dark";
	case Chaos: return "Chaos";
	case Poison: return "Poison";
	default: return "Unknown";
	}
}