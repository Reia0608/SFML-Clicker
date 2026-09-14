#pragma once
#include "stdafx.h"
#include "Text.h"
#include "BigNumber.h"
#include <string>

// A simple single-line numeric input box. Click to focus, type digits/./- to edit,
// Backspace to delete. Just enough for debug value entry - not a full text editor.
class TextInputField
{
private:
	sf::RectangleShape background;
	Text* label; // shows the current input string
	std::string content;
	bool focused = false;
	bool allowDecimal;
	bool allowNegative;
	bool allowSuffix;

public:
	TextInputField(std::string placeholder, bool allowDecimal = true, bool allowNegative = false, bool allowSuffix = false);
	~TextInputField();

	void setLayout(sf::Vector2f position, sf::Vector2f size);
	bool handleClick(sf::Vector2f pos); // focuses/unfocuses depending on where the click landed; returns true if this field was hit
	void handleTextEntered(char32_t unicode); // no-op unless this field is focused
	void handleBackspace(); // no-op unless this field is focused
	void render(sf::RenderWindow* window);

	bool isFocused() const;
	const std::string& getText() const;
	double getValueAsDouble(double fallback = 0.0) const; // parses content, returns fallback if empty/invalid
	BigNumber getValueAsBigNumber(BigNumber fallback = BigNumber(0.0)) const; // parses e.g. "12.5K" using BigNumber's suffix table
	void clear();
};