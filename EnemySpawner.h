#pragma once
#include "stdafx.h"
#include "Enemy.h"
#include "EnemyType.h"
#include "Statistics.h"
#include "ItemManager.h"

class EnemySpawner // more like an EnemyController, because it handles the spawning and scaling of enemies, as well as the transition between regular enemies and bosses
{
	private:
		// List of enemy types that can be spawned
		// Sprites from https://free-game-assets.itch.io/free-slime-mobs-pixel-art-top-down-sprite-pack
		// and: https://free-game-assets.itch.io/pixel-art-slime-enemies-top-down-sprite-pack
			std::vector<EnemyType> enemyTypeList = {
		EnemyType("Textures/PNG/Slime1/Idle/Slime1_Idle_Front_full.png", {64, 64}, BigNumber(10), 0.15f, "Textures/PNG/Slime1/Hurt/Slime1_Hurt_full.png", {64, 64}, 0.15f,
			"Textures/PNG/Slime1/Death/Slime1_Death_Front_full.png", {64, 64}, 0.15f, BigNumber(5)), //0
		EnemyType("Textures/PNG/Slime2/Idle/Slime2_Idle_Front_full.png", {64, 64}, BigNumber(15), 0.12f, "Textures/PNG/Slime2/Hurt/Slime2_Hurt_full.png", {64, 64}, 0.12f,
			"Textures/PNG/Slime2/Death/Slime2_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(6)), //1
		EnemyType("Textures/PNG/Slime3/Idle/Slime3_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime3/Hurt/Slime3_Hurt_full.png", {64, 64}, 0.12f,
			"Textures/PNG/Slime3/Death/Slime3_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(7)), //2
		EnemyType("Textures/PNG/Slime4/Idle/Slime4_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime4/Hurt/Slime4_Hurt_full.png", {64, 64}, 0.12f,
		"Textures/PNG/Slime4/Death/Slime4_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(8)), //3
		EnemyType("Textures/PNG/Slime5/Idle/Slime5_Idle_Front_full.png", {64, 64}, BigNumber(20), 0.12f, "Textures/PNG/Slime5/Hurt/Slime5_Hurt_full.png", {64, 64}, 0.12f,
			"Textures/PNG/Slime5/Death/Slime5_Death_Front_full.png", {64, 64}, 0.12f, BigNumber(9)), //4
			};
		EnemyType bossType = EnemyType(
			"Textures/PNG/Slime6/Idle/Slime6_Idle_Front_full.png", { 64, 64 }, BigNumber(10), 0.15f,
			"Textures/PNG/Slime6/Hurt/Slime6_Hurt_full.png", { 64, 64 }, 0.15f,
			"Textures/PNG/Slime6/Death/Slime6_Death_Front_full.png", { 64, 64 }, 0.15f, BigNumber(100)
		);

			Enemy* enemy;
			Statistics* statistics; // non-owning
			ItemManager* itemManager;   // non-owning

			unsigned int currentEnemyIndex = 0;
			int enemyIndexInStage = 0;
			int currentStageNumber = 1;

			std::function<void(bool wasDefeated)> onBossEncounterEndedCallback;
		
			BigNumber computeScaledHealth(BigNumber baseHealth, int exponent) const;
			BigNumber computeScaledGold(BigNumber baseGold, int exponent) const;
			void spawnNextEncounter();
			void spawnRegularEnemy();
			void spawnBoss();

			BigNumber finishEnemyDefeat();

	public:
		EnemySpawner(Statistics* statistics, ItemManager* itemManager);
		virtual ~EnemySpawner();

		void onResize();
		void render(sf::RenderWindow* window);
		
		bool handleClick(BigNumber damage, bool isCritical);
		bool getIsDead() const;
		bool getIsDeathAnimationFinished() const;

		bool hasEnemy() const { return enemy != nullptr; } // whether an enemy (alive or mid-death-animation) is currently on the field
		float getEnemyGroundY() const { return enemy ? enemy->getSprites()->getGroundY() : 0.0f; } // Y of the enemy's feet on the ground - Stage uses this to depth-sort it against decorations

		BigNumber getEnemyGold() const;
		sf::Vector2f getEnemyPosition() const; // used by Familiar to aim its projectiles
		void setEnemyHealth(BigNumber value); // debug only, needs Enemy::setHealth()
		BigNumber update(float dt);

		int getCurrentStageNumber() const;
		void setCurrentStageNumber(int newStageNumber); // restores stage progress after loading; respawns the current encounter scaled to the new stage

		void setOnBossEncounterEndedCallback(std::function<void(bool wasDefeated)> callback);
};
