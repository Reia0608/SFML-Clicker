#include "FamiliarTab.h"
#include "PublicVariables.h"

void FamiliarTab::relayoutButtons() // the core of the locking system
{
	const float buttonHeight = 50.0f;
	const float margin = 10.0f;
	const float spacing = 10.0f;

	float y = lastContentAreaOrigin.y + margin;
	for (auto& entry : buttonEntries)
	{
		bool visible = entry.isVisible();
		entry.button->setEnabled(visible); // UIButton already skips render()/handleClick() while disabled

		if (!visible) continue; // locked/hidden buttons reserve no space — this is the fix

		entry.button->setLayout(
			{ lastContentAreaOrigin.x + margin, y },
			{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
		);
		y += buttonHeight + spacing;
	}

	contentHeight = (y - lastContentAreaOrigin.y) - spacing + margin;
}

void FamiliarTab::updateBuySlotButtonLabel()
{
	buySlotButton->setLabelText("Unlock a new active Familiar slot (Cost: " + std::to_string(slotCost) + " Soul(s))");
}

void FamiliarTab::onBuySlotButtonClicked()
{
	if (itemManager->trySpendItem(amberEmblemIndex, slotCost))
	{
		// TODO: unlock a new active familiar slot
	}
}

void FamiliarTab::updateEvolveRedBabyDragonButtonLabel()
{
	evolveRedBabyDragonButton->setLabelText("Evolve Red Baby Dragon (Cost: " + std::to_string(evolveRedBabyDragonCost) + " Amber Emblem(s))");
}

void FamiliarTab::onEvolveRedBabyDragonButtonClicked()
{
	if (itemManager->trySpendItem(rubyEmblemIndex, evolveRedBabyDragonCost))
	{
		if (onEvolveRedBabyDragon)
		{
			onEvolveRedBabyDragon();
		}
	}
}

FamiliarTab::FamiliarTab(Hero* hero, ItemManager* itemManager, UnlockManager* unlockManager)
	: hero(hero), itemManager(itemManager), unlockManager(unlockManager)
{
	buySlotButton = new UIButton("", [this]() { onBuySlotButtonClicked(); });
	evolveRedBabyDragonButton = new UIButton("", [this]() { onEvolveRedBabyDragonButtonClicked(); });

	updateBuySlotButtonLabel();
	updateEvolveRedBabyDragonButtonLabel();

	buttonEntries = 
	{
		{ buySlotButton,                        [this]() { return true; } },
		{ evolveRedBabyDragonButton,                [this]() { return true; } },
		// Example: a future feature-gated upgrade, hidden until "SomeAchievement" is claimed:
		// { buyNewUpgradeButton, [this]() { return unlockManager->isUnlocked("SomeFeatureKey"); } },
	};
}

FamiliarTab::~FamiliarTab()
{
	delete buySlotButton;
	delete evolveRedBabyDragonButton;
}

void FamiliarTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;
	relayoutButtons();
}

void FamiliarTab::render(sf::RenderWindow* window)
{
	for (auto& entry : buttonEntries)
	{
		entry.button->render(window); // internally a no-op while disabled
	}
}

bool FamiliarTab::handleClick(sf::Vector2f pos)
{
	for (auto& entry : buttonEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
	return false;
}