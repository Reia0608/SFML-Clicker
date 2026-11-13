#include "Hero.h"
#include "PublicVariables.h"
#include "Randomize.h"

Hero::Hero()
{
	attackPower = BigNumber(2); // Initial attack power

	initSprites();
}

Hero::~Hero()
{
	delete sprites;
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
	if (onCurrencyChanged) onCurrencyChanged(currency, true);
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

void Hero::onResize()
{
	sprites->setPosition({ gameAreaWidth / 2.0f, (windowSize.y / 3.0f) * 2.0f });
}

void Hero::updateDPS(float dt)
{
	heroDPSTracker.update(dt);
	if (onDPSChanged) onDPSChanged(heroDPSTracker.getDPS());
}

void Hero::draw(sf::RenderWindow* window)
{
	window->draw(*sprites);
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
		if (onCurrencyChanged) onCurrencyChanged(currency, false);
		return true;
	}
	return false;
}

void Hero::increaseAttackPower(BigNumber amount)
{
	attackPower = attackPower + amount;
	if (onAttackPowerChanged) onAttackPowerChanged(attackPower);
}

void Hero::setGoldMultiplier(double value)
{
	goldMultiplier = value;
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

BigNumber Hero::rollDamage(bool& outWasCritical, double externalCritChanceBonus, double externalCritDamageBonus)
{
	outWasCritical = false;

	if (critUnlocked)
	{
		float roll = Randomize::returnRandomizeFloat(0.0f, 1.0f);
		if (roll <= static_cast<float>(critChance + externalCritChanceBonus))
		{
			outWasCritical = true;
		}
	}

	if (outWasCritical)
	{
		return attackPower * BigNumber(critDamageMultiplier + externalCritDamageBonus);
	}
	return attackPower;
}

void Hero::grantBonusGold(BigNumber amount)
{
	currency = currency + amount;
	if (onCurrencyChanged) onCurrencyChanged(currency, true);
}

bool Hero::isAutoAttackActive() const
{
	return autoAttack.isActive();
}

void Hero::setAutoAttackActive(bool active)
{
	autoAttack.setActive(active);
}

void Hero::toggleAutoAttack()
{
	autoAttack.toggleActive();
}

bool Hero::tickAutoAttack(float dt)
{
	return autoAttack.update(dt);
}

float Hero::getAutoAttackInterval() const
{
	return autoAttack.getInterval();
}

void Hero::setAutoAttackInterval(float seconds)
{
	autoAttack.setInterval(seconds);
}

void Hero::setAttackPower(BigNumber value)
{
	attackPower = value;
}

void Hero::setCurrency(BigNumber value)
{
	currency = value;
	if (onCurrencyChanged) onCurrencyChanged(currency, false);
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
	autoAttack.setActive(false); // same spirit as critUnlocked above - resets this Hero's OWN on/off state, not whether UnlockManager's "HeroAutoAttackUnlocked" key is set
	if (onCurrencyChanged) onCurrencyChanged(currency, false); // NOTE: does NOT fire onAttackPowerChanged even though attackPower resets too - pre-existing gap, matches setAttackPower()'s comment above (flagged, not fixed)
}

Hero::State Hero::captureState() const
{
	State state;
	state.attackPower = attackPower;
	state.currency = currency;
	state.goldMultiplier = goldMultiplier;
	state.critUnlocked = critUnlocked;
	state.critChance = critChance;
	state.critDamageMultiplier = critDamageMultiplier;
	state.hasAttack1played = hasAttack1played;
	state.hasAttack2played = hasAttack2played;
	state.dpsTrackerState = heroDPSTracker.captureState();
	state.autoAttackState = autoAttack.captureState();
	state.spritesState = sprites->captureState();
	return state;
}

void Hero::restoreState(const State& state)
{
	attackPower = state.attackPower;
	currency = state.currency;
	goldMultiplier = state.goldMultiplier;
	critUnlocked = state.critUnlocked;
	critChance = state.critChance;
	critDamageMultiplier = state.critDamageMultiplier;
	hasAttack1played = state.hasAttack1played;
	hasAttack2played = state.hasAttack2played;
	heroDPSTracker.restoreState(state.dpsTrackerState);
	autoAttack.restoreState(state.autoAttackState);
	sprites->restoreState(state.spritesState);

	if (onCurrencyChanged) onCurrencyChanged(currency, false);
	if (onAttackPowerChanged) onAttackPowerChanged(attackPower);
	if (onDPSChanged) onDPSChanged(heroDPSTracker.getDPS());
}
