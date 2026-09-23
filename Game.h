#pragma once

#include "PublicVariables.h"
#include "stdafx.h"
#include "Times.h"
#include "Stage.h"
#include "Hero.h"
#include "EnemySpawner.h"
#include "ItemBar.h"
#include "TabbedPanel.h"
#include "StatisticsTab.h"
#include "IWindowController.h"
#include "AchievementManager.h"
#include "ItemManager.h"
#include "ISaveable.h"
#include "IDebugController.h"
#include "SaveIndicator.h"
#include "OptionsTab.h"
#include "ShopTab.h"
#include "FamiliarTab.h"
#include "Familiar.h"
#include "FamiliarType.h"
#include "DebugTab.h"
#include "AchievementsTab.h"
#include "UnlockManager.h"


class Game : public IWindowController, public ISaveable, public IDebugController
{
private:
	// List of all available familiar types, for later use in the ShopTab and FamiliarTab
	std::vector<FamiliarType> familiarTypeList =
	{
		// Baby Fire Dragon: a single ranged auto-attack mixing physical + fire damage into one fireball
		FamiliarType(BabyFireDragon, "Baby Fire Dragon", "Fire", Age::Baby,
			"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_idle.png", 0.15f, {96, 96},
			{
				FamiliarAttack{
					"Fireball",                                                                                              // name
					"attack1",                                                                                               // animationKey
					"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_attack1.png",                       // animationAddress
					0.10f,                                                                                                   // animationFrameTimerMax
					3.0f,                                                                                                    // baseCooldown
					0.5f,                                                                                                    // minCooldownLimit
					true,                                                                                                    // isAutoAttack
					true,                                                                                                    // usesPhysicalDamage
					BigNumber(5),                                                                                            // basePhysicalPower
					true,                                                                                                    // usesElementalDamage
					ElementalPower().set(Fire, BigNumber(3)),                                                                // baseElementalPower
					true,                                                                                                    // isRanged
					"Textures/Familiars/Dragons/sprites/effect/fire_bolt/red/fire_bolt_small/fire_bolt_small.png",           // projectileAnimationAddress
					{60, 60},                                                                                                // projectileSpriteSize
					0.10f,                                                                                                   // projectileFrameTimerMax
					6,                                                                                                       // projectileFrameCount - sheet is 360x60px = 6 frames of 60x60
					2.5f,                                                                                                    // projectileScale
					1.8f                                                                                                     // projectileTravelDuration
				}
			}),

			// Fire Dragon: melee physical-only auto-attack (claws, no fireball) + a manually-activated big fireball, pure fire damage
			FamiliarType(FireDragon, "Fire Dragon", "Fire", Age::Juvenile,
				"Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_idle.png", 0.15f, {240, 224},
				{
					FamiliarAttack{
						"Claw Swipe",                                                                                            // name
						"attack1",                                                                                               // animationKey
						"Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_attack1.png",                       // animationAddress
						0.10f,                                                                                                   // animationFrameTimerMax
						7.0f,                                                                                                    // baseCooldown
						1.0f,                                                                                                    // minCooldownLimit
						true,                                                                                                    // isAutoAttack
						true,                                                                                                    // usesPhysicalDamage
						BigNumber(1500),                                                                                          // basePhysicalPower - higher than the baby stage's, since this stage's base stats are stronger
						false,                                                                                                   // usesElementalDamage
						ElementalPower(),                                                                                       // baseElementalPower - unused, claws carry no element
						false                                                                                                    // isRanged - melee, so every projectile field below stays at its default
					},
					FamiliarAttack{
						"Fireball",                                                                                              // name
						"attack2",                                                                                               // animationKey
						"Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_attack2.png",                       // animationAddress
						1.0f,                                                                                                    // animationFrameTimerMax
						10.0f,                                                                                                   // baseCooldown
						0.5f,                                                                                                    // minCooldownLimit - unused, bonus cooldown reduction never applies to a manual ability
						false,                                                                                                   // isAutoAttack - manually activated
						false,                                                                                                   // usesPhysicalDamage
						BigNumber(0),                                                                                            // basePhysicalPower - unused
						true,                                                                                                    // usesElementalDamage
						ElementalPower().set(Fire, BigNumber(3)),                                                                // baseElementalPower
						true,                                                                                                    // isRanged
						"Textures/Familiars/Dragons/sprites/effect/fire_bolt/red/fire_bolt_small/fire_bolt_small.png",           // projectileAnimationAddress
						{60, 60},                                                                                                // projectileSpriteSize
						0.10f,                                                                                                   // projectileFrameTimerMax
						6,                                                                                                       // projectileFrameCount
						5.0f,                                                                                                    // projectileScale - bigger than the auto-attack fireball used to be, it's the "big fireball" now
						1.8f                                                                                                     // projectileTravelDuration
					}
				}),

			// add more familiars here later
	};

	std::string title;
	sf::VideoMode vm;
	sf::RenderWindow* window;
	sf::View gameView;

	unsigned int frameRate;

	Stage* stage;
	Times times;
	Hero* hero;
	EnemySpawner* enemySpawner;
	ItemBar* itemBar;
	TabbedPanel* tabbedPanel;
	Statistics* statistics;
	AchievementManager* achievementManager;
	ItemManager* itemManager;

	OptionsTab* optionsTab = nullptr; // non-owning - owned by tabbedPanel, kept here so saveToFile() can update the load button
	ShopTab* shopTab = nullptr; // non-owning - owned by tabbedPanel, kept here so save/load can persist shop prices
	Familiar* familiar = nullptr; // owned by Game (unlike shopTab/optionsTab, not a TabContent)
	DebugTab* debugTab = nullptr;
	AchievementsTab* achievementsTab = nullptr; // non-owning - owned by tabbedPanel, kept here to wire the unlock->blink callback
	FamiliarTab* familiarTab = nullptr; // non-owning
	UnlockManager* unlockManager;

	// Autosave / save indicator
	sf::Font saveIndicatorFont;
	SaveIndicator* saveIndicator;
	static const std::string saveFilePath;

	// IWindowController implementation
	std::vector<sf::Vector2u> availableResolutions;
	WindowMode currentWindowMode = WindowMode::Windowed;
	int currentResolutionIndex = 0;

	void initWindow();
	void initAvailableResolutions();
	void initFrameRate();
	void initStage();
	void initHero();
	void initStatistics();
	void initEnemySpawner();
	void initItemManager();
	void initFamiliar(FamiliarName name);
	void initUI();
	void initAchievements();
	void initSaveIndicator();
	void initUnlockManager();

	void handleBossEncounterEnded(bool wasDefeated);

	void onWindowResized(sf::Vector2u newSize);
	void pollEvents();
	void update();
	void render();

	void refreshLayoutAfterWindowChange();
	void applyWindowSettings();

public:
	Game();
	virtual ~Game();

	void running();

	int getFrameRate() const;

	// IWindowController
	void setWindowMode(WindowMode mode) override;
	void setResolutionIndex(int index) override;
	WindowMode getWindowMode() const override;
	int getResolutionIndex() const override;
	const std::vector<sf::Vector2u>& getAvailableResolutions() const override;

	// IDebugController
	void debugSetHeroAttackPower(BigNumber value) override;
	void debugSetHeroCurrency(BigNumber value) override;
	void debugSetEnemyHealth(BigNumber value) override;
	void debugSetItemDropChance(float value) override;
	void debugSetHeroCritChance(float value) override;
	void debugSetHeroCritDamage(int percent) override;
	void debugResetHeroStats() override;
	void debugResetItems() override;
	void debugResetShopPrices() override;

	// saving and loading game state
	static std::string trim(const std::string& str);
	void saveToFile(const std::string& path) override;
	void loadFromFile(const std::string& path) override;

	void setDeathAnimationsEnabled(bool enabled);
};