#include "stdafx.h"
#include "ItemManager.h"
#include "Randomize.h"

void ItemManager::initItemDefinitions()
{
	itemDefinitions.push_back({ "Ruby Emblem", 0 });
	itemDefinitions.push_back({ "Amber Emblem", 1 });
	itemDefinitions.push_back({ "Topaz Emblem", 2 });
	itemDefinitions.push_back({ "Emerald Emblem", 3 });
	itemDefinitions.push_back({ "Sapphire Emblem", 4 });
	itemDefinitions.push_back({ "Amethyst Emblem", 5 });
}

ItemManager::ItemManager()
{
	// Sprites from https://kirial1413.itch.io/pixel-emblems
	iconSheet = new Textures("Textures/PNG/Items/emblem.png", iconFrameSize);
	initItemDefinitions();
	inventoryCounts.assign(itemDefinitions.size(), 0);
}

ItemManager::~ItemManager()
{
	delete iconSheet;
}

std::optional<int> ItemManager::rollBossDrop()
{
	float roll = Randomize::returnRandomizeFloat(0.0f, 1.0f);
	if (roll <= dropChance)
	{
		int index = Randomize::returnRandomizeInt(0, static_cast<int>(itemDefinitions.size()) - 1);
		return index; // caller (EnemySpawner) fires onItemDroppedCallback with this; the inventory is only actually incremented via addItem() once the drop animation reaches the ItemBar (see LootDropManager)
	}
	return std::nullopt;
}

int ItemManager::getItemCount() const
{
	return static_cast<int>(itemDefinitions.size());
}

const ItemDefinition& ItemManager::getItemDefinition(int index) const
{
	return itemDefinitions[index];
}

int ItemManager::getInventoryCount(int index) const
{
	return inventoryCounts[index];
}

void ItemManager::setInventoryCount(int index, int count)
{
	if (index >= 0 && index < static_cast<int>(inventoryCounts.size())) inventoryCounts[index] = count;
}

void ItemManager::addItem(int index)
{
	if (index >= 0 && index < static_cast<int>(inventoryCounts.size())) inventoryCounts[index]++;
}

void ItemManager::resetInventory()
{
	std::fill(inventoryCounts.begin(), inventoryCounts.end(), 0);
}

float ItemManager::getDropChance() const
{
	return dropChance;
}

void ItemManager::setDropChance(float value)
{
	dropChance = value;
}

bool ItemManager::trySpendItem(int index, int amount)
{
	if (index < 0 || index >= static_cast<int>(inventoryCounts.size())) return false;
	if (inventoryCounts[index] < amount) return false;
	inventoryCounts[index] -= amount;
	return true;
}

Textures* ItemManager::getIconSheet() const
{
	return iconSheet;
}

sf::Vector2u ItemManager::getIconFrameSize() const
{
	return iconFrameSize;
}