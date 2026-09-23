#pragma once
#include "stdafx.h"
#include "Sprites.h"
#include "EnemyType.h"
#include "Text.h"
#include "BigNumber.h"

class Enemy : public EnemyType
{
private:
	struct DamageNumbers
	{
		Text* text;
		float elapsed;
	};

	BigNumber health;
	float damageTextDuration = 2.0f; // Duration for which the damage text is displayed
	bool deathAnimationStarted = false;
	bool isBoss = false;
	float bossTimeRemaining = 0.0f;

	Sprites* sprites;
	Fonts* fonts;
	std::vector<DamageNumbers> damageNumbers;

	sf::RectangleShape healthBarBackground;
	sf::RectangleShape healthBarFill;
	Text* healthText;

	sf::RectangleShape timerBarBackground;
	sf::RectangleShape timerBarFill;
	Text* timerText = nullptr;

	void initHealth(BigNumber maxHealth);
	void initFonts();
	void initHealthBar();

	void updateHealthBar();
	void showDamage(BigNumber damage, bool isCritical);
	void initBossTimer();
	void updateTimerBar();

public:
	Enemy(std::string spriteAddress, sf::Vector2u spriteSize, BigNumber maxHealth, float frameTimerMax,
		std::string hurtSpriteAddress, sf::Vector2u hurtSpriteSize, float hurtFrameTimerMax,
		std::string deathSpriteAddress, sf::Vector2u deathSpriteSize, float deathFrameTimerMax, BigNumber gold,
		bool isBoss = false, ElementalAffinity elementalAffinity = ElementalAffinity());
	virtual ~Enemy();

	BigNumber getHealth() const;
	const ElementalAffinity& getElementalAffinity() const; // weaknesses/resistances, inherited from EnemyType

	bool isClicked(sf::Vector2f mousePos) const;
	bool isDead() const;
	bool isDeathAnimationFinished() const;
	bool isShowingDamageText() const;

	Sprites* getSprites() const;
	bool getIsDeathAnimationStarted() const;

	void takeDamage(BigNumber damage, bool isCritical);
	void setHealth(BigNumber newHealth); // debug only, clamps to >= 0
	void updateDamageText(float dt);
	void drawDamageText(sf::RenderWindow* window);
	void drawHealthBar(sf::RenderWindow* window);
	void refreshHealthBar();
	void loadDeathAnimation();

	bool getIsBoss() const;
	void updateBossTimer(float dt);
	bool isBossTimeUp() const;
	void drawBossTimer(sf::RenderWindow* window);

	float getBossTimeElapsed() const; // seconds since the boss encounter started (bossTimeLimit - bossTimeRemaining)
};