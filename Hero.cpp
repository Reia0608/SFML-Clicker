#include "Hero.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include "Randomize.h"
#include <cmath>

void Hero::initCurrencyText()
{
	currencyText = new Text(currency.toString(), "Fonts/arial.ttf");
	currencyText->setFillColor(sf::Color::Yellow);
	currencyText->setFillColor(sf::Color::Yellow);
	currencyText->setOutlineColor(sf::Color::Black);
	currencyText->setOutlineThickness(2.0f);
	updateCurrencyText();
}

void Hero::updateCurrencyText()
{
	currencyText->setString("Gold: " + currency.toString());
	LayoutManager::applyAnchor(*currencyText, Anchor::BottomRight, { -10.0f, -10.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void Hero::triggerCurrencyPulse()
{
	currencyPulseElapsed = 0.0f;
	currencyPulseActive = true;
}

void Hero::initClickDamageText()
{
	clickDamageText = new Text("click damage: " + attackPower.toString(), "Fonts/arial.ttf");
	clickDamageText->setFillColor(sf::Color::Red);
	clickDamageText->setOutlineColor(sf::Color::Black);
	clickDamageText->setOutlineThickness(2.0f);
	updateClickDamageText();
}

void Hero::initDPSText()
{
	dpsText = new Text("DPS: 0", "Fonts/arial.ttf");
	dpsText->setFillColor(sf::Color::Cyan);
	dpsText->setOutlineColor(sf::Color::Black);
	dpsText->setOutlineThickness(2.0f);
	updateDPSText();
}

void Hero::updateDPSText()
{
	dpsText->setString("DPS: " + heroDPSTracker.getDPS().toString());
	LayoutManager::applyAnchor(*dpsText, Anchor::BottomRight, { -10.0f, -50.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void Hero::updateClickDamageText()
{
	clickDamageText->setString("Click Damage: " + attackPower.toString());
	LayoutManager::applyAnchor(*clickDamageText, Anchor::BottomRight, { -10.0f, -80.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

Hero::Hero()
{
	attackPower = BigNumber(2); // Initial attack power

	initSprites();
	initCurrencyText();
	initDPSText();
	initClickDamageText();
}

Hero::~Hero()
{
	delete sprites;
	delete currencyText;
	delete dpsText;
	delete clickDamageText;
}

void Hero::initSprites()
{
	//Sprites from https://xzany.itch.io/top-down-adventurer-character

	sprites = new Sprites();
	sprites->addAnimation("idle", "Textures/FREE_Adventurer 2D Pixel Art/Sprites/IDLE/idle_up.png", { 96, 80 }, 0.1f);
	sprites->addAnimation("attack1", "Textures/FREE_Adventurer 2D Pixel Art/Sprites/ATTACK 1/attack1_up.png", { 96, 80 }, 0.08f);
	sprites->addAnimation("attack2", "Textures/FREE_Adventurer 2D Pixel Art/Sprites/ATTACK 2/attack2_up.png", { 96, 80 }, 0.08f);
	sprites->setPosition({ gameAreaWidth / 2.0f, (windowSize.y / 4.0f) * 3.0f });
	sprites->playAnimation("idle");
}

void Hero::gainCurrency(BigNumber amount)
{
	BigNumber multipliedAmount = amount * BigNumber(goldMultiplier);
	currency = currency + multipliedAmount;
	updateCurrencyText();
	triggerCurrencyPulse();
}

void Hero::recordDamageDealt(BigNumber amount)
{
	heroDPSTracker.recordDamage(amount);
}

void Hero::increaseGoldMultiplier(double amount)
{
	goldMultiplier += amount;
}

void Hero::updateSprites(float dt)
{
	// need to look how it's done in the Enemy class
	this->sprites->animate(dt);
}

void Hero::updateCurrencyPulse(float dt)
{
	if (!currencyPulseActive) return;

	currencyPulseElapsed += dt;
	float t = currencyPulseElapsed / currencyPulseDuration;

	if (t >= 1.0f)
	{
		currencyText->setScale({ 1.0f, 1.0f });
		currencyPulseActive = false;
		return;
	}

	const float pi = 3.14159265f;
	float scale = 1.0f + (currencyPulseMaxScale - 1.0f) * std::sin(t * pi);
	currencyText->setScale({ scale, scale });
}

void Hero::onResize()
{
	sprites->setPosition({ gameAreaWidth / 2.0f, (windowSize.y / 3.0f) * 2.0f });
	updateCurrencyText();
	updateDPSText();
	updateClickDamageText();
}

void Hero::updateDPS(float dt)
{
	heroDPSTracker.update(dt);
	updateDPSText();
}

void Hero::draw(sf::RenderWindow* window)
{
	window->draw(*sprites);
	window->draw(*currencyText);
	window->draw(*dpsText);
	window->draw(*clickDamageText);
}

void Hero::playAttack1Animation()
{
	sprites->playAnimation("attack1", true, "idle");   // plays once then returns to the idle animation
}

void Hero::playAttack2Animation()
{
	sprites->playAnimation("attack2", true, "idle");   // plays once then returns to the idle animation
}

BigNumber Hero::getAttackPower() const
{ 
	return attackPower;
}

BigNumber Hero::getHeroDPS() const
{
	return heroDPSTracker.getDPS();
}

BigNumber Hero::getCurrency() const
{
	return currency;
}

double Hero::getGoldMultiplier() const
{
	return goldMultiplier;
}

bool Hero::trySpendCurrency(BigNumber amount)
{
	if (currency >= amount)
	{
		currency = currency - amount;
		updateCurrencyText();
		return true;
	}
	return false;
}

void Hero::increaseAttackPower(BigNumber amount)
{
	attackPower = attackPower + amount;
}

bool Hero::isCritUnlocked() const 
{ 
	return critUnlocked;
}

double Hero::getCritChance() const
{
	return critChance;
}

double Hero::getCritDamageMultiplier() const
{ 
	return critDamageMultiplier;
}

bool Hero::getHasAttack1played() const
{
	return hasAttack1played;
}

bool Hero::getHasAttack2played() const
{
	return hasAttack2played;
}

void Hero::setHasAttack1played(bool value)
{
	hasAttack1played = value;
}

void Hero::setHasAttack2played(bool value)
{
	hasAttack2played = value;
}

void Hero::unlockCriticalHits(double initialChance)
{
	critUnlocked = true;
	critChance = initialChance;
}

void Hero::increaseCritChance(double amount)
{
	critChance += amount;
}

void Hero::increaseCritDamage(double amount)
{
	critDamageMultiplier += amount;
}

BigNumber Hero::rollDamage(bool& outWasCritical)
{
	outWasCritical = false;

	if (critUnlocked)
	{
		float roll = Randomize::returnRandomizeFloat(0.0f, 1.0f);
		if (roll <= static_cast<float>(critChance))
		{
			outWasCritical = true;
		}
	}

	if (outWasCritical)
	{
		return attackPower * BigNumber(critDamageMultiplier);
	}
	return attackPower;
}

void Hero::grantBonusGold(BigNumber amount)
{
	currency = currency + amount;
	updateCurrencyText();
	triggerCurrencyPulse();
}

void Hero::setAttackPower(BigNumber value)
{
	attackPower = value;
}

void Hero::setCurrency(BigNumber value)
{
	currency = value;
	updateCurrencyText();
}

void Hero::setCritChance(double value)
{
	critChance = value;
	if (value > 0.0)
	{
		critUnlocked = true;
	}
}

void Hero::setCritDamageMultiplier(double value)
{
	critDamageMultiplier = value;
}

void Hero::resetToDefaults() // mirrors the member initializers in Hero.h / the constructor
{
	attackPower = BigNumber(2);
	currency = BigNumber(0);
	goldMultiplier = 1.0;
	critUnlocked = false;
	critChance = 0.0;
	critDamageMultiplier = 1.5;
	updateCurrencyText();
}
