#include "FamiliarTab.h"
#include "PublicVariables.h"
#include "Familiar.h"
#include "Statistics.h"
#include <sstream>
#include <iomanip>

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
		rebuildSlotButtons(); // a new (empty) slot row should now appear
	}
}

void FamiliarTab::addAttackUnlockButton(const std::string& displayName, const std::string& purchasableFeatureKey,
	const std::string& grantedFeatureKey, int currencyItemIndex, int currencyCost)
{
	std::string currencyName = itemManager->getItemDefinition(currencyItemIndex).name;
	std::string label = "Unlock " + displayName + " (Cost: " + std::to_string(currencyCost) + " " + currencyName + "(s))";

	UIButton* button = new UIButton(label, [this, currencyItemIndex, currencyCost, grantedFeatureKey]()
		{
			if (itemManager->trySpendItem(currencyItemIndex, currencyCost))
			{
				unlockManager->unlock(grantedFeatureKey);
				relayoutButtons(); // this button just hid itself (see isVisible below) - repack everything below it immediately, same as a slot/evolve purchase would
			}
		});

	buttonEntries.push_back({ button, [this, purchasableFeatureKey, grantedFeatureKey]()
		{
			// visible once its achievement unlocked the purchase, and only until it's actually bought
			return unlockManager->isUnlocked(purchasableFeatureKey) && !unlockManager->isUnlocked(grantedFeatureKey);
		} });
}

void FamiliarTab::addFamiliarUnlockButton(Familiar* targetFamiliar, const std::string& displayName, BigNumber cost)
{
	std::string label = "Unlock " + displayName + " (Cost: " + cost.toString() + " Gold(s))";

	UIButton* button = new UIButton(label, [this, targetFamiliar, cost]()
		{
			if (hero->trySpendCurrency(cost))
			{
				targetFamiliar->unlock();
				statistics->recordShopUpgradePurchased();
				relayoutButtons(); // this button just hid itself (see isVisible below) - repack everything below it immediately, same as a slot/evolve purchase would
			}
		});

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
				rebuildSlotButtons(); // the familiar's display label just changed (e.g. "Baby Water Dragon" -> "Water Dragon") - refresh any slot row currently showing it
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

	// Unlock buttons for every lineage except Fire. Fire's own unlock button stays in ShopTab -
	// it's the one that unlocks this whole tab to begin with, and doubles as a small tutorial for
	// a brand new player, so it's the single exception to "familiar buttons live in FamiliarTab".
	// Cost is always 10x the previous lineage's: Fire = 5000 Gold (in ShopTab), Water = 50000,
	// Wind = 500000. Add one more call here (and one more Familiar* parameter/member, see
	// FamiliarTab.h) whenever a new unlockable lineage is added.
	addFamiliarUnlockButton(waterFamiliar, "Baby Water Dragon", BigNumber(50000));
	addFamiliarUnlockButton(windFamiliar, "Baby Wind Dragon", BigNumber(500000));

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
	// matches what the rest of the game (e.g. slot rows) already calls that stage. Adult/Ancient
	// stages aren't in familiarTypeList yet, so isFamiliarStageAvailable will keep each button
	// hidden once it reaches Juvenile, until those stages are added on Game's side too - nothing
	// here needs to change when that happens. Add one more call here (with that lineage's own
	// 4-stage list and Emblem) whenever a new lineage is added.
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

	// slotButtons intentionally stays empty here - the callbacks above (onSlotPurchased etc.)
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
	hasUnseenUnlock = false; // being viewed right now - stop pulsing, same as AchievementsTab::render()

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