#include "stdafx.h"
#include "Textures.h"

void Textures::initTexture(std::string address)
{
	if (!this->loadFromFile(address))
	{
		std::cerr << "ERROR::TEXTURES::INITTEXTURE::COULD NOT LOAD FILE!" << address << std::endl;
	}
}

void Textures::initTimers()
{
	frameTimer = 0.0f;
	frameTimerMax = 0.0f;
	completedCycle = false;
}

Textures::Textures(std::string address, sf::Vector2u size, int frameCount, int rowIndex)
{
	initTimers();

	textureCoord = { 0, 0 };
	completedCycle = false;

	this->frameSize = size;
	this->rowIndex = rowIndex;
	initTexture(address);

	this->frameCount = (frameCount > 0) ? frameCount : static_cast<int>(this->getSize().x) / static_cast<int>(size.x); // 0 means "use the full sheet width", i.e. the previous behavior
	textureCoord = { 0, rowIndex * static_cast<int>(size.y) };
}

Textures::~Textures()
{

}

void Textures::frameLeft()
{
	this->textureCoord.x -= this->getFrameSize().x;
	if (this->textureCoord.x < 0)
	{
		this->textureCoord.x = (this->frameCount - 1) * static_cast<int>(this->getFrameSize().x);
	}
}

void Textures::frameRight()
{
	this->textureCoord.x += this->getFrameSize().x;
	if (this->textureCoord.x >= this->frameCount * static_cast<int>(this->getFrameSize().x))
	{
		this->textureCoord.x = 0;
		completedCycle = true;
	}
}

void Textures::frameUp()
{
	this->textureCoord.y -= this->getFrameSize().y;
	if (this->textureCoord.y < 0)
	{
		this->textureCoord.y = static_cast<int>(this->getSize().y) - this->getFrameSize().y;
	}
}

void Textures::frameDown()
{
	this->textureCoord.y += this->getFrameSize().y;
	if (this->textureCoord.y >= static_cast<int>(this->getSize().y))
	{
		this->textureCoord.y = 0;
		completedCycle = true;
	}
}

void Textures::resetAnimation()
{
	textureCoord = { 0, rowIndex * static_cast<int>(frameSize.y) };
	completedCycle = false;
	frameTimer = frameTimerMax;
}

void Textures::setFrameTimer(float frameTime)
{
	frameTimer = frameTime;
}

float Textures::getFrameTimer() const
{
	return frameTimer;
}

void Textures::setFrameTimerMax(float frameTime)
{
	frameTimerMax = frameTime;
}

float Textures::getFrameTimerMax() const
{
	return frameTimerMax;
}

void Textures::setCompletedCycle(bool value)
{
	completedCycle = value;
}

sf::Vector2i Textures::getTextureCoord() const
{
	return textureCoord;
}

sf::Vector2u Textures::getFrameSize() const
{
	return frameSize;
}

bool Textures::hasCompletedCycle() const
{
	return completedCycle;
}