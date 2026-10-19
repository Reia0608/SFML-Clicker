#pragma once
#include "stdafx.h"
#include "Textures.h"
#include "ItemDefinition.h"
#include <vector>
#include <optional>

class ItemManager
{
private:
	float dropChance = 1.0f;
	Textures* iconSheet;
	std::vector<ItemDefinition> itemDefinitions;
	std::vector<int> inventoryCounts;

	const sf::Vector2u iconFrameSize = { 32, 32 };

	void initItemDefinitions();

public:
	ItemManager();
	~ItemManager();

	std::optional<int> rollBossDrop(); // rolls for a boss item drop; returns the dropped item's index on success, or nullopt if the roll missed. Does NOT touch the inventory anymore - see addItem()

	int getItemCount() const;
	const ItemDefinition& getItemDefinition(int index) const;
	int getInventoryCount(int index) const;
	void setInventoryCount(int index, int count); // restores a saved inventory count
	void addItem(int index); // grants one of the given item - called once a LootDropManager drop visually reaches the ItemBar
	void resetInventory(); // sets all inventory counts back to 0 (debug use)
	void adjustAllInventoryCounts(int delta); // adds delta (can be negative) to every item's count at once, clamped at 0 - debug use
	float getDropChance() const;
	void setDropChance(float value); // debug use

	bool trySpendItem(int index, int amount); // returns false and does nothing if insufficient

	Textures* getIconSheet() const;
	sf::Vector2u getIconFrameSize() const;
};