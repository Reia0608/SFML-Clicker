#pragma once
#include "UIButton.h"
#include "TabContent.h"
#include "Hero.h"
#include "UnlockManager.h"
#include "ItemBar.h"
#include "BigNumber.h"
#include "PublicVariables.h" // for FamiliarName - used by EvolutionStage and the evolve callbacks below

class Familiar; // forward declaration only - this header just stores pointers (see fireFamiliar/waterFamiliar/windFamiliar below), the full definition is only needed in FamiliarTab.cpp where those pointers are actually used
class Statistics; // forward declaration only - same reasoning, see the statistics member below

class FamiliarTab : public TabContent
{
private:
	int slotCost = 1;
	static constexpr int maxFamiliarSlots = 5; // hard cap on purchasable active-familiar slots - buySlotButton hides for good once this many are owned (see its isVisible rule in the constructor)
	bool hasUnseenUnlock = false; // flags a newly-revealed purchase button (e.g. Fireball after "Juvenile Flame") so the tab button pulses until viewed - same pattern as AchievementsTab::hasUnseenUnlock

	static constexpr int rubyEmblemIndex = 0;
	static constexpr int amberEmblemIndex = 1;
	static constexpr int topazEmblemIndex = 2;
	static constexpr int emeraldEmblemIndex = 3;
	static constexpr int sapphireEmblemIndex = 4;
	static constexpr int AmethystEmblemIndex = 5;

	Hero* hero;   // non-owning - Hero belongs to Game
	ItemManager* itemManager; // non-owning
	UnlockManager* unlockManager; // non-owning
	Statistics* statistics; // non-owning - only needed so familiar purchases made here still count via recordShopUpgradePurchased(), same as every purchase in ShopTab already does

	// One pointer per lineage (species). Game owns the actual Familiar objects (see Game.h's
	// ownedFamiliars) and hands us the pointers once at construction, same as it
	// already does for Hero above. Fire is included here too (unlike in addFamiliarUnlockButton,
	// where it's the one exception that stays in ShopTab - see that method's comment below) since
	// its Physical/Elemental/Cooldown upgrade buttons now live here like every other lineage's.
	// NOTE: add one more pointer here, plus one more addFamiliarUpgradeButtons() call (and, for an
	// unlockable lineage, one more addFamiliarUnlockButton() call) in the constructor, whenever a
	// new lineage is added.
	Familiar* fireFamiliar;  // non-owning
	Familiar* waterFamiliar; // non-owning
	Familiar* windFamiliar;  // non-owning

	// --- Familiar Physical/Elemental/Cooldown upgrade costs ---
	// Moved here from ShopTab (which only ever had Fire's) so every lineage's three upgrade
	// buttons can live together in this tab. Kept as named members - rather than e.g. a vector
	// indexed by lineage - because (a) there are only ever 3 of these per lineage, a vector
	// wouldn't meaningfully shrink the code, and (b) save/load wants a plain, explicit place to
	// read/write each one by name (see getFirePhysicalUpgradeCost() etc. below).
	// Starting cost is 10x the previous lineage's, same rule as the unlock costs in
	// addFamiliarUnlockButton() - only the STARTING cost differs per lineage; the gain-per-purchase
	// amounts and cost growth rates below are shared by all three, since nothing about them is
	// lineage-specific.
	// NOTE: add one more cost triplet here whenever a new lineage is added.
	BigNumber firePhysicalCost = BigNumber(6500);
	BigNumber fireElementalCost = BigNumber(8000);
	BigNumber fireCooldownCost = BigNumber(10000);

	BigNumber waterPhysicalCost = BigNumber(65000);
	BigNumber waterElementalCost = BigNumber(80000);
	BigNumber waterCooldownCost = BigNumber(100000);

	BigNumber windPhysicalCost = BigNumber(650000);
	BigNumber windElementalCost = BigNumber(800000);
	BigNumber windCooldownCost = BigNumber(1000000);

	static constexpr double physicalCostGrowth = 1.2;   // cost multiplies by this on every physical-power purchase
	static constexpr double elementalCostGrowth = 1.2;  // cost multiplies by this on every elemental-power purchase
	static constexpr double cooldownCostGrowth = 1.5;   // cost multiplies by this on every cooldown-reduction purchase

	static constexpr int physicalUpgradeAmount = 3;     // power gained per physical-power purchase
	static constexpr int elementalUpgradeAmount = 2;    // power gained per elemental-power purchase
	static constexpr float cooldownUpgradeAmount = 0.5f; // seconds shaved off per cooldown-reduction purchase

	// Every button addFamiliarUpgradeButtons() creates (3 per lineage, in call order) also gets
	// pushed here - NOT for ownership (buttonEntries already owns and deletes every button, same
	// as always) but so its label can be rebuilt on demand. familiarUpgradeRelabelers holds one
	// matching "rebuild my label from the current cost" step per button, at the same index - both
	// a purchase AND refreshUpgradeButtonLabels() (called after loading a save) call the same step,
	// instead of the cost-to-label text being written out in two separate places that could drift
	// apart. This is the same relabel-after-the-fact need ShopTab's old update*Label() methods
	// served, just shared across 3 lineages worth of buttons instead of one hardcoded button each.
	std::vector<UIButton*> familiarUpgradeButtons; // non-owning
	std::vector<std::function<void()>> familiarUpgradeRelabelers;

	// One entry per evolution stage of a lineage, in order (Baby, Juvenile, Adult, Ancient, ...).
	// Purely descriptive data for THIS tab's own button text - the actual FamiliarType data lives
	// in Game's familiarTypeList (see addFamiliarEvolveButton()'s comment for why those two don't
	// just read from one shared place).
	struct EvolutionStage
	{
		Age age;
		std::string displayLabel; // e.g. "Baby Water Dragon", "Water Dragon", "Adult Water Dragon"
	};

	// Returns targetFamiliar's current position within `stages` (0 = Baby, 1 = Juvenile, ...), or
	// -1 if its current age isn't in that list at all (shouldn't normally happen). Shared by
	// addFamiliarEvolveButton()'s click handler, relabeler, and visibility rule below, so "how do
	// I find where this familiar currently stands" is only written once.
	static int findCurrentStageIndex(Familiar* targetFamiliar, const std::vector<EvolutionStage>& stages);

	// Every button addFamiliarEvolveButton() creates (one per lineage) also gets pushed here - same
	// non-owning/parallel-relabeler pattern as familiarUpgradeButtons/familiarUpgradeRelabelers
	// above, for the same reason: both a purchase and refreshEvolveButtonLabels() (called after
	// loading a save restores a familiar's stage directly) need to rebuild the same button's label.
	std::vector<UIButton*> familiarEvolveButtons; // non-owning
	std::vector<std::function<void()>> familiarEvolveRelabelers;

	UIButton* buySlotButton;

	struct ShopButtonEntry // pairs each button with the rule that decides whether it currently takes up UI space
	{
		UIButton* button;
		std::function<bool()> isVisible;
	};
	std::vector<ShopButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor - buySlotButton, one evolve button per lineage, plus one entry per addAttackUnlockButton()/addFamiliarUnlockButton()/addFamiliarUpgradeButtons() call; slot rows are separate (below), since how many of them exist changes at runtime

	// --- Familiar slots ---
	// Unlike buttonEntries above, the number of slot rows (and, while picking, the number of
	// candidate familiars) changes at runtime, so these buttons can't be built once in the
	// constructor - they're destroyed and rebuilt by rebuildSlotButtons() every time the
	// underlying state changes (a slot is bought/assigned/unassigned) or the picker opens/closes.
	// Game owns the real slot/roster data; this class only displays it, via the callbacks below.
	std::vector<UIButton*> slotButtons;
	int pickingForSlotIndex = -1; // -1 = showing the normal slot list; >=0 = showing the familiar picker for that slot index
	void rebuildSlotButtons();
	void clearSlotButtons();
	void onSlotRowClicked(int slotIndex);           // normal mode: filled -> unassign; empty -> open the picker for this slot
	void onPickerCandidateClicked(int pickedIndex); // picker mode: assign the chosen familiar to pickingForSlotIndex, then close the picker
	void onPickerBackClicked();                     // picker mode: close the picker without assigning anything

	sf::Vector2f lastContentAreaOrigin;          // cached so relayoutButtons() can be called outside onResize
	sf::Vector2f lastContentAreaSize;

	float contentHeight = 0.0f; // total height of the button stack in its unscrolled layout (see getContentHeight)

	void relayoutButtons(); // the core of the locking system - also lays out slotButtons, stacked directly below buttonEntries

	void updateBuySlotButtonLabel();
	void onBuySlotButtonClicked();

	// Builds one "purchase this gated attack" button and pushes it into buttonEntries, following
	// the same two-key pattern every gated attack uses (see FamiliarAttack::unlockFeatureKey):
	// an achievement claim unlocks purchasableFeatureKey, which is what makes THIS button appear;
	// buying it (for currencyCost of itemManager's currencyItemIndex item) unlocks
	// grantedFeatureKey, which is the key the attack itself actually checks (in HUD) to become
	// usable - and also makes this same button disappear again, since there's nothing left to buy.
	// A single parameterized helper instead of a bespoke button+handler+member per attack (like
	// ShopTab's original familiar-upgrade buttons, before Step 3) - adding the next gated attack
	// later is one more call to this in the constructor, not a new set of header members and a
	// new .cpp handler to copy and risk a mistake in.
	void addAttackUnlockButton(const std::string& displayName, const std::string& purchasableFeatureKey,
		const std::string& grantedFeatureKey, int currencyItemIndex, int currencyCost);

	// Builds a one-time "Unlock Baby <displayName>" button and pushes it into buttonEntries.
	// Spends Hero's gold directly (not an Emblem - gold is what the very first, Fire Dragon,
	// unlock button in ShopTab already uses; Emblems are reserved for evolution/ability costs,
	// see addAttackUnlockButton above). Visible until targetFamiliar is unlocked, then hidden for
	// good - same one-time-purchase behavior as ShopTab's original Fire Dragon button, just
	// generalized so a future lineage's unlock button is one more call here instead of a new
	// bespoke button+handler pair (same reasoning as addAttackUnlockButton above).
	void addFamiliarUnlockButton(Familiar* targetFamiliar, const std::string& displayName, BigNumber cost);

	// Builds the Physical Attack / Elemental Attack / Cooldown Reduction upgrade buttons for one
	// lineage and pushes all three into buttonEntries - this is the generalized, three-lineage
	// version of what used to be three bespoke buttons+handlers hardcoded to Fire in ShopTab.
	// physicalCost/elementalCost/cooldownCost are taken BY REFERENCE directly to that lineage's
	// named cost members above, so each button's purchase handler can escalate its own lineage's
	// cost in place without any extra lookup/indexing - and so save/load (via the getters/setters
	// below) is always reading the same memory these buttons are updating.
	// physical/elemental buttons: visible once targetFamiliar is unlocked, always stay visible
	// after that (repeatable purchases, unlike addFamiliarUnlockButton's one-time buttons).
	// cooldown button: same, but ALSO hides once targetFamiliar->isAutoAttackCooldownAtMinimum()
	// - identical visibility rule to ShopTab's original Fire-only cooldown button.
	void addFamiliarUpgradeButtons(Familiar* targetFamiliar, const std::string& lineageLabel,
		BigNumber& physicalCost, BigNumber& elementalCost, BigNumber& cooldownCost);

	std::string formatCooldownAmount(float amount); // e.g. 0.5f -> "0.5" - same formatting ShopTab's original cooldown button used

	// Builds ONE self-updating evolve button for a lineage and pushes it into buttonEntries -
	// unlike every other add* helper here, this ISN'T called once per transition: the same button
	// re-labels itself after each purchase to offer the NEXT transition (Baby->Juvenile, then
	// Juvenile->Adult, then Adult->Ancient), until `stages` runs out or Game doesn't have that next
	// stage's data yet (see isFamiliarStageAvailable below) - at which point it hides, the same
	// "one-time purchase, then gone" shape as addFamiliarUnlockButton, just repeated per tier.
	// Cost is always (how many evolutions targetFamiliar has already had) + 1 Emblem of
	// emblemIndex's kind - 1 for the first transition, 2 for the second, 3 for the third, matching
	// the current design. `stages` is taken by value (copied into this button's own captures) since
	// it's small, fixed, descriptive data that never needs to change after construction - unlike
	// addFamiliarUpgradeButtons' BY-REFERENCE costs, nothing here needs to reach back into a
	// member variable that purchases mutate in place.
	void addFamiliarEvolveButton(Familiar* targetFamiliar, std::vector<EvolutionStage> stages, int emblemIndex);

public:
	FamiliarTab(Hero* hero, ItemManager* itemManager, UnlockManager* unlockManager, Statistics* statistics,
		Familiar* fireFamiliar, Familiar* waterFamiliar, Familiar* windFamiliar);
	~FamiliarTab();

	void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize);
	void render(sf::RenderWindow* window);
	bool handleClick(sf::Vector2f pos);

	// NOTE: this override was missing before (TabContent's default just returns 0.0f), so this
	// tab never actually became scrollable even though contentHeight was already being tracked.
	// Harmless while the tab only had 2 fixed buttons; now that the slot list can grow past the
	// visible area, it needs to actually report its height like every other tab does.
	float getContentHeight() const override { return contentHeight; }

	// Makes the tab button pulse (same visual as a tab that just became available) until this tab
	// is actually viewed - see render() below, which clears the flag, same pattern as
	// AchievementsTab::hasPendingNotification()/markPending().
	bool hasPendingNotification() const override { return hasUnseenUnlock; }
	void markPending() { hasUnseenUnlock = true; } // call from Game whenever an achievement reveals a new purchase button here (e.g. "Juvenile Flame")

	// Wired by Game in initUI(): Game owns familiarTypeList, so it's the one that resolves
	// (target's species, nextAge) into real FamiliarType data and performs the actual evolveTo()
	// once payment succeeds here - FamiliarTab only ever deals in Ages and its own EvolutionStage
	// labels, never needing to know target's species itself (Game reads that via getName()).
	std::function<void(Familiar* target, Age nextAge)> onEvolveFamiliar;

	// Also wired by Game: lets this tab check, BEFORE letting the player pay, whether Game's
	// familiarTypeList actually has data for a given (species, stage) pair yet - so an evolve
	// button only ever offers a transition that can actually happen (see addFamiliarEvolveButton()'s
	// comment). Takes the species explicitly since, unlike onEvolveFamiliar above, there's no
	// Familiar* here to read it from.
	std::function<bool(FamiliarName species, Age age)> isFamiliarStageAvailable;

	// Wired by Game in initUI() - Game owns the real slot/roster state; this tab only displays
	// it and reports the player's intent back through these.
	std::function<void()> onSlotPurchased;                                // fired once the item cost was successfully paid - Game should add one more (empty) slot
	std::function<int()> getSlotCount;                                    // total number of unlocked slots
	std::function<std::string(int slotIndex)> getSlotFamiliarLabel;       // display label of the familiar in that slot, or "" if it's empty
	std::function<std::vector<std::string>()> getAvailableFamiliarNames;  // display label of every unlocked-but-unassigned familiar, in picker order
	std::function<void(int slotIndex, int pickedIndex)> onAssignFamiliarToSlot; // pickedIndex indexes into getAvailableFamiliarNames()
	std::function<void(int slotIndex)> onUnassignSlot;

	// Rebuilds the slot button list from the callbacks above. Called internally after any click
	// that changes slot state, and publicly by Game whenever IT changes that state directly
	// instead (e.g. right after loadFromFile() restores saved slots).
	void refreshSlotButtons();

	// Re-checks every buttonEntries' isVisible() rule and repacks the layout accordingly, without
	// touching slotButtons. Call this whenever something OUTSIDE this tab changes a feature key
	// one of those rules depends on - today, just an achievement claim unlocking a purchase
	// button (see addAttackUnlockButton()) - since nothing inside FamiliarTab itself would
	// otherwise notice that happened. A purchase made from within this tab already calls
	// relayoutButtons() directly and doesn't need this.
	void refreshButtonVisibility();

	// Rebuilds every Physical/Elemental/Cooldown button's displayed text from its lineage's
	// current cost - call this after restoring saved costs (e.g. Game::loadFromFile()), same
	// reason refreshButtonVisibility() exists: nothing inside FamiliarTab would otherwise notice
	// those costs changed from outside a purchase.
	void refreshUpgradeButtonLabels();

	// Rebuilds every lineage's evolve button label from that familiar's CURRENT stage - call this
	// after restoring a saved evolution stage directly (bypassing the button entirely), same
	// reasoning as refreshUpgradeButtonLabels() above.
	void refreshEvolveButtonLabels();

	// --- for save/load ---
	int getSlotCost() const;  // current Amber cost of the NEXT familiar slot purchase - rises by 1 after each slot bought (see onBuySlotButtonClicked())
	void setSlotCost(int cost);

	// Moved here from ShopTab along with the buttons that use them (see the cost members above).
	BigNumber getFirePhysicalUpgradeCost() const;
	BigNumber getFireElementalUpgradeCost() const;
	BigNumber getFireCooldownUpgradeCost() const;
	void setFirePhysicalUpgradeCost(BigNumber cost);
	void setFireElementalUpgradeCost(BigNumber cost);
	void setFireCooldownUpgradeCost(BigNumber cost);

	BigNumber getWaterPhysicalUpgradeCost() const;
	BigNumber getWaterElementalUpgradeCost() const;
	BigNumber getWaterCooldownUpgradeCost() const;
	void setWaterPhysicalUpgradeCost(BigNumber cost);
	void setWaterElementalUpgradeCost(BigNumber cost);
	void setWaterCooldownUpgradeCost(BigNumber cost);

	BigNumber getWindPhysicalUpgradeCost() const;
	BigNumber getWindElementalUpgradeCost() const;
	BigNumber getWindCooldownUpgradeCost() const;
	void setWindPhysicalUpgradeCost(BigNumber cost);
	void setWindElementalUpgradeCost(BigNumber cost);
	void setWindCooldownUpgradeCost(BigNumber cost);
};