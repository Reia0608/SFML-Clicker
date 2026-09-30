#pragma once
#include "stdafx.h"
#include "ItemManager.h"
#include "Text.h"
#include <vector>

class ItemBar
{
private:
	sf::RectangleShape background;
	ItemManager* itemManager;   // non-owning, part of Game

	struct ItemSlot
	{
		sf::Sprite* icon = NULL;
		Text* countText = NULL;
		sf::Vector2f layoutPosition = { 0,0 };
	};
	std::vector<ItemSlot> slots;

	void initSlots();
	void updateLayout();

public:
	ItemBar(ItemManager* itemManager);
	~ItemBar();

	void onResize();
	void render(sf::RenderWindow* window);

	sf::Vector2f getSlotPosition(int index) const; // world position of the given item's icon (its on-screen slot in the bar) - used by LootDropManager as the fly-to target for item drops. Returns {0,0} if index is out of range.
};