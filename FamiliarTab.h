#pragma once
#include "UIButton.h"
#include "TabContent.h"
#include "Hero.h"
#include "UnlockManager.h"
#include "ItemBar.h"
#include "BigNumber.h"
#include "Text.h"
#include "PublicVariables.h" // for FamiliarName - used by EvolutionStage and the evolve callbacks below

class Familiar; // forward declaration only - this header just stores pointers (see fireFamiliar/waterFamiliar/windFamiliar below), the full definition is only needed in FamiliarTab.cpp where those pointers are actually used
class Statistics; // forward declaration only - same reasoning, see the statistics member below

// What the slot grid and the familiar picker both need to draw ONE familiar as a picture: its
// idle animation's sprite sheet, and the source rect of just that animation's FIRST frame (so a
// slot/picker tile always shows a still image, never actually animates), plus its current
// display label (species + current age, e.g. "Baby Water Dragon") for the picker's caption.
// Resolved by Game (it owns familiarTypeList, the only place idleAnimationAddress/spriteSize live
// for a given species+stage pair) - this tab only ever draws whatever it's handed, it never
// resolves a Familiar* into sprite data itself.
struct FamiliarIcon
{
	std::string texturePath; // empty = no familiar - see isEmpty()
	sf::IntRect sourceRect;  // the idle sheet's first frame, in that sheet's own pixel coordinates
	std::string displayLabel;

	bool isEmpty() const { return texturePath.empty(); } // an empty slot resolves to a default-constructed FamiliarIcon, same as "" used to mean for the old text-label version of this
};

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
	// Starting cost is fixed per lineage below - these are repeatable purchases with their own
	// escalating cost, unrelated to a familiar's one-time UNLOCK cost, which instead follows
	// UnlockManager's shared, species-agnostic unlock-order ladder (see addFamiliarUnlockButton()).
	// The gain-per-purchase amounts and cost growth rates below are shared by all three lineages,
	// since nothing about them is lineage-specific.
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

	std::vector<UIButton*> familiarUnlockButtons; // non-owning (buttonEntries owns/deletes, same as every other button here)
	std::vector<std::function<void()>> familiarUnlockRelabelers; // one per unlock button - needed because EVERY still-locked familiar's displayed price depends on UnlockManager's shared unlock-order ladder, so buying any ONE of them has to refresh ALL of them, not just its own (see refreshUnlockButtonLabels())

	UIButton* buySlotButton;

	struct ShopButtonEntry // pairs each button with the rule that decides whether it currently takes up UI space
	{
		UIButton* button;
		std::function<bool()> isVisible;
	};
	std::vector<ShopButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor - buySlotButton, one evolve button per lineage, plus one entry per addAttackUnlockButton()/addFamiliarUnlockButton()/addFamiliarUpgradeButtons() call; slot rows are separate (below), since how many of them exist changes at runtime

	// --- Familiar slots ---
	// Unlike buttonEntries above, the number of slot tiles (and, while picking, the number of
	// candidate familiars) changes at runtime, so these buttons can't be built once in the
	// constructor - they're destroyed and rebuilt by rebuildSlotTiles() every time the underlying
	// state changes (a slot is bought/assigned/unassigned). Game owns the real slot/roster data;
	// this class only displays it, via the callbacks below.
	// Each slot tile is square and sized so slotsPerRow of them exactly fill the tab's current
	// width (computed fresh in relayoutButtons() from lastContentAreaSize, not a fixed constant
	// like this used to be) - a slot is meant to read as "about half the panel", not a small fixed
	// box with empty space sitting next to it, and it grows/shrinks with the window like
	// everything else in this tab.
	static constexpr int slotsPerRow = 2; // how many slot tiles sit side by side before wrapping to the next row

	struct SlotTile
	{
		UIButton* button; // background + click handling - shows the occupant's icon when filled, a blank background when empty (the "+" on top is drawn separately, see drawPlusMark)
		bool isEmpty = true;
		sf::Vector2f layoutPosition{}; // cached from the last relayoutButtons() so render() can draw the "+" at the same spot without re-deriving it
		sf::Vector2f layoutSize{};
	};
	std::vector<SlotTile> slotTiles;
	void rebuildSlotTiles(); // destroys and recreates every slot tile from getSlotCount()/getSlotFamiliarIcon()
	void clearSlotTiles();
	void onSlotRowClicked(int slotIndex); // filled -> unassign; empty -> open the picker popup for this slot
	void drawPlusMark(sf::RenderWindow* window, sf::Vector2f tileTopLeft, sf::Vector2f tileSize); // the two-bar "+" drawn on top of an empty slot's button - see slotTiles' comment on why this isn't just the button's own icon

	// --- Familiar picker popup ---
	// A real modal popup: opening it doesn't replace the slot grid, it floats a dimmed panel on
	// top of EVERYTHING - the whole window, including the play area and item bar, not just this
	// tab's own panel - so the player gets a proper overview of every available familiar while
	// choosing. That's why rendering it is split off into the public renderPicker() below instead
	// of happening inside this tab's own render(): TabbedPanel scissor-clips a scrollable tab's
	// render() to just its own content-area rectangle (see TabbedPanel::render()), so anything
	// drawn from inside THIS class's render() could never reach past the tab panel's own width.
	// Game calls renderPicker() itself, after tabbedPanel->render() has already finished (same
	// spot it already draws saveIndicator/ruler from), which is outside any scissor and reaches
	// the full window. pickingForSlotIndex is -1 while closed, or the slot index being filled
	// while open; isPickerOpen() below is Game's own cue for both that render-order call and for
	// not letting a click fall through to a HUD manual-attack button or the hero's own click
	// damage while the popup is covering the play area (see isPickerOpen()'s own comment).
	int pickingForSlotIndex = -1;

	static constexpr float pickerTileSize = 160.0f;    // each candidate's icon, square, this many pixels on a side - sized to read on the same scale as the (now much bigger) slot tiles, not as a small thumbnail
	static constexpr float pickerCellWidth = 200.0f;   // column width reserved per candidate - wider than the icon itself so a two-line caption underneath isn't cramped against its neighbor
	static constexpr float pickerCellSpacing = 20.0f;
	static constexpr float pickerRowSpacing = 24.0f;
	static constexpr float pickerCaptionHeight = 40.0f; // room reserved under each icon for up to two lines of caption text
	static constexpr int pickerMaxColumnsPerRow = 4;    // upper bound on columns - the actual column count also shrinks to fit the window's current width, and never exceeds how many candidates there are

	struct PickerCandidateTile
	{
		UIButton* button;
		Text* caption; // species + current age, e.g. "Baby Water Dragon" - wrapped to pickerCellWidth in relayoutPicker()
	};
	std::vector<PickerCandidateTile> pickerCandidates;
	Text* pickerTitle = nullptr;
	Text* pickerEmptyMessage = nullptr; // shown instead of the candidate grid when there's nothing left to assign
	UIButton* pickerCloseButton = nullptr;
	sf::RectangleShape pickerPanelBackground;

	void openPicker(int slotIndex);
	void closePicker();
	void rebuildPickerCandidates(); // (re)builds pickerCandidates/pickerTitle/pickerEmptyMessage/pickerCloseButton from getAvailableFamiliarIcons() - called every time the popup opens, since the available roster can only have changed since it last closed
	void clearPickerCandidates();
	void onPickerCandidateClicked(int pickedIndex); // assigns the chosen familiar to pickingForSlotIndex, then closes the popup
	bool handlePickerClick(sf::Vector2f pos); // routes a click while the popup is open - close button, a candidate, or (falling through) the dimmed backdrop itself, which also closes it. Always returns true: nothing underneath should react to a click while the popup is open
	void relayoutPicker(); // positions the popup panel + everything in it, centered on the WHOLE window (the windowSize global, not this tab's content area - see the class comment above) - called from relayoutButtons() so it stays centered through a resize, and right after rebuildPickerCandidates()

	sf::Vector2f lastContentAreaOrigin;          // cached so relayoutButtons() can be called outside onResize
	sf::Vector2f lastContentAreaSize;

	float contentHeight = 0.0f; // total height of the button stack in its unscrolled layout (see getContentHeight)

	void relayoutButtons(); // the core of the locking system - also lays out slotTiles in their grid, stacked directly below buttonEntries, and keeps the picker popup (if open) centered

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
	// passiveDealsDamage: true only for a gated PASSIVE that deals its own damage (today: Burn) -
	// fires onDamagingPassiveUnlocked (see below) the moment it's actually purchased, so Game can
	// flash StatisticsTab's tab button the same way it does for a familiar unlock. Left false (the
	// default) for every gated manual attack (Fireball, Bubble Stream, Wind Swipes) and for a
	// passive that doesn't deal damage itself (Soaked, Keen Gale).
	void addAttackUnlockButton(const std::string& displayName, const std::string& purchasableFeatureKey,
		const std::string& grantedFeatureKey, int currencyItemIndex, int currencyCost, bool passiveDealsDamage = false);

	// Builds a one-time "Unlock Baby <displayName>" button and pushes it into buttonEntries, plus a
	// relabeler into familiarUnlockRelabelers (see refreshUnlockButtonLabels()). Spends Hero's gold
	// directly (not an Emblem - gold is what the very first, Fire Dragon, unlock button in ShopTab
	// already uses; Emblems are reserved for evolution/ability costs, see addAttackUnlockButton
	// above). The cost is never fixed here - it's always UnlockManager's live
	// getNextFamiliarUnlockCost(), which climbs 10x every time ANY familiar gets unlocked, regardless
	// of species (see UnlockManager.h). familiarKey is that species' save-string (e.g. "WaterDragon")
	// - what UnlockManager actually remembers the unlock ORDER by, and what later locks in this
	// familiar's own elemental-damage achievement threshold once it's bought. Visible until
	// targetFamiliar is unlocked, then hidden for good.
	void addFamiliarUnlockButton(Familiar* targetFamiliar, const std::string& familiarKey, const std::string& displayName);

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
	// Harmless while the tab only had 2 fixed buttons; now that the slot grid can grow past the
	// visible area, it needs to actually report its height like every other tab does.
	float getContentHeight() const override { return contentHeight; }

	// Makes the tab button pulse (same visual as a tab that just became available) until this tab
	// is actually viewed - see render() below, which clears the flag, same pattern as
	// AchievementsTab::hasPendingNotification()/markPending().
	bool hasPendingNotification() const override { return hasUnseenUnlock; }
	void markPending() { hasUnseenUnlock = true; } // call from Game whenever an achievement reveals a new purchase button here (e.g. "Juvenile Flame")

	// True while the familiar picker popup is open. Game checks this for two reasons: (1) right
	// after tabbedPanel->render(), to know whether to also call renderPicker() below (see that
	// method's own comment, and this class's comment on pickingForSlotIndex, for why the popup
	// can't just render itself from inside this tab's own render()); (2) in its own mouse-click
	// handling, to skip hud->handleClick()/the hero's own click-damage for a play-area click while
	// the popup is covering it - otherwise a click reaching through the dimmed backdrop could
	// still fire a manual-attack HUD button underneath, since that's checked before the click ever
	// reaches tabbedPanel (and therefore this tab's own handleClick(), which already absorbs every
	// click while the popup is open regardless of where on screen it lands).
	bool isPickerOpen() const { return pickingForSlotIndex >= 0; }

	// Draws the popup itself - backdrop + panel + candidates/close button - covering the WHOLE
	// window, not just this tab's own panel. Call this from Game, right after tabbedPanel->render()
	// has already finished (see this class's comment on pickingForSlotIndex for why it has to be
	// called from there rather than from inside this tab's own render()). A no-op while
	// isPickerOpen() is false, so it's always safe to call unconditionally if that's simpler on
	// the caller's side.
	void renderPicker(sf::RenderWindow* window);

	// Wired by Game in initUI(): fired once a familiar is actually bought here - Water/Wind's own
	// unlock buttons, see addFamiliarUnlockButton() - or once a damage-dealing passive (e.g. Burn)
	// is actually purchased, see addAttackUnlockButton()'s passiveDealsDamage flag. Lets Game flash
	// StatisticsTab's own tab button the same moment new per-familiar rows become visible there,
	// without FamiliarTab needing to know StatisticsTab exists. Fire Dragon's own unlock (bought
	// from ShopTab, not here) is covered separately by ShopTab::onFamiliarUnlocked.
	std::function<void()> onFamiliarUnlocked;
	std::function<void()> onDamagingPassiveUnlocked;

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
	std::function<FamiliarIcon(int slotIndex)> getSlotFamiliarIcon;       // icon info for the familiar in that slot, or a default-constructed (isEmpty()) FamiliarIcon if it's empty
	std::function<std::vector<FamiliarIcon>()> getAvailableFamiliarIcons; // icon info for every unlocked-but-unassigned familiar, in picker order
	std::function<void(int slotIndex, int pickedIndex)> onAssignFamiliarToSlot; // pickedIndex indexes into getAvailableFamiliarIcons()
	std::function<void(int slotIndex)> onUnassignSlot;

	// Rebuilds the slot tile grid from the callbacks above. Called internally after any click
	// that changes slot state, and publicly by Game whenever IT changes that state directly
	// instead (e.g. right after loadFromFile() restores saved slots).
	void refreshSlotButtons();

	// Re-checks every buttonEntries' isVisible() rule and repacks the layout accordingly, without
	// touching slotTiles. Call this whenever something OUTSIDE this tab changes a feature key
	// one of those rules depends on - today, just an achievement claim unlocking a purchase
	// button (see addAttackUnlockButton above) - since nothing inside FamiliarTab itself would
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

	// Rebuilds every still-locked familiar's unlock-button price from UnlockManager's live
	// unlock-order ladder - call this after ANY familiar gets unlocked, from wherever that
	// happened (this tab's own buttons call it directly; Game also calls it after ShopTab's Fire
	// purchase succeeds, and after restoring a saved unlock order on load), since buying one
	// familiar changes the displayed price of every OTHER one still locked.
	void refreshUnlockButtonLabels();

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
