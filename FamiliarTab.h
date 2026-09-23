#pragma once
#include "UIButton.h"
#include "TabContent.h"
#include "Hero.h"
#include "UnlockManager.h"
#include "ItemBar.h"
#include "BigNumber.h"

class FamiliarTab: public TabContent
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

	Hero* hero;   // non-owning — Hero belongs to Game
	ItemManager* itemManager; // non-owning
	UnlockManager* unlockManager; // non-owning

	UIButton* buySlotButton;
	UIButton* evolveRedBabyDragonButton;

	struct ShopButtonEntry // pairs each button with the rule that decides whether it currently takes up UI space
	{
		UIButton* button;
		std::function<bool()> isVisible;
	};
	std::vector<ShopButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor
	sf::Vector2f lastContentAreaOrigin;          // cached so relayoutButtons() can be called outside onResize
	sf::Vector2f lastContentAreaSize;

	float contentHeight = 0.0f; // total height of the button stack in its unscrolled layout (see getContentHeight)

	void relayoutButtons(); // the core of the locking system

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

	// Wired by Game in initUI(): Game owns both familiarTypeList and the Familiar instance,
	// so it performs the actual evolution once payment succeeds here.
	std::function<void()> onEvolveRedBabyDragon;
};