#include "stdafx.h"
#include "TabbedPanel.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include <algorithm>

void TabbedPanel::updateLayout()
{
	float panelX = gameAreaWidth + itemBarWidth;

	background.setSize({ tabPanelWidth, windowSize.y });
	background.setPosition({ panelX, 0.0f });

	tabHeaderStrip.setSize({ tabPanelWidth, headerHeight });
	tabHeaderStrip.setPosition({ panelX, 0.0f });

	titleStrip.setSize({ tabPanelWidth, titleHeight });
	titleStrip.setPosition({ panelX, headerHeight });

	// Only currently-visible tabs get a slot in the header strip - a hidden tab (e.g. Familiars
	// before a familiar has been bought) reserves no space, same "hidden takes no space" rule
	// ShopTab/FamiliarTab already use for their own buttons.
	std::vector<size_t> visibleIndices;
	for (size_t i = 0; i < tabButtons.size(); i++)
	{
		if (tabIsVisible[i]()) visibleIndices.push_back(i);
	}

	if (!visibleIndices.empty())
	{
		float buttonWidth = tabPanelWidth / visibleIndices.size();
		for (size_t slot = 0; slot < visibleIndices.size(); slot++)
		{
			size_t i = visibleIndices[slot];
			tabButtons[i]->setLayout({ panelX + slot * buttonWidth, 0.0f }, { buttonWidth, headerHeight });
		}
	}

	contentAreaOrigin = { panelX, headerHeight + titleHeight };
	contentAreaSize = { tabPanelWidth, windowSize.y - headerHeight - titleHeight };
	for (auto* content : tabContents)   // layout all the tabs, not only the active ones, so it looks correct
	{
		content->onResize(contentAreaOrigin, contentAreaSize);
	}
}

TabbedPanel::TabbedPanel()
{
	background.setFillColor(sf::Color(55, 55, 55));
	background.setOutlineColor(sf::Color::Black);
	background.setOutlineThickness(1.0f);

	tabHeaderStrip.setFillColor(sf::Color(30, 30, 30));

	titleStrip.setFillColor(sf::Color(40, 40, 40)); // sits between the dark header strip and the lighter panel background

	titleLabel = new Text("", "Fonts/arial.ttf");
	titleLabel->setFillColor(sf::Color::White);
	titleLabel->setCharacterSize(18);

	scrollbarTrack.setFillColor(sf::Color(20, 20, 20, 180));
	scrollbarThumb.setFillColor(sf::Color(150, 150, 150, 220));

	updateLayout();
}

TabbedPanel::~TabbedPanel()
{
	for (auto* button : tabButtons) delete button;
	for (auto* content : tabContents) delete content;
	delete titleLabel;
}

void TabbedPanel::addTab(std::string name, std::string iconImagePath, TabContent* content, std::function<bool()> isVisible)
{
	size_t index = tabContents.size();

	UIButton* headerButton = UIButton::createIconButton(iconImagePath, [this, index]() { activeTabIndex = index; });
	tabButtons.push_back(headerButton);
	tabContents.push_back(content);
	tabNames.push_back(name);
	scrollOffsets.push_back(0.0f); // every tab starts scrolled to the top

	tabIsVisible.push_back(isVisible ? isVisible : std::function<bool()>([]() { return true; }));
	bool initiallyVisible = tabIsVisible.back()();
	tabWasVisible.push_back(initiallyVisible); // seed with the current state, so a tab visible from the start never blinks
	tabRevealPending.push_back(false);

	updateLayout();
}

void TabbedPanel::resyncVisibilityAfterLoad()
{
	for (size_t i = 0; i < tabIsVisible.size(); i++)
	{
		tabWasVisible[i] = tabIsVisible[i](); // whatever the save file just restored counts as "already seen", not a fresh reveal
		tabRevealPending[i] = false;
	}
}

void TabbedPanel::onResize()
{
	updateLayout();
}

sf::FloatRect TabbedPanel::getScrollbarThumbRect(float scrollOffset, float maxScroll, float contentHeight) const
{
	float trackX = contentAreaOrigin.x + contentAreaSize.x - scrollbarWidth - scrollbarMargin;

	float visibleFraction = contentAreaSize.y / contentHeight; // 0..1 - how much of the content fits on screen at once
	float thumbHeight = std::max(scrollbarMinThumbHeight, contentAreaSize.y * visibleFraction);
	float scrollFraction = maxScroll > 0.0f ? scrollOffset / maxScroll : 0.0f; // 0 = top, 1 = bottom
	float thumbY = contentAreaOrigin.y + scrollFraction * (contentAreaSize.y - thumbHeight);

	return sf::FloatRect({ trackX, thumbY }, { scrollbarWidth, thumbHeight });
}

void TabbedPanel::drawScrollbar(sf::RenderWindow* window, float scrollOffset, float maxScroll, float contentHeight)
{
	float trackX = contentAreaOrigin.x + contentAreaSize.x - scrollbarWidth - scrollbarMargin;

	scrollbarTrack.setPosition({ trackX, contentAreaOrigin.y });
	scrollbarTrack.setSize({ scrollbarWidth, contentAreaSize.y });
	window->draw(scrollbarTrack);

	sf::FloatRect thumbRect = getScrollbarThumbRect(scrollOffset, maxScroll, contentHeight);
	scrollbarThumb.setPosition(thumbRect.position);
	scrollbarThumb.setSize(thumbRect.size);
	window->draw(scrollbarThumb);
}

void TabbedPanel::render(sf::RenderWindow* window)
{
	window->draw(background);
	window->draw(tabHeaderStrip);

	std::vector<bool> currentlyVisible(tabButtons.size());
	bool visibilityChanged = false;

	for (size_t i = 0; i < tabButtons.size(); i++)
	{
		bool visible = tabIsVisible[i]();
		currentlyVisible[i] = visible;

		if (visible != tabWasVisible[i])
		{
			visibilityChanged = true;
			if (visible) tabRevealPending[i] = true; // just became available - blink until the player opens it
			tabWasVisible[i] = visible;
		}

		tabButtons[i]->setEnabled(visible);
		if (!visible) continue; // hidden tabs reserve no header space and are never rendered/clickable

		bool pending = tabRevealPending[i] || tabContents[i]->hasPendingNotification();
		tabButtons[i]->setNotifying(i != activeTabIndex && pending);
		tabButtons[i]->render(window);
	}

	if (visibilityChanged) updateLayout(); // a tab appeared - redistribute the header strip to include it

	window->draw(titleStrip);
	if (activeTabIndex < tabNames.size())
	{
		titleLabel->setString(tabNames[activeTabIndex]);
		LayoutManager::applyAnchor(*titleLabel, Anchor::MiddleCenter, { 0.0f, 0.0f }, titleStrip.getPosition(), titleStrip.getSize());
		window->draw(*titleLabel);
	}

	if (!tabContents.empty() && currentlyVisible[activeTabIndex])
	{
		TabContent* content = tabContents[activeTabIndex];
		tabRevealPending[activeTabIndex] = false; // being viewed right now - stop blinking

		float contentHeight = content->getContentHeight();
		float maxScroll = std::max(0.0f, contentHeight - contentAreaSize.y);
		scrollOffsets[activeTabIndex] = std::min(scrollOffsets[activeTabIndex], maxScroll); // clamp in case the window grew and there is less to scroll now

		if (maxScroll > 0.0f)   // tab's content is taller than the visible area -> it is scrollable
		{
			// re-layout the active tab with a scrolled origin so its widgets move up as you scroll down
			content->onResize({ contentAreaOrigin.x, contentAreaOrigin.y - scrollOffsets[activeTabIndex] }, contentAreaSize);

			// draw only inside the visible content area (scissor rectangle), so rows that are scrolled out of sight
			// don't get drawn over the header or other UI. The scissor clips in target pixel space without touching
			// the view's transform, so widgets render at exactly their laid-out positions with no scaling.
			sf::View previousView = window->getView();
			sf::Vector2u targetSize = window->getSize();
			sf::View clippedView = previousView; // keep the same camera/transform as for the rest of the frame
			clippedView.setScissor(sf::FloatRect(
				{ contentAreaOrigin.x / static_cast<float>(targetSize.x), contentAreaOrigin.y / static_cast<float>(targetSize.y) },
				{ contentAreaSize.x / static_cast<float>(targetSize.x), contentAreaSize.y / static_cast<float>(targetSize.y) }
			));
			window->setView(clippedView);

			content->render(window);

			window->setView(previousView); // restore the normal view (default scissor = whole target) for everything drawn after this panel

			drawScrollbar(window, scrollOffsets[activeTabIndex], maxScroll, contentHeight); // drawn outside the scissor so it's never clipped
		}
		else
		{
			content->render(window);   // fits without scrolling - draw it as before, no clipping needed
		}
	}
}

bool TabbedPanel::handleScrollbarMousePressed(sf::Vector2f pos)
{
	if (tabContents.empty()) return false;

	TabContent* content = tabContents[activeTabIndex];
	float contentHeight = content->getContentHeight();
	float maxScroll = std::max(0.0f, contentHeight - contentAreaSize.y);
	if (maxScroll <= 0.0f) return false; // this tab fits without scrolling - no scrollbar is even drawn

	sf::FloatRect thumbRect = getScrollbarThumbRect(scrollOffsets[activeTabIndex], maxScroll, contentHeight);

	if (thumbRect.contains(pos))
	{
		isDraggingScrollbar = true;
		dragGrabOffsetY = pos.y - thumbRect.position.y; // keeps the thumb's position under the cursor as you drag, instead of snapping its top to the cursor
		return true;
	}

	sf::FloatRect trackRect({ thumbRect.position.x, contentAreaOrigin.y }, { scrollbarWidth, contentAreaSize.y });
	if (trackRect.contains(pos))
	{
		// clicked the bare track above/below the thumb - jump the thumb so its center lands on the click,
		// then start dragging from there so you can keep dragging without having to click a second time
		isDraggingScrollbar = true;
		dragGrabOffsetY = thumbRect.size.y / 2.0f;
		updateScrollbarDrag(pos, maxScroll, contentHeight);
		return true;
	}

	return false;
}

void TabbedPanel::updateScrollbarDrag(sf::Vector2f mousePos, float maxScroll, float contentHeight)
{
	float visibleFraction = contentAreaSize.y / contentHeight;
	float thumbHeight = std::max(scrollbarMinThumbHeight, contentAreaSize.y * visibleFraction);
	float trackSpan = contentAreaSize.y - thumbHeight; // how far the thumb's top can travel, top to bottom

	float thumbY = mousePos.y - dragGrabOffsetY;
	float scrollFraction = trackSpan > 0.0f
		? std::clamp((thumbY - contentAreaOrigin.y) / trackSpan, 0.0f, 1.0f)
		: 0.0f;

	scrollOffsets[activeTabIndex] = scrollFraction * maxScroll;
}

bool TabbedPanel::handleClick(sf::Vector2f pos)
{
	for (auto* button : tabButtons)
	{
		if (button->handleClick(pos)) return true;
	}

	if (handleScrollbarMousePressed(pos)) return true; // grabbing the scrollbar takes priority over the content underneath it

	if (!tabContents.empty())
	{
		// The active scrollable tab is re-laid-out in screen coordinates every frame (see render()), so its widgets
		// already account for the current scroll offset - hit-test them directly without shifting the click again.
		// Shifting here as well would double-apply the offset and make scrolled buttons unclickable.
		// Clicks outside the content area also reach the tab, e.g. text input fields rely on this to defocus when you click away
		return tabContents[activeTabIndex]->handleClick(pos);
	}

	return false;
}

void TabbedPanel::handleMouseMoved(sf::Vector2f pos)
{
	if (!isDraggingScrollbar || tabContents.empty()) return;

	TabContent* content = tabContents[activeTabIndex];
	float contentHeight = content->getContentHeight();
	float maxScroll = std::max(0.0f, contentHeight - contentAreaSize.y);
	if (maxScroll <= 0.0f)
	{
		isDraggingScrollbar = false; // the tab stopped being scrollable mid-drag (e.g. window resize) - nothing left to drag
		return;
	}

	updateScrollbarDrag(pos, maxScroll, contentHeight);
}

void TabbedPanel::handleMouseReleased()
{
	isDraggingScrollbar = false;
}

void TabbedPanel::handleMouseWheelScrolling(sf::Vector2f mousePos, float deltaY)
{
	if (tabContents.empty()) return;

	sf::FloatRect contentArea(contentAreaOrigin, contentAreaSize);
	if (!contentArea.contains(mousePos)) return; // only scroll while the cursor is over the tab's content area

	TabContent* content = tabContents[activeTabIndex];
	float maxScroll = std::max(0.0f, content->getContentHeight() - contentAreaSize.y);
	if (maxScroll <= 0.0f) return; // this tab fits without scrolling

	const float step = 80.0f; // pixels of scroll per wheel notch
	scrollOffsets[activeTabIndex] = std::min(maxScroll, std::max(0.0f, scrollOffsets[activeTabIndex] - deltaY * step));
}

void TabbedPanel::handleTextEntered(char32_t unicode)
{
	if (!tabContents.empty()) tabContents[activeTabIndex]->handleTextEntered(unicode);
}

void TabbedPanel::handleBackspace()
{
	if (!tabContents.empty()) tabContents[activeTabIndex]->handleBackspace();
}
