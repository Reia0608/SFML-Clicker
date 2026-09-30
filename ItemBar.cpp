#include "stdafx.h"
#include "ItemBar.h"
#include "PublicVariables.h"

ItemBar::ItemBar(ItemManager* itemManager) : itemManager(itemManager)
{
	background.setFillColor(sf::Color(40, 40, 40));
	background.setOutlineColor(sf::Color::Black);
	background.setOutlineThickness(1.0f);

	initSlots();
	updateLayout();
}

ItemBar::~ItemBar()
{
	for (auto& slot : slots)
	{
		delete slot.icon;
		delete slot.countText;
	}
}

void ItemBar::initSlots()
{
	sf::Vector2u frameSize = itemManager->getIconFrameSize();

	for (int i = 0; i < itemManager->getItemCount(); i++)
	{
		ItemSlot slot;

		slot.icon = new sf::Sprite(*itemManager->getIconSheet());
		int col = itemManager->getItemDefinition(i).columnIndex;
		slot.icon->setTextureRect(sf::IntRect({ col * static_cast<int>(frameSize.x), 0 }, sf::Vector2i(frameSize)));
		slot.icon->setOrigin({ frameSize.x / 2.0f, frameSize.y / 2.0f });
		slot.icon->setScale({ 2.5f, 2.5f });

		slot.countText = new Text("0", "Fonts/arial.ttf");
		slot.countText->setCharacterSize(12);
		slot.countText->setFillColor(sf::Color::White);
		slot.countText->setOutlineColor(sf::Color::Black);
		slot.countText->setOutlineThickness(1.0f);

		slots.push_back(slot);
	}
}

void ItemBar::updateLayout()
{
	background.setSize({ itemBarWidth, windowSize.y });
	background.setPosition({ gameAreaWidth, 0.0f });

	const float margin = 10.0f;
	const float slotSpacing = 50.0f;
	float centerX = gameAreaWidth + itemBarWidth / 2.0f;

	for (size_t i = 0; i < slots.size(); i++)
	{
		float y = margin + i * slotSpacing;
		slots[i].icon->setPosition({ centerX, y + 25.0f });
		slots[i].layoutPosition = { centerX, y };
	}
}

void ItemBar::onResize()
{
	updateLayout();
}

void ItemBar::render(sf::RenderWindow* window)
{
	window->draw(background);

	for (size_t i = 0; i < slots.size(); i++)
	{
		int count = itemManager->getInventoryCount(static_cast<int>(i));

		slots[i].icon->setColor(count > 0 ? sf::Color::White : sf::Color(255, 255, 255, 70));   // uncollected items are drawn with a lower opacity

		slots[i].countText->setString(std::to_string(count));
		sf::FloatRect textBounds = slots[i].countText->getLocalBounds();
		slots[i].countText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f });
		slots[i].countText->setPosition(slots[i].layoutPosition);

		window->draw(*slots[i].icon);
		window->draw(*slots[i].countText);
	}
}

sf::Vector2f ItemBar::getSlotPosition(int index) const
{
	if (index < 0 || index >= static_cast<int>(slots.size())) return { 0.0f, 0.0f };
	return slots[index].icon->getPosition(); // the icon's own drawn position, not layoutPosition (which the count text uses, offset 25px above the icon)
}