#pragma once
#include "stdafx.h"
#include "Enemy.h"
#include "EnemyType.h"
#include "Statistics.h"
#include "ItemManager.h"
#include "Elements.h"

class EnemySpawner // more like an EnemyController, because it handles the spawning and scaling of enemies, as well as the transition between regular enemies and bosses
{
private:
	// List of enemy types that can be spawned
	// Sprites from https://free-game-assets.itch.io/free-slime-mobs-pixel-art-top-down-sprite-pack
	// and: https://free-game-assets.itch.io/pixel-art-slime-enemies-top-down-sprite-pack
	std::vector<EnemyType> enemyTypeList = {
EnemyType("Textures/PNG/Slime1/Idle/Slime1_Idle_Front_full.png", {64, 64}, BigNumber(10), 0.15f, "Textures/PNG/Slime1/Hurt/Slime1_Hurt_full.png", {64, 64}, 0.15f,
	"Textures/PNG/Slime1/Death/Slime1_Death_Front_full.png", {64, 64}, 0.15f, BigNumber(5))
	.setElementalAffinity(ElementalAffinity().setWeakness(Fire)).setName("Green Slime"), //0 - green slime, weak to fire
EnemyType("Textures/PNG/Slime2/Idle/Slime2_Idle_Front_full.png", {64, 64}, BigNumber(15), 0.12f, "Textures/PNG/Slime2/Hurt/Slime2_Hurt_full.png", {64, 64}, 0.12f,
	"Textures/PNG/Slime2/Death/Slime2_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(6))
	.setElementalAffinity(ElementalAffinity().setWeakness(Fire).setWeakness(Water)).setName("Undead Slime"), //1 - undead slime, weak to fire
EnemyType("Textures/PNG/Slime3/Idle/Slime3_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime3/Hurt/Slime3_Hurt_full.png", {64, 64}, 0.12f,
	"Textures/PNG/Slime3/Death/Slime3_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(7))
	.setElementalAffinity(ElementalAffinity().setWeakness(Water).setResistance(Fire)).setName("Lava Slime"), //2 - lava slime, resistant to fire
EnemyType("Textures/PNG/Slime4/Idle/Slime4_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime4/Hurt/Slime4_Hurt_full.png", {64, 64}, 0.12f,
"Textures/PNG/Slime4/Death/Slime4_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(8))
	.setElementalAffinity(ElementalAffinity().setWeakness(Fire).setResistance(Wind).setResistance(Water)).setName("Ice Slime"), //3 - ice slime, weak to fire
EnemyType("Textures/PNG/Slime5/Idle/Slime5_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime5/Hurt/Slime5_Hurt_full.png", {64, 64}, 0.12f,
	"Textures/PNG/Slime5/Death/Slime5_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(9)).setName("Electric Slime"), //4 - neutral, no elemental profile set yet
	};
	EnemyType bossType = EnemyType(
		"Textures/PNG/Slime6/Idle/Slime6_Idle_Front_full.png", { 64, 64 }, BigNumber(10), 0.15f,
		"Textures/PNG/Slime6/Hurt/Slime6_Hurt_full.png", { 64, 64 }, 0.15f,
		"Textures/PNG/Slime6/Death/Slime6_Death_Front_full.png", { 64, 64 }, 0.15f, BigNumber(100)
	).setElementalAffinity(ElementalAffinity().setWeakness(Water, 1.5).setResistance(Fire, 0.75)).setName("Slime King"); // example: bosses can have their own, differently-tuned profile

	Enemy* enemy;
	Statistics* statistics; // non-owning
	ItemManager* itemManager;   // non-owning

	unsigned int currentEnemyIndex = 0;
	int enemyIndexInStage = 0;
	int currentStageNumber = 1;

	std::function<void(bool wasDefeated)> onBossEncounterEndedCallback;
	std::function<void(BigNumber amount, sf::Vector2f position)> onGoldDroppedCallback;   // fired from finishEnemyDefeat() whenever an enemy leaves gold behind - Game wires this to LootDropManager::onGoldDropped
	std::function<void(int itemIndex, sf::Vector2f position)> onItemDroppedCallback;      // fired from finishEnemyDefeat() whenever a boss item roll succeeds - Game wires this to LootDropManager::onItemDropped

	BigNumber computeScaledHealth(BigNumber baseHealth, int exponent) const;
	BigNumber computeScaledGold(BigNumber baseGold, int exponent) const;
	void spawnNextEncounter();
	void spawnRegularEnemy();
	void spawnBoss();

	void finishEnemyDefeat(); // no longer returns the gold reward - it fires onGoldDroppedCallback/onItemDroppedCallback instead, so LootDropManager can animate the drop before the reward is actually granted

public:
	EnemySpawner(Statistics* statistics, ItemManager* itemManager);
	virtual ~EnemySpawner();

	void onResize();
	void render(sf::RenderWindow* window); // world layer: just the sprite, so it still depth-sorts correctly against decorations
	void drawHUD(sf::RenderWindow* window); // HUD layer: health bar, boss timer, floating damage numbers - always on top, never occluded by a decoration (see Enemy::drawHealthBar/drawBossTimer/drawDamageText)

	bool handleClick(BigNumber damage, bool isCritical, sf::Color baseColor = sf::Color::Red, ElementMask elementMask = 0); // baseColor identifies the damage's source (Hero/familiar/passive) in its floating number; defaults to Red (Hero's own color) so the Hero's own call site didn't need to change. elementMask, when non-zero, overrides baseColor with that element's gradient (see Enemy::showDamage) - defaults to 0 so the Hero's and passive-damage's call sites don't need to change either.
	bool getIsDead() const;
	bool getIsDeathAnimationFinished() const;

	bool hasEnemy() const { return enemy != nullptr; } // whether an enemy (alive or mid-death-animation) is currently on the field
	float getEnemyGroundY() const { return enemy ? enemy->getSprites()->getGroundY() : 0.0f; } // Y of the enemy's feet on the ground - Stage uses this to depth-sort it against decorations

	BigNumber getEnemyGold() const;
	std::string getEnemyName() const; // display name of the current enemy, empty if none is on the field
	int getEnemyIndexInStage() const; // 0-based index of the current regular enemy within the stage (out of enemiesPerStage); meaningless during a boss encounter
	bool getIsCurrentEnemyBoss() const; // true if the enemy currently on the field is the boss
	double getEnemyElementalMultiplier(ElementMask attackElementMask) const; // 1.0 if no enemy on field or attack is elementless. Kept for any other existing caller (e.g. a weakness/resistance UI indicator) - blends every element in the mask into one averaged multiplier.
	double getEnemyElementalMultiplierForElement(Elements element) const;    // 1.0 if no enemy on field. Resolves ONE element's own weakness/resistance exactly, with no averaging - used by Familiar so a multi-element attack scales each element correctly (see ElementalPower::computeDamage).
	sf::Vector2f getEnemyPosition() const; // used by Familiar to aim its projectiles
	void setEnemyHealth(BigNumber value); // debug only, needs Enemy::setHealth()
	void update(float dt); // no longer returns the gold reward - see finishEnemyDefeat()

	int getCurrentStageNumber() const;
	void setCurrentStageNumber(int newStageNumber); // restores stage progress after loading; respawns the current encounter scaled to the new stage

	void setOnBossEncounterEndedCallback(std::function<void(bool wasDefeated)> callback);
	void setOnGoldDroppedCallback(std::function<void(BigNumber amount, sf::Vector2f position)> callback);
	void setOnItemDroppedCallback(std::function<void(int itemIndex, sf::Vector2f position)> callback);

	// Everything about the current encounter that can change frame-to-frame - used by
	// frame-stepping (see FrameStepHistory). Best-effort only: restoreState() can bring back the
	// CURRENT enemy's own stats (health, boss timer, animation), but can't undo the enemy itself
	// dying and a new one spawning in between - that would mean destroying/recreating Enemy
	// objects, which this debugging feature intentionally doesn't attempt. In practice that only
	// matters if a single stepped frame happens to land exactly on a kill.
	struct State
	{
		bool hasEnemy = false;
		Enemy::State enemyState; // only meaningful if hasEnemy
		unsigned int currentEnemyIndex = 0;
		int enemyIndexInStage = 0;
		int currentStageNumber = 1;
	};

	State captureState() const;
	void restoreState(const State& state);
};