#pragma once
#include "stdafx.h"

class TabContent
{
	public:
		virtual ~TabContent() {}

		virtual void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) = 0;
		virtual void render(sf::RenderWindow* window) = 0;
		virtual bool handleClick(sf::Vector2f pos) = 0;

		// Total height of this tab's content in its unscrolled layout, measured from the top of the
		// content area. If it exceeds the visible content area, TabbedPanel automatically enables scrolling.
		virtual float getContentHeight() const { return 0.0f; }

		// default no-ops so existing tabs don't need to implement these; only tabs with
		// text input fields (e.g. DebugTab) need to override them.
		virtual void handleTextEntered(char32_t unicode) {}
		virtual void handleBackspace() {}

		// for blinking Tabs
		virtual bool hasPendingNotification() const { return false; }
};