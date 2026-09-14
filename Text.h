#pragma once

#include "stdafx.h"
#include "Fonts.h"

class Text : public sf::Text
{
private:
	bool ownsFont; // checks if the destructor is allowed to delete that font

	std::string originalString;
	Fonts* fonts;

	static Fonts* createFonts(std::string fontAddress);
	void initString(std::string str);
public:
	Text(std::string str, std::string fontAddress); // version loads its own font
	Text(std::string str, Fonts* existingFont); // version that does not own the font
	~Text();

	void streamText(std::string str = "", float var = 0.0f);
};
