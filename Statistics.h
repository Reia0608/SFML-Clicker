#pragma once
#include "stdafx.h"

class Statistics
{
private:
	int enemiesDefeated = 0;
	int bossesDefeated = 0;
	int highestStageReached = 1;     // Placeholder until stages exist
	int shopUpgradesPurchased = 0;
	int totalClicks = 0;
	float playTimeSeconds = 0.0f;
	int criticalHitsLanded = 0;

public:
	void recordEnemyDefeated();
	void recordBossDefeated();
	void recordShopUpgradePurchased();
	void recordClick();
	void updatePlayTime(float dt);
	void setHighestStageReached(int stage);

	int getEnemiesDefeated() const;
	int getBossesDefeated() const;
	int getHighestStageReached() const;
	int getShopUpgradesPurchased() const;
	int getTotalClicks() const;
	std::string getFormattedPlayTime() const;

	// Critical hit tracking
	void recordCriticalHit();
	int getCriticalHitsLanded() const;

	float getPlayTimeSeconds() const;
};