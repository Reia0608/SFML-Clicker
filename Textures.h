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

		// The repeating sub-range frameRight() can loop within instead of always advancing
		// straight through to the end of the sheet - lets an attack fire one shot per pass
		// through a few frames, as many times as it needs to (see FamiliarAttack's
		// projectileCount/loopStartFrame/loopEndFrame). 0-based, inclusive; -1 means "no loop
		// range configured". loopActive is the on/off switch: the range can be configured well
		// before playback ever reaches it, and turned off again once enough shots have fired,
		// without forgetting the range itself.
		int loopStartFrameIndex = -1;
		int loopEndFrameIndex = -1;
		bool loopActive = false;

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

		void setLoopRange(int startFrameIndex, int endFrameIndex); // 0-based, inclusive - configures the sub-range frameRight() can repeat within (see loopStartFrameIndex/loopEndFrameIndex above)
		void setLoopActive(bool active); // true: frameRight() wraps within the configured loop range instead of advancing past it. false: lets the current pass finish naturally and continue on - see frameRight()
};
