#pragma once
#include "stdafx.h"
#include "Text.h"
#include <functional>

class UIButton
{
private:
	sf::RectangleShape background;
	Text* label;
	std::function<void()> onClick;

	bool enabled = true;
	bool firstTimePurchase = false;

	bool pressed = false;
	float pressAnimation = 0.0f; // 0 = resting, 1 = fully pressed in; animated between the two

	bool notifying = false; // when true, button pulses to flag unseen content
	sf::Clock notifyClock;  // independent time base for the pulse, unaffected by press animation

	sf::Clock animationClock; // measures dt between render() calls, no external update() needed

	static constexpr float pressAnimationDuration = 0.08f; // seconds to fully press/release

	static constexpr float pressInset = 3.0f; // pixels the button shrinks/shifts when fully pressed

	void updatePressAnimation();
	static sf::Color blendColor(sf::Color from, sf::Color to, float t); // New

public:
	UIButton(std::string labelText, std::function<void()> onClickCallback);
	~UIButton();

	void setLayout(sf::Vector2f position, sf::Vector2f size);
	void setLabelText(std::string text);
	bool handleClick(sf::Vector2f pos);
	void render(sf::RenderWindow* window);

	// button visibility and interactivity
	void setEnabled(bool isEnabled);
	bool isEnabled() const;

	void setNotifying(bool isNotifying); // starts/stops the pulse; resets phase when turning on
};