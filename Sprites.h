#pragma once
#include "Textures.h"
#include <unordered_map>


class Sprites : public sf::Sprite
{
	private:
		sf::Texture tempTexture;
		std::unordered_map<std::string, Textures*> animations;
		std::string currentState; // current animation state
		bool isOneShot = false; // flag to indicate if the current animation is a one time animation
		std::string returnToState; // The state to return to after a one-shot animation
		sf::Vector2u frameSize = { 0, 0 }; // size of the frames of the first added animation - needed by getGroundY() because the origin sits at the frame center

	public:
		Sprites();
		~Sprites();

		bool isOneShotPlaying() const;

		float getGroundY() const; // Y coordinate of this sprite's feet on the ground (bottom edge of the scaled frame) - used for depth sorting against decorations

		void addAnimation(std::string stateName, std::string spriteAddress, sf::Vector2u spriteSize, float frameTimerMax, int frameCount = 0, int rowIndex = 0, unsigned int activeFrame = 0);
		void playAnimation(std::string stateName, bool oneShot = false, std::string returnState = "");
		void playAnimation(std::string stateName, bool oneShot, std::string returnState, unsigned int activeFrame); // Overloaded version to launch a different animation on the activeFrame of the first animation
		void animate(float dt = 0.016667f);
		void setSmooth(bool smooth);

		void setAnimationLoopRange(std::string stateName, unsigned int loopStartFrame, unsigned int loopEndFrame); // configures that animation's repeating frame range (1-based, inclusive, same counting as FamiliarAttack::activeFrame) - see Textures::setLoopRange
		void setAnimationLooping(std::string stateName, bool looping); // true: that animation's playback wraps within its configured loop range instead of advancing past it. false: lets the current pass finish naturally and continue on - see Textures::setLoopActive

		bool isAnimationCurrentlyShowing(std::string stateName) const; // true if `stateName` is the one actually playing right now (one-shot or not)
		unsigned int getCurrentFrame(std::string stateName) const; // 1-based frame that animation is currently sitting on (same counting as FamiliarAttack::activeFrame) - only meaningful while it IS the one playing

		// True exactly once, the tick stateName's animation's displayed frame genuinely changes -
		// outNewFrame (1-based) is only meaningful when this returns true. Only ever true for
		// whichever animation is actually currentState, since a non-playing animation's frame
		// never changes - no need to separately check isAnimationCurrentlyShowing() first. Lets
		// gameplay timing (see FamiliarAttack's pendingShotTargetFrame, read in Familiar::update())
		// react to the exact same clock the sprite is drawn from.
		bool consumeFrameChange(std::string stateName, unsigned int& outNewFrame);

		// Only the CURRENTLY PLAYING animation's frame state needs saving/restoring - every other
		// registered animation sits frozen wherever it last was until playAnimation() switches back
		// to it, which resets it anyway (see Sprites::playAnimation), so there's nothing to lose by
		// leaving them out. Used by frame-stepping (see FrameStepHistory).
		struct State
		{
			std::string currentState;
			bool isOneShot = false;
			std::string returnToState;
			Textures::State currentAnimationFrame;
		};

		State captureState() const;
		void restoreState(const State& state);
};
