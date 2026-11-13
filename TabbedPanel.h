#pragma once
#include "stdafx.h"
#include "UIButton.h"
#include "TabContent.h"
#include "Text.h"
#include <functional>

class TabbedPanel
{
private:
	sf::RectangleShape background;
	sf::RectangleShape tabHeaderStrip;   // Placeholder where I'll later place the tab buttons.
	sf::RectangleShape titleStrip;       // thin strip just below the tab buttons, holding the active tab's one-word title
	sf::RectangleShape scrollbarTrack;   // faint full-height track on the right edge of the content area
	sf::RectangleShape scrollbarThumb;   // the part you drag - sized/positioned to match how much is scrolled

	std::vector<UIButton*> tabButtons;
	std::vector<TabContent*> tabContents;
	std::vector<std::string> tabNames;   // one-word name per tab, used as the page title (the button itself now shows only an icon)
	std::vector<float> scrollOffsets;    // per-tab vertical scroll offset in pixels, aligned with tabContents (0 = top)
	std::vector<std::function<bool()>> tabIsVisible; // per-tab visibility predicate (default: always visible)
	std::vector<bool> tabWasVisible;     // last frame's visibility, to detect a tab newly becoming available
	std::vector<bool> tabRevealPending;  // true from the moment a tab first becomes visible until its content is actually viewed - drives the "blink until first opened" pulse
	size_t activeTabIndex = 0;

	bool isDraggingScrollbar = false;   // true from the moment the thumb (or track) is pressed until the mouse button is released
	float dragGrabOffsetY = 0.0f;       // distance from the thumb's top edge to where the mouse grabbed it, so dragging doesn't snap the thumb to be centered on the cursor

	Text* titleLabel; // one reusable label - its text is swapped to the active tab's name each frame instead of keeping one per tab

	static constexpr float headerHeight = 40.0f;   // height of the strip holding the tab buttons
	static constexpr float titleHeight = 32.0f;    // height of the strip holding the page title, directly below the header
	static constexpr float scrollbarWidth = 6.0f;  // width of the scrollbar track/thumb
	static constexpr float scrollbarMargin = 2.0f; // gap between the scrollbar and the panel's right edge
	static constexpr float scrollbarMinThumbHeight = 20.0f; // keeps the thumb grabbable even for very long content
	sf::Vector2f contentAreaOrigin;                 // top-left corner of the area below header+title where tab content is drawn
	sf::Vector2f contentAreaSize;                   // size of that area

	void updateLayout();
	void drawScrollbar(sf::RenderWindow* window, float scrollOffset, float maxScroll, float contentHeight);

	// Single source of truth for where the thumb is - drawScrollbar() and the click/drag handling below
	// both call this, so the rectangle you can grab is always exactly the rectangle you see on screen.
	sf::FloatRect getScrollbarThumbRect(float scrollOffset, float maxScroll, float contentHeight) const;

	// Checks a mouse-down against the active tab's scrollbar (thumb or bare track). Returns true and
	// starts a drag if it hit; otherwise leaves everything alone so the click can fall through to tab
	// buttons/content as before.
	bool handleScrollbarMousePressed(sf::Vector2f pos);

	// Shared by the initial track-click jump and every drag-move afterward: turns a mouse Y position
	// into a scroll offset for the active tab, honoring dragGrabOffsetY so the thumb tracks the cursor.
	void updateScrollbarDrag(sf::Vector2f mousePos, float maxScroll, float contentHeight);

public:
	TabbedPanel();
	~TabbedPanel();

	// name: shown both as the page title at the top of the tab's content and (if loading iconImagePath
	// fails) as a fallback - keep it to one word, it's reused as-is for the title.
	// iconImagePath: the picture shown on the tab's header button, e.g. "Textures/UI/gold_coin_pouch.png".
	// isVisible: if omitted, the tab is always shown. Otherwise the tab's header button (and the space it
	// takes in the header strip) only appears once isVisible() returns true - e.g. the Familiars tab staying
	// hidden until a familiar has been bought - and it blinks (like an unseen achievement) until first opened.
	void addTab(std::string name, std::string iconImagePath, TabContent* content, std::function<bool()> isVisible = nullptr);

	// Re-seeds every tab's "was visible" state to whatever is true right now, and clears any pending
	// blink. Call this once, right after Game::loadFromFile() finishes. Tabs are added (addTab()) before
	// the save file is read, so a tab that was already unlocked in a PREVIOUS session still looks like
	// a brand-new reveal the moment loadFromFile() restores that unlock. This tells TabbedPanel "that's
	// not new, don't blink" for anything the save file just caught up on - a tab that becomes visible
	// from an actual purchase/unlock made during play, after this call, still blinks exactly as before.
	void resyncVisibilityAfterLoad();

	void onResize();
	void render(sf::RenderWindow* window);
	bool handleClick(sf::Vector2f pos);
	void handleMouseMoved(sf::Vector2f pos);     // continues a scrollbar drag, if one is in progress
	void handleMouseReleased();                  // ends a scrollbar drag, if one is in progress
	void handleMouseWheelScrolling(sf::Vector2f mousePos, float deltaY); // scrolls the active tab if its content overflows the visible area
	void handleTextEntered(char32_t unicode);
	void handleBackspace();
};
