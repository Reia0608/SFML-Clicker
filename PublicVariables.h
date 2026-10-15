#pragma once

#include "stdafx.h"
#include <algorithm>

// One value per SPECIES a familiar can be - stable across that familiar's own evolutions. Which
// evolution stage a given familiar is currently at is a separate axis entirely (see the Age enum
// below) - a Familiar object's identity is the (FamiliarName, Age) PAIR, never FamiliarName alone.
// FamiliarNameCount is not a real species - keep it last; it only exists to size per-species
// containers like Game::ownedFamiliars. Add a new species above it whenever one is implemented.
enum FamiliarName
{
	None,
	FireDragon,
	WaterDragon,
	WindDragon,
	EarthDragon,
	IceDragon,
	FamiliarNameCount
};

enum Age
{
	Baby,		// 0
	Juvenile,	// 1
	Adult,		// 2
	Ancient		// 3
};

inline sf::Vector2f windowSize = { 800.0f, 640.0f }; // width of the whole screen including gameAreaWidth and the UI part
inline float itemBarWidth = 60.0f;

// tabPanelWidth used to be a fixed 380px, so resizing the window (or switching resolution in
// OptionsTab) never gave the panel - and therefore every TabContent laid out against it, e.g.
// AchievementsTab - any more or less horizontal room. On a small window it felt cramped; on a
// much bigger one all the extra width just went to gameAreaWidth while the panel (and its text,
// which isn't wrapped) stayed exactly as tight as before. It's now derived from windowSize.x
// every time the window changes size - see recalculateTabPanelWidth() - clamped between
// tabPanelWidthMin/Max so it never becomes too narrow to read or absurdly wide on ultrawide monitors.
inline float tabPanelWidth = 380.0f;
inline float tabPanelWidthRatio = 0.35f; // target: 35% of the current window width, before clamping
inline float tabPanelWidthMin = 300.0f;  // narrowest the panel is allowed to get
inline float tabPanelWidthMax = 460.0f;  // widest the panel is allowed to get, so a huge window doesn't hand it all the space

inline float uiPanelWidth = itemBarWidth + tabPanelWidth;
inline float gameAreaWidth = windowSize.x - uiPanelWidth; // width of the play area (Hero, Enemy and Background)
inline float spriteScale = 2.0f;
inline double enemyHealthGrowthRate = 1.10;   // typically between 1.10 and 1.30
inline double goldGrowthRate = 1.12;
inline double bossHealthMultiplier = 5.0;    // the boss has 5 times the amount of life of a regular enemy
inline double bossGoldMultiplier = 5.0;
inline int enemiesPerStage = 10;              // number of regular enemies before the boss
inline float bossTimeLimit = 15.0f;
inline bool deathAnimationsEnabled = true;	  // for options menu, if false, enemies will die instantly without playing their death animation

// Recomputes tabPanelWidth (and uiPanelWidth, which depends on it) from the current windowSize.x.
// Must be called BEFORE recalculateGameAreaWidth() any time windowSize changes, since
// gameAreaWidth is derived from uiPanelWidth, which is derived from tabPanelWidth.
inline void recalculateTabPanelWidth()
{
	tabPanelWidth = std::clamp(windowSize.x * tabPanelWidthRatio, tabPanelWidthMin, tabPanelWidthMax);
	uiPanelWidth = itemBarWidth + tabPanelWidth;
}

inline void recalculateGameAreaWidth()
{
	gameAreaWidth = windowSize.x - uiPanelWidth;
	if (gameAreaWidth < 64.0f)
	{
		gameAreaWidth = 64.0f;
	}
}