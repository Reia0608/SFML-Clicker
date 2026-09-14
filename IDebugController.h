#pragma once
#include "stdafx.h"
#include "BigNumber.h"

// Lets DebugTab poke at game state without depending on Game (or Hero/EnemySpawner/etc.)
// directly - same interface-segregation pattern as ISaveable and IWindowController.
class IDebugController
{
	public:
		virtual ~IDebugController() = default;

		virtual void debugSetHeroAttackPower(BigNumber value) = 0;
		virtual void debugSetHeroCurrency(BigNumber value) = 0;
		virtual void debugSetEnemyHealth(BigNumber value) = 0;
		virtual void debugSetItemDropChance(float value) = 0;
		virtual void debugSetHeroCritChance(float value) = 0;
		virtual void debugSetHeroCritDamage(int percent) = 0; // e.g. 150 means 150% -> multiplier 1.5

		virtual void debugResetHeroStats() = 0;
		virtual void debugResetItems() = 0;
		virtual void debugResetShopPrices() = 0;
};