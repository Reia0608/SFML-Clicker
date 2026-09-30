#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <SFML/Graphics/Color.hpp>

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

// Two-color left-to-right gradient used for an element's floating damage-number text
// (see FloatingDamageText). `start` is the color the FIRST character of the number is closest
// to, `end` is what the LAST character is closest to - characters in between are interpolated.
struct ElementColorGradient
{
	sf::Color start;
	sf::Color end;
};

// The single place to retune floating damage-text colors per element while play-testing -
// nothing else needs to change. Values below are a starting point:
inline const ElementColorGradient& getElementColorGradient(Elements element)
{
	static const std::array<ElementColorGradient, ElementCount> gradients = { {
			/* Fire   */ { sf::Color(255, 40, 0),    sf::Color(255, 170, 0) },  // red -> orange
			/* Water  */ { sf::Color(0, 90, 220),    sf::Color(130, 215, 255) },// deep blue -> pale sky blue
			/* Earth  */ { sf::Color(110, 70, 30),   sf::Color(120, 200, 90) }, // brown -> green
			/* Wind   */ { sf::Color(120, 220, 130), sf::Color(180, 235, 255) },// green -> light blue
			/* Ice    */ { sf::Color(0, 220, 230),   sf::Color(255, 255, 255) },// cyan -> white
			/* Metal  */ { sf::Color(150, 155, 165), sf::Color(235, 240, 245) },// gunmetal gray -> bright silver
			/* Light  */ { sf::Color(255, 250, 200), sf::Color(255, 255, 255) },// warm pale yellow -> white
			/* Dark   */ { sf::Color(55, 0, 85),     sf::Color(170, 30, 200) },// deep violet -> magenta-purple
			/* Chaos  */ { sf::Color(210, 0, 160),   sf::Color(255, 150, 0) }, // magenta -> orange (jarring on purpose - "unstable")
			/* Poison */ { sf::Color(90, 0, 140),    sf::Color(150, 235, 30) } // violet -> acid green
		} };
	return gradients[static_cast<size_t>(element)];
}

// The element whose gradient represents a (possibly multi-element) mask's floating text -
// currently just the lowest set bit. Multi-element attacks (e.g. a future Fire|Water "Steam"
// ability) don't exist in content yet; when they do, this is the one place that needs to change
// to blend/choose between the elements instead of just picking the first.
// Caller must check mask != 0 first - returns ElementCount (an invalid index) if mask is empty.
inline Elements firstElementInMask(ElementMask mask)
{
	for (int i = 0; i < static_cast<int>(ElementCount); i++)
	{
		Elements element = static_cast<Elements>(i);
		if (mask & elementBit(element)) return element;
	}
	return ElementCount;
}