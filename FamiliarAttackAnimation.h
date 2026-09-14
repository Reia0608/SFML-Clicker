#pragma once
#include "stdafx.h"
#include "Text.h"

class FamiliarAttackAnimation
{
	private:
		sf::Texture* attack1Texture;
		sf::Texture* attack2Texture;
		sf::Sprite attack1Sprite;
		sf::Sprite attack2Sprite;
		float attack1AnimationDuration = 0.5f; // seconds
		float attack2AnimationDuration = 0.5f; // seconds
		float attack1Elapsed = 0.0f;
		float attack2Elapsed = 0.0f;
		bool isAttack1Playing = false;
		bool isAttack2Playing = false;
	public:
		FamiliarAttackAnimation();
		~FamiliarAttackAnimation();
		void playAttack1();
		void playAttack2();
		void update(float dt);
		void draw(sf::RenderWindow* window);
};
