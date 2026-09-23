#include "stdafx.h"
#include "Game.h"
#include "Times.h"
#include "Stage.h"
#include "ShopTab.h"
#include "OptionsTab.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <array>

const std::string Game::saveFilePath = "Save/savegame.json";

void Game::initWindow()
{
	title = "SFML Clicker by Sami Ounis";
	vm = sf::VideoMode(sf::Vector2u(windowSize));
	window = new sf::RenderWindow(vm, title);
	window->setFramerateLimit(getFrameRate());

	window->setMinimumSize(sf::Vector2u(static_cast<unsigned int>(uiPanelWidth + 200.0f), 400u)); // window can not get smaller than UI panel + minimum game area size

	gameView = sf::View(sf::FloatRect({ 0.0f, 0.0f }, windowSize));
	window->setView(gameView);
}

void Game::initAvailableResolutions()
{
	availableResolutions = {
		{ 800, 600 },
		{ 1024, 768 },
		{ 1280, 720 },
		{ 1366, 768 },
		{ 1600, 900 },
		{ 1920, 1080 }
	};

	for (size_t i = 0; i < availableResolutions.size(); i++)
	{
		if (availableResolutions[i] == sf::Vector2u(windowSize))
		{
			currentResolutionIndex = static_cast<int>(i);
			break;
		}
	}
}

void Game::initFrameRate()
{
	frameRate = 60;
}

void Game::initStage()
{
	stage = new Stage();
}

void Game::initHero()
{
	hero = new Hero();
}

void Game::initStatistics()
{
	statistics = new Statistics();
}

void Game::initEnemySpawner()
{
	enemySpawner = new EnemySpawner(statistics, itemManager);
}

void Game::pollEvents()
{
	while (const std::optional event = window->pollEvent())
	{
		if (event->is<sf::Event::Closed>())
		{
			window->close();
		}
		else if (const auto* resized = event->getIf<sf::Event::Resized>())
		{
			onWindowResized(resized->size);
		}
		else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
		{
			if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
			{
				window->close();
			}
		}
		else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if (mousePressed->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f clickPos = window->mapPixelToCoords(mousePressed->position);
				if (clickPos.x < gameAreaWidth)
				{
					statistics->recordClick(); // records clicks only if the click was in the game area (not on the UI panel)
				}

				if (!tabbedPanel->handleClick(clickPos) && clickPos.x < gameAreaWidth)
				{
					bool wasCritical = false;
					BigNumber damage = hero->rollDamage(wasCritical);
					bool hitEnemy = enemySpawner->handleClick(damage, wasCritical);
					if (hitEnemy)
					{
						if (hero->getHasAttack2played())
						{
							hero->setHasAttack2played(false);
							hero->playAttack1Animation();
							hero->setHasAttack1played(true);
						}
						else if (hero->getHasAttack1played())
						{
							hero->setHasAttack1played(false);
							hero->playAttack2Animation();
							hero->setHasAttack2played(true);
						}
						else
						{
							hero->playAttack1Animation();
							hero->setHasAttack1played(true);
						}

						hero->recordDamageDealt(damage);
						if (wasCritical)
						{
							statistics->recordCriticalHit();
						}
					}
				}
			}
		}
		else if (const auto* wheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>())
		{
			if (wheelScrolled->wheel == sf::Mouse::Wheel::Vertical) // only the vertical wheel scrolls tab content
			{
				tabbedPanel->handleMouseWheelScrolling(window->mapPixelToCoords(wheelScrolled->position), wheelScrolled->delta);
			}
		}
		else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>())
		{
			tabbedPanel->handleTextEntered(textEntered->unicode);
		}
	}
}

void Game::initUI()
{
	itemBar = new ItemBar(itemManager);
	tabbedPanel = new TabbedPanel();
	shopTab = new ShopTab(hero, statistics, familiar, unlockManager);
	tabbedPanel->addTab("Shop", shopTab);
	familiarTab = new FamiliarTab(hero, itemManager, unlockManager);
	tabbedPanel->addTab("Familiars", familiarTab);

	familiarTab->onEvolveRedBabyDragon = [this]()
		{
			if (!familiar) return;
			familiar->evolveTo(familiarTypeList[1]); // FireDragon (Juvenile) - the evolution of BabyFireDragon

			// The new stage's auto-attack has its own baseCooldown/minCooldownLimit, so the
			// familiar's carried-over bonusCooldownReduction may no longer be enough to have hit
			// the new floor even if it had hit the old one - re-check visibility so the cooldown
			// reduction button reappears if there's room to buy more.
			if (shopTab) shopTab->refreshVisibility();
		};

	achievementsTab = new AchievementsTab(achievementManager);
	tabbedPanel->addTab("Achievements", achievementsTab);
	achievementManager->onAchievementUnlocked = [this](const Achievement& achievement)
		{
			achievementsTab->markPending();
		};

	achievementManager->onAchievementClaimed = [this](const Achievement& achievement) // NEW
		{
			if (achievement.name == "Hoarder")
			{
				shopTab->increaseGoldMultiplierGain(2.0f);
			}

			/*if (achievement.name == "Familiar Bond")
			{
				unlockManager->unlock();
			}*/

			// This is the single place to wire "claiming X unlocks feature Y" � for both shop buttons and achievement chains:
			// if (achievement.name == "Big Spender") unlockManager->unlock("EliteSpenderUnlocked");
			// if (achievement.name == "Veteran Slayer") unlockManager->unlock("SomeShopUpgradeKey");
		};

	tabbedPanel->addTab("Statistics", new StatisticsTab(hero, statistics));
	optionsTab = new OptionsTab(this, this);
	tabbedPanel->addTab("Options", optionsTab);
	debugTab = new DebugTab(this);
	tabbedPanel->addTab("Debug", debugTab);
}

void Game::initItemManager()
{
	itemManager = new ItemManager();
}

void Game::initFamiliar(FamiliarName name)
{
	switch (name)
	{
	case BabyFireDragon:
		familiar = new Familiar(familiarTypeList[0]);
		familiar->setOnDamageDealtCallback([this](BigNumber damage) // fires once per landed hit - melee-instant or projectile-arrival alike
			{
				enemySpawner->handleClick(damage, false); // familiar attacks (auto or manual) don't crit for now
			});
		familiar->setElementalMultiplierResolver([this](Elements element)
			{
				return enemySpawner->getEnemyElementalMultiplierForElement(element);
			});
		break;
	default: break;
	}
}

void Game::initAchievements()
{
	achievementManager = new AchievementManager(hero, statistics, familiar, itemManager, unlockManager);
}

void Game::initSaveIndicator()
{
	if (!saveIndicatorFont.openFromFile("Fonts/arial.ttf"))
	{
		std::cout << "Warning: could not load font for SaveIndicator" << std::endl;
	}
	saveIndicator = new SaveIndicator(saveIndicatorFont);
}

void Game::initUnlockManager()
{
	unlockManager = new UnlockManager();
}

void Game::handleBossEncounterEnded(bool wasDefeated)
{
	std::cout << "[Game] handleBossEncounterEnded called, wasDefeated=" << wasDefeated << std::endl; // DEBUG - remove once confirmed working

	// Autosave after every boss encounter, regardless of whether it was won.
	saveToFile(saveFilePath);
}

void Game::onWindowResized(sf::Vector2u newSize)
{
	windowSize = sf::Vector2f(newSize);
	refreshLayoutAfterWindowChange();

	gameView = sf::View(sf::FloatRect({ 0.0f, 0.0f }, windowSize));
	window->setView(gameView);

	stage->onResize();
	hero->onResize();
	familiar->onResize();
	enemySpawner->onResize();
	itemBar->onResize();
	tabbedPanel->onResize();
	// the y origin value is currently fixed! If the Tab Panel Height can change, it should be adjusted here as well
	debugTab->onResize({ windowSize.x - (tabPanelWidth), 40.0f }, { tabPanelWidth , windowSize.y });
}

void Game::update()
{
	pollEvents();

	times.nextDeltaTime();

	float dt = times.getDeltaTime();

	statistics->updatePlayTime(dt);
	achievementManager->update(); // checks every frame if any achievements have been unlocked

	stage->updateTexts();
	stage->setStageNumber(enemySpawner->getCurrentStageNumber());

	BigNumber goldEarned = enemySpawner->update(dt);
	if (goldEarned > BigNumber(0))
	{
		hero->gainCurrency(goldEarned);
		statistics->recordGoldEarned(goldEarned); // feeds the "Gold Rush" achievement
	}

	familiar->update(dt, enemySpawner->getEnemyPosition()); // ticks cooldowns, auto-fires ready auto-attacks, advances in-flight projectiles

	hero->updateSprites(dt);
	hero->updateCurrencyPulse(dt);
	hero->updateDPS(dt);

	saveIndicator->update(dt);
}

void Game::render()
{
	window->clear();

	std::vector<Stage::DepthActor> actors;
	actors.push_back({ hero->getGroundY(), [this]() { hero->draw(window); } });
	if (familiar) actors.push_back({ familiar->getGroundY(), [this]() { familiar->draw(window); } });
	if (enemySpawner->hasEnemy()) actors.push_back({ enemySpawner->getEnemyGroundY(), [this]() { enemySpawner->render(window); } });

	std::vector<Stage::HudElement> hudElements;
	hudElements.push_back({ [this]() { hero->drawHUD(window); } });
	if (familiar) hudElements.push_back({ [this]() { familiar->drawHUD(window); } });
	// hudElements.push_back({ [this]() { enemySpawner->drawHUD(window); } }); // once EnemySpawner splits out its damage/health text the same way

	stage->drawMap(window, actors, hudElements);
	itemBar->render(window);
	tabbedPanel->render(window);
	saveIndicator->render(window); // last of all, on top of the UI panel too

	window->display();
}

void Game::refreshLayoutAfterWindowChange()
{
	recalculateGameAreaWidth();

	gameView = sf::View(sf::FloatRect({ 0.0f, 0.0f }, windowSize));
	window->setView(gameView);

	stage->onResize();
	hero->onResize();
	familiar->onResize();
	enemySpawner->onResize();
	itemBar->onResize();
	tabbedPanel->onResize();
	saveIndicator->onResize();
}

void Game::applyWindowSettings()
{
	if (currentWindowMode == WindowMode::Fullscreen)
	{
		std::vector<sf::VideoMode> fullscreenModes = sf::VideoMode::getFullscreenModes();
		sf::VideoMode chosenMode = fullscreenModes.empty() ? sf::VideoMode::getDesktopMode() : fullscreenModes.front();

		window->create(chosenMode, title, sf::Style::Default, sf::State::Fullscreen);
		windowSize = sf::Vector2f(chosenMode.size);
	}
	else
	{
		sf::Vector2u resolution = availableResolutions[currentResolutionIndex];
		std::uint32_t style = (currentWindowMode == WindowMode::Borderless) ? sf::Style::None : sf::Style::Default;

		window->create(sf::VideoMode(resolution), title, style, sf::State::Windowed);
		windowSize = sf::Vector2f(resolution);

		window->setMinimumSize(sf::Vector2u(static_cast<unsigned int>(uiPanelWidth + 200.0f), 400u));
	}

	window->setFramerateLimit(getFrameRate());
	refreshLayoutAfterWindowChange();
}

Game::Game()
{
	initFrameRate();
	initWindow();
	initAvailableResolutions();
	initStage();
	initHero();

	// TODO: Initialization depends on unlocked familiars
	initFamiliar(BabyFireDragon);
	initStatistics();
	initItemManager(); // needs to be called before initEnemySpawner()/initUI()
	initUnlockManager(); // before initAchievements()
	initAchievements(); // needs to be called before initEnemySpawner() and after initHero() and initStatistics()
	initEnemySpawner();
	initUI();
	initSaveIndicator();

	enemySpawner->setOnBossEncounterEndedCallback([this](bool wasDefeated)
		{
			handleBossEncounterEnded(wasDefeated);
		});

	std::cout << "[Game] Boss-encounter-ended callback registered" << std::endl; // DEBUG - remove once confirmed working

	loadFromFile(saveFilePath); // Auto-load on startup, if a save file exists
}

Game::~Game()
{
	delete hero;
	delete stage;
	delete window;
	delete itemBar;
	delete tabbedPanel;
	delete statistics;
	delete achievementManager;
	delete itemManager;
	delete saveIndicator;
	delete familiar;
	delete unlockManager;
}

void Game::running()
{
	while (window->isOpen())
	{
		update();
		render();
	}
}

int Game::getFrameRate() const
{
	return frameRate;
}

// IWindowController
void Game::setWindowMode(WindowMode mode)
{
	currentWindowMode = mode;
	applyWindowSettings();
}

void Game::setResolutionIndex(int index)
{
	if (index < 0 || index >= static_cast<int>(availableResolutions.size())) return;
	currentResolutionIndex = index;
	if (currentWindowMode != WindowMode::Fullscreen)
	{
		applyWindowSettings();
	}
}

WindowMode Game::getWindowMode() const
{
	return currentWindowMode;
}

int Game::getResolutionIndex() const
{
	return currentResolutionIndex;
}

const std::vector<sf::Vector2u>& Game::getAvailableResolutions() const
{
	return availableResolutions;
}

void Game::debugSetHeroAttackPower(BigNumber value)
{
	hero->setAttackPower(value);
}

void Game::debugSetHeroCurrency(BigNumber value)
{
	hero->setCurrency(value);
}

void Game::debugSetEnemyHealth(BigNumber value)
{
	enemySpawner->setEnemyHealth(value);
}

void Game::debugSetItemDropChance(float value)
{
	itemManager->setDropChance(value);
}

void Game::debugSetHeroCritChance(float value)
{
	hero->setCritChance(value); // also unlocks crit if not already unlocked
}

void Game::debugSetHeroCritDamage(int percent)
{
	hero->setCritDamageMultiplier(percent / 100.0); // 150 -> 1.5
}

void Game::debugResetHeroStats()
{
	hero->resetToDefaults();
}

void Game::debugResetItems()
{
	itemManager->resetInventory();
}

void Game::debugResetShopPrices()
{
	shopTab->resetPricesToDefault();
}

std::string Game::trim(const std::string& str)
{
	const char* toTrim = " \t\r\n";
	size_t indexFirstNonSpaceChar = str.find_first_not_of(toTrim);
	if (indexFirstNonSpaceChar == std::string::npos) return "";
	size_t indexLastNonSpaceChar = str.find_last_not_of(toTrim);
	return str.substr(indexFirstNonSpaceChar, indexLastNonSpaceChar - indexFirstNonSpaceChar + 1);
}

void Game::saveToFile(const std::string& path)
{
	try
	{
		std::cout << "try section: Saving game state to " << path << std::endl; // DEBUG

		std::filesystem::path targetPath(path);
		if (targetPath.has_parent_path())
		{
			std::filesystem::create_directories(targetPath.parent_path()); // e.g. create "Save/" if it doesn't exist yet
		}

		const std::string tmp = path + ".tmp";
		std::ofstream ofs(tmp, std::ios::trunc);
		if (!ofs) return;

		ofs << std::fixed << std::setprecision(6);

		// Game section
		ofs << "[Game]\n";
		ofs << "version=1\n";
		ofs << "windowMode=" << static_cast<int>(currentWindowMode) << '\n';
		ofs << "resolutionIndex=" << currentResolutionIndex << '\n';
		ofs << "deathAnimationsEnabled=" << (deathAnimationsEnabled ? true : false) << '\n';
		ofs << "stageNumber=" << stage->getStageNumber() << '\n';

		// Hero
		ofs << "[Hero]\n";
		ofs << "currency=" << hero->getCurrency().toApproxDouble() << '\n';
		ofs << "attackPower=" << hero->getAttackPower().toApproxDouble() << '\n';
		ofs << "goldMultiplier=" << hero->getGoldMultiplier() << '\n';
		ofs << "critUnlocked=" << (hero->isCritUnlocked() ? 1 : 0) << '\n';
		ofs << "critChance=" << hero->getCritChance() << '\n';
		ofs << "critDamageMultiplier=" << hero->getCritDamageMultiplier() << '\n';

		// Familiars
		if (familiar)
		{
			ofs << "[Familiar]\n";
			ofs << "name=" << (familiar ? familiar->getName() : None) << '\n';
			ofs << "unlocked=" << (familiar && familiar->isUnlocked() ? 1 : 0) << '\n';
			// Only the purchased BONUS stats are saved here - base stats are fully determined by
			// `name` (which evolution stage) via familiarTypeList, so they never need saving, and
			// evolving no longer invalidates anything that was saved (see Familiar::evolveTo).
			ofs << "bonusPhysicalPower=" << familiar->getBonusPhysicalPower().toApproxDouble() << '\n';
			for (int i = 0; i < static_cast<int>(ElementCount); i++)
			{
				ofs << "bonusElementalPower" << i << "=" << familiar->getBonusElementalPower(static_cast<Elements>(i)).toApproxDouble() << '\n';
			}
			ofs << "bonusCooldownReduction=" << familiar->getBonusCooldownReduction() << '\n';
		}

		// Statistics
		ofs << "[Statistics]\n";
		ofs << "enemiesDefeated=" << statistics->getEnemiesDefeated() << '\n';
		ofs << "bossesDefeated=" << statistics->getBossesDefeated() << '\n';
		ofs << "highestStageReached=" << statistics->getHighestStageReached() << '\n';
		ofs << "shopUpgradesPurchased=" << statistics->getShopUpgradesPurchased() << '\n';
		ofs << "totalClicks=" << statistics->getTotalClicks() << '\n';
		ofs << "playTimeSeconds=" << statistics->getPlayTimeSeconds() << '\n';
		ofs << "criticalHitsLanded=" << statistics->getCriticalHitsLanded() << '\n';

		// Achievements (names + unlocked + claimed state)
		ofs << "[Achievements]\n";
		for (const auto& a : achievementManager->getAchievements())
		{
			// store as name|unlocked|claimed (name should not contain '|')
			ofs << "ach=" << a.name << "|" << (a.unlocked ? 1 : 0) << "|" << (a.claimed ? 1 : 0) << '\n';
		}

		// Unlocks (ShopTab Buttons, new Achievements etc...)
		ofs << "[Unlocks]\n";
		for (const auto& key : unlockManager->getUnlockedFeatures())
		{
			ofs << "feature=" << key << '\n';
		}

		// Items (store counts if possible)
		ofs << "[Items]\n";
		int itemCount = itemManager->getItemCount();
		ofs << "count=" << itemCount << '\n';
		for (int i = 0; i < itemCount; ++i)
		{
			ofs << "inv" << i << "=" << itemManager->getInventoryCount(i) << '\n';
		}

		// Shop prices - these aren't derivable from Hero's stats, so they need their own section
		if (shopTab)
		{
			ofs << "[Shop]\n";
			ofs << "attackUpgradeCost=" << shopTab->getAttackUpgradeCost().toApproxDouble() << '\n';
			ofs << "goldMultiplierCost=" << shopTab->getGoldMultiplierCost().toApproxDouble() << '\n';
			ofs << "critChanceCost=" << shopTab->getCritChanceCost().toApproxDouble() << '\n';
			ofs << "critDamageCost=" << shopTab->getCritDamageCost().toApproxDouble() << '\n';
			ofs << "familiarPhysicalCost=" << shopTab->getFamiliarPhysicalAttackCost().toApproxDouble() << '\n';
			ofs << "familiarElementalCost=" << shopTab->getFamiliarElementalAttackCost().toApproxDouble() << '\n';
			ofs << "familiarCooldownCost=" << shopTab->getFamiliarCooldownReductionCost().toApproxDouble() << '\n';
			ofs << "goldMultiplierIncreaseAmount=" << shopTab->getGoldMultiplierIncreaseAmount() << '\n';
			ofs << "goldMultiplierUpgradesPurchased=" << shopTab->getGoldMultiplierUpgradesPurchased() << '\n';
		}

		ofs.close();

		std::filesystem::rename(tmp, path);

		std::cout << "Game state saved to " << path << std::endl; // DEBUG

		if (saveIndicator)
		{
			saveIndicator->show(); // briefly shows "Saving..." - for both manual saves (Options tab) and autosave
		}

		if (optionsTab)
		{
			optionsTab->enableLoadButton(true); // a save file now exists, so the button becomes visible/clickable
		}
	}
	catch (std::exception e)
	{
		std::cout << "Error saving game state: " << e.what() << std::endl;
	}
}

void Game::loadFromFile(const std::string& path)
{
	try
	{
		std::ifstream ifs(path);
		if (!ifs) return;

		std::string line;
		std::string section;

		// Temporary storage
		int fileWindowMode = static_cast<int>(currentWindowMode);
		int fileResolutionIndex = currentResolutionIndex;
		bool fileDeathAnimationsEnabled = deathAnimationsEnabled;
		int fileStageNumber = stage->getStageNumber();

		double heroCurrency = 0.0;
		double heroAttack = 0.0;
		double heroGoldMult = 1.0;
		bool heroCritUnlocked = false;
		double heroCritChance = 0.0;
		double heroCritDmgMult = 1.5;

		// Defaults to current values, so older save files without a [Familiar] section leave it untouched (New)
		bool familiarUnlocked = familiar && familiar->isUnlocked();
		FamiliarName familiarName = BabyFireDragon; // default to BabyFireDragon if no familiar is saved
		double familiarBonusPhysicalPower = familiar ? familiar->getBonusPhysicalPower().toApproxDouble() : 0.0;
		std::array<double, ElementCount> familiarBonusElementalPower{};
		for (int i = 0; i < static_cast<int>(ElementCount); i++)
		{
			familiarBonusElementalPower[i] = familiar ? familiar->getBonusElementalPower(static_cast<Elements>(i)).toApproxDouble() : 0.0;
		}
		float familiarBonusCooldownReduction = familiar ? familiar->getBonusCooldownReduction() : 0.0f;

		int enemiesDefeated = 0;
		int bossesDefeated = 0;
		int highestStageReached = stage->getStageNumber();
		int shopUpgradesPurchased = 0;
		int totalClicks = 0;
		double playTimeSeconds = 0.0;
		int criticalHitsLanded = 0;

		// Achievements/items are parsed but may not be restorable due to missing setters
		struct SavedAchievement { std::string name; bool unlocked; bool claimed; };
		std::vector<SavedAchievement> achievementsSaved;
		std::vector<int> itemCounts;

		// Unlocks
		std::vector<std::string> unlockedFeaturesSaved;

		// Defaults to current values, so older save files without a [Shop] section leave prices untouched
		double shopAttackUpgradeCost = shopTab->getAttackUpgradeCost().toApproxDouble();
		double shopGoldMultiplierCost = shopTab->getGoldMultiplierCost().toApproxDouble();
		double shopCritChanceCost = shopTab->getCritChanceCost().toApproxDouble();
		double shopCritDamageCost = shopTab->getCritDamageCost().toApproxDouble();
		double shopFamiliarPhysicalAttackCost = shopTab->getFamiliarPhysicalAttackCost().toApproxDouble();
		double shopFamiliarElementalAttackCost = shopTab->getFamiliarElementalAttackCost().toApproxDouble();
		double shopFamiliarCooldownCost = shopTab->getFamiliarCooldownReductionCost().toApproxDouble();
		double shopGoldMultiplierIncreaseAmount = shopTab->getGoldMultiplierIncreaseAmount();
		int shopGoldMultiplierUpgradesPurchased = shopTab->getGoldMultiplierUpgradesPurchased();

		while (std::getline(ifs, line))
		{
			line = trim(line);
			if (line.empty() || line[0] == ';' || line[0] == '#') continue;
			if (line.front() == '[' && line.back() == ']')
			{
				section = line.substr(1, line.size() - 2);
				continue;
			}

			auto pos = line.find('=');
			if (pos == std::string::npos) continue;
			std::string key = trim(line.substr(0, pos));
			std::string value = trim(line.substr(pos + 1));

			if (section == "Game")
			{
				if (key == "windowMode") fileWindowMode = std::stoi(value);
				else if (key == "resolutionIndex") fileResolutionIndex = std::stoi(value);
				else if (key == "deathAnimationsEnabled") fileDeathAnimationsEnabled = (std::stoi(value) != false);
				else if (key == "stageNumber") fileStageNumber = std::stoi(value);
			}
			else if (section == "Hero")
			{
				if (key == "currency") heroCurrency = std::stod(value);
				else if (key == "attackPower") heroAttack = std::stod(value);
				else if (key == "goldMultiplier") heroGoldMult = std::stod(value);
				else if (key == "critUnlocked") heroCritUnlocked = (std::stoi(value) != 0);
				else if (key == "critChance") heroCritChance = std::stod(value);
				else if (key == "critDamageMultiplier") heroCritDmgMult = std::stod(value);
			}
			else if (section == "Familiar")
			{
				if (key == "name") familiarName = static_cast<FamiliarName>(std::stoi(value));
				else if (key == "unlocked") familiarUnlocked = (std::stoi(value) != 0);
				else if (key == "bonusPhysicalPower") familiarBonusPhysicalPower = std::stod(value);
				else if (key == "bonusCooldownReduction") familiarBonusCooldownReduction = std::stof(value);
				else if (key.rfind("bonusElementalPower", 0) == 0) // e.g. "bonusElementalPower0", "bonusElementalPower1", ...
				{
					int elementIndex = std::stoi(key.substr(std::string("bonusElementalPower").size()));
					if (elementIndex >= 0 && elementIndex < static_cast<int>(ElementCount))
					{
						familiarBonusElementalPower[elementIndex] = std::stod(value);
					}
				}
			}
			else if (section == "Statistics")
			{
				if (key == "enemiesDefeated") enemiesDefeated = std::stoi(value);
				else if (key == "bossesDefeated") bossesDefeated = std::stoi(value);
				else if (key == "highestStageReached") highestStageReached = std::stoi(value);
				else if (key == "shopUpgradesPurchased") shopUpgradesPurchased = std::stoi(value);
				else if (key == "totalClicks") totalClicks = std::stoi(value);
				else if (key == "playTimeSeconds") playTimeSeconds = std::stod(value);
				else if (key == "criticalHitsLanded") criticalHitsLanded = std::stoi(value);
			}
			else if (section == "Achievements")
			{
				if (key == "ach")
				{
					auto separator1 = value.find('|');
					if (separator1 != std::string::npos)
					{
						std::string name = value.substr(0, separator1);
						auto separator2 = value.find('|', separator1 + 1);
						bool unlocked = (std::stoi(value.substr(separator1 + 1, separator2 == std::string::npos ? std::string::npos : separator2 - separator1 - 1)) != 0);
						bool claimed = (separator2 != std::string::npos) && (std::stoi(value.substr(separator2 + 1)) != 0); // old save files without a claimed field default to false
						achievementsSaved.push_back({ name, unlocked, claimed });
					}
				}
			}
			else if (section == "Unlocks")
			{
				if (key == "feature") unlockedFeaturesSaved.push_back(value);
			}
			else if (section == "Items")
			{
				if (key == "count")
				{
					int cnt = std::stoi(value);
					itemCounts.resize(cnt, 0);
				}
				else if (key.rfind("inv", 0) == 0)
				{
					int idx = std::stoi(key.substr(3));
					if (idx >= 0 && idx < static_cast<int>(itemCounts.size()))
						itemCounts[idx] = std::stoi(value);
				}
			}
			else if (section == "Shop")
			{
				if (key == "attackUpgradeCost") shopAttackUpgradeCost = std::stod(value);
				else if (key == "goldMultiplierCost") shopGoldMultiplierCost = std::stod(value);
				else if (key == "critChanceCost") shopCritChanceCost = std::stod(value);
				else if (key == "critDamageCost") shopCritDamageCost = std::stod(value);
				else if (key == "familiarPhysicalCost") shopFamiliarPhysicalAttackCost = std::stod(value);
				else if (key == "familiarElementalCost") shopFamiliarElementalAttackCost = std::stod(value);
				else if (key == "familiarCooldownCost") shopFamiliarCooldownCost = std::stod(value);
				else if (key == "goldMultiplierIncreaseAmount") shopGoldMultiplierIncreaseAmount = std::stod(value);
				else if (key == "goldMultiplierUpgradesPurchased") shopGoldMultiplierUpgradesPurchased = std::stoi(value);
			}
		}

		std::cout << "Game state loaded from " << path << std::endl; // DEBUG

		// Apply loaded values where possible
		// Window settings
		setWindowMode(static_cast<WindowMode>(fileWindowMode));
		setResolutionIndex(fileResolutionIndex);
		setDeathAnimationsEnabled(fileDeathAnimationsEnabled);

		// Stage
		stage->setStageNumber(fileStageNumber);
		enemySpawner->setCurrentStageNumber(fileStageNumber); // without this, update() overwrites stage->setStageNumber() next frame
		statistics->setHighestStageReached(highestStageReached);

		// Hero: currency
		{
			BigNumber cur = hero->getCurrency();
			double curD = cur.toApproxDouble();
			if (heroCurrency > curD)
			{
				hero->gainCurrency(BigNumber(heroCurrency - curD));
			}
			else if (heroCurrency < curD)
			{
				// spend the surplus to reach the desired value
				hero->trySpendCurrency(BigNumber(curD - heroCurrency));
			}
		}

		// Hero: attack power (only supports increasing to target)
		{
			BigNumber cur = hero->getAttackPower();
			double curD = cur.toApproxDouble();
			if (heroAttack > curD)
			{
				hero->increaseAttackPower(BigNumber(heroAttack - curD));
			}
			// if saved attack is lower than current we cannot decrease with existing API
		}

		// Hero: gold multiplier (only supports increasing)
		{
			double cur = hero->getGoldMultiplier();
			if (heroGoldMult > cur)
				hero->increaseGoldMultiplier(heroGoldMult - cur);
			// decreasing multiplier not supported by public API
		}

		// Hero: crits
		if (heroCritUnlocked && !hero->isCritUnlocked())
		{
			hero->unlockCriticalHits(heroCritChance);
			if (heroCritDmgMult > hero->getCritDamageMultiplier())
				hero->increaseCritDamage(heroCritDmgMult - hero->getCritDamageMultiplier());
		}
		else if (hero->isCritUnlocked())
		{
			// ensure chances/damage are at least saved values
			if (heroCritChance > hero->getCritChance())
				hero->increaseCritChance(heroCritChance - hero->getCritChance());
			if (heroCritDmgMult > hero->getCritDamageMultiplier())
				hero->increaseCritDamage(heroCritDmgMult - hero->getCritDamageMultiplier());
		}

		// Familiars: unlock if saved as unlocked
		if (familiar && familiarUnlocked && !familiar->isUnlocked())
		{
			familiar->unlock();
			shopTab->refreshVisibility(); // replaces setFamiliarButtonActive(false) / setFamiliarButtonsActive(true)
			familiar->setBonusPhysicalPower(BigNumber(familiarBonusPhysicalPower));
			for (int i = 0; i < static_cast<int>(ElementCount); i++)
			{
				familiar->setBonusElementalPower(static_cast<Elements>(i), BigNumber(familiarBonusElementalPower[i]));
			}
			familiar->setBonusCooldownReduction(familiarBonusCooldownReduction);
		}

		// Initialize familiar based on saved name (if any) IMPLEMENT LATER CORRECTLY WITH familiarList[i]
		/*
		if (familiarName == BabyFireDragon)
		{
			initFamiliar(BabyFireDragon);
		}
		if (familiarName == BabyIceDragon)
		{
			initFamiliar(BabyIceDragon);
		}
		if (familiarName == FireDragon)
		{
			initFamiliar(FireDragon);
		}
		if (familiarName == IceDragon)
		{
			initFamiliar(IceDragon);
		}*/

		// Statistics: increase counters to saved values where possible
		{
			int curEnemies = statistics->getEnemiesDefeated();
			for (int i = curEnemies; i < enemiesDefeated; ++i) statistics->recordEnemyDefeated();

			int curBosses = statistics->getBossesDefeated();
			for (int i = curBosses; i < bossesDefeated; ++i) statistics->recordBossDefeated();

			int curShop = statistics->getShopUpgradesPurchased();
			for (int i = curShop; i < shopUpgradesPurchased; ++i) statistics->recordShopUpgradePurchased();

			int curClicks = statistics->getTotalClicks();
			for (int i = curClicks; i < totalClicks; ++i) statistics->recordClick();

			float curPlay = statistics->getPlayTimeSeconds();
			if (playTimeSeconds > curPlay) statistics->updatePlayTime(static_cast<float>(playTimeSeconds - curPlay));

			int curCrits = statistics->getCriticalHitsLanded();
			for (int i = curCrits; i < criticalHitsLanded; ++i) statistics->recordCriticalHit();
		}

		// Unlocks
		for (const auto& featureKey : unlockedFeaturesSaved)
		{
			unlockManager->restoreUnlocked(featureKey);
		}

		// Achievements
		for (const auto& saved : achievementsSaved)
		{
			achievementManager->setUnlocked(saved.name, saved.unlocked);
			achievementManager->setClaimed(saved.name, saved.claimed);
		}

		// Items
		for (size_t i = 0; i < itemCounts.size(); i++)
		{
			itemManager->setInventoryCount(static_cast<int>(i), itemCounts[i]);
		}

		// Shop prices
		if (shopTab)
		{
			shopTab->setAttackUpgradeCost(BigNumber(shopAttackUpgradeCost));
			shopTab->setGoldMultiplierCost(BigNumber(shopGoldMultiplierCost));
			shopTab->setCritChanceCost(BigNumber(shopCritChanceCost));
			shopTab->setCritDamageCost(BigNumber(shopCritDamageCost));
			shopTab->setFamiliarPhysicalAttackCost(BigNumber(shopFamiliarPhysicalAttackCost));
			shopTab->setFamiliarElementalAttackCost(BigNumber(shopFamiliarElementalAttackCost));
			shopTab->setFamiliarCooldownReductionCost(BigNumber(shopFamiliarCooldownCost));
			shopTab->setGoldMultiplierIncreaseAmount(shopGoldMultiplierIncreaseAmount);
			shopTab->setGoldMultiplierUpgradesPurchased(shopGoldMultiplierUpgradesPurchased);
			shopTab->refreshLabels();
		}

		// Refresh layout / UI after load
		refreshLayoutAfterWindowChange();

		if (optionsTab)
		{
			optionsTab->refreshLabels(); // otherwise the Options tab keeps showing the pre-load defaults after auto-load
		}

		std::cout << "Game state applied successfully." << std::endl; // DEBUG
	}
	catch (std::exception e)
	{
		std::cout << "Error loading game state: " << e.what() << std::endl;
	}
}

void Game::setDeathAnimationsEnabled(bool enabled)
{
	deathAnimationsEnabled = enabled;
}