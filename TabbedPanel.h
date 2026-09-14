#pragma once
#include "stdafx.h"
#include "UIButton.h"
#include "TabContent.h"

class TabbedPanel
{
	private:
		sf::RectangleShape background;
		sf::RectangleShape tabHeaderStrip;   // Placeholder where I'll later place the tab buttons.

		std::vector<UIButton*> tabButtons;
		std::vector<TabContent*> tabContents;
		size_t activeTabIndex = 0;

		void updateLayout();

	public:
		TabbedPanel();
		~TabbedPanel();

		void addTab(std::string name, TabContent* content);
		void onResize();
		void render(sf::RenderWindow* window);
		bool handleClick(sf::Vector2f pos);
		void handleTextEntered(char32_t unicode);
		void handleBackspace();
};