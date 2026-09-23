#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "UIButton.h"
#include "Hero.h"
#include "BigNumber.h"
#include "Statistics.h"
#include "Familiar.h"
#include "UnlockManager.h"


class ShopTab : public TabContent
{
private:
	Hero* hero;   // non-owning � Hero belongs to Game, not ShopTab
	Familiar* familiar; // non-owning
	Statistics* statistics; // non-owning
	UnlockManager* unlockManager; // non-owning

	UIButton* buyAttackButton;
	UIButton* buyGoldMultiplierButton;
	UIButton* buyCritChanceButton;
	UIButton* buyCritDamageButton;
	UIButton* buyFamiliarButton;
	UIButton* buyFamiliarUpgradePhysicalAttackButton;
	UIButton* buyFamiliarUpgradeElementalAttackButton;
	UIButton* buyFamiliarCooldownReductionButton;

	struct ShopButtonEntry // pairs each button with the rule that decides whether it currently takes up UI space
	{
		UIButton* button;
		std::function<bool()> isVisible;
	};
	std::vector<ShopButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor
	sf::Vector2f lastContentAreaOrigin;          // cached so relayoutButtons() can be called outside onResize
	sf::Vector2f lastContentAreaSize;

	float contentHeight = 0.0f; // total height of the button stack in its unscrolled layout (see getContentHeight)

	BigNumber attackUpgradeCost = BigNumber(50);
	BigNumber attackUpgradeAmount = BigNumber(1);
	BigNumber goldMultiplierCost = BigNumber(30);
	double goldMultiplierIncreaseAmount = 0.5;

	BigNumber critChanceCost = BigNumber(200);
	double critChanceUnlockValue = 0.005;
	double critChanceIncreaseAmount = 0.0025;

	BigNumber familiarCost = BigNumber(5000); // one-time unlock cost, no scaling
	BigNumber familiarPhysicalAttackUpgradeCost = BigNumber(6500);
	BigNumber familiarPhysicalAttackUpgradeAmount = BigNumber(3);
	BigNumber familiarElementalAttackUpgradeCost = BigNumber(8000);
	BigNumber familiarElementalAttackUpgradeAmount = BigNumber(2);
	BigNumber familiarCooldownReductionCost = BigNumber(10000);
	float familiarCooldownReductionAmount = 0.5f;

	BigNumber critDamageCost = BigNumber(150);
	double critDamageIncreaseAmount = 0.10;

	int goldMultiplierUpgradesPurchased = 0; // needed to retroactively rebalance the multiplier when the increase amount changes

	void relayoutButtons(); // positions only currently-visible buttons, called from onResize() and after any purchase that could change visibility
	void updateButtonLabel();
	void onBuyAttackClicked();
	void updateGoldMultiplierButtonLabel();
	void onBuyGoldMultiplierClicked();

	// Familiar-related methods
	void updateFamiliarButtonLabel();
	void onBuyFamiliarClicked();
	void updateFamiliarPhysicalAttackButtonLabel();
	void onBuyFamiliarPhysicalAttackUpgradeClicked();
	void updateFamiliarElementalAttackButtonLabel();
	void onBuyFamiliarElementalAttackUpgradeClicked();
	void updateFamiliarCooldownButtonLabel();
	void onBuyFamiliarCooldownReductionClicked();

	std::string formatCooldownAmount(double amount);

public:
	ShopTab(Hero* hero, Statistics* statistics, Familiar* familiar, UnlockManager* unlockManager);
	~ShopTab();

	void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
	void render(sf::RenderWindow* window) override;
	bool handleClick(sf::Vector2f pos) override;

	float getContentHeight() const override { return contentHeight; }
	void updateCritChanceButtonLabel();
	void onBuyCritChanceClicked();
	void updateCritDamageButtonLabel();
	void onBuyCritDamageClicked();

	void increaseGoldMultiplierGain(float amount); // called once when "Hoarder" unlocks: doubles goldMultiplierIncreaseAmount permanently,
	// and rebalances hero's current multiplier as if every past purchase had used the new amount

	// for save/ load
	// Persistence: current prices aren't derivable from Hero's stats alone (e.g. attack power
	// can come from other sources later), so they need to be saved/restored explicitly.
	BigNumber getAttackUpgradeCost() const;
	BigNumber getGoldMultiplierCost() const;
	BigNumber getCritChanceCost() const;
	BigNumber getCritDamageCost() const;
	BigNumber getFamiliarPhysicalAttackCost() const;
	BigNumber getFamiliarElementalAttackCost() const;
	BigNumber getFamiliarCooldownReductionCost() const;

	// persistence for the gold-multiplier upgrade's scaling state (affected by the Hoarder achievement)
	double getGoldMultiplierIncreaseAmount() const;
	int getGoldMultiplierUpgradesPurchased() const;

	void setAttackUpgradeCost(BigNumber cost);
	void setGoldMultiplierCost(BigNumber cost);
	void setCritChanceCost(BigNumber cost);
	void setCritDamageCost(BigNumber cost);
	void setFamiliarPhysicalAttackCost(BigNumber cost);
	void setFamiliarElementalAttackCost(BigNumber cost);
	void setFamiliarCooldownReductionCost(BigNumber cost);

	void setGoldMultiplierIncreaseAmount(double value); // restores saved per-purchase gain (e.g. after Hoarder doubled it)
	void setGoldMultiplierUpgradesPurchased(int count);  // restores saved purchase count, used only for future rebalancing math

	void refreshLabels(); // re-reads all costs into the button labels, and the familiar button's visibility - call after restoring saved state
	void resetPricesToDefault(); // debug only, restores the four costs to their starting values (does not touch familiar unlock status)
	void refreshVisibility(); // public wrapper around relayoutButtons(), for exactly that load-time case
};