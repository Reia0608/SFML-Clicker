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
		std::vector<float> scrollOffsets;    // per-tab vertical scroll offset in pixels, aligned with tabContents (0 = top)
		size_t activeTabIndex = 0;

		static constexpr float headerHeight = 40.0f;   // height of the strip holding the tab buttons
		sf::Vector2f contentAreaOrigin;                 // top-left corner of the area below the header where tab content is drawn
		sf::Vector2f contentAreaSize;                   // size of that area

		void updateLayout();

	public:
		TabbedPanel();
		~TabbedPanel();

		void addTab(std::string name, TabContent* content);
		void onResize();
		void render(sf::RenderWindow* window);
		bool handleClick(sf::Vector2f pos);
		void handleMouseWheelScrolling(sf::Vector2f mousePos, float deltaY); // scrolls the active tab if its content overflows the visible area
		void handleTextEntered(char32_t unicode);
		void handleBackspace();
};