#include "stdafx.h"
#include "ShopTab.h"
#include <sstream>
#include <iomanip>

void ShopTab::relayoutButtons() // the core of the locking system
{
	const float buttonHeight = 50.0f;
	const float margin = 10.0f;
	const float spacing = 10.0f;

	float y = lastContentAreaOrigin.y + margin;
	for (auto& entry : buttonEntries)
	{
		bool visible = entry.isVisible();
		entry.button->setEnabled(visible); // UIButton already skips render()/handleClick() while disabled

		if (!visible) continue; // locked/hidden buttons reserve no space � this is the fix

		entry.button->setLayout(
			{ lastContentAreaOrigin.x + margin, y },
			{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
		);
		y += buttonHeight + spacing;
	}

	contentHeight = (y - lastContentAreaOrigin.y) - spacing + margin;
}

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
		goldMultiplierUpgradesPurchased++;
		goldMultiplierCost = goldMultiplierCost * BigNumber(1.5); // increase the cost of the next upgrade
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

	relayoutButtons();
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
	buyFamiliarCooldownReductionButton->setLabelText("Familiar cooldown reduction by " + formatCooldownAmount(familiarCooldownReductionAmount) + " seconds (Cost: " + familiarCooldownReductionCost.toString() + ")");
}

void ShopTab::onBuyFamiliarCooldownReductionClicked()
{
	if (hero->trySpendCurrency(familiarCooldownReductionCost))
	{
		familiar->increaseCooldownReduction(familiarCooldownReductionAmount);
		statistics->recordShopUpgradePurchased();
		familiarCooldownReductionCost = familiarCooldownReductionCost * BigNumber(1.5);
		updateFamiliarCooldownButtonLabel();
		familiar->updateFamiliarDPSText();

		relayoutButtons();
	}
}

std::string ShopTab::formatCooldownAmount(double amount)
{
	std::ostringstream oss;
	oss << std::setprecision(3) << familiarCooldownReductionAmount;
	std::string amountStr = oss.str();
	return amountStr;
}

ShopTab::ShopTab(Hero* hero, Statistics* statistics, Familiar* familiar, UnlockManager* unlockManager)
	: hero(hero), statistics(statistics), familiar(familiar), unlockManager(unlockManager)
{
	buyAttackButton = new UIButton("", [this]() { onBuyAttackClicked(); });
	buyGoldMultiplierButton = new UIButton("", [this]() { onBuyGoldMultiplierClicked(); });
	buyCritChanceButton = new UIButton("", [this]() { onBuyCritChanceClicked(); });
	buyCritDamageButton = new UIButton("", [this]() { onBuyCritDamageClicked(); });
	buyFamiliarButton = new UIButton("", [this]() { onBuyFamiliarClicked(); });
	buyFamiliarUpgradePhysicalAttackButton = new UIButton("", [this]() { onBuyFamiliarPhysicalAttackUpgradeClicked(); });
	buyFamiliarUpgradeElementalAttackButton = new UIButton("", [this]() { onBuyFamiliarElementalAttackUpgradeClicked(); });
	buyFamiliarCooldownReductionButton = new UIButton("", [this]() { onBuyFamiliarCooldownReductionClicked(); });

	updateButtonLabel();
	updateGoldMultiplierButtonLabel();
	updateCritChanceButtonLabel();
	updateCritDamageButtonLabel();
	updateFamiliarButtonLabel();
	updateFamiliarPhysicalAttackButtonLabel();
	updateFamiliarElementalAttackButtonLabel();
	updateFamiliarCooldownButtonLabel();

	buttonEntries = {
		{ buyAttackButton,                        [this]() { return true; } },
		{ buyGoldMultiplierButton,                [this]() { return true; } },
		{ buyCritChanceButton,					  [this]() { return true; } },
		{ buyCritDamageButton,                    [this]() { return this->unlockManager->isUnlocked("CritDamageUnlocked"); } },
		{ buyFamiliarButton,                      [this]() { return !this->familiar->isUnlocked(); } },
		{ buyFamiliarUpgradePhysicalAttackButton, [this]() { return this->familiar->isUnlocked(); } },
		{ buyFamiliarUpgradeElementalAttackButton,[this]() { return this->familiar->isUnlocked(); } },
		{ buyFamiliarCooldownReductionButton,     [this]() { return this->familiar->isUnlocked() && !this->familiar->isAutoAttackCooldownAtMinimum(); } },
		// Example: a future feature-gated upgrade, hidden until "SomeAchievement" is claimed:
		// { buyNewUpgradeButton, [this]() { return unlockManager->isUnlocked("SomeFeatureKey"); } },
	};
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
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;
	relayoutButtons();
}

void ShopTab::render(sf::RenderWindow* window)
{
	for (auto& entry : buttonEntries)
	{
		entry.button->render(window); // internally a no-op while disabled
	}
}

bool ShopTab::handleClick(sf::Vector2f pos)
{
	for (auto& entry : buttonEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
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
			unlockManager->unlock("CritDamageUnlocked");
			relayoutButtons();
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

void ShopTab::increaseGoldMultiplierGain(float amount)
{
	goldMultiplierIncreaseAmount *= amount; // 0.5 -> 1.0, permanently affects every future purchase

	const double baseMultiplier = 1.0; // Hero's starting multiplier before any purchases
	double rebalanced = baseMultiplier + goldMultiplierUpgradesPurchased * goldMultiplierIncreaseAmount;
	hero->setGoldMultiplier(rebalanced);

	updateGoldMultiplierButtonLabel(); // label now previews the doubled increment too
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

double ShopTab::getGoldMultiplierIncreaseAmount() const
{
	return goldMultiplierIncreaseAmount;
}

int ShopTab::getGoldMultiplierUpgradesPurchased() const
{
	return goldMultiplierUpgradesPurchased;
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

void ShopTab::setGoldMultiplierIncreaseAmount(double value)
{
	goldMultiplierIncreaseAmount = value;
}

void ShopTab::setGoldMultiplierUpgradesPurchased(int count)
{
	goldMultiplierUpgradesPurchased = count;
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

	relayoutButtons();
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

void ShopTab::refreshVisibility() // public entry point for external callers (e.g. Game::loadFromFile)
{
	relayoutButtons();
}