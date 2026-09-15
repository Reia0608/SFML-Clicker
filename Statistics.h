#pragma once
#include "stdafx.h"
#include "BigNumber.h"

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
	BigNumber totalGoldEarned = BigNumber(0);         // lifetime gold earned, never decreases (unlike Hero::currency)
	float fastestBossKillSeconds = -1.0f;             // -1 = no boss defeated yet; otherwise the fastest kill time recorded

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

	// gold milestone tracking
	void recordGoldEarned(BigNumber amount);     // call every time the hero gains gold (adds, never subtracts)
	BigNumber getTotalGoldEarned() const;

	// speed-boss tracking
	void recordBossKillTime(float seconds);      // keeps the fastest (lowest) time seen; ignores slower runs
	float getFastestBossKillSeconds() const;     // returns -1.0f if no boss has been defeated yet
};