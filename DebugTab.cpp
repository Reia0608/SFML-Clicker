#include "stdafx.h"
#include "DebugTab.h"

DebugTab::DebugTab(IDebugController* debugController) : debugController(debugController)
{
	attackDamageInput = new TextInputField("1", true, false, true);
	goldInput = new TextInputField("10", true, false, true);
	enemyHealthInput = new TextInputField("10", true, false, true);
	dropChanceInput = new TextInputField("0.1");   // 0-1
	critChanceInput = new TextInputField("0.1");   // 0-1
	critDamageInput = new TextInputField("150", false); // whole percent, e.g. 150 = 150%

	setAttackDamage = new UIButton("Set Attack Damage", [this]() { onSetAttackDamageClicked(); });
	setGold = new UIButton("Set Gold", [this]() { onSetGoldClicked(); });
	setEnemyHealth = new UIButton("Set Enemy Health", [this]() { onSetEnemyHealthClicked(); });
	dropChance = new UIButton("Set Drop Chance", [this]() { onSetDropChanceClicked(); });
	critChance = new UIButton("Set Crit Chance", [this]() { onSetCritChanceClicked(); });
	critDamage = new UIButton("Set Crit Damage %", [this]() { onSetCritDamageClicked(); });
	resetStats = new UIButton("Reset Hero Stats", [this]() { onResetStatsClicked(); });
	resetItems = new UIButton("Reset Items", [this]() { onResetItemsClicked(); });
	resetPrices = new UIButton("Reset Shop Prices", [this]() { onResetPricesClicked(); });
	addAllItems = new UIButton("Add 10 to All Items", [this]() { onAddAllItemsClicked(); });
	removeAllItems = new UIButton("Remove 10 from All Items", [this]() { onRemoveAllItemsClicked(); });
}

DebugTab::~DebugTab()
{
	delete attackDamageInput;
	delete goldInput;
	delete enemyHealthInput;
	delete dropChanceInput;
	delete critChanceInput;
	delete critDamageInput;

	delete setAttackDamage;
	delete setGold;
	delete setEnemyHealth;
	delete dropChance;
	delete critChance;
	delete critDamage;
	delete resetStats;
	delete resetItems;
	delete resetPrices;
	delete addAllItems;
	delete removeAllItems;
}

void DebugTab::onSetAttackDamageClicked()
{
	debugController->debugSetHeroAttackPower(attackDamageInput->getValueAsBigNumber(BigNumber(1.0)));
}

void DebugTab::onSetGoldClicked()
{
	debugController->debugSetHeroCurrency(goldInput->getValueAsBigNumber(BigNumber(10.0)));
}

void DebugTab::onSetEnemyHealthClicked()
{
	debugController->debugSetEnemyHealth(enemyHealthInput->getValueAsBigNumber(BigNumber(10.0)));
}

void DebugTab::onSetDropChanceClicked()
{
	debugController->debugSetItemDropChance(static_cast<float>(dropChanceInput->getValueAsDouble(0.1)));
}

void DebugTab::onSetCritChanceClicked()
{
	debugController->debugSetHeroCritChance(static_cast<float>(critChanceInput->getValueAsDouble(0.1)));
}

void DebugTab::onSetCritDamageClicked()
{
	debugController->debugSetHeroCritDamage(static_cast<int>(critDamageInput->getValueAsDouble(150.0)));
}

void DebugTab::onResetStatsClicked()
{
	debugController->debugResetHeroStats();
}

void DebugTab::onResetItemsClicked()
{
	debugController->debugResetItems();
}

void DebugTab::onResetPricesClicked()
{
	debugController->debugResetShopPrices();
}

void DebugTab::onAddAllItemsClicked()
{
	debugController->debugAdjustAllItems(10);
}

void DebugTab::onRemoveAllItemsClicked()
{
	debugController->debugAdjustAllItems(-10);
}

void DebugTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	const float margin = 10.0f;
	const float rowHeight = 34.0f;
	const float spacing = 8.0f;
	const float fieldWidth = 90.0f;

	float fieldX = contentAreaOrigin.x + margin;
	float buttonX = fieldX + fieldWidth + spacing;
	float buttonWidth = contentAreaSize.x - margin * 2.0f - fieldWidth - spacing;

	TextInputField* fields[] = { attackDamageInput, goldInput, enemyHealthInput, dropChanceInput, critChanceInput, critDamageInput };
	UIButton* setButtons[] = { setAttackDamage, setGold, setEnemyHealth, dropChance, critChance, critDamage };

	for (int i = 0; i < 6; i++)
	{
		float y = contentAreaOrigin.y + margin + i * (rowHeight + spacing);
		fields[i]->setLayout({ fieldX, y }, { fieldWidth, rowHeight });
		setButtons[i]->setLayout({ buttonX, y }, { buttonWidth, rowHeight });
	}

	float resetRowY = contentAreaOrigin.y + margin + 6 * (rowHeight + spacing) + spacing;
	UIButton* resetButtons[] = { resetStats, resetItems, resetPrices, addAllItems, removeAllItems };
	for (int i = 0; i < 5; i++)
	{
		float y = resetRowY + i * (rowHeight + spacing);
		resetButtons[i]->setLayout({ contentAreaOrigin.x + margin, y }, { contentAreaSize.x - margin * 2.0f, rowHeight });
	}
}

void DebugTab::render(sf::RenderWindow* window)
{
	attackDamageInput->render(window);
	goldInput->render(window);
	enemyHealthInput->render(window);
	dropChanceInput->render(window);
	critChanceInput->render(window);
	critDamageInput->render(window);

	setAttackDamage->render(window);
	setGold->render(window);
	setEnemyHealth->render(window);
	dropChance->render(window);
	critChance->render(window);
	critDamage->render(window);
	resetStats->render(window);
	resetItems->render(window);
	resetPrices->render(window);
	addAllItems->render(window);
	removeAllItems->render(window);
}

bool DebugTab::handleClick(sf::Vector2f pos)
{
	// Call handleClick on every field (not just until one hits) - each field needs to know
	// about clicks that land elsewhere too, so it can defocus itself.
	bool hitField = false;
	TextInputField* fields[] = { attackDamageInput, goldInput, enemyHealthInput, dropChanceInput, critChanceInput, critDamageInput };
	for (auto* field : fields)
	{
		if (field->handleClick(pos))
		{
			hitField = true;
		}
	}
	if (hitField) return true;

	if (setAttackDamage->handleClick(pos)) return true;
	if (setGold->handleClick(pos)) return true;
	if (setEnemyHealth->handleClick(pos)) return true;
	if (dropChance->handleClick(pos)) return true;
	if (critChance->handleClick(pos)) return true;
	if (critDamage->handleClick(pos)) return true;
	if (resetStats->handleClick(pos)) return true;
	if (resetItems->handleClick(pos)) return true;
	if (resetPrices->handleClick(pos)) return true;
	if (addAllItems->handleClick(pos)) return true;
	if (removeAllItems->handleClick(pos)) return true;
	return false;
}

void DebugTab::handleTextEntered(char32_t unicode)
{
	attackDamageInput->handleTextEntered(unicode);
	goldInput->handleTextEntered(unicode);
	enemyHealthInput->handleTextEntered(unicode);
	dropChanceInput->handleTextEntered(unicode);
	critChanceInput->handleTextEntered(unicode);
	critDamageInput->handleTextEntered(unicode);
}

void DebugTab::handleBackspace()
{
	attackDamageInput->handleBackspace();
	goldInput->handleBackspace();
	enemyHealthInput->handleBackspace();
	dropChanceInput->handleBackspace();
	critChanceInput->handleBackspace();
	critDamageInput->handleBackspace();
}