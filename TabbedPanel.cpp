#include "stdafx.h"
#include "TabbedPanel.h"
#include "PublicVariables.h"

void TabbedPanel::updateLayout()
{
	float panelX = gameAreaWidth + itemBarWidth;

	background.setSize({ tabPanelWidth, windowSize.y });
	background.setPosition({ panelX, 0.0f });

	const float headerHeight = 40.0f;
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

	sf::Vector2f contentOrigin = { panelX, headerHeight };
	sf::Vector2f contentSize = { tabPanelWidth, windowSize.y - headerHeight };
	for (auto* content : tabContents)   // layout all the tabs, not only the active ones, so it looks correct
	{
		content->onResize(contentOrigin, contentSize);
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
		tabContents[activeTabIndex]->render(window);
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
		return tabContents[activeTabIndex]->handleClick(pos);
	}

	return false;
}

void TabbedPanel::handleTextEntered(char32_t unicode)
{
	if (!tabContents.empty()) tabContents[activeTabIndex]->handleTextEntered(unicode);
}

void TabbedPanel::handleBackspace()
{
	if (!tabContents.empty()) tabContents[activeTabIndex]->handleBackspace();
}
