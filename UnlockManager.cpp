#include "stdafx.h"
#include "UnlockManager.h"

bool UnlockManager::isUnlocked(const std::string& featureKey) const
{
	if (featureKey.empty()) return true;
	return unlockedFeatures.count(featureKey) > 0;
}

void UnlockManager::unlock(const std::string& featureKey)
{
	unlockedFeatures.insert(featureKey);
}

std::vector<std::string> UnlockManager::getUnlockedFeatures() const
{
	return std::vector<std::string>(unlockedFeatures.begin(), unlockedFeatures.end());
}

void UnlockManager::restoreUnlocked(const std::string& featureKey)
{
	unlockedFeatures.insert(featureKey);
}