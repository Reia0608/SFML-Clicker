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
};