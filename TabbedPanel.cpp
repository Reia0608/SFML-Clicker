#include "stdafx.h"
#include "TabbedPanel.h"
#include "PublicVariables.h"
#include <algorithm>

void TabbedPanel::updateLayout()
{
	float panelX = gameAreaWidth + itemBarWidth;

	background.setSize({ tabPanelWidth, windowSize.y });
	background.setPosition({ panelX, 0.0f });

	tabHeaderStrip.setSize({ tabPanelWidth, headerHeight });
	tabHeaderStrip.setPosition({ panelX, 0.0f });

	if (!tabButtons.empty()) // distribute tabs evenly in the header list
	{
		float buttonWidth = tabPanelWidth / tabButtons.size();
		for (size_t i = 0; i < tabButtons.size(); i++)
		{
			tabButtons[i]->setLayout({ panelX + i * buttonWidth, 0.0f }, { buttonWidth, headerHeight });
		}
	}

	contentAreaOrigin = { panelX, headerHeight };
	contentAreaSize = { tabPanelWidth, windowSize.y - headerHeight };
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

	updateLayout();
}

TabbedPanel::~TabbedPanel()
{
	for (auto* button : tabButtons) delete button;
	for (auto* content : tabContents) delete content;
}

void TabbedPanel::addTab(std::string name, TabContent* content)
{
	size_t index = tabContents.size();

	UIButton* headerButton = new UIButton(name, [this, index]() { activeTabIndex = index; });
	tabButtons.push_back(headerButton);
	tabContents.push_back(content);
	scrollOffsets.push_back(0.0f); // every tab starts scrolled to the top

	updateLayout();
}

void TabbedPanel::onResize()
{
	updateLayout();
}

void TabbedPanel::render(sf::RenderWindow* window)
{
	window->draw(background);
	window->draw(tabHeaderStrip);

	for (size_t i = 0; i < tabButtons.size(); i++)
	{
		tabButtons[i]->setNotifying(i != activeTabIndex && tabContents[i]->hasPendingNotification());
		tabButtons[i]->render(window);
	}

	if (!tabContents.empty())
	{
		TabContent* content = tabContents[activeTabIndex];

		float maxScroll = std::max(0.0f, content->getContentHeight() - contentAreaSize.y);
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
		}
		else
		{
			content->render(window);   // fits without scrolling - draw it as before, no clipping needed
		}
	}
}

bool TabbedPanel::handleClick(sf::Vector2f pos)
{
	for (auto* button : tabButtons)
	{
		if (button->handleClick(pos)) return true;
	}

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
