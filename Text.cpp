#include "stdafx.h"
#include <sstream>
#include "Text.h"

Fonts* Text::createFonts(std::string fontAddress)
{
	return new Fonts(fontAddress);
}

void Text::initString(std::string str)
{
	originalString = str;
	this->setString(originalString);
}

Text::Text(std::string str, std::string fontAddress)
	: sf::Text(*(fonts = createFonts(fontAddress))), ownsFont(true)
{
	// fonts is being assigned as part of the basic classes arguments on purpose,
	// because sf:Text already needs the valid Font in the constructor
	initString(str);
}

Text::Text(std::string str, Fonts* existingFont)
	: sf::Text(*(fonts = existingFont)), ownsFont(false)
{
	initString(str);
}

Text::~Text()
{
	if (ownsFont)
	{
		delete fonts;
	}
}

void Text::streamText(std::string str, float var)
{
	std::stringstream sStream;
	sStream << str << var;

	this->setString(sStream.str());
}