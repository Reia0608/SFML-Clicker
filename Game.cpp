#include "stdafx.h"
#include "Game.h"
#include "Times.h"
#include "Stage.h"
#include "ShopTab.h"
#include "OptionsTab.h"
#include "LayoutManager.h"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <array>
#include <cmath>
#include <algorithm>

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
					if (hud->handleClick(clickPos))
					{
						// consumed by a HUD button - skip recordClick/tabbedPanel/enemy hit entirely
					}
					else if (clickPos.x < gameAreaWidth)
					{
						statistics->recordClick();
						if (!tabbedPanel->handleClick(clickPos) && clickPos.x < gameAreaWidth)
						{
							// ...existing enemy-hit block, unchanged
						}
					}
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
	tabbedPanel->addTab("Familiars", familiarTab, [this]() { return familiar && familiar->isUnlocked(); });

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

	// Familiar slots - Game owns the actual roster/slot state, FamiliarTab only displays it and
	// reports the player's intent back through these (see the "Familiar slots" block in Game.h).
	familiarTab->onSlotPurchased = [this]() { onFamiliarSlotPurchased(); };
	familiarTab->getSlotCount = [this]() { return getFamiliarSlotCount(); };
	familiarTab->getSlotFamiliarLabel = [this](int slotIndex) { return getFamiliarSlotLabel(slotIndex); };
	familiarTab->getAvailableFamiliarNames = [this]() { return getAvailableFamiliarNamesForSlotting(); };
	familiarTab->onAssignFamiliarToSlot = [this](int slotIndex, int pickedIndex) { assignFamiliarToSlot(slotIndex, pickedIndex); };
	familiarTab->onUnassignSlot = [this](int slotIndex) { unassignFamiliarSlot(slotIndex); };
	familiarTab->refreshSlotButtons(); // populate the slot list now that the callbacks above are wired (starts empty - slots are bought, not free)

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

	hud = new HUD([this]() { return enemySpawner->getEnemyPosition(); });
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
		familiar->setOnDamageDealtCallback([this](BigNumber damage, ElementMask mask) // fires once per landed hit - melee-instant or projectile-arrival alike
			{
				enemySpawner->handleClick(damage, false, Familiar::dpsColor, mask); // familiar attacks (auto or manual) don't crit for now; colored to match familiarDPSText unless mask carries an element, in which case the element's gradient wins (see Enemy::showDamage)
			});
		familiar->setElementalMultiplierResolver([this](Elements element)
			{
				return enemySpawner->getEnemyElementalMultiplierForElement(element);
			});
		break;
	default: break;
	}
}

int Game::findFamiliarTypeIndex(FamiliarName name) const
{
	for (size_t i = 0; i < familiarTypeList.size(); i++)
	{
		if (familiarTypeList[i].name == name) return static_cast<int>(i);
	}
	return -1; // saved stage isn't defined in familiarTypeList (yet) - e.g. an Ice dragon line the current build doesn't have entries for
}

int Game::getUnlockedFamiliarCount() const
{
	// Only one familiar exists today, so this is 0 or 1. See the TODO on the declaration for
	// what this should become once there's an actual roster of familiars to sum over.
	return (familiar && familiar->isUnlocked()) ? 1 : 0;
}

int Game::getFamiliarEvolutionCount(const Familiar* f) const
{
	if (!f || !f->isUnlocked()) return 0;

	int typeIndex = findFamiliarTypeIndex(f->getName());
	if (typeIndex < 0) return 0; // current stage isn't in familiarTypeList (yet) - treat as no evolution bonus rather than guessing

	// Age (PublicVariables.h) is declared Baby, Juvenile, Adult, Ancient with no explicit numbering, so
	// the compiler assigns Baby=0, Juvenile=1, Adult=2 - which happens to already be exactly
	// "how many times this familiar has evolved" for every stage defined today (BabyFireDragon
	// is Age::Baby=0, FireDragon is Age::Juvenile=1). Reusing that ordinal here avoids adding a
	// dedicated evolution counter anywhere.
	//
	// This only holds as long as two things stay true:
	//  1) Age's declaration order never changes and nothing is ever inserted BEFORE/BETWEEN its
	//     existing entries (appending a new one after Adult is fine - its ordinal is simply the
	//     next number). Reordering would silently shift every existing stage's ordinal and change
	//     already-tuned passive damage for saves/stages that didn't actually change.
	//  2) Every future evolution advances Age by exactly one tier - i.e. no stage ever skips a
	//     tier (Baby straight to Adult) or stays on the same tier as its predecessor. If either
	//     happens, this count would no longer equal the real number of evolutions (skipping a
	//     tier overcounts by however many tiers were skipped; staying on the same tier undercounts
	//     by not registering that evolution at all).
	// If either assumption stops holding, switch to a dedicated evolutionIndex field on
	// FamiliarType/Familiar instead (see the version of this feature before this comment).
	return static_cast<int>(familiarTypeList[typeIndex].age);
}

int Game::getTotalFamiliarEvolutionCount() const
{
	// Sums getFamiliarEvolutionCount() across every unlocked familiar. Only one familiar to sum
	// today; see the TODO on the declaration for how this should generalize.
	return getFamiliarEvolutionCount(familiar);
}

double Game::computeFamiliarPassiveDamagePerSecond() const
{
	int unlockedCount = getUnlockedFamiliarCount();
	if (unlockedCount <= 0) return 0.0; // no familiar unlocked yet - no passive damage at all

	int additionalFamiliars = unlockedCount - 1; // the first familiar unlocked just grants the base amount, with no bonus yet
	double familiarCountMultiplier = std::pow(1.0 + familiarPassiveDamagePerAdditionalFamiliar, additionalFamiliars);

	int totalEvolutions = getTotalFamiliarEvolutionCount();
	double evolutionMultiplier = std::pow(1.0 + familiarPassiveDamagePerEvolution, totalEvolutions);

	return familiarPassiveBaseDamagePerSecond * familiarCountMultiplier * evolutionMultiplier;
}

namespace
{
	sf::FloatRect unionRects(const sf::FloatRect& a, const sf::FloatRect& b)
	{
		float left = std::min(a.position.x, b.position.x);
		float top = std::min(a.position.y, b.position.y);
		float right = std::max(a.position.x + a.size.x, b.position.x + b.size.x);
		float bottom = std::max(a.position.y + a.size.y, b.position.y + b.size.y);
		return sf::FloatRect({ left, top }, { right - left, bottom - top });
	}
}

void Game::initHUDStatPanel()
{
	hudStatPanel.setFillColor(sf::Color(10, 8, 6, 140)); // dark, semi-transparent - same style as TabbedPanel's own background rectangle
	hudStatPanel.setOutlineColor(sf::Color::Black);
	hudStatPanel.setOutlineThickness(1.0f);
	updateHUDStatPanelLayout(); // seed a valid initial size/position; also recomputed every frame from Game::update()
}

void Game::updateHUDStatPanelLayout()
{
	sf::FloatRect bounds = hero->getStatsBounds(); // currency/Hero DPS/click damage are always shown

	if (familiar && familiar->isUnlocked()) // passive familiar DPS - same unlock gate as its visibility in render()
	{
		bounds = unionRects(bounds, passiveFamiliarDPSText->getGlobalBounds());
	}

	for (Familiar* activeFamiliar : familiarSlots) // familiar auto-attack DPS - only slotted familiars are ever drawn/relevant here, same as in render()
	{
		if (!activeFamiliar) continue;
		bounds = unionRects(bounds, activeFamiliar->getDPSTextBounds());
	}

	hudStatPanel.setPosition({ bounds.position.x - hudStatPanelPadding, bounds.position.y - hudStatPanelPadding });
	hudStatPanel.setSize({ bounds.size.x + hudStatPanelPadding * 2.0f, bounds.size.y + hudStatPanelPadding * 2.0f });
}

void Game::initPassiveFamiliarDPSText()
{
	passiveFamiliarDPSText = new Text("Passive Familiar DPS: " + passiveFamiliarDPSTracker.getDPS().toString(), "Fonts/arial.ttf");
	// Pale lavender - source-identity color for "passive familiar damage", the third of the trio
	// alongside Hero::dpsText (pale gold) and Familiar::familiarDPSText (pale silver-blue). See
	// the comments there for why these are deliberately desaturated rather than vivid hues.
	passiveFamiliarDPSText->setFillColor(passiveDPSColor);
	passiveFamiliarDPSText->setOutlineColor(sf::Color::Black);
	passiveFamiliarDPSText->setOutlineThickness(2.0f);
	updatePassiveFamiliarDPSText();
}

void Game::updatePassiveFamiliarDPSText()
{
	passiveFamiliarDPSText->setString("Passive Familiar DPS: " + passiveFamiliarDPSTracker.getDPS().toString());
	// -120: sits below Familiar::familiarDPSText's -160 - passive damage unlocks as soon as any
	// familiar is unlocked, while Familiar DPS additionally requires that familiar to be slotted
	// (a later step), so this line appears first/lower in the stack.
	LayoutManager::applyAnchor(*passiveFamiliarDPSText, Anchor::BottomRight, { -10.0f, -120.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void Game::updateFamiliarPassiveDamage(float dt)
{
	passiveFamiliarDPSTracker.update(dt); // ages out old events every frame, whether or not a new one is recorded below - same pattern as Familiar::update()/Hero::updateDPS()
	updatePassiveFamiliarDPSText();

	if (!familiar || !familiar->isUnlocked()) return; // passive damage only starts once the first familiar is unlocked

	passiveDamageTickTimer += dt;
	while (passiveDamageTickTimer >= passiveDamageTickInterval) // a `while`, not an `if`, so a lag spike still deals every tick it was owed rather than dropping ticks
	{
		passiveDamageTickTimer -= passiveDamageTickInterval;

		BigNumber passiveDamage = BigNumber(std::round(computeFamiliarPassiveDamagePerSecond()));
		enemySpawner->handleClick(passiveDamage, false, passiveDPSColor); // passive damage never crits; colored to match passiveFamiliarDPSText; no-op if there's currently no enemy on the field
		passiveFamiliarDPSTracker.recordDamage(passiveDamage); // recorded regardless of whether an enemy was actually there to take it - matches how DPS trackers elsewhere in the codebase count "damage attempted this tick", not confirmed hits
	}
}

BigNumber Game::getFamiliarPassiveDPS() const
{
	return passiveFamiliarDPSTracker.getDPS();
}

void Game::onFamiliarSlotPurchased()
{
	familiarSlots.push_back(nullptr); // one more empty slot - the player assigns a familiar to it afterward, via FamiliarTab's picker
}

int Game::getFamiliarSlotCount() const
{
	return static_cast<int>(familiarSlots.size());
}

std::string Game::getFamiliarSlotLabel(int slotIndex) const
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(familiarSlots.size())) return "";

	Familiar* occupant = familiarSlots[slotIndex];
	if (!occupant) return "";

	// Same trick as getFamiliarEvolutionCount(): look the occupant's current stage up in
	// familiarTypeList rather than storing a separate display-name field anywhere.
	int typeIndex = findFamiliarTypeIndex(occupant->getName());
	return typeIndex >= 0 ? familiarTypeList[typeIndex].label : "Familiar"; // fallback if its current stage somehow isn't in familiarTypeList
}

std::vector<std::string> Game::getAvailableFamiliarNamesForSlotting() const
{
	std::vector<std::string> names;

	// TODO: once multiple owned familiars exist, iterate over all of them here instead of just
	// the single `familiar` member (see the matching TODOs on the passive-damage helpers above).
	if (familiar && familiar->isUnlocked() && !isFamiliarInAnySlot(familiar))
	{
		int typeIndex = findFamiliarTypeIndex(familiar->getName());
		names.push_back(typeIndex >= 0 ? familiarTypeList[typeIndex].label : "Familiar");
	}

	return names;
}

bool Game::isFamiliarInAnySlot(const Familiar* f) const
{
	for (Familiar* occupant : familiarSlots)
	{
		if (occupant == f) return true;
	}
	return false;
}

void Game::assignFamiliarToSlot(int slotIndex, int pickedIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(familiarSlots.size())) return;
	if (familiarSlots[slotIndex] != nullptr) return; // already occupied - FamiliarTab only opens the picker for empty slots, this is just a safety net

	std::vector<std::string> available = getAvailableFamiliarNamesForSlotting();
	if (pickedIndex < 0 || pickedIndex >= static_cast<int>(available.size())) return;

	// TODO: once multiple owned familiars exist, resolve `pickedIndex` against that full list
	// instead of assuming it can only ever mean `familiar` (see getAvailableFamiliarNamesForSlotting()).
	if (familiar && familiar->isUnlocked() && !isFamiliarInAnySlot(familiar))
	{
		familiarSlots[slotIndex] = familiar;
	}
}

void Game::unassignFamiliarSlot(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(familiarSlots.size())) return;
	familiarSlots[slotIndex] = nullptr; // the familiar itself is untouched (still unlocked, keeps its bonuses/evolution) - it just goes back to the available pool
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

void Game::initLootDropManager()
{
	lootDropManager = new LootDropManager(hero, itemBar, itemManager);

	// The reward is only actually granted here, once a drop's fly-to-target animation completes -
	// see LootDropManager::update()/onGoldDropped()/onItemDropped().
	lootDropManager->setOnGoldCollectedCallback([this](BigNumber amount)
		{
			hero->gainCurrency(amount);
			statistics->recordGoldEarned(amount); // feeds the "Gold Rush" achievement
		});

	lootDropManager->setOnItemCollectedCallback([this](int itemIndex)
		{
			itemManager->addItem(itemIndex);
		});
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
	hud->onResize();
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

	if (enemySpawner->hasEnemy())
	{
		if (enemySpawner->getIsCurrentEnemyBoss())
		{
			stage->setEnemyInfoText("Boss: " + enemySpawner->getEnemyName());
		}
		else
		{
			stage->setEnemyInfoText(enemySpawner->getEnemyName() + " ("
				+ std::to_string(enemySpawner->getEnemyIndexInStage() + 1) + "/" + std::to_string(enemiesPerStage) + ")");
		}
	}
	else
	{
		stage->setEnemyInfoText("");
	}

	enemySpawner->update(dt); // gold/item rewards are now granted asynchronously, once LootDropManager's drop animation reaches its HUD target (see initLootDropManager())
	lootDropManager->update(dt);

	for (Familiar* activeFamiliar : familiarSlots) // only SLOTTED familiars auto-attack/animate - an unlocked-but-unassigned familiar sits out of the play area entirely
	{
		if (!activeFamiliar) continue; // empty slot
		activeFamiliar->update(dt, enemySpawner->getEnemyPosition()); // ticks cooldowns, auto-fires ready auto-attacks, advances in-flight projectiles
	}
	updateFamiliarPassiveDamage(dt); // deals passive damage once per elapsed second, active as soon as a familiar is unlocked - independent of the slot loop above

	hud->sync(familiarSlots);
	hud->update();
	hero->updateSprites(dt);
	hero->updateCurrencyPulse(dt);
	hero->updateDPS(dt);

	updateHUDStatPanelLayout(); // must run after every stat text above (and the familiar/passive updates earlier) has refreshed for this frame, since it wraps their current bounds

	saveIndicator->update(dt);
}

void Game::render()
{
	window->clear();

	std::vector<Stage::DepthActor> actors;
	actors.push_back({ hero->getGroundY(), [this]() { hero->draw(window); } });
	for (Familiar* activeFamiliar : familiarSlots) // only SLOTTED familiars appear in the play area - see the matching loop in update()
	{
		if (!activeFamiliar) continue;
		actors.push_back({ activeFamiliar->getGroundY(), [activeFamiliar, this]() { activeFamiliar->draw(window); } });
	}
	if (enemySpawner->hasEnemy()) actors.push_back({ enemySpawner->getEnemyGroundY(), [this]() { enemySpawner->render(window); } });

	std::vector<Stage::HudElement> hudElements;
	// Drawn first so every stat-text element pushed after this paints on top of it - see the
	// member comment on hudStatPanel in Game.h for why this backdrop exists.
	hudElements.push_back({ [this]() { window->draw(hudStatPanel); } });
	hudElements.push_back({ [this]() { hero->drawHUD(window); } });
	if (familiar && familiar->isUnlocked()) // same gate as computeFamiliarPassiveDamagePerSecond()/updateHUDStatPanelLayout() - hidden until passive damage actually starts, not shown at "0" the whole game
	{
		hudElements.push_back({ [this]() { window->draw(*passiveFamiliarDPSText); } });
	}
	for (Familiar* activeFamiliar : familiarSlots)
	{
		if (!activeFamiliar) continue;
		hudElements.push_back({ [activeFamiliar, this]() { activeFamiliar->drawHUD(window); } });
		hudElements.push_back({ [this]() { hud->render(window); } });
	}
	// hudElements.push_back({ [this]() { enemySpawner->drawHUD(window); } }); // once EnemySpawner splits out its damage/health text the same way

	stage->drawMap(window, actors, hudElements);
	itemBar->render(window);
	lootDropManager->render(window); // drawn after itemBar so a flying item stays visible on top of the bar all the way to its slot, instead of disappearing behind the icon
	tabbedPanel->render(window);
	saveIndicator->render(window); // last of all, on top of the UI panel too

	window->display();
}

void Game::refreshLayoutAfterWindowChange()
{
	recalculateTabPanelWidth(); // must run first: gameAreaWidth is derived from uiPanelWidth, which is derived from tabPanelWidth
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
	updateHUDStatPanelLayout(); // repositions all the anchored texts above changed gameAreaWidth/windowSize, so their bounds moved too - recompute the panel wrapping them (also recomputed every frame from update(), this just avoids one stale frame right after a resize)
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
		recalculateTabPanelWidth(); // refresh uiPanelWidth for the new resolution before using it below - refreshLayoutAfterWindowChange() (called at the end of this function) also calls this, harmlessly recomputing the same value

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
	passiveFamiliarDPSTracker.setWindowDuration(passiveDamageTickInterval); // keep the DPS window matched to the tick interval - see the member comment in Game.h
	initHUDStatPanel();
	initPassiveFamiliarDPSText();
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

	initLootDropManager(); // needs hero, itemBar and itemManager, so it must come after initUI()

	enemySpawner->setOnGoldDroppedCallback([this](BigNumber amount, sf::Vector2f position)
		{
			lootDropManager->onGoldDropped(amount, position);
		});

	enemySpawner->setOnItemDroppedCallback([this](int itemIndex, sf::Vector2f position)
		{
			lootDropManager->onItemDropped(itemIndex, position);
		});

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
	delete lootDropManager;
	delete saveIndicator;
	delete familiar;
	delete unlockManager;
	delete passiveFamiliarDPSText;
	delete hud;
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

		// Familiar slots - separate from [Familiar] above, since this is about WHERE a familiar
		// is placed, not the familiar's own stats/evolution.
		ofs << "[FamiliarSlots]\n";
		ofs << "slotCount=" << getFamiliarSlotCount() << '\n';
		for (int i = 0; i < getFamiliarSlotCount(); i++)
		{
			// TODO: once multiple owned familiars exist, this needs a real per-familiar
			// identifier instead of a bool - today there's only ever one possible occupant
			// (`familiar`), so "is this slot occupied at all" is the only thing worth saving.
			ofs << "occupied" << i << "=" << (familiarSlots[i] != nullptr ? 1 : 0) << '\n';
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

		// Defaults to empty (0 slots), so older save files without a [FamiliarSlots] section
		// just leave the player with no slots - same "absent section changes nothing new" idea
		// as the other defaults above, since 0 also happens to be this feature's starting state.
		std::vector<bool> fileFamiliarSlotOccupied;

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
			else if (section == "FamiliarSlots")
			{
				if (key == "slotCount")
				{
					int cnt = std::stoi(value);
					fileFamiliarSlotOccupied.assign(cnt, false);
				}
				else if (key.rfind("occupied", 0) == 0) // e.g. "occupied0", "occupied1", ...
				{
					int idx = std::stoi(key.substr(std::string("occupied").size()));
					if (idx >= 0 && idx < static_cast<int>(fileFamiliarSlotOccupied.size()))
						fileFamiliarSlotOccupied[idx] = (std::stoi(value) != 0);
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

		// Restore the familiar's saved evolution stage. `familiar` was already constructed as
		// BabyFireDragon back in Game() (so ShopTab/AchievementManager/its callbacks could all be
		// wired to it) - so if the save says it should be further along, we evolve that SAME
		// instance in place via evolveTo(), same as FamiliarTab's evolve button does. We
		// deliberately do NOT call initFamiliar() again here: that would allocate a brand new
		// Familiar object, leak the original (nothing currently calls delete on the old pointer
		// before overwriting it), and leave ShopTab/AchievementManager/etc. still pointing at the
		// now-orphaned original instead of the reloaded one.
		if (familiar && familiar->getName() != familiarName)
		{
			int typeIndex = findFamiliarTypeIndex(familiarName);
			if (typeIndex >= 0)
			{
				familiar->evolveTo(familiarTypeList[typeIndex]);
			}
			// else: this save was written by a build with a familiar line (e.g. the Ice dragons)
			// that isn't in familiarTypeList yet - leave the familiar at its current stage rather
			// than silently doing nothing or indexing out of range.
		}

		// Restore familiar slots. `familiarSlots` is rebuilt to the saved slot count, then the
		// (single) familiar is placed into whichever slot was saved as occupied. Since there's
		// only one possible occupant today, only the FIRST occupied flag found actually gets a
		// familiar assigned - see the matching TODO on saveToFile(). This must run after the
		// unlock/evolution restoration just above, since it depends on familiar->isUnlocked().
		familiarSlots.assign(fileFamiliarSlotOccupied.size(), nullptr);
		if (familiar && familiar->isUnlocked())
		{
			for (size_t i = 0; i < fileFamiliarSlotOccupied.size(); i++)
			{
				if (fileFamiliarSlotOccupied[i])
				{
					familiarSlots[i] = familiar;
					break; // only one familiar exists to assign - ignore any further occupied flags
				}
			}
		}
		if (familiarTab) familiarTab->refreshSlotButtons(); // slot state just changed outside FamiliarTab's own click handlers - make sure its displayed list catches up

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