#pragma once
#include "stdafx.h"
#include "Sprites.h"
#include "EnemyType.h"
#include "Text.h"
#include "BigNumber.h"
#include "FloatingDamageText.h"
#include "Elements.h"
#include <memory>

class Enemy : public EnemyType
{
private:
	BigNumber health;
	float damageTextDuration = 2.0f; // Duration for which the damage text is displayed
	bool deathAnimationStarted = false;
	bool isBoss = false;
	float bossTimeRemaining = 0.0f;

	Sprites* sprites;
	Fonts* fonts;
	std::vector<std::unique_ptr<FloatingDamageText>> damageNumbers;

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
	// baseColor is the source-identity color (Hero red / familiar blue / passive pink). For a
	// physical hit (elementMask == 0) it's the text's fill. For an elemental hit it becomes the
	// text's OUTLINE instead, while the fill switches to that element's gradient (see
	// Elements.h::getElementColorGradient) - so both "who hit" and "what element" stay visible
	// at once. Crit still overrides everything to solid orange fill / black outline.
	void showDamage(BigNumber damage, bool isCritical, sf::Color baseColor = sf::Color::Red, ElementMask elementMask = 0);
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

	// baseColor identifies the damage's source (Hero/familiar/passive) in its floating number if
	// elementMask is 0 (a purely physical hit); elementMask != 0 uses that element's gradient
	// instead (crit always overrides both to solid orange). Both default so existing non-familiar,
	// non-elemental call sites (Hero's own click) don't need to change.
	void takeDamage(BigNumber damage, bool isCritical, sf::Color baseColor = sf::Color::Red, ElementMask elementMask = 0);
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