#include "stdafx.h"
#include "Images.h"

void Images::initTexture()
{
	texture = new sf::Texture(this->getSize());
}

void Images::initSprite()
{
	sprite = new sf::Sprite(*texture);
}

Images::Images(sf::Vector2u imageSize)
{
	this->resize(imageSize);
	initTexture();
	initSprite();
}

Images::~Images()
{
	delete texture;
	delete sprite;
}

void Images::updateTexture()
{
	texture->update(*this);
}

sf::Sprite Images::getSprite()
{
	return *sprite;
}