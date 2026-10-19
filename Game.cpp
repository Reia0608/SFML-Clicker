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
	title = "My Familiars by Sami Ounis";
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
			else if (keyPressed->scancode == pauseKey)
			{
				hud->togglePause();
				delete frameStepHistory;
				frameStepHistory = hud->isPaused() ? new FrameStepHistory() : nullptr;
			}
			else if (hud->isPaused() && keyPressed->scancode == stepForwardKey)
			{
				stepFrameForward();
			}
			else if (hud->isPaused() && keyPressed->scancode == stepBackwardKey)
			{
				stepFrameBackward();
			}
			else if (keyPressed->scancode == rulerKey)
			{
				ruler->toggle();
			}
		}
		else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
		{
			if (mousePressed->button == sf::Mouse::Button::Left)
			{
				sf::Vector2f clickPos = window->mapPixelToCoords(mousePressed->position);

				// A HUD manual-attack button takes priority over everything below it - consuming
				// the click here means it never also reaches tabbedPanel or deals the hero's own
				// click damage.
				bool consumedByHud = clickPos.x < gameAreaWidth && hud->handleClick(clickPos);
				if (!consumedByHud)
				{
					if (clickPos.x < gameAreaWidth)
					{
						statistics->recordClick();
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
	familiarTab = new FamiliarTab(hero, itemManager, unlockManager, statistics,
		ownedFamiliars[FireDragon], ownedFamiliars[WaterDragon], ownedFamiliars[WindDragon]);
	tabbedPanel->addTab("Familiars", familiarTab, [this]() { return familiar && familiar->isUnlocked(); });

	// Generalized evolve wiring, used by every lineage's self-updating evolve button (see
	// FamiliarTab::addFamiliarEvolveButton()). FamiliarTab only knows the NAME of the next stage
	// (from its own stage-order list) - Game is the one that owns familiarTypeList, so it's the
	// one that resolves that name into the actual FamiliarType data and performs evolveTo().
	familiarTab->onEvolveFamiliar = [this](Familiar* target, Age nextAge)
	{
		int typeIndex = findFamiliarTypeIndex(target->getName(), nextAge);
		if (typeIndex < 0) return; // shouldn't happen - FamiliarTab already checked isFamiliarStageAvailable before spending anything - but fail safe rather than indexing out of range
		target->evolveTo(familiarTypeList[typeIndex]);

		// The new stage's auto-attack has its own baseCooldown/minCooldownLimit, so a carried-over
		// bonusCooldownReduction may no longer be enough to have hit the new floor even if it had
		// hit the old one - re-check FamiliarTab's own visibility rules so that lineage's cooldown
		// button reappears if there's room to buy more (this used to check shopTab, back when the
		// cooldown button still lived there - see Step 3).
		if (familiarTab) familiarTab->refreshButtonVisibility();
	};

	// Lets FamiliarTab know whether the NEXT stage it wants to evolve into actually has data in
	// familiarTypeList yet, BEFORE it lets the player pay for it - stages beyond what's currently
	// defined (e.g. Adult/Ancient, for now) stay hidden rather than silently charging Emblems for
	// an evolution that can't actually happen.
	familiarTab->isFamiliarStageAvailable = [this](FamiliarName name, Age age)
	{
		return findFamiliarTypeIndex(name, age) >= 0;
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

	achievementManager->onAchievementClaimed = [this](const Achievement& achievement)
	{
		if (achievement.name == "Hoarder")
		{
			shopTab->increaseGoldMultiplierGain(2.0f);
		}

		if (achievement.name == "Juvenile Flame")
		{
			// Doesn't grant the Fireball attack itself - just reveals FamiliarTab's purchase
			// button for it (see FamiliarTab.cpp's addAttackUnlockButton() call). The button's
			// own click handler sets the separate "FireDragonFireballUnlocked" key once bought.
			unlockManager->unlock("FireDragonFireballPurchasable");
			if (familiarTab) familiarTab->markPending(); // flash the Familiars tab button - same pulse as when it first became available
		}

		if (achievement.name == "Juvenile Tide")
		{
			unlockManager->unlock("WaterDragonBubbleStreamPurchasable");
			if (familiarTab) familiarTab->markPending();
		}

		if (achievement.name == "Juvenile Gale")
		{
			unlockManager->unlock("WindDragonWindSwipesPurchasable");
			if (familiarTab) familiarTab->markPending();
		}

		// FamiliarTab's purchase buttons only recompute their own visibility when something
		// inside FamiliarTab triggers it (a slot/evolve purchase, a resize) - claiming an
		// achievement here never touches FamiliarTab at all, so without this the newly-
		// unlocked button wouldn't appear until the next resize or slot change. Unconditional
		// (not just for "Juvenile Flame") so any future achievement that unlocks a
		// FamiliarTab purchase button doesn't need its own reminder to add this call.
		if (familiarTab) familiarTab->refreshButtonVisibility();

		/*if (achievement.name == "Familiar Bond")
		{
			unlockManager->unlock();
		}*/

		// This is the single place to wire "claiming X unlocks feature Y" for both shop buttons and achievement chains:
		// if (achievement.name == "Big Spender") unlockManager->unlock("EliteSpenderUnlocked");
		// if (achievement.name == "Veteran Slayer") unlockManager->unlock("SomeShopUpgradeKey");
	};

	tabbedPanel->addTab("Statistics", new StatisticsTab(hero, statistics));
	optionsTab = new OptionsTab(this, this);
	tabbedPanel->addTab("Options", optionsTab);
	debugTab = new DebugTab(this);
	tabbedPanel->addTab("Debug", debugTab);

	hud = new HUD([this]() { return enemySpawner->getEnemyPosition(); }, unlockManager);
}

void Game::initItemManager()
{
	itemManager = new ItemManager();
}

void Game::initFamiliars()
{
	// Every species currently playable, each starting at its Baby stage - add one more entry here
	// whenever a new species gets its own familiarTypeList entries (see the big comment above
	// ownedFamiliars in Game.h for what else goes with it).
	const std::vector<FamiliarName> species = { FireDragon, WaterDragon, WindDragon };

	for (FamiliarName speciesName : species)
	{
		int typeIndex = findFamiliarTypeIndex(speciesName, Age::Baby);
		if (typeIndex < 0) continue; // this species' baby stage isn't defined in familiarTypeList (yet) - leave its slot nullptr rather than crash

		Familiar* f = new Familiar(familiarTypeList[typeIndex]);
		f->setOnDamageDealtCallback([this](BigNumber damage, ElementMask mask) // fires once per landed hit - melee-instant or projectile-arrival alike
			{
				enemySpawner->handleClick(damage, false, Familiar::dpsColor, mask); // familiar attacks (auto or manual) don't crit for now; colored to match activeFamiliarDPSText unless mask carries an element, in which case the element's gradient wins (see Enemy::showDamage)
			});
		f->setElementalMultiplierResolver([this](Elements element)
			{
				return enemySpawner->getEnemyElementalMultiplierForElement(element);
			});

		ownedFamiliars[static_cast<size_t>(speciesName)] = f;
	}

	familiar = ownedFamiliars[static_cast<size_t>(FireDragon)]; // see the member comment in Game.h - temporary alias until Fire's remaining special cases are migrated onto the roster
}

int Game::findFamiliarTypeIndex(FamiliarName name, Age age) const
{
	for (size_t i = 0; i < familiarTypeList.size(); i++)
	{
		if (familiarTypeList[i].name == name && familiarTypeList[i].age == age) return static_cast<int>(i);
	}
	return -1; // that (species, stage) pair isn't defined in familiarTypeList (yet) - e.g. an Ice dragon, or an Adult/Ancient stage, the current build doesn't have entries for
}

std::string Game::familiarSaveName(FamiliarName name)
{
	switch (name)
	{
		case FireDragon: return "FireDragon";
		case WaterDragon: return "WaterDragon";
		case WindDragon: return "WindDragon";
		case EarthDragon: return "EarthDragon";
		case IceDragon: return "IceDragon";
		default: return ""; // None, or anything else - callers treat an empty name as "no familiar"
	}
}

FamiliarName Game::familiarNameFromSaveName(const std::string& saveName)
{
	if (saveName == "FireDragon") return FireDragon;
	if (saveName == "WaterDragon") return WaterDragon;
	if (saveName == "WindDragon") return WindDragon;
	if (saveName == "EarthDragon") return EarthDragon;
	if (saveName == "IceDragon") return IceDragon;
	return None; // empty (no occupant), or a name this build doesn't recognize
}

int Game::getUnlockedFamiliarCount() const
{
	int count = 0;
	for (Familiar* f : ownedFamiliars)
	{
		if (f && f->isUnlocked()) count++;
	}
	return count;
}

int Game::getFamiliarEvolutionCount(const Familiar* f) const
{
	if (!f || !f->isUnlocked()) return 0;

	// Age (PublicVariables.h) is declared Baby, Juvenile, Adult, Ancient with no explicit numbering, so
	// the compiler assigns Baby=0, Juvenile=1, Adult=2 - which happens to already be exactly
	// "how many times this familiar has evolved". Reusing that ordinal here avoids adding a
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
	return static_cast<int>(f->getAge());
}

int Game::getTotalFamiliarEvolutionCount() const
{
	int total = 0;
	for (Familiar* f : ownedFamiliars)
	{
		total += getFamiliarEvolutionCount(f); // already 0 for a locked/null familiar, no need to check isUnlocked() again here
	}
	return total;
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

	bool anyFamiliarActive = std::any_of(familiarSlots.begin(), familiarSlots.end(), [](Familiar* f) { return f != nullptr; });
	if (anyFamiliarActive) // combined active-familiar DPS - same unlock gate as its visibility in render()
	{
		bounds = unionRects(bounds, activeFamiliarDPSText->getGlobalBounds());
	}

	hudStatPanel.setPosition({ bounds.position.x - hudStatPanelPadding, bounds.position.y - hudStatPanelPadding });
	hudStatPanel.setSize({ bounds.size.x + hudStatPanelPadding * 2.0f, bounds.size.y + hudStatPanelPadding * 2.0f });
}

void Game::initPassiveFamiliarDPSText()
{
	passiveFamiliarDPSText = new Text("Passive Familiar DPS: " + passiveFamiliarDPSTracker.getDPS().toString(), "Fonts/arial.ttf");
	// Pale lavender - source-identity color for "passive familiar damage", the third of the trio
	// alongside Hero::dpsText (pale gold) and Game::activeFamiliarDPSText (pale silver-blue). See
	// the comments there for why these are deliberately desaturated rather than vivid hues.
	passiveFamiliarDPSText->setFillColor(passiveDPSColor);
	passiveFamiliarDPSText->setOutlineColor(sf::Color::Black);
	passiveFamiliarDPSText->setOutlineThickness(2.0f);
	updatePassiveFamiliarDPSText();
}

void Game::updatePassiveFamiliarDPSText()
{
	passiveFamiliarDPSText->setString("Passive Familiar DPS: " + passiveFamiliarDPSTracker.getDPS().toString());
	// -120: sits below Game::activeFamiliarDPSText's -160 - passive damage unlocks as soon as any
	// familiar is unlocked, while Familiar DPS additionally requires that familiar to be slotted
	// (a later step), so this line appears first/lower in the stack.
	LayoutManager::applyAnchor(*passiveFamiliarDPSText, Anchor::BottomRight, { -10.0f, -120.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void Game::initActiveFamiliarDPSText()
{
	activeFamiliarDPSText = new Text("Familiar DPS: " + BigNumber(0).toString(), "Fonts/arial.ttf");
	// Pale silver-blue - same source-identity color every slotted familiar's own DPS text used to
	// use individually (Familiar::dpsColor) - now shared by the one combined line that replaces
	// all of them, so floating damage numbers on the enemy still match this HUD line's color.
	activeFamiliarDPSText->setFillColor(Familiar::dpsColor);
	activeFamiliarDPSText->setOutlineColor(sf::Color::Black);
	activeFamiliarDPSText->setOutlineThickness(2.0f);
	updateActiveFamiliarDPSText();
}

void Game::updateActiveFamiliarDPSText()
{
	BigNumber total(0);
	for (Familiar* activeFamiliar : familiarSlots)
	{
		if (activeFamiliar) total = total + activeFamiliar->getFamiliarDPS();
	}
	activeFamiliarDPSText->setString("Familiar DPS: " + total.toString());
	// -160: top of the BottomRight stat stack - this requires a familiar to be actively slotted
	// (not just unlocked), which happens later than everything below it, including
	// passiveFamiliarDPSText's passive DPS line at -120 (unlocks as soon as any familiar is
	// unlocked, no slotting needed).
	LayoutManager::applyAnchor(*activeFamiliarDPSText, Anchor::BottomRight, { -10.0f, -160.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
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

	// Looks the occupant's current (species, stage) pair up in familiarTypeList rather than
	// storing a separate display-name field anywhere.
	int typeIndex = findFamiliarTypeIndex(occupant->getName(), occupant->getAge());
	return typeIndex >= 0 ? familiarTypeList[typeIndex].label : "Familiar"; // fallback if its current stage somehow isn't in familiarTypeList
}

std::vector<Familiar*> Game::getAvailableFamiliarsForSlotting() const
{
	// Every owned (Fire/Water/Wind), unlocked, currently-unassigned familiar, in ownedFamiliars'
	// fixed lineage order. Both getAvailableFamiliarNamesForSlotting() (what FamiliarTab's picker
	// shows) and assignFamiliarToSlot() (what a picked row actually resolves to) derive from this
	// SAME list, so a pickedIndex the picker handed back always means the same familiar here -
	// they'd silently disagree if each rebuilt its own filtered list independently.
	std::vector<Familiar*> available;
	for (Familiar* f : ownedFamiliars)
	{
		if (f && f->isUnlocked() && !isFamiliarInAnySlot(f))
		{
			available.push_back(f);
		}
	}
	return available;
}

std::vector<std::string> Game::getAvailableFamiliarNamesForSlotting() const
{
	std::vector<std::string> names;
	for (Familiar* f : getAvailableFamiliarsForSlotting())
	{
		int typeIndex = findFamiliarTypeIndex(f->getName(), f->getAge());
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

	std::vector<Familiar*> available = getAvailableFamiliarsForSlotting();
	if (pickedIndex < 0 || pickedIndex >= static_cast<int>(available.size())) return;

	familiarSlots[slotIndex] = available[pickedIndex];
	repositionSlottedFamiliars();
}

void Game::unassignFamiliarSlot(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= static_cast<int>(familiarSlots.size())) return;
	familiarSlots[slotIndex] = nullptr; // the familiar itself is untouched (still unlocked, keeps its bonuses/evolution) - it just goes back to the available pool
	repositionSlottedFamiliars();
}

void Game::repositionSlottedFamiliars()
{
	int totalActive = 0;
	for (Familiar* f : familiarSlots)
	{
		if (f) totalActive++;
	}

	int row = 0;
	for (Familiar* f : familiarSlots)
	{
		if (!f) continue; // empty slots don't consume a row - occupants stay packed together rather than leaving gaps
		f->setSlotPosition(row, totalActive);
		row++;
	}
}

void Game::updateFamiliarFeatureGates()
{
	// "BabyFireDragonActive" is the requiredFeature on the "Juvenile Flame" achievement (see
	// AchievementManager::initAchievements()) - that achievement should stay out of the
	// Achievements tab entirely until the player actually has a live Baby Fire Dragon to care
	// about evolving, not just an unlocked-but-benched one. The isUnlocked() check up front is
	// just to skip the (cheap, but pointless) repeat unlock() calls once this is already set.
	if (!unlockManager->isUnlocked("BabyFireDragonActive") && familiar && familiar->isUnlocked() && isFamiliarInAnySlot(familiar))
	{
		unlockManager->unlock("BabyFireDragonActive");
	}

	// Same two-step gating for Water and Wind - see the matching "Juvenile Tide"/"Juvenile Gale"
	// achievements in AchievementManager.cpp and the onAchievementClaimed wiring in initUI().
	Familiar* water = ownedFamiliars[WaterDragon];
	if (!unlockManager->isUnlocked("BabyWaterDragonActive") && water && water->isUnlocked() && isFamiliarInAnySlot(water))
	{
		unlockManager->unlock("BabyWaterDragonActive");
	}

	Familiar* wind = ownedFamiliars[WindDragon];
	if (!unlockManager->isUnlocked("BabyWindDragonActive") && wind && wind->isUnlocked() && isFamiliarInAnySlot(wind))
	{
		unlockManager->unlock("BabyWindDragonActive");
	}
}

void Game::initAchievements()
{
	achievementManager = new AchievementManager(hero, statistics, familiar, itemManager, unlockManager,
		ownedFamiliars[WaterDragon], ownedFamiliars[WindDragon]);
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
	for (Familiar* activeFamiliar : familiarSlots) // every currently slotted familiar, not just Fire - unslotted ones aren't drawn, so they don't need repositioning
	{
		if (activeFamiliar) activeFamiliar->onResize();
	}
	enemySpawner->onResize();
	itemBar->onResize();
	tabbedPanel->onResize();
	// the y origin value is currently fixed! If the Tab Panel Height can change, it should be adjusted here as well
	debugTab->onResize({ windowSize.x - (tabPanelWidth), 40.0f }, { tabPanelWidth , windowSize.y });
	hud->onResize();
	ruler->onResize();
}

void Game::update()
{
	pollEvents();

	times.nextDeltaTime();

	float dt = times.getDeltaTime();

	if (hud->isPaused())
	{
		return; // game state frozen - render() still runs every frame and draws the last state, plus the HUD's "Paused" overlay
	}

	runSimulationTick(dt);
}

void Game::runSimulationTick(float dt)
{
	statistics->updatePlayTime(dt);
	updateFamiliarFeatureGates(); // before achievementManager->update(), so a requiredFeature it flips on can be seen by an achievement's condition() this same frame
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
	updateActiveFamiliarDPSText(); // combines every slotted familiar's own DPS into the one HUD line - must run after the loop above so each familiar's tracker is current for this frame
	updateFamiliarPassiveDamage(dt); // deals passive damage once per elapsed second, active as soon as a familiar is unlocked - independent of the slot loop above

	hud->sync(familiarSlots);
	hud->update();
	hero->updateSprites(dt);
	hero->updateCurrencyPulse(dt);
	hero->updateDPS(dt);

	updateHUDStatPanelLayout(); // must run after every stat text above (and the familiar/passive updates earlier) has refreshed for this frame, since it wraps their current bounds

	saveIndicator->update(dt);
}

void Game::stepFrameForward()
{
	if (!frameStepHistory) return; // only reachable while paused, which is when this is created - defensive only

	frameStepHistory->recordBeforeStep(hero, ownedFamiliars, enemySpawner);

	times.nextDeltaTime();
	float dt = times.getDeltaTime();
	runSimulationTick(dt);
}

void Game::stepFrameBackward()
{
	if (!frameStepHistory || !frameStepHistory->canStepBack()) return;

	frameStepHistory->stepBack(hero, ownedFamiliars, enemySpawner);
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
	hudElements.push_back({ [this]() { stage->drawHUD(window); } }); // stage name + current enemy/boss label - always on top, never occluded by a decoration
	// Drawn next so every stat-text element pushed after this paints on top of it - see the
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
	bool anyFamiliarActive = std::any_of(familiarSlots.begin(), familiarSlots.end(), [](Familiar* f) { return f != nullptr; });
	if (anyFamiliarActive) // one combined "Familiar DPS" line for every slotted familiar, instead of each one drawing its own overlapping text
	{
		hudElements.push_back({ [this]() { window->draw(*activeFamiliarDPSText); } });
	}
	if (enemySpawner->hasEnemy()) hudElements.push_back({ [this]() { enemySpawner->drawHUD(window); } }); // health bar, boss timer, floating damage numbers - always on top, never occluded by a decoration

	stage->drawMap(window, actors, hudElements);
	itemBar->render(window);
	lootDropManager->render(window); // drawn after itemBar so a flying item stays visible on top of the bar all the way to its slot, instead of disappearing behind the icon
	tabbedPanel->render(window);
	saveIndicator->render(window); // last of all, on top of the UI panel too
	if (ruler->isVisible())
	{
		ruler->render(window);
		if (hud->isPaused())
		{
			sf::Vector2f mouseWorldPos = window->mapPixelToCoords(sf::Mouse::getPosition(*window));
			ruler->renderCrosshair(window, mouseWorldPos);
		}
	}

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
	for (Familiar* activeFamiliar : familiarSlots) // every currently slotted familiar, not just Fire - unslotted ones aren't drawn, so they don't need repositioning
	{
		if (activeFamiliar) activeFamiliar->onResize();
	}
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

	initFamiliars(); // builds one locked Familiar per species (Fire/Water/Wind) - see Game.h's ownedFamiliars comment
	passiveFamiliarDPSTracker.setWindowDuration(passiveDamageTickInterval); // keep the DPS window matched to the tick interval - see the member comment in Game.h
	initHUDStatPanel();
	initPassiveFamiliarDPSText();
	initActiveFamiliarDPSText();
	initStatistics();
	initItemManager(); // needs to be called before initEnemySpawner()/initUI()
	initUnlockManager(); // before initAchievements()
	initAchievements(); // needs to be called before initEnemySpawner() and after initHero() and initStatistics()
	initEnemySpawner();
	initUI();
	initSaveIndicator();
	ruler = new Ruler();

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
	for (Familiar* f : ownedFamiliars) delete f; // deletes Fire/Water/Wind - `familiar` is just an alias into this same array, not deleted separately
	delete unlockManager;
	delete passiveFamiliarDPSText;
	delete activeFamiliarDPSText;
	delete hud;
	delete frameStepHistory;
	delete ruler;
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

void Game::debugAdjustAllItems(int delta)
{
	itemManager->adjustAllInventoryCounts(delta);
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
		// Actual current window pixel size - separate from resolutionIndex above, which only
		// tracks which PRESET was last explicitly chosen (Options tab dropdown). A manual
		// drag-resize or maximize/restore never touches resolutionIndex (there's no matching
		// preset for an arbitrary size), so without this the window would silently snap back to
		// its last preset size on the next load instead of staying exactly how it was left.
		ofs << "windowWidth=" << windowSize.x << '\n';
		ofs << "windowHeight=" << windowSize.y << '\n';
		// Window position on screen - skipped for Fullscreen, same reasoning as windowWidth/Height
		// above (no border to drag, and the OS always places a fullscreen window at (0,0) anyway).
		if (currentWindowMode != WindowMode::Fullscreen)
		{
			sf::Vector2i windowPos = window->getPosition();
			ofs << "windowPosX=" << windowPos.x << '\n';
			ofs << "windowPosY=" << windowPos.y << '\n';
		}
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

		// Familiars - one [Familiar:X] section per species (X = "FireDragon"/"WaterDragon"/...,
		// see familiarSaveName()), identical fields in each. No "name=" field is needed - the
		// section header already names the species unambiguously, and ownedFamiliars is indexed
		// by species directly, so there's nothing left to disambiguate. Only the purchased BONUS
		// stats and current `age` (evolution stage) are saved - base stats are fully determined by
		// (species, age) via familiarTypeList, so they never need saving, and evolving no longer
		// invalidates anything that was saved (see Familiar::evolveTo).
		for (size_t i = 0; i < ownedFamiliars.size(); i++)
		{
			Familiar* f = ownedFamiliars[i];
			if (!f) continue;

			ofs << "[Familiar:" << familiarSaveName(static_cast<FamiliarName>(i)) << "]\n";
			ofs << "unlocked=" << (f->isUnlocked() ? 1 : 0) << '\n';
			ofs << "age=" << static_cast<int>(f->getAge()) << '\n';
			ofs << "bonusPhysicalPower=" << f->getBonusPhysicalPower().toApproxDouble() << '\n';
			for (int e = 0; e < static_cast<int>(ElementCount); e++)
			{
				ofs << "bonusElementalPower" << e << "=" << f->getBonusElementalPower(static_cast<Elements>(e)).toApproxDouble() << '\n';
			}
			ofs << "bonusCooldownReduction=" << f->getBonusCooldownReduction() << '\n';
		}

		// Familiar slots - separate from [Familiar:X] above, since this is about WHERE a familiar
		// is placed, not its own stats/evolution. Each slot stores WHICH species occupies it (its
		// familiarSaveName(), or an empty value for an empty slot) rather than a bare occupied
		// bool, so any owned species - not just Fire - can be restored to its slot.
		ofs << "[FamiliarSlots]\n";
		ofs << "slotCount=" << getFamiliarSlotCount() << '\n';
		if (familiarTab) ofs << "slotCost=" << familiarTab->getSlotCost() << '\n'; // the escalating Amber cost of the NEXT slot - without this, reloading would always offer Slot N+1 at the Slot 1 price
		for (int i = 0; i < getFamiliarSlotCount(); i++)
		{
			Familiar* occupant = familiarSlots[i];
			std::string occupantName = occupant ? familiarSaveName(occupant->getName()) : "";
			ofs << "occupant" << i << "=" << occupantName << '\n';
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
			ofs << "goldMultiplierIncreaseAmount=" << shopTab->getGoldMultiplierIncreaseAmount() << '\n';
			ofs << "goldMultiplierUpgradesPurchased=" << shopTab->getGoldMultiplierUpgradesPurchased() << '\n';
		}
		// Every lineage's three familiar-upgrade costs, moved to FamiliarTab along with their
		// buttons (see Step 3). Fire's keep their original key names so an old save's Fire prices
		// still load correctly; Water/Wind get the same shape under their own lineage-prefixed keys.
		if (familiarTab)
		{
			ofs << "familiarPhysicalCost=" << familiarTab->getFirePhysicalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarElementalCost=" << familiarTab->getFireElementalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarCooldownCost=" << familiarTab->getFireCooldownUpgradeCost().toApproxDouble() << '\n';

			ofs << "familiarWaterPhysicalCost=" << familiarTab->getWaterPhysicalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarWaterElementalCost=" << familiarTab->getWaterElementalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarWaterCooldownCost=" << familiarTab->getWaterCooldownUpgradeCost().toApproxDouble() << '\n';

			ofs << "familiarWindPhysicalCost=" << familiarTab->getWindPhysicalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarWindElementalCost=" << familiarTab->getWindElementalUpgradeCost().toApproxDouble() << '\n';
			ofs << "familiarWindCooldownCost=" << familiarTab->getWindCooldownUpgradeCost().toApproxDouble() << '\n';
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
		// Defaults to the window's current size, so an older save file without these keys (from
		// before manual resize/maximize was tracked) leaves it untouched - same "absent key changes
		// nothing" idea as every other value in this function.
		double fileWindowWidth = windowSize.x;
		double fileWindowHeight = windowSize.y;
		// No sensible "current position" default exists here the same way size/mode have one -
		// (0,0) is a legitimate real position (and a negative one too, on a multi-monitor setup
		// with a monitor to the left of the primary), so presence is tracked explicitly instead of
		// inferring it from the value itself. Left unmoved (not even recentered) unless BOTH are found.
		bool fileHasWindowPosX = false;
		bool fileHasWindowPosY = false;
		int fileWindowPosX = 0;
		int fileWindowPosY = 0;
		bool fileDeathAnimationsEnabled = deathAnimationsEnabled;
		int fileStageNumber = stage->getStageNumber();

		double heroCurrency = 0.0;
		double heroAttack = 0.0;
		double heroGoldMult = 1.0;
		bool heroCritUnlocked = false;
		double heroCritChance = 0.0;
		double heroCritDmgMult = 1.5;

		// Defaults to current values, so an older save (or one missing a species' section
		// entirely) leaves that species untouched - same "absent data changes nothing" idea the
		// rest of this function already uses. Indexed directly by FamiliarName, same as
		// ownedFamiliars - no separate `name` field is needed here, unlike before this species/
		// stage split: which species a given entry means is now just its own array index.
		struct SavedFamiliarData
		{
			bool unlocked = false;
			Age age = Age::Baby;
			double bonusPhysicalPower = 0.0;
			std::array<double, ElementCount> bonusElementalPower{};
			float bonusCooldownReduction = 0.0f;
		};
		std::array<SavedFamiliarData, static_cast<size_t>(FamiliarNameCount)> savedFamiliars;
		for (size_t i = 0; i < ownedFamiliars.size(); i++)
		{
			Familiar* f = ownedFamiliars[i];
			if (!f) continue;

			savedFamiliars[i].unlocked = f->isUnlocked();
			savedFamiliars[i].age = f->getAge();
			savedFamiliars[i].bonusPhysicalPower = f->getBonusPhysicalPower().toApproxDouble();
			for (int e = 0; e < static_cast<int>(ElementCount); e++)
			{
				savedFamiliars[i].bonusElementalPower[e] = f->getBonusElementalPower(static_cast<Elements>(e)).toApproxDouble();
			}
			savedFamiliars[i].bonusCooldownReduction = f->getBonusCooldownReduction();
		}

		// Defaults to empty (0 slots), so older save files without a [FamiliarSlots] section
		// just leave the player with no slots - same "absent section changes nothing new" idea
		// as the other defaults above, since 0 also happens to be this feature's starting state.
		// Each entry is "" (empty slot) or a familiarSaveName() identifying the occupant species.
		std::vector<std::string> fileFamiliarSlotOccupant;

		// Defaults to the current value, so an older save file without this key (from before slot
		// cost escalated) leaves it untouched - same "absent key changes nothing" idea as every
		// other price default in this function.
		int fileSlotCost = familiarTab->getSlotCost();

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
		// Fire's three upgrade costs moved to FamiliarTab (see Step 3) - still defaulted to
		// their current value here so an older save file without these keys leaves them untouched,
		// same reasoning as every other price above.
		double shopFamiliarPhysicalAttackCost = familiarTab->getFirePhysicalUpgradeCost().toApproxDouble();
		double shopFamiliarElementalAttackCost = familiarTab->getFireElementalUpgradeCost().toApproxDouble();
		double shopFamiliarCooldownCost = familiarTab->getFireCooldownUpgradeCost().toApproxDouble();
		double shopFamiliarWaterPhysicalCost = familiarTab->getWaterPhysicalUpgradeCost().toApproxDouble();
		double shopFamiliarWaterElementalCost = familiarTab->getWaterElementalUpgradeCost().toApproxDouble();
		double shopFamiliarWaterCooldownCost = familiarTab->getWaterCooldownUpgradeCost().toApproxDouble();
		double shopFamiliarWindPhysicalCost = familiarTab->getWindPhysicalUpgradeCost().toApproxDouble();
		double shopFamiliarWindElementalCost = familiarTab->getWindElementalUpgradeCost().toApproxDouble();
		double shopFamiliarWindCooldownCost = familiarTab->getWindCooldownUpgradeCost().toApproxDouble();
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
				else if (key == "windowWidth") fileWindowWidth = std::stod(value);
				else if (key == "windowHeight") fileWindowHeight = std::stod(value);
				else if (key == "windowPosX") { fileWindowPosX = std::stoi(value); fileHasWindowPosX = true; }
				else if (key == "windowPosY") { fileWindowPosY = std::stoi(value); fileHasWindowPosY = true; }
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
			else if (section.rfind("Familiar:", 0) == 0) // e.g. "Familiar:FireDragon", "Familiar:WaterDragon", "Familiar:WindDragon"
			{
				FamiliarName species = familiarNameFromSaveName(section.substr(std::string("Familiar:").size()));
				if (species != None) // recognized species - anything else (a future species an older build wrote) is silently skipped, same "leave it untouched" spirit as everything else here
				{
					SavedFamiliarData& saved = savedFamiliars[static_cast<size_t>(species)];
					if (key == "unlocked") saved.unlocked = (std::stoi(value) != 0);
					else if (key == "age") saved.age = static_cast<Age>(std::stoi(value));
					else if (key == "bonusPhysicalPower") saved.bonusPhysicalPower = std::stod(value);
					else if (key == "bonusCooldownReduction") saved.bonusCooldownReduction = std::stof(value);
					else if (key.rfind("bonusElementalPower", 0) == 0) // e.g. "bonusElementalPower0", "bonusElementalPower1", ...
					{
						int elementIndex = std::stoi(key.substr(std::string("bonusElementalPower").size()));
						if (elementIndex >= 0 && elementIndex < static_cast<int>(ElementCount))
						{
							saved.bonusElementalPower[elementIndex] = std::stod(value);
						}
					}
				}
			}
			else if (section == "FamiliarSlots")
			{
				if (key == "slotCount")
				{
					int cnt = std::stoi(value);
					fileFamiliarSlotOccupant.assign(cnt, "");
				}
				else if (key == "slotCost") fileSlotCost = std::stoi(value);
				else if (key.rfind("occupant", 0) == 0) // e.g. "occupant0", "occupant1", ...
				{
					int idx = std::stoi(key.substr(std::string("occupant").size()));
					if (idx >= 0 && idx < static_cast<int>(fileFamiliarSlotOccupant.size()))
						fileFamiliarSlotOccupant[idx] = value;
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
				else if (key == "familiarWaterPhysicalCost") shopFamiliarWaterPhysicalCost = std::stod(value);
				else if (key == "familiarWaterElementalCost") shopFamiliarWaterElementalCost = std::stod(value);
				else if (key == "familiarWaterCooldownCost") shopFamiliarWaterCooldownCost = std::stod(value);
				else if (key == "familiarWindPhysicalCost") shopFamiliarWindPhysicalCost = std::stod(value);
				else if (key == "familiarWindElementalCost") shopFamiliarWindElementalCost = std::stod(value);
				else if (key == "familiarWindCooldownCost") shopFamiliarWindCooldownCost = std::stod(value);
				else if (key == "goldMultiplierIncreaseAmount") shopGoldMultiplierIncreaseAmount = std::stod(value);
				else if (key == "goldMultiplierUpgradesPurchased") shopGoldMultiplierUpgradesPurchased = std::stoi(value);
			}
		}

		std::cout << "Game state loaded from " << path << std::endl; // DEBUG

		// Apply loaded values where possible
		// Window settings
		setWindowMode(static_cast<WindowMode>(fileWindowMode));
		setResolutionIndex(fileResolutionIndex); // both calls above already (re)created the native window at a PRESET size

		// Then, if the saved size doesn't just match that preset (i.e. the window was manually
		// resized or maximized/restored since), snap it to the exact saved pixel size on top -
		// skipped for Fullscreen, which always fills the desktop resolution and has no window
		// border to drag/maximize in the first place.
		if (currentWindowMode != WindowMode::Fullscreen && fileWindowWidth > 0.0 && fileWindowHeight > 0.0)
		{
			sf::Vector2u savedWindowSize(static_cast<unsigned int>(fileWindowWidth), static_cast<unsigned int>(fileWindowHeight));
			window->setSize(savedWindowSize);
			onWindowResized(savedWindowSize); // recomputes windowSize/layout to match, same as a live drag-resize or maximize/restore would
		}

		// Window position - independent of the size restore above (an old save might have one
		// without the other), same Fullscreen exclusion.
		if (currentWindowMode != WindowMode::Fullscreen && fileHasWindowPosX && fileHasWindowPosY)
		{
			window->setPosition(sf::Vector2i(fileWindowPosX, fileWindowPosY));
		}

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

		// Familiars: restore each species' unlock/bonus-stat/evolution state in place. Every
		// ownedFamiliars[i] was already constructed back in Game() (so ShopTab/FamiliarTab/
		// AchievementManager/its callbacks could all be wired to it) - so if the save says it
		// should be further along, we unlock/evolve that SAME instance rather than replacing it.
		// We deliberately do NOT call initFamiliars() again here: that would allocate brand new
		// Familiar objects, leak the originals (nothing currently calls delete on the old
		// pointers before overwriting them), and leave every already-wired pointer still pointing
		// at the now-orphaned originals instead of the reloaded ones.
		bool anyFireJustUnlocked = false;
		for (size_t i = 0; i < ownedFamiliars.size(); i++)
		{
			Familiar* f = ownedFamiliars[i];
			if (!f) continue;
			const SavedFamiliarData& saved = savedFamiliars[i];

			if (saved.unlocked)
			{
				if (!f->isUnlocked())
				{
					f->unlock();
					if (static_cast<FamiliarName>(i) == FireDragon) anyFireJustUnlocked = true; // ShopTab's unlock button needs a direct nudge below - it's the one unlock button not already covered by familiarTab->refreshButtonVisibility()
				}

				// Restored every time the save says this familiar is unlocked - not just on the
				// locked->unlocked transition above - so loading a different/older save file mid-session
				// (e.g. via OptionsTab's Load button) also overwrites whatever bonus stats this familiar
				// already has from the current session, instead of silently keeping them.
				f->setBonusPhysicalPower(BigNumber(saved.bonusPhysicalPower));
				for (int e = 0; e < static_cast<int>(ElementCount); e++)
				{
					f->setBonusElementalPower(static_cast<Elements>(e), BigNumber(saved.bonusElementalPower[e]));
				}
				f->setBonusCooldownReduction(saved.bonusCooldownReduction);
			}

			if (f->getAge() != saved.age)
			{
				int typeIndex = findFamiliarTypeIndex(f->getName(), saved.age);
				if (typeIndex >= 0)
				{
					f->evolveTo(familiarTypeList[typeIndex]);
				}
				// else: this save was written by a build with a stage (e.g. Adult/Ancient) that
				// isn't in familiarTypeList yet - leave it at its current stage rather than
				// silently doing nothing or indexing out of range.
			}
		}

		if (anyFireJustUnlocked) shopTab->refreshVisibility(); // replaces setFamiliarButtonActive(false) / setFamiliarButtonsActive(true)

		// Every restored familiar above bypassed its own buttons' click handlers entirely, so
		// their displayed labels/visibility are still showing whatever was current before
		// loading - refresh all of them in one pass. Each of these three already loops over
		// every species' buttons (see Steps 3/4), so this covers Fire/Water/Wind together.
		if (familiarTab)
		{
			familiarTab->refreshEvolveButtonLabels();
			familiarTab->refreshUpgradeButtonLabels();
			familiarTab->refreshButtonVisibility();
		}

		// Restore familiar slots. Each saved slot names WHICH species occupies it (see
		// saveToFile()), so this must run after the unlock restoration just above, since it
		// depends on isUnlocked() already being true for whichever species it restores.
		familiarSlots.assign(fileFamiliarSlotOccupant.size(), nullptr);
		for (size_t i = 0; i < fileFamiliarSlotOccupant.size(); i++)
		{
			FamiliarName species = familiarNameFromSaveName(fileFamiliarSlotOccupant[i]);
			if (species == None) continue; // empty slot, or a species this build doesn't recognize

			Familiar* occupant = ownedFamiliars[static_cast<size_t>(species)];
			if (occupant && occupant->isUnlocked())
			{
				familiarSlots[i] = occupant;
			}
		}
		repositionSlottedFamiliars(); // this bypassed assignFamiliarToSlot() entirely, so nothing else positioned the restored occupant(s) on screen
		if (familiarTab)
		{
			familiarTab->setSlotCost(fileSlotCost); // restore the escalated price before refreshing the button label below
			familiarTab->refreshSlotButtons(); // slot state just changed outside FamiliarTab's own click handlers - make sure its displayed list catches up
		}

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
			shopTab->setGoldMultiplierIncreaseAmount(shopGoldMultiplierIncreaseAmount);
			shopTab->setGoldMultiplierUpgradesPurchased(shopGoldMultiplierUpgradesPurchased);
			shopTab->refreshLabels();
		}
		// Every lineage's three upgrade costs, moved to FamiliarTab (see Step 3). Labels/
		// visibility are already refreshed by the familiar-restore block above, so no need to
		// repeat that here - just write the costs themselves.
		if (familiarTab)
		{
			familiarTab->setFirePhysicalUpgradeCost(BigNumber(shopFamiliarPhysicalAttackCost));
			familiarTab->setFireElementalUpgradeCost(BigNumber(shopFamiliarElementalAttackCost));
			familiarTab->setFireCooldownUpgradeCost(BigNumber(shopFamiliarCooldownCost));

			familiarTab->setWaterPhysicalUpgradeCost(BigNumber(shopFamiliarWaterPhysicalCost));
			familiarTab->setWaterElementalUpgradeCost(BigNumber(shopFamiliarWaterElementalCost));
			familiarTab->setWaterCooldownUpgradeCost(BigNumber(shopFamiliarWaterCooldownCost));

			familiarTab->setWindPhysicalUpgradeCost(BigNumber(shopFamiliarWindPhysicalCost));
			familiarTab->setWindElementalUpgradeCost(BigNumber(shopFamiliarWindElementalCost));
			familiarTab->setWindCooldownUpgradeCost(BigNumber(shopFamiliarWindCooldownCost));

			familiarTab->refreshUpgradeButtonLabels(); // the buttons were showing their starting costs - make them show the just-restored ones
			familiarTab->refreshButtonVisibility();    // repacks the layout in case a restored cooldown cost means a button should now be hidden (cooldown already at its floor)
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