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

		if (!visible) continue; // locked/hidden buttons reserve no space - this is the fix

		entry.button->setLayout(
			{ lastContentAreaOrigin.x + margin, y },
			{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
		);
		y += buttonHeight + spacing;
	}

	// Slot rows (or picker candidates, while picking) stack directly below buttonEntries. Every
	// button in slotButtons is always meant to be shown right now - rebuildSlotButtons() only
	// ever creates exactly the rows that belong in the current mode - so no visibility check is
	// needed here, unlike the buttonEntries loop above.
	for (auto* button : slotButtons)
	{
		button->setLayout(
			{ lastContentAreaOrigin.x + margin, y },
			{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
		);
		y += buttonHeight + spacing;
	}

	contentHeight = (y - lastContentAreaOrigin.y) - spacing + margin;
}

void FamiliarTab::clearSlotButtons()
{
	for (auto* button : slotButtons) delete button;
	slotButtons.clear();
}

void FamiliarTab::rebuildSlotButtons()
{
	clearSlotButtons();

	if (pickingForSlotIndex >= 0)
	{
		// Picker mode: one button per currently unlocked-but-unassigned familiar, plus a Back button.
		std::vector<std::string> candidates = getAvailableFamiliarNames ? getAvailableFamiliarNames() : std::vector<std::string>();

		if (candidates.empty())
		{
			// No separate "< Back" button needed here - this one already does the same thing,
			// and having two back-to-the-same-place buttons on an otherwise empty screen is odd.
			slotButtons.push_back(new UIButton("No unlocked familiars available - tap to go back", [this]() { onPickerBackClicked(); }));
		}
		else
		{
			for (int i = 0; i < static_cast<int>(candidates.size()); i++)
			{
				int pickedIndex = i; // captured by value below - `i` itself is about to go out of scope
				slotButtons.push_back(new UIButton(candidates[i], [this, pickedIndex]() { onPickerCandidateClicked(pickedIndex); }));
			}
			slotButtons.push_back(new UIButton("< Back", [this]() { onPickerBackClicked(); }));
		}
	}
	else
	{
		// Normal mode: one row per unlocked slot - filled shows the familiar's name (tap to
		// remove), empty shows a "+" prompt (tap to open the picker for that slot).
		int slotCount = getSlotCount ? getSlotCount() : 0;
		for (int i = 0; i < slotCount; i++)
		{
			int slotIndex = i; // captured by value below - `i` itself is about to go out of scope
			std::string label = getSlotFamiliarLabel ? getSlotFamiliarLabel(slotIndex) : "";
			std::string buttonText = label.empty() ? "+ (Empty slot - tap to assign)" : (label + " (tap to remove)");
			slotButtons.push_back(new UIButton(buttonText, [this, slotIndex]() { onSlotRowClicked(slotIndex); }));
		}
	}

	relayoutButtons();
}

void FamiliarTab::onSlotRowClicked(int slotIndex)
{
	std::string label = getSlotFamiliarLabel ? getSlotFamiliarLabel(slotIndex) : "";
	if (!label.empty())
	{
		// filled - tapping it removes the familiar, sending it back to the unassigned pool
		if (onUnassignSlot) onUnassignSlot(slotIndex);
		rebuildSlotButtons();
	}
	else
	{
		// empty - open the picker for this slot
		pickingForSlotIndex = slotIndex;
		rebuildSlotButtons();
	}
}

void FamiliarTab::onPickerCandidateClicked(int pickedIndex)
{
	if (pickingForSlotIndex >= 0 && onAssignFamiliarToSlot)
	{
		onAssignFamiliarToSlot(pickingForSlotIndex, pickedIndex);
	}
	pickingForSlotIndex = -1;
	rebuildSlotButtons();
}

void FamiliarTab::onPickerBackClicked()
{
	pickingForSlotIndex = -1;
	rebuildSlotButtons();
}

void FamiliarTab::refreshSlotButtons()
{
	rebuildSlotButtons();
}

void FamiliarTab::updateBuySlotButtonLabel()
{
	buySlotButton->setLabelText("Unlock a new active Familiar slot (Cost: " + std::to_string(slotCost) + " Ambers(s))");
}

void FamiliarTab::onBuySlotButtonClicked()
{
	if (itemManager->trySpendItem(amberEmblemIndex, slotCost))
	{
		if (onSlotPurchased) onSlotPurchased();
		rebuildSlotButtons(); // a new (empty) slot row should now appear
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
		rebuildSlotButtons(); // the familiar's display label may have changed (e.g. "Baby Fire Dragon" -> "Fire Dragon") - refresh any slot row currently showing it
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

	// slotButtons intentionally stays empty here - the callbacks above (onSlotPurchased etc.)
	// aren't wired by Game yet at construction time, so there's nothing meaningful to build until
	// Game calls refreshSlotButtons() itself, right after wiring them in initUI().
}

FamiliarTab::~FamiliarTab()
{
	delete buySlotButton;
	delete evolveRedBabyDragonButton;
	clearSlotButtons();
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
	for (auto* button : slotButtons)
	{
		button->render(window);
	}
}

bool FamiliarTab::handleClick(sf::Vector2f pos)
{
	for (auto& entry : buttonEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
	for (auto* button : slotButtons)
	{
		if (button->handleClick(pos)) return true;
	}
	return false;
}