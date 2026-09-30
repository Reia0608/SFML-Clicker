#pragma once
#include "stdafx.h"
#include "UIButton.h"
#include "TabContent.h"
#include <functional>

class TabbedPanel
{
private:
	sf::RectangleShape background;
	sf::RectangleShape tabHeaderStrip;   // Placeholder where I'll later place the tab buttons.

	std::vector<UIButton*> tabButtons;
	std::vector<TabContent*> tabContents;
	std::vector<float> scrollOffsets;    // per-tab vertical scroll offset in pixels, aligned with tabContents (0 = top)
	std::vector<std::function<bool()>> tabIsVisible; // per-tab visibility predicate (default: always visible)
	std::vector<bool> tabWasVisible;     // last frame's visibility, to detect a tab newly becoming available
	std::vector<bool> tabRevealPending;  // true from the moment a tab first becomes visible until its content is actually viewed - drives the "blink until first opened" pulse
	size_t activeTabIndex = 0;

	static constexpr float headerHeight = 40.0f;   // height of the strip holding the tab buttons
	sf::Vector2f contentAreaOrigin;                 // top-left corner of the area below the header where tab content is drawn
	sf::Vector2f contentAreaSize;                   // size of that area

	void updateLayout();

public:
	TabbedPanel();
	~TabbedPanel();

	// isVisible: if omitted, the tab is always shown. Otherwise the tab's header button (and the space it
	// takes in the header strip) only appears once isVisible() returns true - e.g. the Familiars tab staying
	// hidden until a familiar has been bought - and it blinks (like an unseen achievement) until first opened.
	void addTab(std::string name, TabContent* content, std::function<bool()> isVisible = nullptr);
	void onResize();
	void render(sf::RenderWindow* window);
	bool handleClick(sf::Vector2f pos);
	void handleMouseWheelScrolling(sf::Vector2f mousePos, float deltaY); // scrolls the active tab if its content overflows the visible area
	void handleTextEntered(char32_t unicode);
	void handleBackspace();
};