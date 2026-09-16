#pragma once

#include "stdafx.h"

enum FamiliarName
{
	None,
	BabyFireDragon,
	BabyIceDragon,
	FireDragon,
	IceDragon,
};

enum Age
{
	Baby,
	Juvenile,
	Adult
};

inline sf::Vector2f windowSize = { 800.0f, 640.0f }; // width of the whole screen including gameAreaWidth and the UI part
inline float itemBarWidth = 60.0f;
inline float tabPanelWidth = 380.0f;
inline float uiPanelWidth = itemBarWidth + tabPanelWidth;
inline float gameAreaWidth = windowSize.x - uiPanelWidth; // width of the play area (Hero, Enemy and Background)
inline float spriteScale = 2.0f;
inline double enemyHealthGrowthRate = 1.15;   // typically between 1.10 and 1.30
inline double goldGrowthRate = 1.12;
inline double bossHealthMultiplier = 5.0;    // the boss has 5 times the amount of life of a regular enemy
inline double bossGoldMultiplier = 5.0;
inline int enemiesPerStage = 10;              // number of regular enemies before the boss
inline float bossTimeLimit = 15.0f;
inline bool deathAnimationsEnabled = true;	  // for options menu, if false, enemies will die instantly without playing their death animation

inline void recalculateGameAreaWidth()
{
	gameAreaWidth = windowSize.x - uiPanelWidth;
	if (gameAreaWidth < 64.0f)
	{
		gameAreaWidth = 64.0f;
	}
}