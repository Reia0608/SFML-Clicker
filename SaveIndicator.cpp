#include "stdafx.h"
#include "SaveIndicator.h"

SaveIndicator::SaveIndicator(const sf::Font& font)
	: label(font, "Saving...", 16)
	, visibleDuration(3.0f)
	, fadeDuration(0.6f)
	, timer(0.0f)
	, active(false)
{
	// Kleines "Diskette"-Icon aus einfachen Formen, damit kein Asset-File nötig ist.
	diskBody.setSize({ 18.0f, 18.0f });
	diskBody.setFillColor(sf::Color(60, 60, 60));

	diskLabel.setSize({ 12.0f, 6.0f });
	diskLabel.setFillColor(sf::Color(220, 220, 220));

	diskNotch.setSize({ 5.0f, 5.0f });
	diskNotch.setFillColor(sf::Color(30, 30, 30));

	label.setFillColor(sf::Color::White);

	onResize();
}

void SaveIndicator::onResize()
{
	const float margin = 10.0f;

	// Oben links in der Game-Area. Falls stattdessen oben rechts (neben dem UI-Panel)
	// gewünscht ist: x-Werte auf (windowSize.x - uiPanelWidth - <breite des Icons+Texts> - margin) setzen.
	diskBody.setPosition({ margin, margin });
	diskLabel.setPosition({ margin + 3.0f, margin + 2.0f });
	diskNotch.setPosition({ margin + 13.0f, margin });

	label.setPosition({ margin + 18.0f + 6.0f, margin + 1.0f });
}

void SaveIndicator::show()
{
	active = true;
	timer = 0.0f;
	applyAlpha(255);
}

void SaveIndicator::update(float dt)
{
	if (!active) return;

	timer += dt;

	if (timer <= visibleDuration)
	{
		applyAlpha(255);
	}
	else if (timer <= visibleDuration + fadeDuration)
	{
		float fadeT = (timer - visibleDuration) / fadeDuration;
		std::uint8_t alpha = static_cast<std::uint8_t>(255.0f * (1.0f - fadeT));
		applyAlpha(alpha);
	}
	else
	{
		active = false;
	}
}

void SaveIndicator::applyAlpha(std::uint8_t alpha)
{
	sf::Color body = diskBody.getFillColor();   body.a = alpha;  diskBody.setFillColor(body);
	sf::Color lbl = diskLabel.getFillColor();   lbl.a = alpha;   diskLabel.setFillColor(lbl);
	sf::Color notch = diskNotch.getFillColor(); notch.a = alpha; diskNotch.setFillColor(notch);
	sf::Color txt = label.getFillColor();       txt.a = alpha;   label.setFillColor(txt);
}

void SaveIndicator::render(sf::RenderWindow* window)
{
	if (!active) return;

	window->draw(diskBody);
	window->draw(diskLabel);
	window->draw(diskNotch);
	window->draw(label);
}