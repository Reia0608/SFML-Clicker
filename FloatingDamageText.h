#pragma once
#include "stdafx.h"
#include "Text.h"
#include "Fonts.h"
#include <vector>
#include <memory>
#include <string>

// A single floating combat-text popup (a damage number, a crit, etc). Owns one Text per
// character so it can render a genuine left-to-right two-color gradient across the string -
// something a single sf::Text/Text can't do on its own, since SFML gives a Text only one flat
// fill color. Handles its own rise + fade over its lifetime; the owner just calls update()
// every frame and drops it once update() returns false.
//
// Extracted out of Enemy (where this used to live as Enemy::DamageNumbers/showDamage/
// updateDamageText/drawDamageText) so any other source of floating text can reuse it without
// going through Enemy.
class FloatingDamageText
{
private:
	struct Glyph
	{
		std::unique_ptr<Text> text;
		sf::Color baseFillColor; // pre-fade color for this glyph; alpha is re-derived from elapsed/duration every update()
	};

	std::vector<Glyph> glyphs;
	float elapsed = 0.0f;
	float duration;
	float riseSpeed;

	// Builds one Text per character of `str` (using `fonts`, not owned by any of them), measures
	// each, then places the whole run centered on `originPosition` (matching the old single-Text
	// ::setOrigin(bounds.size / 2) centering) with fill colors interpolated start -> end by
	// character index. Every glyph gets the same outlineColor/outlineThickness.
	void buildGlyphs(const std::string& str, Fonts* fonts, unsigned int characterSize, sf::Color start, sf::Color end,
		sf::Vector2f originPosition, sf::Color outlineColor, float outlineThickness);

public:
	// Flat single fill color - used for crits (always solid orange, see Enemy::showDamage) and for
	// any physical/no-element hit (baseColor = whichever source dealt it: Hero red, familiar
	// blue, passive pink). outlineColor/outlineThickness default to the plain black outline every
	// floating number has always had.
	FloatingDamageText(const std::string& str, Fonts* fonts, sf::Color color, sf::Vector2f position,
		sf::Color outlineColor = sf::Color::Black, float outlineThickness = 2.0f,
		unsigned int characterSize = 30, float duration = 2.0f, float riseSpeed = 30.0f);

	// Two-color left-to-right fill gradient - used for elemental hits (see
	// Elements.h::getElementColorGradient). outlineColor is where the "who hit" source-identity
	// color (Hero red / familiar blue / passive pink) goes when combining source + element - see
	// Enemy::showDamage's elemental branch, which passes baseColor here with a thicker outline
	// than the default 2.0f so it stays legible against a colored (not black) outline.
	FloatingDamageText(const std::string& str, Fonts* fonts, sf::Color gradientStart, sf::Color gradientEnd, sf::Vector2f position,
		sf::Color outlineColor = sf::Color::Black, float outlineThickness = 2.0f,
		unsigned int characterSize = 30, float duration = 2.0f, float riseSpeed = 30.0f);

	FloatingDamageText(const FloatingDamageText&) = delete;
	FloatingDamageText& operator=(const FloatingDamageText&) = delete;

	bool update(float dt); // returns false once this entry has fully faded and should be removed/deleted by the owner
	void draw(sf::RenderWindow* window) const;
};