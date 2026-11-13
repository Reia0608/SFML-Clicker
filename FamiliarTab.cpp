#include "FamiliarTab.h"
#include "PublicVariables.h"
#include "Familiar.h"
#include "Statistics.h"
#include "LayoutManager.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace
{
	// Reflows `str` into up to two lines (joined with '\n') so that neither is wider than
	// maxWidth, measured with `text`'s actual current font/size - same measure-by-setting-the-
	// real-string technique AchievementsTab's own wrapTextToWidth() uses, kept as its own small
	// copy here rather than shared, since neither UIButton::wrapText (private to that class) nor
	// AchievementsTab's version (file-local) are reachable from here. Used only for the picker's
	// candidate captions (e.g. "Baby Water Dragon"), which are a bit too wide for one line at
	// pickerCellWidth.
	std::string wrapCaptionToWidth(Text* text, const std::string& str, float maxWidth)
	{
		if (maxWidth <= 0.0f) return str;

		std::istringstream words(str);
		std::string word;
		std::string result;
		std::string currentLine;

		while (words >> word)
		{
			std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
			text->setString(candidate);
			if (text->getLocalBounds().size.x > maxWidth && !currentLine.empty())
			{
				result += currentLine + "\n";
				currentLine = word;
			}
			else
			{
				currentLine = candidate;
			}
		}
		result += currentLine;
		return result;
	}
}

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

	// Slot grid - slotsPerRow square tiles per row, stacked directly below buttonEntries. Each
	// tile's side length is derived from the tab's own current width so slotsPerRow of them
	// exactly fill it (minus margins/spacing) - a slot is meant to read as "about half the panel",
	// not a small fixed box with empty space next to it - so it grows/shrinks with the window
	// instead of being a fixed constant. Walked one row at a time (rather than one tile at a time)
	// purely so the row-height contribution to `y` is only written once, the same shape the old
	// single-column loop used.
	float slotTileSize = std::max(1.0f, (lastContentAreaSize.x - margin * 2.0f - spacing * (slotsPerRow - 1)) / slotsPerRow);
	for (size_t rowStart = 0; rowStart < slotTiles.size(); rowStart += slotsPerRow)
	{
		for (size_t i = rowStart; i < slotTiles.size() && i < rowStart + slotsPerRow; i++)
		{
			size_t col = i - rowStart;
			sf::Vector2f pos = { lastContentAreaOrigin.x + margin + col * (slotTileSize + spacing), y };
			slotTiles[i].layoutPosition = pos;
			slotTiles[i].layoutSize = { slotTileSize, slotTileSize };
			slotTiles[i].button->setLayout(pos, { slotTileSize, slotTileSize });
		}
		y += slotTileSize + spacing;
	}

	contentHeight = (y - lastContentAreaOrigin.y) - spacing + margin;

	relayoutPicker(); // keep the popup (if open) centered on the current window size too - a no-op while closed
}

void FamiliarTab::clearSlotTiles()
{
	for (auto& tile : slotTiles) delete tile.button;
	slotTiles.clear();
}

void FamiliarTab::rebuildSlotTiles()
{
	clearSlotTiles();

	int slotCount = getSlotCount ? getSlotCount() : 0;
	for (int i = 0; i < slotCount; i++)
	{
		int slotIndex = i; // captured by value below - `i` itself is about to go out of scope
		FamiliarIcon icon = getSlotFamiliarIcon ? getSlotFamiliarIcon(slotIndex) : FamiliarIcon{};

		SlotTile tile;
		tile.isEmpty = icon.isEmpty();
		tile.button = tile.isEmpty
			? new UIButton("", [this, slotIndex]() { onSlotRowClicked(slotIndex); }) // blank background - the "+" is drawn on top in render(), see drawPlusMark
			: UIButton::createIconButton(icon.texturePath, icon.sourceRect, [this, slotIndex]() { onSlotRowClicked(slotIndex); });
		slotTiles.push_back(tile);
	}

	relayoutButtons();
}

void FamiliarTab::onSlotRowClicked(int slotIndex)
{
	FamiliarIcon icon = getSlotFamiliarIcon ? getSlotFamiliarIcon(slotIndex) : FamiliarIcon{};
	if (!icon.isEmpty())
	{
		// filled - tapping it removes the familiar, sending it back to the unassigned pool
		if (onUnassignSlot) onUnassignSlot(slotIndex);
		rebuildSlotTiles();
	}
	else
	{
		openPicker(slotIndex);
	}
}

void FamiliarTab::drawPlusMark(sf::RenderWindow* window, sf::Vector2f tileTopLeft, sf::Vector2f tileSize)
{
	const float thickness = 6.0f;
	const float armLength = tileSize.x * 0.5f; // each arm reaches half the tile's width/height out from the center
	sf::Vector2f center = tileTopLeft + tileSize / 2.0f;

	sf::RectangleShape horizontalBar({ armLength, thickness });
	horizontalBar.setOrigin({ armLength / 2.0f, thickness / 2.0f });
	horizontalBar.setPosition(center);
	horizontalBar.setFillColor(sf::Color(220, 220, 220));
	window->draw(horizontalBar);

	sf::RectangleShape verticalBar({ thickness, armLength });
	verticalBar.setOrigin({ thickness / 2.0f, armLength / 2.0f });
	verticalBar.setPosition(center);
	verticalBar.setFillColor(sf::Color(220, 220, 220));
	window->draw(verticalBar);
}

void FamiliarTab::openPicker(int slotIndex)
{
	pickingForSlotIndex = slotIndex;
	rebuildPickerCandidates(); // also lays the popup out immediately, via relayoutPicker() at the end of rebuildPickerCandidates()
}

void FamiliarTab::closePicker()
{
	pickingForSlotIndex = -1;
	clearPickerCandidates();
}

void FamiliarTab::clearPickerCandidates()
{
	for (auto& candidate : pickerCandidates)
	{
		delete candidate.button;
		delete candidate.caption;
	}
	pickerCandidates.clear();

	delete pickerCloseButton; pickerCloseButton = nullptr;
	delete pickerTitle; pickerTitle = nullptr;
	delete pickerEmptyMessage; pickerEmptyMessage = nullptr;
}

void FamiliarTab::rebuildPickerCandidates()
{
	clearPickerCandidates();

	pickerTitle = new Text("Choose a familiar", "Fonts/arial.ttf");
	pickerTitle->setCharacterSize(22);
	pickerTitle->setFillColor(sf::Color::White);

	pickerCloseButton = new UIButton("Cancel", [this]() { closePicker(); });

	std::vector<FamiliarIcon> candidates = getAvailableFamiliarIcons ? getAvailableFamiliarIcons() : std::vector<FamiliarIcon>();

	if (candidates.empty())
	{
		pickerEmptyMessage = new Text("No unlocked familiars available", "Fonts/arial.ttf");
		pickerEmptyMessage->setCharacterSize(18);
		pickerEmptyMessage->setFillColor(sf::Color(200, 200, 200));
	}
	else
	{
		for (int i = 0; i < static_cast<int>(candidates.size()); i++)
		{
			int pickedIndex = i; // captured by value below - `i` itself is about to go out of scope
			const FamiliarIcon& icon = candidates[i];

			PickerCandidateTile tile;
			tile.button = UIButton::createIconButton(icon.texturePath, icon.sourceRect, [this, pickedIndex]() { onPickerCandidateClicked(pickedIndex); });
			tile.caption = new Text(icon.displayLabel, "Fonts/arial.ttf");
			tile.caption->setCharacterSize(16);
			tile.caption->setFillColor(sf::Color::White);
			pickerCandidates.push_back(tile);
		}
	}

	relayoutPicker();
}

void FamiliarTab::onPickerCandidateClicked(int pickedIndex)
{
	if (pickingForSlotIndex >= 0 && onAssignFamiliarToSlot)
	{
		onAssignFamiliarToSlot(pickingForSlotIndex, pickedIndex);
	}
	closePicker();
	rebuildSlotTiles();
}

bool FamiliarTab::handlePickerClick(sf::Vector2f pos)
{
	if (pickerCloseButton && pickerCloseButton->handleClick(pos)) return true;

	for (auto& candidate : pickerCandidates)
	{
		if (candidate.button->handleClick(pos)) return true;
	}

	// Clicked the dimmed backdrop, or anywhere else in the panel that isn't a button - treat it
	// as "tap outside to cancel", the same as the Cancel button, rather than doing nothing.
	closePicker();
	return true; // absorb the click either way - nothing underneath should react while the popup is open
}

void FamiliarTab::relayoutPicker()
{
	if (pickingForSlotIndex < 0) return; // popup closed - nothing to lay out

	const float margin = 28.0f;
	const float titleAreaHeight = 46.0f; // space reserved at the top of the panel for pickerTitle
	const float closeButtonWidth = 140.0f;
	const float closeButtonHeight = 40.0f;
	const float closeButtonGap = 24.0f; // gap between the candidate grid (or empty message) and the Cancel button
	const float emptyMessageAreaHeight = 30.0f;

	int candidateCount = static_cast<int>(pickerCandidates.size());

	// Column count is capped by pickerMaxColumnsPerRow, but also shrinks to whatever actually
	// fits in the WHOLE window's current width, and never exceeds how many candidates there are -
	// so a narrow window (or very few candidates) never reserves empty, wasted columns.
	float availableWidth = windowSize.x - margin * 2.0f;
	int maxColumnsThatFit = std::max(1, static_cast<int>((availableWidth + pickerCellSpacing) / (pickerCellWidth + pickerCellSpacing)));
	int columns = std::max(1, std::min({ pickerMaxColumnsPerRow, maxColumnsThatFit, std::max(1, candidateCount) }));
	int rows = candidateCount == 0 ? 1 : (candidateCount + columns - 1) / columns;

	float gridWidth = columns * pickerCellWidth + (columns - 1) * pickerCellSpacing;
	float gridHeight = rows * (pickerTileSize + pickerCaptionHeight) + (rows - 1) * pickerRowSpacing;
	float bodyHeight = candidateCount == 0 ? emptyMessageAreaHeight : gridHeight;

	float panelWidth = std::min(windowSize.x - margin * 2.0f, margin * 2.0f + gridWidth);
	float panelHeight = margin + titleAreaHeight + bodyHeight + closeButtonGap + closeButtonHeight + margin;

	// Centered on the WHOLE window - this is a real modal popup, not scoped to this tab's own
	// panel, so it floats over the play area and item bar too (see pickingForSlotIndex's comment
	// in the header for why).
	sf::Vector2f panelOrigin = {
		(windowSize.x - panelWidth) / 2.0f,
		std::max(0.0f, (windowSize.y - panelHeight) / 2.0f)
	};

	pickerPanelBackground.setPosition(panelOrigin);
	pickerPanelBackground.setSize({ panelWidth, panelHeight });

	LayoutManager::applyAnchor(*pickerTitle, Anchor::TopCenter, { 0.0f, margin }, panelOrigin, { panelWidth, panelHeight });

	float gridOriginX = panelOrigin.x + (panelWidth - gridWidth) / 2.0f;
	float gridOriginY = panelOrigin.y + margin + titleAreaHeight;

	for (int i = 0; i < candidateCount; i++)
	{
		int col = i % columns;
		int row = i / columns;
		sf::Vector2f tilePos = {
			gridOriginX + col * (pickerCellWidth + pickerCellSpacing) + (pickerCellWidth - pickerTileSize) / 2.0f,
			gridOriginY + row * (pickerTileSize + pickerCaptionHeight + pickerRowSpacing)
		};
		pickerCandidates[i].button->setLayout(tilePos, { pickerTileSize, pickerTileSize });

		Text* caption = pickerCandidates[i].caption;
		caption->setString(wrapCaptionToWidth(caption, caption->getString().toAnsiString(), pickerCellWidth));
		sf::Vector2f captionAreaOrigin = { gridOriginX + col * (pickerCellWidth + pickerCellSpacing), tilePos.y + pickerTileSize };
		LayoutManager::applyAnchor(*caption, Anchor::TopCenter, { 0.0f, 2.0f }, captionAreaOrigin, { pickerCellWidth, pickerCaptionHeight });
	}

	if (pickerEmptyMessage)
	{
		LayoutManager::applyAnchor(*pickerEmptyMessage, Anchor::MiddleCenter, { 0.0f, 0.0f }, { gridOriginX, gridOriginY }, { gridWidth, bodyHeight });
	}

	float closeButtonX = panelOrigin.x + (panelWidth - closeButtonWidth) / 2.0f;
	float closeButtonY = panelOrigin.y + panelHeight - margin - closeButtonHeight;
	pickerCloseButton->setLayout({ closeButtonX, closeButtonY }, { closeButtonWidth, closeButtonHeight });
}

void FamiliarTab::renderPicker(sf::RenderWindow* window)
{
	if (pickingForSlotIndex < 0) return; // closed - nothing to draw (lets Game call this unconditionally every frame)

	// Dims the WHOLE window - the play area and item bar too, not just this tab's own panel. Safe
	// to draw at full window coordinates here specifically BECAUSE this is called from Game, after
	// tabbedPanel->render() has already restored the normal (unscissored) view - see this method's
	// own comment in the header for why it can't be called from inside this tab's own render().
	sf::RectangleShape backdrop(windowSize);
	backdrop.setPosition({ 0.0f, 0.0f });
	backdrop.setFillColor(sf::Color(0, 0, 0, 160));
	window->draw(backdrop);

	pickerPanelBackground.setFillColor(sf::Color(55, 55, 55));
	pickerPanelBackground.setOutlineColor(sf::Color::Black);
	pickerPanelBackground.setOutlineThickness(1.0f);
	window->draw(pickerPanelBackground);

	window->draw(*pickerTitle);

	for (auto& candidate : pickerCandidates)
	{
		candidate.button->render(window);
		window->draw(*candidate.caption);
	}
	if (pickerEmptyMessage) window->draw(*pickerEmptyMessage);

	pickerCloseButton->render(window);
}

void FamiliarTab::refreshSlotButtons()
{
	rebuildSlotTiles();
}

void FamiliarTab::refreshButtonVisibility()
{
	relayoutButtons();
}

void FamiliarTab::updateBuySlotButtonLabel()
{
	buySlotButton->setLabelText("Unlock a new active Familiar slot (Cost: " + std::to_string(slotCost) + " Ambers(s))");
}

void FamiliarTab::onBuySlotButtonClicked()
{
	if (itemManager->trySpendItem(amberEmblemIndex, slotCost))
	{
		slotCost++; // next slot costs 1 more Amber than this one did (Slot 1 = 1, Slot 2 = 2, ...)
		updateBuySlotButtonLabel();
		if (onSlotPurchased) onSlotPurchased();
		rebuildSlotTiles(); // a new (empty) slot tile should now appear
	}
}

void FamiliarTab::addAttackUnlockButton(const std::string& displayName, const std::string& purchasableFeatureKey,
	const std::string& grantedFeatureKey, int currencyItemIndex, int currencyCost, bool passiveDealsDamage)
{
	std::string currencyName = itemManager->getItemDefinition(currencyItemIndex).name;
	std::string label = "Unlock " + displayName + " (Cost: " + std::to_string(currencyCost) + " " + currencyName + "(s))";

	UIButton* button = new UIButton(label, [this, currencyItemIndex, currencyCost, grantedFeatureKey, passiveDealsDamage]()
		{
			if (itemManager->trySpendItem(currencyItemIndex, currencyCost))
			{
				unlockManager->unlock(grantedFeatureKey);
				if (passiveDealsDamage && onDamagingPassiveUnlocked) onDamagingPassiveUnlocked(); // this familiar's own damage-dealing passive is now active - let Game flash StatisticsTab
				relayoutButtons(); // this button just hid itself (see isVisible below) - repack everything below it immediately, same as a slot/evolve purchase would
			}
		});

	buttonEntries.push_back({ button, [this, purchasableFeatureKey, grantedFeatureKey]()
		{
			// visible once its achievement unlocked the purchase, and only until it's actually bought
			return unlockManager->isUnlocked(purchasableFeatureKey) && !unlockManager->isUnlocked(grantedFeatureKey);
		} });
}

void FamiliarTab::addFamiliarUnlockButton(Familiar* targetFamiliar, const std::string& familiarKey, const std::string& displayName)
{
	UIButton* button = new UIButton("", [this, targetFamiliar, familiarKey]()
		{
			if (hero->trySpendCurrency(unlockManager->getNextFamiliarUnlockCost()))
			{
				targetFamiliar->unlock();
				unlockManager->recordFamiliarUnlocked(familiarKey);
				unlockManager->unlock(familiarKey + "Unlocked"); // lets this lineage's passive-ability achievement (see AchievementManager.cpp) become visible
				statistics->recordShopUpgradePurchased();
				if (onFamiliarUnlocked) onFamiliarUnlocked(); // let Game flash StatisticsTab's tab button - new rows just became visible there
				refreshUnlockButtonLabels(); // every OTHER still-locked unlock button's price just went up 10x too
				relayoutButtons(); // this button just hid itself (see isVisible below) - repack everything below it immediately, same as a slot/evolve purchase would
			}
		});

	familiarUnlockButtons.push_back(button);
	familiarUnlockRelabelers.push_back([this, button, displayName]()
		{
			button->setLabelText("Unlock " + displayName + " (Cost: " + unlockManager->getNextFamiliarUnlockCost().toString() + " Gold(s))");
		});
	familiarUnlockRelabelers.back()(); // set the initial label text right away

	buttonEntries.push_back({ button, [targetFamiliar]()
		{
			return !targetFamiliar->isUnlocked(); // one-time purchase - hidden for good once bought
		} });
}

std::string FamiliarTab::formatCooldownAmount(float amount)
{
	std::ostringstream oss;
	oss << std::setprecision(3) << amount;
	return oss.str();
}

void FamiliarTab::refreshUpgradeButtonLabels()
{
	for (auto& relabel : familiarUpgradeRelabelers) relabel();
}

void FamiliarTab::addFamiliarUpgradeButtons(Familiar* targetFamiliar, const std::string& lineageLabel,
	BigNumber& physicalCost, BigNumber& elementalCost, BigNumber& cooldownCost)
{
	// physicalCost/elementalCost/cooldownCost are references - since this one function body is
	// shared by all three lineages, it can't know at compile time whether it's e.g. updating
	// firePhysicalCost or waterPhysicalCost. Capturing "&physicalCost" below doesn't capture this
	// local parameter itself - C++ follows a reference straight through to whatever it's bound
	// to - so it ends up capturing a direct reference to that specific lineage's own cost member
	// (which outlives these buttons, so this stays safe for as long as FamiliarTab itself exists).

	// --- Physical Attack ---
	size_t physicalIndex = familiarUpgradeButtons.size(); // this button's own fixed slot - captured by value below, so it stays correct even after later lineages add more buttons
	UIButton* physicalButton = new UIButton("", [this, targetFamiliar, &physicalCost, physicalIndex]()
		{
			if (hero->trySpendCurrency(physicalCost))
			{
				targetFamiliar->increasePhysicalAttack(BigNumber(physicalUpgradeAmount));
				physicalCost = physicalCost * BigNumber(physicalCostGrowth);
				statistics->recordShopUpgradePurchased();
				familiarUpgradeRelabelers[physicalIndex]();
			}
		});
	familiarUpgradeButtons.push_back(physicalButton);
	familiarUpgradeRelabelers.push_back([physicalButton, &physicalCost, lineageLabel]()
		{
			physicalButton->setLabelText(lineageLabel + " Physical Attack +" + std::to_string(physicalUpgradeAmount) + " (Cost: " + physicalCost.toString() + ")");
		});
	familiarUpgradeRelabelers.back()(); // set the initial label text right away
	buttonEntries.push_back({ physicalButton, [targetFamiliar]() { return targetFamiliar->isUnlocked(); } });

	// --- Elemental Attack ---
	size_t elementalIndex = familiarUpgradeButtons.size();
	UIButton* elementalButton = new UIButton("", [this, targetFamiliar, &elementalCost, elementalIndex]()
		{
			if (hero->trySpendCurrency(elementalCost))
			{
				targetFamiliar->increaseElementalAttack(BigNumber(elementalUpgradeAmount));
				elementalCost = elementalCost * BigNumber(elementalCostGrowth);
				statistics->recordShopUpgradePurchased();
				familiarUpgradeRelabelers[elementalIndex]();
			}
		});
	familiarUpgradeButtons.push_back(elementalButton);
	familiarUpgradeRelabelers.push_back([elementalButton, &elementalCost, lineageLabel]()
		{
			elementalButton->setLabelText(lineageLabel + " Elemental Attack +" + std::to_string(elementalUpgradeAmount) + " (Cost: " + elementalCost.toString() + ")");
		});
	familiarUpgradeRelabelers.back()();
	buttonEntries.push_back({ elementalButton, [targetFamiliar]() { return targetFamiliar->isUnlocked(); } });

	// --- Cooldown Reduction ---
	size_t cooldownIndex = familiarUpgradeButtons.size();
	UIButton* cooldownButton = new UIButton("", [this, targetFamiliar, &cooldownCost, cooldownIndex]()
		{
			if (hero->trySpendCurrency(cooldownCost))
			{
				targetFamiliar->increaseCooldownReduction(cooldownUpgradeAmount);
				cooldownCost = cooldownCost * BigNumber(cooldownCostGrowth);
				statistics->recordShopUpgradePurchased();
				familiarUpgradeRelabelers[cooldownIndex]();
				relayoutButtons(); // this button may have just hidden itself (cooldown hit its floor) - repack immediately, same as ShopTab's original cooldown button did
			}
		});
	familiarUpgradeButtons.push_back(cooldownButton);
	familiarUpgradeRelabelers.push_back([this, cooldownButton, &cooldownCost, lineageLabel]()
		{
			cooldownButton->setLabelText(lineageLabel + " cooldown reduction by " + formatCooldownAmount(cooldownUpgradeAmount) + " seconds (Cost: " + cooldownCost.toString() + ")");
		});
	familiarUpgradeRelabelers.back()();
	buttonEntries.push_back({ cooldownButton, [targetFamiliar]()
		{
			return targetFamiliar->isUnlocked() && !targetFamiliar->isAutoAttackCooldownAtMinimum();
		} });
}

int FamiliarTab::findCurrentStageIndex(Familiar* targetFamiliar, const std::vector<EvolutionStage>& stages)
{
	for (size_t i = 0; i < stages.size(); i++)
	{
		if (stages[i].age == targetFamiliar->getAge()) return static_cast<int>(i);
	}
	return -1; // targetFamiliar's current age isn't in its own stage list - shouldn't normally happen
}

void FamiliarTab::refreshEvolveButtonLabels()
{
	for (auto& relabel : familiarEvolveRelabelers) relabel();
}

void FamiliarTab::refreshUnlockButtonLabels()
{
	for (auto& relabel : familiarUnlockRelabelers) relabel();
}

void FamiliarTab::addFamiliarEvolveButton(Familiar* targetFamiliar, std::vector<EvolutionStage> stages, int emblemIndex)
{
	std::string emblemName = itemManager->getItemDefinition(emblemIndex).name;

	size_t evolveIndex = familiarEvolveButtons.size(); // this button's own fixed slot - captured by value below, same reasoning as the upgrade buttons above
	UIButton* button = new UIButton("", [this, targetFamiliar, stages, emblemIndex, evolveIndex]()
		{
			int currentPos = findCurrentStageIndex(targetFamiliar, stages);
			if (currentPos < 0 || currentPos + 1 >= static_cast<int>(stages.size())) return; // nothing left in `stages` to evolve into

			Age nextAge = stages[currentPos + 1].age;
			if (isFamiliarStageAvailable && !isFamiliarStageAvailable(targetFamiliar->getName(), nextAge)) return; // Game doesn't have that stage's data yet - safety net, button should already be hidden

			int cost = currentPos + 1; // 1st transition costs 1 Emblem, 2nd costs 2, 3rd costs 3

			if (itemManager->trySpendItem(emblemIndex, cost))
			{
				if (onEvolveFamiliar) onEvolveFamiliar(targetFamiliar, nextAge);
				statistics->recordShopUpgradePurchased();
				familiarEvolveRelabelers[evolveIndex](); // show the NEXT transition now, or hide if that was the last one
				rebuildSlotTiles(); // the familiar's icon just changed (e.g. the baby sprite to the evolved one) - refresh any slot tile currently showing it
				relayoutButtons();    // this button's own visibility may have just changed too
			}
		});
	familiarEvolveButtons.push_back(button);

	familiarEvolveRelabelers.push_back([this, button, targetFamiliar, stages, emblemName]()
		{
			int currentPos = findCurrentStageIndex(targetFamiliar, stages);
			if (currentPos < 0 || currentPos + 1 >= static_cast<int>(stages.size())) return; // button is hidden in this case anyway (see visibility rule below) - label content doesn't matter

			int cost = currentPos + 1;
			button->setLabelText("Evolve " + stages[currentPos].displayLabel + " to " + stages[currentPos + 1].displayLabel +
				" (Cost: " + std::to_string(cost) + " " + emblemName + "(s))");
		});
	familiarEvolveRelabelers.back()(); // set the initial label text right away

	buttonEntries.push_back({ button, [this, targetFamiliar, stages]()
		{
			if (!targetFamiliar->isUnlocked()) return false; // not bought yet - a locked familiar's default Age happens to equal stages[0], so without this check it would look exactly like "currently at Baby, ready to evolve"

			int currentPos = findCurrentStageIndex(targetFamiliar, stages);
			if (currentPos < 0 || currentPos + 1 >= static_cast<int>(stages.size())) return false; // already at (or past) the last stage `stages` defines

			Age nextAge = stages[currentPos + 1].age;
			return !isFamiliarStageAvailable || isFamiliarStageAvailable(targetFamiliar->getName(), nextAge); // hide rather than offer a transition Game can't actually perform yet
		} });
}

FamiliarTab::FamiliarTab(Hero* hero, ItemManager* itemManager, UnlockManager* unlockManager, Statistics* statistics,
	Familiar* fireFamiliar, Familiar* waterFamiliar, Familiar* windFamiliar)
	: hero(hero), itemManager(itemManager), unlockManager(unlockManager), statistics(statistics),
	fireFamiliar(fireFamiliar), waterFamiliar(waterFamiliar), windFamiliar(windFamiliar)
{
	buySlotButton = new UIButton("", [this]() { onBuySlotButtonClicked(); });

	updateBuySlotButtonLabel();

	buttonEntries =
	{
		{ buySlotButton, [this]() { return (getSlotCount ? getSlotCount() : 0) < maxFamiliarSlots; } }, // hides for good once the 5th slot is bought
	};

	// Fireball (Fire Dragon's manual ability) - hidden until the "Juvenile Flame" achievement is
	// claimed, then purchasable for 1 Ruby Emblem; see addAttackUnlockButton()'s comment in the
	// header for the full two-key pattern.
	addAttackUnlockButton("Fireball (Fire Dragon)", "FireDragonFireballPurchasable", "FireDragonFireballUnlocked", rubyEmblemIndex, 1);

	// Same two-key pattern for Water's and Wind's own gated manual abilities - the achievement
	// claim (see AchievementManager.cpp's "Juvenile Tide"/"Juvenile Gale") reveals these buttons,
	// buying them sets the unlockFeatureKey their FamiliarAttack entry in familiarTypeList
	// actually checks (see Game.h). Add one more call like this per future gated attack.
	addAttackUnlockButton("Bubble Stream (Water Dragon)", "WaterDragonBubbleStreamPurchasable", "WaterDragonBubbleStreamUnlocked", sapphireEmblemIndex, 1);
	addAttackUnlockButton("Wind Swipes (Wind Dragon)", "WindDragonWindSwipesPurchasable", "WindDragonWindSwipesUnlocked", emeraldEmblemIndex, 1);

	// Passive abilities - same two-key pattern, unlocked by their own achievement (see
	// AchievementManager.cpp's "Flame Mastery"/"Tidal Mastery"/"Gale Mastery" and Game.cpp's
	// onAchievementClaimed) rather than by evolving. The granted key here (e.g.
	// "FireDragonBurnUnlocked") is what Steps 6-8 will actually check before applying each
	// passive's effect. Burn passes passiveDealsDamage=true (it's the one passive here that
	// actually deals its own damage - see FamiliarPassive::dealsDamage) so StatisticsTab's tab
	// button flashes the moment it's bought; Soaked/Keen Gale leave it at the default false.
	addAttackUnlockButton("Burn (Fire Dragon)", "FireDragonBurnPurchasable", "FireDragonBurnUnlocked", rubyEmblemIndex, 1, true);
	addAttackUnlockButton("Soaked (Water Dragon)", "WaterDragonSoakedPurchasable", "WaterDragonSoakedUnlocked", sapphireEmblemIndex, 1);
	addAttackUnlockButton("Keen Gale (Wind Dragon)", "WindDragonKeenGalePurchasable", "WindDragonKeenGaleUnlocked", emeraldEmblemIndex, 1);

	// Unlock buttons for every lineage except Fire. Fire's own unlock button stays in ShopTab -
	// it's the one that unlocks this whole tab to begin with, and doubles as a small tutorial for
	// a brand new player, so it's the single exception to "familiar buttons live in FamiliarTab".
	// Cost is no longer fixed per lineage - it's UnlockManager's shared unlock-order ladder (see
	// UnlockManager.h): whichever familiar is unlocked FIRST (today, always Fire, since its button
	// is the only one reachable before this tab even reveals itself) pays the base cost; every one
	// after that - Water or Wind, whichever gets bought first - pays 10x, and the last one 10x
	// again. Add one more call here (and one more Familiar* parameter/member, see FamiliarTab.h)
	// whenever a new unlockable lineage is added.
	addFamiliarUnlockButton(waterFamiliar, "WaterDragon", "Baby Water Dragon");
	addFamiliarUnlockButton(windFamiliar, "WindDragon", "Baby Wind Dragon");

	// Physical/Elemental/Cooldown upgrade buttons for every lineage, Fire included this time -
	// these three used to be bespoke ShopTab buttons hardcoded to Fire; now that every lineage
	// needs its own triplet, they all live here instead, built by the same shared helper. Add one
	// more call here (and one more cost triplet, see FamiliarTab.h) whenever a new lineage is added.
	addFamiliarUpgradeButtons(fireFamiliar, "Fire", firePhysicalCost, fireElementalCost, fireCooldownCost);
	addFamiliarUpgradeButtons(waterFamiliar, "Water", waterPhysicalCost, waterElementalCost, waterCooldownCost);
	addFamiliarUpgradeButtons(windFamiliar, "Wind", windPhysicalCost, windElementalCost, windCooldownCost);

	// One self-updating evolve button per lineage (see addFamiliarEvolveButton()'s comment) - the
	// display labels here intentionally match familiarTypeList's own `label` field exactly (e.g.
	// "Water Dragon" for the Juvenile stage, not "Juvenile Water Dragon") so this tab's wording
	// matches what the rest of the game (e.g. slot tiles' picker captions) already calls that stage.
	// Adult/Ancient stages aren't in familiarTypeList yet, so isFamiliarStageAvailable will keep
	// each button hidden once it reaches Juvenile, until those stages are added on Game's side
	// too - nothing here needs to change when that happens. Add one more call here (with that
	// lineage's own 4-stage list and Emblem) whenever a new lineage is added.
	addFamiliarEvolveButton(fireFamiliar, {
		{ Age::Baby, "Baby Fire Dragon" },
		{ Age::Juvenile, "Fire Dragon" },
		{ Age::Adult, "Adult Fire Dragon" },
		{ Age::Ancient, "Ancient Fire Dragon" },
		}, rubyEmblemIndex);

	addFamiliarEvolveButton(waterFamiliar, {
		{ Age::Baby, "Baby Water Dragon" },
		{ Age::Juvenile, "Water Dragon" },
		{ Age::Adult, "Adult Water Dragon" },
		{ Age::Ancient, "Ancient Water Dragon" },
		}, sapphireEmblemIndex);

	addFamiliarEvolveButton(windFamiliar, {
		{ Age::Baby, "Baby Wind Dragon" },
		{ Age::Juvenile, "Wind Dragon" },
		{ Age::Adult, "Adult Wind Dragon" },
		{ Age::Ancient, "Ancient Wind Dragon" },
		}, emeraldEmblemIndex);

	// slotTiles intentionally stays empty here - the callbacks above (onSlotPurchased etc.)
	// aren't wired by Game yet at construction time, so there's nothing meaningful to build until
	// Game calls refreshSlotButtons() itself, right after wiring them in initUI().
}

FamiliarTab::~FamiliarTab()
{
	// Deletes buySlotButton too - it's also kept as a named member for its label-update method to
	// reach, but buttonEntries is the single place that owns every ShopTab-style button, including
	// every addAttackUnlockButton()/addFamiliarUnlockButton()/addFamiliarUpgradeButtons()/
	// addFamiliarEvolveButton() one, none of which have a named member at all.
	for (auto& entry : buttonEntries) delete entry.button;
	clearSlotTiles();
	clearPickerCandidates();
}

void FamiliarTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;
	relayoutButtons();
}

void FamiliarTab::render(sf::RenderWindow* window)
{
	hasUnseenUnlock = false; // being viewed right now - stop pulsing, same as AchievementsTab::render()

	for (auto& entry : buttonEntries)
	{
		entry.button->render(window); // internally a no-op while disabled
	}
	for (auto& tile : slotTiles)
	{
		tile.button->render(window);
		if (tile.isEmpty) drawPlusMark(window, tile.layoutPosition, tile.layoutSize);
	}

	// The picker popup itself is deliberately NOT drawn here - see renderPicker()'s own comment
	// for why Game has to call it separately, after tabbedPanel->render() has already finished.
}

bool FamiliarTab::handleClick(sf::Vector2f pos)
{
	if (pickingForSlotIndex >= 0) return handlePickerClick(pos); // popup open - it owns every click until closed, wherever on screen it lands

	for (auto& entry : buttonEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
	for (auto& tile : slotTiles)
	{
		if (tile.button->handleClick(pos)) return true;
	}
	return false;
}

int FamiliarTab::getSlotCost() const { return slotCost; }
void FamiliarTab::setSlotCost(int cost) { slotCost = cost; updateBuySlotButtonLabel(); }

BigNumber FamiliarTab::getFirePhysicalUpgradeCost() const { return firePhysicalCost; }
BigNumber FamiliarTab::getFireElementalUpgradeCost() const { return fireElementalCost; }
BigNumber FamiliarTab::getFireCooldownUpgradeCost() const { return fireCooldownCost; }
void FamiliarTab::setFirePhysicalUpgradeCost(BigNumber cost) { firePhysicalCost = cost; }
void FamiliarTab::setFireElementalUpgradeCost(BigNumber cost) { fireElementalCost = cost; }
void FamiliarTab::setFireCooldownUpgradeCost(BigNumber cost) { fireCooldownCost = cost; }

BigNumber FamiliarTab::getWaterPhysicalUpgradeCost() const { return waterPhysicalCost; }
BigNumber FamiliarTab::getWaterElementalUpgradeCost() const { return waterElementalCost; }
BigNumber FamiliarTab::getWaterCooldownUpgradeCost() const { return waterCooldownCost; }
void FamiliarTab::setWaterPhysicalUpgradeCost(BigNumber cost) { waterPhysicalCost = cost; }
void FamiliarTab::setWaterElementalUpgradeCost(BigNumber cost) { waterElementalCost = cost; }
void FamiliarTab::setWaterCooldownUpgradeCost(BigNumber cost) { waterCooldownCost = cost; }

BigNumber FamiliarTab::getWindPhysicalUpgradeCost() const { return windPhysicalCost; }
BigNumber FamiliarTab::getWindElementalUpgradeCost() const { return windElementalCost; }
BigNumber FamiliarTab::getWindCooldownUpgradeCost() const { return windCooldownCost; }
void FamiliarTab::setWindPhysicalUpgradeCost(BigNumber cost) { windPhysicalCost = cost; }
void FamiliarTab::setWindElementalUpgradeCost(BigNumber cost) { windElementalCost = cost; }
void FamiliarTab::setWindCooldownUpgradeCost(BigNumber cost) { windCooldownCost = cost; }
