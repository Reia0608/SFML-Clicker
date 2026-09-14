#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "Text.h"
#include "Hero.h"
#include "Statistics.h"
#include <vector>

class StatisticsTab : public TabContent
{
	private:
		Hero* hero;             // non-owning
		Statistics* statistics; // non-owning
		std::vector<Text*> lines;

		void refreshValues();

	public:
		StatisticsTab(Hero* hero, Statistics* statistics);
		~StatisticsTab();

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;
};