#pragma once

#include "stdafx.h"

class Fonts :public sf::Font
{
	private:
		void initFonts(std::string address);

	public:
		Fonts(std::string address);
		~Fonts();
};
