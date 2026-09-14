#include "stdafx.h"
#include "TextInputField.h"
#include <algorithm>
#include <cctype>

TextInputField::TextInputField(std::string placeholder, bool allowDecimal, bool allowNegative, bool allowSuffix)
	: allowDecimal(allowDecimal), allowNegative(allowNegative), allowSuffix(allowSuffix)
{
	background.setFillColor(sf::Color(40, 40, 40));
	background.setOutlineColor(sf::Color(120, 120, 120));
	background.setOutlineThickness(1.0f);

	label = new Text(placeholder, "Fonts/arial.ttf");
	label->setCharacterSize(14);
	label->setFillColor(sf::Color::White);
}

TextInputField::~TextInputField()
{
	delete label;
}

void TextInputField::setLayout(sf::Vector2f position, sf::Vector2f size)
{
	background.setPosition(position);
	background.setSize(size);
	label->setPosition({ position.x + 6.0f, position.y + size.y / 2.0f - 8.0f });
}

bool TextInputField::handleClick(sf::Vector2f pos)
{
	bool hit = background.getGlobalBounds().contains(pos);
	focused = hit; // clicking outside always defocuses, clicking inside always focuses
	background.setOutlineColor(focused ? sf::Color(240, 200, 60) : sf::Color(120, 120, 120));
	return hit;
}

void TextInputField::handleTextEntered(char32_t unicode)
{
	if (!focused) return;

	if (unicode == 8) // some backends still deliver backspace as a text character
	{
		handleBackspace();
		return;
	}

	if (unicode >= '0' && unicode <= '9')
	{
		content += static_cast<char>(unicode);
	}
	else if (allowDecimal && unicode == '.' && content.find('.') == std::string::npos)
	{
		content += '.';
	}
	else if (allowNegative && unicode == '-' && content.empty())
	{
		content += '-';
	}
	else if (allowSuffix && unicode < 128 && std::isalpha(static_cast<unsigned char>(unicode)))
	{
		content += static_cast<char>(unicode);
	}
	else
	{
		return; // ignore letters, symbols, etc.
	}

	label->setString(content);
}

void TextInputField::handleBackspace()
{
	if (!focused || content.empty()) return;
	content.pop_back();
	label->setString(content);
}

void TextInputField::render(sf::RenderWindow* window)
{
	window->draw(background);
	window->draw(*label);
}

bool TextInputField::isFocused() const
{
	return focused;
}

const std::string& TextInputField::getText() const
{
	return content;
}

double TextInputField::getValueAsDouble(double fallback) const
{
	if (content.empty() || content == "-" || content == ".") return fallback;
	try
	{
		return std::stod(content);
	}
	catch (...)
	{
		return fallback;
	}
}

BigNumber TextInputField::getValueAsBigNumber(BigNumber fallback) const
{
	if (content.empty() || content == "-" || content == ".") return fallback;

	// split off any trailing letters (the suffix, e.g. "K", "Mil") from the leading number
	size_t splitPos = content.size();
	while (splitPos > 0 && std::isalpha(static_cast<unsigned char>(content[splitPos - 1])))
	{
		splitPos--;
	}
	std::string numberPart = content.substr(0, splitPos);
	std::string suffixPart = content.substr(splitPos);

	double mantissa;
	try
	{
		mantissa = std::stod(numberPart);
	}
	catch (...)
	{
		return fallback;
	}

	int tier = 0;
	if (!suffixPart.empty())
	{
		bool matched = false;
		for (size_t i = 1; i < BigNumber::suffixes.size(); i++) // skip index 0, its suffix is "" and already handled above
		{
			std::string suf = BigNumber::suffixes[i];
			if (suf.size() == suffixPart.size() &&
				std::equal(suf.begin(), suf.end(), suffixPart.begin(),
					[](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b)); }))
			{
				tier = static_cast<int>(i);
				matched = true;
				break;
			}
		}
		if (!matched) return fallback; // unrecognized suffix, e.g. a typo
	}

	BigNumber result;
	result.mantissa = mantissa;
	result.tier = tier;
	result.normalize();
	return result;
}

void TextInputField::clear()
{
	content.clear();
	label->setString("");
}