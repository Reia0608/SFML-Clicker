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
};
