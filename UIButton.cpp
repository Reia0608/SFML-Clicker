#include "stdafx.h"
#include "UIButton.h"
#include "LayoutManager.h"

void UIButton::updatePressAnimation()
{
	if (pressed && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) pressed = false; // detects LMB release without needing a dedicated event
	float dt = animationClock.restart().asSeconds();
	float step = dt / pressAnimationDuration;
	if (pressed) pressAnimation = std::min(1.0f, pressAnimation + step);
	else pressAnimation = std::max(0.0f, pressAnimation - step);
}

sf::Color UIButton::blendColor(sf::Color from, sf::Color to, float t)
{
	auto lerp = [t](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * t); };
	return sf::Color(lerp(from.r, to.r), lerp(from.g, to.g), lerp(from.b, to.b), from.a);
}

UIButton::UIButton(std::string labelText, std::function<void()> onClickCallback)
	: onClick(onClickCallback)
{
	background.setFillColor(sf::Color(70, 100, 140));
	background.setOutlineColor(sf::Color::Black);
	background.setOutlineThickness(1.0f);

	label = new Text(labelText, "Fonts/arial.ttf");
	label->setFillColor(sf::Color::White);
	label->setCharacterSize(14);
}

UIButton::~UIButton()
{
	delete label;
}

void UIButton::setLayout(sf::Vector2f position, sf::Vector2f size)
{
	background.setPosition(position);
	background.setSize(size);
	LayoutManager::applyAnchor(*label, Anchor::MiddleCenter, { 0.0f, 0.0f }, position, size);
}

void UIButton::setLabelText(std::string text)
{
	label->setString(text);
	LayoutManager::applyAnchor(*label, Anchor::MiddleCenter, { 0.0f, 0.0f }, background.getPosition(), background.getSize());
}

bool UIButton::handleClick(sf::Vector2f pos)
{
	// If the button is disabled, ignore clicks
	if (!enabled) return false;

	if (background.getGlobalBounds().contains(pos))
	{
		pressed = true; // starts the "pressed in" animation, released automatically once LMB is no longer held
		if (onClick)
		{
			onClick();
		}
		return true;
	}
	return false;
}

void UIButton::render(sf::RenderWindow* window)
{
	// if the button is disabled, it is not rendered (invisible)
	if (!enabled) return;

	updatePressAnimation();

	sf::Color baseColor = background.getFillColor();
	if (notifying)
	{
		float t = notifyClock.getElapsedTime().asSeconds();
		float phase = (std::sin(t * 5.0f) + 1.0f) / 2.0f; // 0..1 pulse
		baseColor = blendColor(baseColor, sf::Color(255, 215, 60), phase * 0.6f); // pulse toward gold
	}

	float inset = pressInset * pressAnimation;
	sf::RectangleShape visual = background; // local copy, so "background" (used for hit-testing) stays untouched
	visual.setPosition(background.getPosition() + sf::Vector2f(inset, inset));
	visual.setSize(background.getSize() - sf::Vector2f(inset * 2.0f, inset * 2.0f));
	visual.setFillColor(blendColor(baseColor, sf::Color(45, 65, 95), pressAnimation)); // darkens while pressed, on top of the pulse

	window->draw(visual);
	window->draw(*label);
}

void UIButton::setEnabled(bool isEnabled)
{
	enabled = isEnabled;
}

bool UIButton::isEnabled() const
{
	return enabled;
}

void UIButton::setNotifying(bool isNotifying)
{
	if (isNotifying && !notifying) notifyClock.restart(); // start each pulse cycle fresh
	notifying = isNotifying;
}
