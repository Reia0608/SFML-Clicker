#include "EnemySpawner.h"
#include "PublicVariables.h"
#include "Randomize.h"
#include <cmath>
#include <optional>


BigNumber EnemySpawner::computeScaledHealth(BigNumber baseHealth, int exponent) const
{
	double rawValue = baseHealth.toApproxDouble() * std::pow(enemyHealthGrowthRate, exponent);
	double rounded = std::round(rawValue);
	return BigNumber(rounded);
}

BigNumber EnemySpawner::computeScaledGold(BigNumber baseGold, int exponent) const
{
	double rawValue = baseGold.toApproxDouble() * std::pow(goldGrowthRate, exponent);
	double rounded = std::round(rawValue);
	return BigNumber(rounded);
}

void EnemySpawner::spawnRegularEnemy()
{
	int randomIndex = 0;
	do
	{
		randomIndex = Randomize::returnRandomizeInt(0, enemyTypeList.size() - 1);
	} while (currentEnemyIndex == randomIndex);
	currentEnemyIndex = randomIndex;

	EnemyType selectedEnemyType = enemyTypeList[randomIndex];
	int exponent = (currentStageNumber - 1) * enemiesPerStage + enemyIndexInStage;
	BigNumber scaledHealth = computeScaledHealth(selectedEnemyType.maxHealth, exponent);
	BigNumber scaledGold = computeScaledGold(selectedEnemyType.gold, exponent);

	enemy = new Enemy(
		selectedEnemyType.spriteAddress, selectedEnemyType.spriteSize, scaledHealth, selectedEnemyType.frameTimerMax,
		selectedEnemyType.hurtSpriteAddress, selectedEnemyType.hurtSpriteSize, selectedEnemyType.hurtFrameTimerMax,
		selectedEnemyType.deathSpriteAddress, selectedEnemyType.deathSpriteSize, selectedEnemyType.deathFrameTimerMax,
		scaledGold, false, selectedEnemyType.elementalAffinity
	);
	enemy->getSprites()->setPosition({ gameAreaWidth / 2.0f, 250.0f });
	enemy->refreshHealthBar();
}

void EnemySpawner::spawnBoss()
{
	int exponent = currentStageNumber * enemiesPerStage;
	BigNumber scaledHealth = computeScaledHealth(bossType.maxHealth, exponent) * BigNumber(bossHealthMultiplier);
	scaledHealth = BigNumber(std::round(scaledHealth.toApproxDouble()));

	BigNumber scaledGold = computeScaledGold(bossType.gold, exponent) * BigNumber(bossGoldMultiplier);
	scaledGold = BigNumber(std::round(scaledGold.toApproxDouble()));

	enemy = new Enemy(
		bossType.spriteAddress, bossType.spriteSize, scaledHealth, bossType.frameTimerMax,
		bossType.hurtSpriteAddress, bossType.hurtSpriteSize, bossType.hurtFrameTimerMax,
		bossType.deathSpriteAddress, bossType.deathSpriteSize, bossType.deathFrameTimerMax,
		scaledGold, true, bossType.elementalAffinity
	);
	enemy->getSprites()->setPosition({ gameAreaWidth / 2.0f, 200.0f });
	enemy->refreshHealthBar();
}

void EnemySpawner::spawnNextEncounter()
{
	if (enemyIndexInStage < enemiesPerStage)
	{
		spawnRegularEnemy();
	}
	else
	{
		spawnBoss();
	}
}

void EnemySpawner::finishEnemyDefeat()
{
	BigNumber goldEarned = enemy->gold;
	bool wasBoss = enemy->getIsBoss();
	sf::Vector2f dropPosition = enemy->getSprites()->getPosition(); // where the drop visuals should spawn from

	if (wasBoss)
	{
		statistics->recordBossDefeated();
		statistics->setHighestStageReached(currentStageNumber);

		std::optional<int> droppedItemIndex = itemManager->rollBossDrop();
		if (droppedItemIndex.has_value() && onItemDroppedCallback)
		{
			onItemDroppedCallback(droppedItemIndex.value(), dropPosition); // the actual inventory increment happens later, once LootDropManager's drop reaches the ItemBar (see ItemManager::addItem)
		}

		currentStageNumber++;
		enemyIndexInStage = 0;
		statistics->recordBossKillTime(enemy->getBossTimeElapsed());

		if (onBossEncounterEndedCallback)
		{
			std::cout << "[EnemySpawner] Boss defeated, firing callback" << std::endl; // DEBUG - remove once confirmed working
			onBossEncounterEndedCallback(true); // boss was defeated
		}
		else
		{
			std::cout << "[EnemySpawner] Boss defeated, but no callback is set!" << std::endl; // DEBUG - remove once confirmed working
		}
	}
	else
	{
		statistics->recordEnemyDefeated();
		enemyIndexInStage++;
	}

	if (goldEarned > BigNumber(0) && onGoldDroppedCallback)
	{
		onGoldDroppedCallback(goldEarned, dropPosition); // the actual currency gain happens later, once LootDropManager's drop reaches the Gold counter (see Hero::gainCurrency)
	}

	delete enemy;
	enemy = nullptr;
	spawnNextEncounter();
}

EnemySpawner::EnemySpawner(Statistics* statistics, ItemManager* itemManager)
	: statistics(statistics), itemManager(itemManager)
{
	spawnNextEncounter();
}

EnemySpawner::~EnemySpawner()
{
	delete enemy;
}

void EnemySpawner::onResize()
{
	if (enemy != nullptr)
	{
		enemy->getSprites()->setPosition({ gameAreaWidth / 2.0f, 250.0f });
		enemy->refreshHealthBar();
	}
}

void EnemySpawner::render(sf::RenderWindow* window)
{
	if (enemy != nullptr)
	{
		window->draw(*enemy->getSprites());
		enemy->drawDamageText(window);
		enemy->drawHealthBar(window);
		enemy->drawBossTimer(window); // has internal guard. Does nothing if the enemy is not a boss
	}
}

bool EnemySpawner::handleClick(BigNumber damage, bool isCritical, sf::Color baseColor, ElementMask elementMask)
{
	if (enemy != nullptr && !(enemy->getIsDeathAnimationStarted()))
	{
		enemy->takeDamage(damage, isCritical, baseColor, elementMask);
		return true;
	}
	return false;
}

bool EnemySpawner::getIsDead() const
{
	return enemy->isDead();
}

bool EnemySpawner::getIsDeathAnimationFinished() const
{
	return enemy->isDeathAnimationFinished();
}

BigNumber EnemySpawner::getEnemyGold() const
{
	return enemy->gold;
}

std::string EnemySpawner::getEnemyName() const
{
	return enemy != nullptr ? enemy->name : "";
}

int EnemySpawner::getEnemyIndexInStage() const
{
	return enemyIndexInStage;
}

bool EnemySpawner::getIsCurrentEnemyBoss() const
{
	return enemy != nullptr && enemy->getIsBoss();
}

double EnemySpawner::getEnemyElementalMultiplier(ElementMask attackElementMask) const
{
	if (enemy == nullptr) return 1.0;
	return enemy->getElementalAffinity().getMultiplierForMask(attackElementMask);
}

double EnemySpawner::getEnemyElementalMultiplierForElement(Elements element) const
{
	if (enemy == nullptr) return 1.0;
	return enemy->getElementalAffinity().getMultiplier(element);
}

sf::Vector2f EnemySpawner::getEnemyPosition() const
{
	if (enemy != nullptr)
	{
		return enemy->getSprites()->getPosition();
	}

	return { gameAreaWidth / 2.0f, 100.0f }; // same fallback used when (re)spawning an enemy
}

void EnemySpawner::setEnemyHealth(BigNumber value) // debug only
{
	if (enemy != nullptr)
	{
		enemy->setHealth(value);
	}
}

void EnemySpawner::update(float dt)
{
	if (enemy == nullptr)
	{
		spawnNextEncounter();
		return;
	}

	if (enemy->isShowingDamageText())
	{
		enemy->updateDamageText(dt);
	}

	if (enemy->getIsBoss() && !enemy->isDead() && !enemy->getIsDeathAnimationStarted())
	{
		enemy->updateBossTimer(dt);

		if (enemy->isBossTimeUp())   // if the boss is not defeated in time, the stage is repeated
		{
			delete enemy;
			enemy = nullptr;
			enemyIndexInStage = 0;   // back to the first regular enemy of the stage
			spawnNextEncounter();

			if (onBossEncounterEndedCallback)
			{
				std::cout << "[EnemySpawner] Boss time up, firing callback" << std::endl; // DEBUG - remove once confirmed working
				onBossEncounterEndedCallback(false); // boss was NOT defeated (timer ran out)
			}
			else
			{
				std::cout << "[EnemySpawner] Boss time up, but no callback is set!" << std::endl; // DEBUG - remove once confirmed working
			}

			return;
		}
	}

	if (enemy->isDead())
	{
		if (!deathAnimationsEnabled)
		{
			finishEnemyDefeat();
			return;
		}

		enemy->loadDeathAnimation();
		enemy->getSprites()->animate(dt);

		if (enemy->isDeathAnimationFinished())
		{
			finishEnemyDefeat();
			return;
		}

		return;
	}
	else
	{
		enemy->getSprites()->animate(dt);
	}
}

int EnemySpawner::getCurrentStageNumber() const
{
	return currentStageNumber;
}

void EnemySpawner::setCurrentStageNumber(int newStageNumber)
{
	currentStageNumber = newStageNumber;
	enemyIndexInStage = 0; // start at the first regular enemy of the restored stage
	delete enemy;
	enemy = nullptr;
	spawnNextEncounter(); // respawn with health/gold correctly scaled for the restored stage
}

void EnemySpawner::setOnBossEncounterEndedCallback(std::function<void(bool wasDefeated)> callback)
{
	onBossEncounterEndedCallback = std::move(callback);
}

void EnemySpawner::setOnGoldDroppedCallback(std::function<void(BigNumber amount, sf::Vector2f position)> callback)
{
	onGoldDroppedCallback = std::move(callback);
}

void EnemySpawner::setOnItemDroppedCallback(std::function<void(int itemIndex, sf::Vector2f position)> callback)
{
	onItemDroppedCallback = std::move(callback);
}