#include "stdafx.h"
#include "UnlockManager.h"
#include <algorithm>

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

void UnlockManager::recordFamiliarUnlocked(const std::string& familiarKey)
{
	bool alreadyRecorded = std::find(familiarUnlockOrder.begin(), familiarUnlockOrder.end(), familiarKey) != familiarUnlockOrder.end();
	if (alreadyRecorded) return; // a stray repeat call must never shift anything unlocked after it

	familiarUnlockOrder.push_back(familiarKey);
}

bool UnlockManager::hasAnyFamiliarUnlocked() const
{
	return !familiarUnlockOrder.empty();
}

BigNumber UnlockManager::getNextFamiliarUnlockCost() const
{
	return BigNumber(familiarUnlockBaseCost).scaleBy(10.0, static_cast<int>(familiarUnlockOrder.size()));
}

BigNumber UnlockManager::getElementalDamageThreshold(const std::string& familiarKey) const
{
	auto it = std::find(familiarUnlockOrder.begin(), familiarUnlockOrder.end(), familiarKey);
	if (it == familiarUnlockOrder.end()) return BigNumber(familiarAchievementBaseThreshold); // not recorded yet - shouldn't normally be queried this early

	int position = static_cast<int>(std::distance(familiarUnlockOrder.begin(), it));
	return BigNumber(familiarAchievementBaseThreshold).scaleBy(10.0, position);
}

std::vector<std::string> UnlockManager::getFamiliarUnlockOrder() const
{
	return familiarUnlockOrder;
}

void UnlockManager::restoreFamiliarUnlockOrder(const std::string& familiarKey)
{
	familiarUnlockOrder.push_back(familiarKey);
}
