#include "stdafx.h"
#include "ShopTab.h"
#include <sstream>
#include <iomanip>

void ShopTab::updateButtonLabel()
{
	buyAttackButton->setLabelText("Attack +" + attackUpgradeAmount.toString() + " (Cost: " + attackUpgradeCost.toString() + ")");
}

void ShopTab::onBuyAttackClicked()
{
	if (hero->trySpendCurrency(attackUpgradeCost))
	{
		hero->increaseAttackPower(attackUpgradeAmount);
		attackUpgradeCost = attackUpgradeCost * BigNumber(1.1);
		statistics->recordShopUpgradePurchased();
		updateButtonLabel();
		hero->updateClickDamageText();
	}
}

void ShopTab::updateGoldMultiplierButtonLabel()
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(2) << (hero->getGoldMultiplier() + goldMultiplierIncreaseAmount);

	buyGoldMultiplierButton->setLabelText("Gold x" + out.str() + " (Cost: " + goldMultiplierCost.toString() + ")");
}

void ShopTab::onBuyGoldMultiplierClicked()
{
	if (hero->trySpendCurrency(goldMultiplierCost))
	{
		hero->increaseGoldMultiplier(goldMultiplierIncreaseAmount);
		goldMultiplierCost = goldMultiplierCost * BigNumber(1.5);
		statistics->recordShopUpgradePurchased();
		updateGoldMultiplierButtonLabel();
	}
}

void ShopTab::updateFamiliarButtonLabel()
{
	buyFamiliarButton->setLabelText("Unlock Baby Fire Dragon (Cost: " + familiarCost.toString() + ")");
}

void ShopTab::onBuyFamiliarClicked()
{
	if (familiar->isUnlocked()) return; // safety net, button should already be hidden at this point

	if (hero->trySpendCurrency(familiarCost))
	{
		familiar->unlock();
		statistics->recordShopUpgradePurchased();
		buyFamiliarButton->setEnabled(false); // one-time purchase, hide permanently
	}

	buyFamiliarUpgradePhysicalAttackButton->setEnabled(true);
	buyFamiliarUpgradeElementalAttackButton->setEnabled(true);
	buyFamiliarCooldownReductionButton->setEnabled(true);
}

void ShopTab::updateFamiliarPhysicalAttackButtonLabel()
{
	buyFamiliarUpgradePhysicalAttackButton->setLabelText("Familiar Physical Attack +" + familiarPhysicalAttackUpgradeAmount.toString() + " (Cost: " + familiarPhysicalAttackUpgradeCost.toString() + ")");
}

void ShopTab::onBuyFamiliarPhysicalAttackUpgradeClicked()
{
	if (hero->trySpendCurrency(familiarPhysicalAttackUpgradeCost))
	{
		familiar->increasePhysicalAttack(familiarPhysicalAttackUpgradeAmount);
		statistics->recordShopUpgradePurchased();
		familiarPhysicalAttackUpgradeCost = familiarPhysicalAttackUpgradeCost * BigNumber(1.2);
		updateFamiliarPhysicalAttackButtonLabel();
		familiar->updateFamiliarDPSText();
	}
}

void ShopTab::updateFamiliarElementalAttackButtonLabel()
{
	buyFamiliarUpgradeElementalAttackButton->setLabelText("Familiar Elemental Attack +" + familiarElementalAttackUpgradeAmount.toString() + " (Cost: " + familiarElementalAttackUpgradeCost.toString() + ")");
}

void ShopTab::onBuyFamiliarElementalAttackUpgradeClicked()
{
	if (hero->trySpendCurrency(familiarElementalAttackUpgradeCost))
	{
		familiar->increaseElementalAttack(familiarElementalAttackUpgradeAmount);
		statistics->recordShopUpgradePurchased();
		familiarElementalAttackUpgradeCost = familiarElementalAttackUpgradeCost * BigNumber(1.2);
		updateFamiliarElementalAttackButtonLabel();
		familiar->updateFamiliarDPSText();
	}
}

void ShopTab::updateFamiliarCooldownButtonLabel()
{
	buyFamiliarCooldownReductionButton->setLabelText("Familiar cooldown reduction by " + std::to_string(familiarCooldownReductionAmount) + " (Cost: " + familiarCooldownReductionCost.toString() + ")");
}

void ShopTab::onBuyFamiliarCooldownReductionClicked()
{
	if (hero->trySpendCurrency(familiarCooldownReductionCost))
	{
		familiar->reduceAttackCooldown(familiarCooldownReductionAmount);
		statistics->recordShopUpgradePurchased();
		familiarCooldownReductionCost = familiarCooldownReductionCost * BigNumber(1.5);
		updateFamiliarCooldownButtonLabel();
		familiar->updateFamiliarDPSText();

		if (familiar->isAttackIntervalAtMinimum())
		{
			buyFamiliarCooldownReductionButton->setEnabled(false); // max reduction reached - hide permanently
		}
	}
}

ShopTab::ShopTab(Hero* hero, Statistics* statistics, Familiar* familiar) : hero(hero), statistics(statistics), familiar(familiar)
{
	buyAttackButton = new UIButton("", [this]() { onBuyAttackClicked(); });
	buyGoldMultiplierButton = new UIButton("", [this]() { onBuyGoldMultiplierClicked(); });
	buyCritChanceButton = new UIButton("", [this]() { onBuyCritChanceClicked(); });
	buyCritDamageButton = new UIButton("", [this]() { onBuyCritDamageClicked(); });
	buyFamiliarButton = new UIButton("", [this]() { onBuyFamiliarClicked(); });
	updateButtonLabel();
	updateGoldMultiplierButtonLabel();
	updateCritChanceButtonLabel();
	updateCritDamageButtonLabel();
	buyFamiliarButton->setEnabled(!familiar->isUnlocked()); // hidden once already unlocked (e.g. after loading a save)
	updateFamiliarButtonLabel();

	// create and unlock Familiar Upgrade Buttons
	buyFamiliarUpgradePhysicalAttackButton = new UIButton("", [this]() { onBuyFamiliarPhysicalAttackUpgradeClicked(); });
	buyFamiliarUpgradeElementalAttackButton = new UIButton("", [this]() { onBuyFamiliarElementalAttackUpgradeClicked(); });
	buyFamiliarCooldownReductionButton = new UIButton("", [this]() { onBuyFamiliarCooldownReductionClicked(); });
	updateFamiliarPhysicalAttackButtonLabel();
	updateFamiliarElementalAttackButtonLabel();
	updateFamiliarCooldownButtonLabel();

	if (familiar->isUnlocked())
	{
		buyFamiliarUpgradePhysicalAttackButton->setEnabled(familiar->isUnlocked());
		buyFamiliarUpgradeElementalAttackButton->setEnabled(familiar->isUnlocked());
		buyFamiliarCooldownReductionButton->setEnabled(familiar->isUnlocked());
	}
}

ShopTab::~ShopTab()
{
	delete buyAttackButton;
	delete buyGoldMultiplierButton;
	delete buyCritChanceButton;
	delete buyCritDamageButton;
	delete buyFamiliarButton;
	delete buyFamiliarUpgradePhysicalAttackButton;
	delete buyFamiliarUpgradeElementalAttackButton;
	delete buyFamiliarCooldownReductionButton;
}

void ShopTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	const float buttonHeight = 50.0f;
	const float margin = 10.0f;
	const float spacing = 10.0f;

	buyAttackButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyGoldMultiplierButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + buttonHeight + spacing },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyCritChanceButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 2 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyCritDamageButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 3 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyFamiliarButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 4 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyFamiliarUpgradePhysicalAttackButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 5 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyFamiliarUpgradeElementalAttackButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 6 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	buyFamiliarCooldownReductionButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 7 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	// total height of the button stack in its unscrolled layout (8 buttons), so TabbedPanel can enable scrolling if it overflows
	contentHeight = margin * 2.0f + 8 * (buttonHeight + spacing) - spacing;
}

void ShopTab::render(sf::RenderWindow* window)
{
	buyAttackButton->render(window);
	buyGoldMultiplierButton->render(window);
	buyCritChanceButton->render(window);
	buyCritDamageButton->render(window);
	buyFamiliarButton->render(window);
	if (familiar->isUnlocked())
	{
		buyFamiliarUpgradePhysicalAttackButton->render(window);
		buyFamiliarUpgradeElementalAttackButton->render(window);
		buyFamiliarCooldownReductionButton->render(window);
	}
}

bool ShopTab::handleClick(sf::Vector2f pos)
{
	if (buyAttackButton->handleClick(pos)) return true;
	if (buyGoldMultiplierButton->handleClick(pos)) return true;
	if (buyCritChanceButton->handleClick(pos)) return true;
	if (buyCritDamageButton->handleClick(pos)) return true;
	if (buyFamiliarButton->handleClick(pos)) return true;
	if (buyFamiliarUpgradePhysicalAttackButton->handleClick(pos)) return true;
	if (buyFamiliarUpgradeElementalAttackButton->handleClick(pos)) return true;
	if (buyFamiliarCooldownReductionButton->handleClick(pos)) return true;
	return false;
}

void ShopTab::updateCritChanceButtonLabel()
{
	if (!hero->isCritUnlocked())
	{
		buyCritChanceButton->setLabelText("Unlock Crit Chance (0.5%) (Cost: " + critChanceCost.toString() + ")");
	}
	else
	{
		std::ostringstream out;
		out << std::fixed << std::setprecision(2) << (hero->getCritChance() * 100.0 + critChanceIncreaseAmount * 100.0);
		buyCritChanceButton->setLabelText("Crit Chance " + out.str() + "% (Cost: " + critChanceCost.toString() + ")");
	}
}

void ShopTab::onBuyCritChanceClicked()
{
	if (hero->trySpendCurrency(critChanceCost))
	{
		if (!hero->isCritUnlocked())
		{
			hero->unlockCriticalHits(critChanceUnlockValue);
		}
		else
		{
			hero->increaseCritChance(critChanceIncreaseAmount);
		}
		critChanceCost = critChanceCost + critChanceCost;
		statistics->recordShopUpgradePurchased();
		updateCritChanceButtonLabel();
	}
}

void ShopTab::updateCritDamageButtonLabel()
{
	std::ostringstream out;
	out << std::fixed << std::setprecision(0) << ((hero->getCritDamageMultiplier() + critDamageIncreaseAmount) * 100.0);
	buyCritDamageButton->setLabelText("Crit Damage " + out.str() + "% (Cost: " + critDamageCost.toString() + ")");
}

void ShopTab::onBuyCritDamageClicked()
{
	if (hero->trySpendCurrency(critDamageCost))
	{
		hero->increaseCritDamage(critDamageIncreaseAmount);
		critDamageCost = critDamageCost + critDamageCost;
		statistics->recordShopUpgradePurchased();
		updateCritDamageButtonLabel();
	}
}

BigNumber ShopTab::getAttackUpgradeCost() const
{
	return attackUpgradeCost;
}

BigNumber ShopTab::getGoldMultiplierCost() const
{
	return goldMultiplierCost;
}

BigNumber ShopTab::getCritChanceCost() const
{
	return critChanceCost;
}

BigNumber ShopTab::getCritDamageCost() const
{
	return critDamageCost;
}

BigNumber ShopTab::getFamiliarPhysicalAttackCost() const
{
	return familiarPhysicalAttackUpgradeCost;
}

BigNumber ShopTab::getFamiliarElementalAttackCost() const
{
	return familiarElementalAttackUpgradeCost;
}

BigNumber ShopTab::getFamiliarCooldownReductionCost() const
{
	return familiarCooldownReductionCost;
}

void ShopTab::setAttackUpgradeCost(BigNumber cost)
{
	attackUpgradeCost = cost;
}

void ShopTab::setGoldMultiplierCost(BigNumber cost)
{
	goldMultiplierCost = cost;
}

void ShopTab::setCritChanceCost(BigNumber cost)
{
	critChanceCost = cost;
}

void ShopTab::setCritDamageCost(BigNumber cost)
{
	critDamageCost = cost;
}

void ShopTab::setFamiliarPhysicalAttackCost(BigNumber cost)
{
	familiarPhysicalAttackUpgradeCost = cost;
}

void ShopTab::setFamiliarElementalAttackCost(BigNumber cost)
{
	familiarElementalAttackUpgradeCost = cost;
}

void ShopTab::setFamiliarCooldownReductionCost(BigNumber cost)
{
	familiarCooldownReductionCost = cost;
}

void ShopTab::setFamiliarButtonActive(bool enable)
{
	buyFamiliarButton->setEnabled(enable);
}

void ShopTab::setFamiliarButtonsActive(bool enable)
{
	buyFamiliarUpgradePhysicalAttackButton->setEnabled(enable);
	buyFamiliarUpgradeElementalAttackButton->setEnabled(enable);
	buyFamiliarCooldownReductionButton->setEnabled(enable);
}

void ShopTab::setFamiliarCooldownButtonActive(bool enable)
{
	buyFamiliarCooldownReductionButton->setEnabled(enable);
}

void ShopTab::refreshLabels()
{
	updateButtonLabel();
	updateGoldMultiplierButtonLabel();
	updateCritChanceButtonLabel();
	updateCritDamageButtonLabel();
	updateFamiliarButtonLabel();
	updateFamiliarPhysicalAttackButtonLabel();
	updateFamiliarElementalAttackButtonLabel();
	updateFamiliarCooldownButtonLabel();

	if (familiar->isAttackIntervalAtMinimum())
	{
		buyFamiliarCooldownReductionButton->setEnabled(false);
	}
}

void ShopTab::resetPricesToDefault()
{
	attackUpgradeCost = BigNumber(50); // matches the member initializers in ShopTab.h
	goldMultiplierCost = BigNumber(30);
	critChanceCost = BigNumber(200);
	critDamageCost = BigNumber(150);
	familiarPhysicalAttackUpgradeCost = BigNumber(6500);
	familiarElementalAttackUpgradeCost = BigNumber(8000);
	familiarCooldownReductionCost = BigNumber(10000);
	refreshLabels();
}