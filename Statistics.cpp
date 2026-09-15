#include "stdafx.h"
#include "Statistics.h"
#include <sstream>
#include <iomanip>

void Statistics::recordEnemyDefeated() 
{
	enemiesDefeated++; 
}

void Statistics::recordBossDefeated() 
{ 
	bossesDefeated++; 
}

void Statistics::recordShopUpgradePurchased() 
{ 
	shopUpgradesPurchased++; 
}

void Statistics::recordClick()
{
	totalClicks++;
}

void Statistics::updatePlayTime(float dt) 
{ 
	playTimeSeconds += dt;
}

void Statistics::setHighestStageReached(int stage)
{ 
	if (stage > highestStageReached) highestStageReached = stage; 
}

int Statistics::getEnemiesDefeated() const 
{
	return enemiesDefeated;
}

int Statistics::getBossesDefeated() const 
{ 
	return bossesDefeated;
}

int Statistics::getHighestStageReached() const
{
	return highestStageReached; 
}

int Statistics::getShopUpgradesPurchased() const 
{ 
	return shopUpgradesPurchased;
}

int Statistics::getTotalClicks() const
{
	return totalClicks; 
}

std::string Statistics::getFormattedPlayTime() const
{
	int totalSeconds = static_cast<int>(playTimeSeconds);
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;

	std::ostringstream out;
	out << std::setfill('0') << std::setw(2) << hours << ":"
		<< std::setfill('0') << std::setw(2) << minutes << ":"
		<< std::setfill('0') << std::setw(2) << seconds;
	return out.str();
}

void Statistics::recordCriticalHit()
{ 
	criticalHitsLanded++;
}

int Statistics::getCriticalHitsLanded() const
{
	return criticalHitsLanded; 
}

float Statistics::getPlayTimeSeconds() const
{
	return playTimeSeconds; 
}

void Statistics::recordGoldEarned(BigNumber amount)
{
	totalGoldEarned = totalGoldEarned + amount;
}

BigNumber Statistics::getTotalGoldEarned() const
{
	return totalGoldEarned;
}

void Statistics::recordBossKillTime(float seconds)
{
	if (fastestBossKillSeconds < 0.0f || seconds < fastestBossKillSeconds)
	{
		fastestBossKillSeconds = seconds;
	}
}

float Statistics::getFastestBossKillSeconds() const
{
	return fastestBossKillSeconds;
}