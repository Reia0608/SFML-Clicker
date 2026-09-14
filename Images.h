#pragma once
#include "stdafx.h"

class Images : public sf::Image
{
private:
	sf::Texture* texture;
	sf::Sprite* sprite;

	void initTexture();
	void initSprite();

public:
	Images(sf::Vector2u imageSize);
	~Images();

	void updateTexture();
	sf::Sprite getSprite();
};