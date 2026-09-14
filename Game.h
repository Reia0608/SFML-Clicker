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
#include "Familiar.h"
#include "FamiliarType.h"
#include "DebugTab.h"
#include "AchievementsTab.h"


class Game : public IWindowController, public ISaveable, public IDebugController
{
private:
	// List of all available familiar types, for later use in the ShopTab and FamiliarTab
	std::vector<FamiliarType> familiarTypeList =
	{
		FamiliarType(BabyFireDragon, // 0
		"Baby Fire Dragon",
		"Fire",
		"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_idle.png",
		0.15f,
		"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_attack1.png",
		0.10f,
		{96, 96},
		"Textures/Familiars/Dragons/sprites/effect/fire_bolt/red/fire_bolt_small/fire_bolt_small.png",
		0.10f,
		{60, 60},
		6,     // sheet is 360x60px = 6 frames of 60x60
		2.5f,  // scale - tweak this to make the fireball bigger/smaller on screen
		1.8f), // travelDuration in seconds - tweak this to make it fly slower/faster
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

	void handleBossEncounterEnded(bool wasDefeated);

	void setFamiliarButtonsActive(bool enable);

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