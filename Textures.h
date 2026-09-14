#pragma once
#include "stdafx.h"

class Textures : public sf::Texture
{
	private:
		sf::Vector2u frameSize;
		sf::Vector2i textureCoord;

		float frameTimer;
		float frameTimerMax;
		bool completedCycle;

		int frameCount; // how many frames this animation cycles through (may be less than the sheet's full row width)
		int rowIndex; // which row (0-based) this animation's frames sit on, for multi-row spritesheets

		void initTexture(std::string address);
		void initTimers();

	public:

		Textures(std::string address, sf::Vector2u size, int frameCount = 0, int rowIndex = 0);
		~Textures();

		void frameLeft();
		void frameRight();
		void frameUp();
		void frameDown();
		void resetAnimation();

		float getFrameTimer() const;
		void setFrameTimer(float frameTime);

		float getFrameTimerMax() const;
		void setFrameTimerMax(float frameTime);

		void setCompletedCycle(bool value);

		sf::Vector2i getTextureCoord() const;

		sf::Vector2u getFrameSize() const;

		bool hasCompletedCycle() const; // true as soon as frameRight() returns to 0
};
