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
#include "LootDropManager.h"
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
#include "DPSTracker.h"
#include "Text.h"
#include "HUD.h"


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
						BigNumber(1500),                                                                                         // basePhysicalPower - higher than the baby stage's, since this stage's base stats are stronger
						false,                                                                                                   // usesElementalDamage
						ElementalPower(),                                                                                        // baseElementalPower - unused, claws carry no element
						false                                                                                                    // isRanged - melee, so every projectile field below stays at its default
					},
					FamiliarAttack{
						"Fireball",                                                                                              // name
						"attack2",                                                                                               // animationKey
						"Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_attack2.png",                       // animationAddress
						0.10f,                                                                                                   // animationFrameTimerMax
						10.0f,                                                                                                   // baseCooldown
						0.5f,                                                                                                    // minCooldownLimit - unused, bonus cooldown reduction never applies to a manual ability
						false,                                                                                                   // isAutoAttack - manually activated
						false,                                                                                                   // usesPhysicalDamage
						BigNumber(0),                                                                                            // basePhysicalPower - unused
						true,                                                                                                    // usesElementalDamage
						ElementalPower().set(Fire, BigNumber(5000)),                                                             // baseElementalPower
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
	LootDropManager* lootDropManager; // purely visual: animates gold/item drops from enemy -> ground -> HUD target; the actual reward is granted through its collected-callbacks, wired in initLootDropManager()

	OptionsTab* optionsTab = nullptr; // non-owning - owned by tabbedPanel, kept here so saveToFile() can update the load button
	ShopTab* shopTab = nullptr; // non-owning - owned by tabbedPanel, kept here so save/load can persist shop prices
	Familiar* familiar = nullptr; // owned by Game (unlike shopTab/optionsTab, not a TabContent)
	DebugTab* debugTab = nullptr;
	AchievementsTab* achievementsTab = nullptr; // non-owning - owned by tabbedPanel, kept here to wire the unlock->blink callback
	FamiliarTab* familiarTab = nullptr; // non-owning
	UnlockManager* unlockManager;
	HUD* hud = nullptr;

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
	int findFamiliarTypeIndex(FamiliarName name) const; // index into familiarTypeList for the given stage, or -1 if that stage isn't defined there (yet) - used by loadFromFile() to restore the familiar's saved evolution stage via Familiar::evolveTo(), without reconstructing the Familiar object itself (which would orphan every pointer already handed out to it - ShopTab, AchievementManager, its damage/resolver callbacks)

	// --- Familiar passive damage ---
	// Once at least one familiar is unlocked, it deals passive damage automatically every second,
	// completely independent of familiar slots (see the block below) - a familiar contributes this
	// passive damage whether or not it's ever assigned to an active slot.
	// The three constants below are deliberately tunable members (not hardcoded) since the exact
	// numbers are still being balanced; see computeFamiliarPassiveDamagePerSecond() for the formula.
	float familiarPassiveBaseDamagePerSecond = 10.0f;        // damage/sec granted once the very first familiar is unlocked
	float familiarPassiveDamagePerAdditionalFamiliar = 1.5f; // +150%, compounded once per ADDITIONAL familiar unlocked (evolutions don't count here)
	float familiarPassiveDamagePerEvolution = 0.5f;          // +50%, compounded once per evolution stage, summed across every unlocked familiar
	float passiveDamageTickTimer = 0.0f;                     // seconds accumulated since the last passive damage tick
	static constexpr float passiveDamageTickInterval = 1.0f; // passive damage is dealt in one lump sum every this-many seconds ("10 damage every second")
	DPSTracker passiveFamiliarDPSTracker; // same pattern as Hero::heroDPSTracker/Familiar::passiveFamiliarDPSTracker - its windowDuration is kept in sync with passiveDamageTickInterval in Game::Game(), so one lump-sum tick per window reads back as a steady DPS instead of spiking then dropping to 0
	Text* passiveFamiliarDPSText = nullptr; // "Passive Familiar DPS" HUD line - owned directly by Game (unlike Hero's/Familiar's own DPS text) since this stat belongs to the roster as a whole, not to any single drawable object
	void initPassiveFamiliarDPSText(); // creates passiveFamiliarDPSText; called once from Game::Game()
	void updatePassiveFamiliarDPSText(); // refreshes passiveFamiliarDPSText's string + position; called every frame from updateFamiliarPassiveDamage()

	// Dusty rose/pink - passive familiar damage's source-identity color. Originally pale lavender
	// (205,195,225), but that sat too close in hue to Familiar::dpsColor's silver-blue
	// (190,210,235) - both cool, both similarly light, ~25 degrees apart on the wheel - so at
	// small sizes with a black outline the two floating-number colors were nearly indistinguishable.
	// Pink is ~120+ degrees from blue, staying just as pale/desaturated but far enough apart to
	// read clearly even against the same background, and sidesteps blue/purple confusion under
	// some color vision deficiencies (tritanopia) that a smaller lavender tweak wouldn't have.
	// Used both for passiveFamiliarDPSText and passed to EnemySpawner::handleClick() whenever a
	// passive tick lands, so its floating damage numbers on the enemy match its own HUD line (see
	// Familiar::dpsColor for the equivalent on the familiar's own auto-attack/ability damage).
	static constexpr sf::Color passiveDPSColor = sf::Color(230, 165, 195);

	// Semi-transparent backing panel behind the whole bottom-right stat-text stack (currency,
	// Hero DPS, click damage, familiar DPS, passive DPS). The three source-identity DPS colors
	// above are deliberately pale/desaturated so they don't collide with future vivid elemental
	// colors, but that same paleness reads poorly against some future biomes (rocky mountain
	// grays especially) with nothing behind it - this panel guarantees a consistent dark backdrop
	// regardless of what the game world's art looks like there, rather than hand-tuning per biome.
	// Sized dynamically every frame (see updateHUDStatPanelLayout()) around whatever text is
	// actually visible right now, rather than a fixed size - the stack can be as short as 3 lines
	// (no familiar unlocked yet) or as long as 5 (unlocked, evolved, slotted), and "Gold: 1.46Mil"
	// is a lot wider than "Gold: 12" - a fixed box would either clip long text or float around short text.
	sf::RectangleShape hudStatPanel;
	static constexpr float hudStatPanelPadding = 8.0f; // gap between the panel's edge and the text bounds it wraps
	void initHUDStatPanel();         // creates hudStatPanel; called once from Game::Game()
	void updateHUDStatPanelLayout(); // resizes/repositions hudStatPanel to wrap whatever stat text is currently visible - called every frame from update(), after all of that text has been refreshed for the frame

	// TODO: once multiple familiars exist, both of these should iterate over every OWNED familiar
	// (unlocked, regardless of whether it's currently placed in an active slot) instead of just
	// the single `familiar` member - a slotted-out familiar still contributes passively.
	int getUnlockedFamiliarCount() const;       // how many familiars are currently unlocked, in total (today: 0 or 1)
	int getFamiliarEvolutionCount(const Familiar* f) const; // how many times `f` has evolved, derived from its current stage's Age (see .cpp for the Age->count reasoning and its caveats) - no separate counter stored anywhere
	int getTotalFamiliarEvolutionCount() const; // sum of getFamiliarEvolutionCount() across every unlocked familiar

	double computeFamiliarPassiveDamagePerSecond() const; // combines the constants above with the current roster into one damage/sec figure
	void updateFamiliarPassiveDamage(float dt);           // ticks passiveDamageTickTimer and deals passive damage once per elapsed second

	// --- Familiar slots ---
	// Unlocking a familiar (ShopTab) now only adds it to the player's roster - it no longer
	// appears in the play area or auto-attacks on its own. Only a familiar currently ASSIGNED to
	// one of these slots is actually drawn/updated (auto-attacks, abilities, HUD). Passive damage
	// above is deliberately unaffected by any of this and keeps counting every owned familiar.
	// Slots themselves are bought via FamiliarTab's existing buySlotButton (regular item emblems
	// for now, per FamiliarTab.h's TODO about a dedicated item later).
	std::vector<Familiar*> familiarSlots; // size == number of unlocked slots; nullptr = empty, non-null = points at an owned familiar (today: only ever `familiar` - see the TODOs above)

	void onFamiliarSlotPurchased();                                // adds one new, empty slot
	int getFamiliarSlotCount() const;
	std::string getFamiliarSlotLabel(int slotIndex) const;         // occupant's display label (from familiarTypeList, like getFamiliarSlotLabel()'s sibling helpers above), or "" if that slot is empty
	std::vector<std::string> getAvailableFamiliarNamesForSlotting() const; // display label of every owned, unlocked, currently-unassigned familiar - in the order assignFamiliarToSlot()'s pickedIndex expects
	void assignFamiliarToSlot(int slotIndex, int pickedIndex);     // pickedIndex indexes into getAvailableFamiliarNamesForSlotting(); no-op if either index is invalid or the slot's already occupied
	void unassignFamiliarSlot(int slotIndex);
	bool isFamiliarInAnySlot(const Familiar* f) const;

	void initUI();
	void initAchievements();
	void initSaveIndicator();
	void initUnlockManager();
	void initLootDropManager(); // constructs lootDropManager and wires its collected-callbacks to Hero/ItemManager - must run after initHero()/initUI()/initItemManager()

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

	BigNumber getFamiliarPassiveDPS() const; // public so a future HUD/tab element can display it, same as Hero::getHeroDPS()/Familiar::getFamiliarDPS()

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