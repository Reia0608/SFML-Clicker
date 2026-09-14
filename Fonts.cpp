#include "stdafx.h"
#include "Fonts.h"

void Fonts::initFonts(std::string address)
{
	if (!this->openFromFile(address))
	{
		std::cout << "ERROR::FONTS::INITFONTS::COULD NOT LOAD FILE" << address << std::endl;
	}
	this->setSmooth(false);
}

Fonts::Fonts(std::string address)
{
	initFonts(address);
}

Fonts::~Fonts()
{

}