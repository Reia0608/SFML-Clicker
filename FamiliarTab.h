#pragma once
#include "UIButton.h"
#include "TabContent.h"
#include "Hero.h"
#include "UnlockManager.h"
#include "ItemBar.h"
#include "BigNumber.h"

class FamiliarTab : public TabContent
{
private:
	int slotCost = 1;
	int evolveRedBabyDragonCost = 1;

	static constexpr int rubyEmblemIndex = 0;
	static constexpr int amberEmblemIndex = 1;
	static constexpr int topazEmblemIndex = 2;
	static constexpr int emeraldEmblemIndex = 3;
	static constexpr int sapphireEmblemIndex = 4;
	static constexpr int AmethystEmblemIndex = 5;

	Hero* hero;   // non-owning - Hero belongs to Game
	ItemManager* itemManager; // non-owning
	UnlockManager* unlockManager; // non-owning

	UIButton* buySlotButton;
	UIButton* evolveRedBabyDragonButton;

	struct ShopButtonEntry // pairs each button with the rule that decides whether it currently takes up UI space
	{
		UIButton* button;
		std::function<bool()> isVisible;
	};
	std::vector<ShopButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor - buySlotButton/evolveRedBabyDragonButton only; slot rows are separate (below), since how many of them exist changes at runtime

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
	void updateEvolveRedBabyDragonButtonLabel();
	void onEvolveRedBabyDragonButtonClicked();

public:
	FamiliarTab(Hero* hero, ItemManager* itemManager, UnlockManager* unlockManager);
	~FamiliarTab();

	void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize);
	void render(sf::RenderWindow* window);
	bool handleClick(sf::Vector2f pos);

	// NOTE: this override was missing before (TabContent's default just returns 0.0f), so this
	// tab never actually became scrollable even though contentHeight was already being tracked.
	// Harmless while the tab only had 2 fixed buttons; now that the slot list can grow past the
	// visible area, it needs to actually report its height like every other tab does.
	float getContentHeight() const override { return contentHeight; }

	// Wired by Game in initUI(): Game owns both familiarTypeList and the Familiar instance,
	// so it performs the actual evolution once payment succeeds here.
	std::function<void()> onEvolveRedBabyDragon;

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
};