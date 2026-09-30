#include "stdafx.h"
#include "FloatingDamageText.h"
#include <algorithm>
#include <cstdint>

void FloatingDamageText::buildGlyphs(const std::string& str, Fonts* fonts, unsigned int characterSize, sf::Color start, sf::Color end,
	sf::Vector2f originPosition, sf::Color outlineColor, float outlineThickness)
{
	// First pass: build and measure every glyph before placing any of them, so the full run's
	// width is known up front and the whole thing can be centered on originPosition (same visual
	// effect as the old single-Text::setOrigin(bounds.size / 2)).
	struct Measured
	{
		std::unique_ptr<Text> text;
		float width;
	};
	std::vector<Measured> measured;
	measured.reserve(str.size());

	float totalWidth = 0.0f;
	float maxHeight = 0.0f;

	for (char c : str)
	{
		auto glyphText = std::make_unique<Text>(std::string(1, c), fonts); // does not own `fonts` - see Text's second constructor
		glyphText->setCharacterSize(characterSize);
		glyphText->setOutlineColor(outlineColor);
		glyphText->setOutlineThickness(outlineThickness);

		sf::FloatRect bounds = glyphText->getGlobalBounds();
		totalWidth += bounds.size.x;
		maxHeight = std::max(maxHeight, bounds.size.y);

		measured.push_back({ std::move(glyphText), bounds.size.x });
	}

	// Second pass: color (interpolated by character index - a handful of digits/symbols, so this
	// reads as a smooth gradient without needing true width-weighted/per-pixel interpolation) and
	// place left-to-right starting far enough left that the whole run centers on originPosition.
	float x = originPosition.x - totalWidth / 2.0f;
	float y = originPosition.y - maxHeight / 2.0f;
	size_t count = measured.size();

	glyphs.reserve(count);
	for (size_t i = 0; i < count; i++)
	{
		float t = count > 1 ? static_cast<float>(i) / static_cast<float>(count - 1) : 0.0f;
		sf::Color color(
			static_cast<std::uint8_t>(start.r + (end.r - start.r) * t),
			static_cast<std::uint8_t>(start.g + (end.g - start.g) * t),
			static_cast<std::uint8_t>(start.b + (end.b - start.b) * t)
		);

		measured[i].text->setFillColor(color);
		measured[i].text->setPosition({ x, y });

		glyphs.push_back({ std::move(measured[i].text), color });
		x += measured[i].width;
	}
}

FloatingDamageText::FloatingDamageText(const std::string& str, Fonts* fonts, sf::Color color, sf::Vector2f position,
	sf::Color outlineColor, float outlineThickness, unsigned int characterSize, float duration, float riseSpeed)
	: duration(duration), riseSpeed(riseSpeed)
{
	buildGlyphs(str, fonts, characterSize, color, color, position, outlineColor, outlineThickness); // flat color = gradient with identical start/end
}

FloatingDamageText::FloatingDamageText(const std::string& str, Fonts* fonts, sf::Color gradientStart, sf::Color gradientEnd, sf::Vector2f position,
	sf::Color outlineColor, float outlineThickness, unsigned int characterSize, float duration, float riseSpeed)
	: duration(duration), riseSpeed(riseSpeed)
{
	buildGlyphs(str, fonts, characterSize, gradientStart, gradientEnd, position, outlineColor, outlineThickness);
}

bool FloatingDamageText::update(float dt)
{
	elapsed += dt;
	float t = elapsed / duration;
	if (t >= 1.0f) return false;

	std::uint8_t alpha = static_cast<std::uint8_t>(255 * (1.0f - t));
	float dy = riseSpeed * dt;

	for (auto& glyph : glyphs)
	{
		sf::Vector2f pos = glyph.text->getPosition();
		pos.y -= dy;
		glyph.text->setPosition(pos);

		sf::Color fillColor = glyph.baseFillColor;
		fillColor.a = alpha;
		glyph.text->setFillColor(fillColor);

		sf::Color outlineColor = glyph.text->getOutlineColor();
		outlineColor.a = alpha;
		glyph.text->setOutlineColor(outlineColor);
	}

	return true;
}

void FloatingDamageText::draw(sf::RenderWindow* window) const
{
	for (auto& glyph : glyphs)
	{
		window->draw(*glyph.text);
	}
}