#include "stdafx.h"
#include "Sprites.h"
#include "PublicVariables.h"

Sprites::Sprites() : sf::Sprite(tempTexture)
{

}

Sprites::~Sprites()
{
	for (auto& pair : animations)
	{
		delete pair.second;
	}
}

void Sprites::addAnimation(std::string stateName, std::string spriteAddress, sf::Vector2u spriteSize, float frameTimerMax, int frameCount, int rowIndex, unsigned int activeFrame)
{
	Textures* newTextures = new Textures(spriteAddress, spriteSize);
	newTextures->setFrameTimerMax(frameTimerMax);
	newTextures->setFrameTimer(frameTimerMax);
	this->setScale({ spriteScale, spriteScale });
	newTextures->setSmooth(false); // if I want to get some smooth sprites, I will have to think about a different place to implement this
	animations[stateName] = newTextures;

	// if this is the first animation added, set it as the current state
	if (animations.size() == 1)
	{
		this->setTexture(*newTextures);
		this->setOrigin({ spriteSize.x / 2.0f, spriteSize.y / 2.0f }); // only ok if all animations have the same size
		frameSize = spriteSize; // remember the frame size the origin was based on - getGroundY() needs it to find where the feet are
		this->setTextureRect(sf::IntRect(newTextures->getTextureCoord(), sf::Vector2i(spriteSize)));
		currentState = stateName;
	}
}

bool Sprites::isOneShotPlaying() const
{
	return isOneShot;
}

float Sprites::getGroundY() const
{
	// The origin sits at the frame center (see addAnimation), so getPosition().y points to the MIDDLE of the sprite.
	// The feet are half a frame below that, scaled by the current scale factor - this is where depth sorting anchors.
	return getPosition().y + (static_cast<float>(frameSize.y) / 2.0f) * getScale().y;
}

void Sprites::playAnimation(std::string stateName, bool oneShot, std::string returnTo)
{
	currentState = stateName;
	isOneShot = oneShot;
	returnToState = returnTo;

	Textures* newAnimation = animations[stateName];
	newAnimation->resetAnimation();

	this->setTexture(*newAnimation);
	this->setTextureRect(sf::IntRect(newAnimation->getTextureCoord(), sf::Vector2i(newAnimation->getFrameSize())));
}

void Sprites::playAnimation(std::string stateName, bool oneShot, std::string returnTo, unsigned int activeFrame)
{
	currentState = stateName;
	isOneShot = oneShot;
	returnToState = returnTo;

	Textures* newAnimation = animations[stateName];
	newAnimation->resetAnimation();

	this->setTexture(*newAnimation);
	this->setTextureRect(sf::IntRect(newAnimation->getTextureCoord(), sf::Vector2i(newAnimation->getFrameSize())));
}

void Sprites::animate(float dt)
{
	Textures* currentAnimation = animations[currentState];

	currentAnimation->setFrameTimer(currentAnimation->getFrameTimer() - dt);
	if (currentAnimation->getFrameTimer() <= 0.0f)
	{
		currentAnimation->frameRight();
		{
			this->setTextureRect(sf::IntRect(currentAnimation->getTextureCoord(), sf::Vector2i(currentAnimation->getFrameSize())));
		}
		currentAnimation->setFrameTimer(currentAnimation->getFrameTimerMax());

		if (isOneShot && currentAnimation->hasCompletedCycle())
		{
			playAnimation(returnToState); // Return to the idle state after one shot animation completes
		}
	}
}

void Sprites::setSmooth(bool smooth)
{
	auto it = animations.find(currentState);
	if (it != animations.end())
	{
		it->second->setSmooth(smooth);
	}
}

void Sprites::setAnimationLoopRange(std::string stateName, unsigned int loopStartFrame, unsigned int loopEndFrame)
{
	auto it = animations.find(stateName);
	if (it == animations.end()) return;
	// FamiliarAttack/Sprites count frames from 1 (matching how you'd describe a spritesheet out
	// loud); Textures counts from 0 internally - convert at this boundary.
	it->second->setLoopRange(static_cast<int>(loopStartFrame) - 1, static_cast<int>(loopEndFrame) - 1);
}

void Sprites::setAnimationLooping(std::string stateName, bool looping)
{
	auto it = animations.find(stateName);
	if (it == animations.end()) return;
	it->second->setLoopActive(looping);
}

bool Sprites::isAnimationCurrentlyShowing(std::string stateName) const
{
	return currentState == stateName;
}

unsigned int Sprites::getCurrentFrame(std::string stateName) const
{
	auto it = animations.find(stateName);
	if (it == animations.end()) return 1;
	int frameIndex = it->second->getTextureCoord().x / static_cast<int>(it->second->getFrameSize().x);
	return static_cast<unsigned int>(frameIndex) + 1; // Textures counts from 0 internally - convert back to 1-based
}
