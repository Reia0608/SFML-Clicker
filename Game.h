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
#include <array>


class Game : public IWindowController, public ISaveable, public IDebugController
{
private:
	// The player owns at most one Familiar object per SPECIES (FamiliarName) - evolveTo() mutates
	// that SAME object in place as it advances through Age stages (see Familiar::evolveTo()),
	// rather than replacing it, so one object per species is enough no matter how many stages it
	// has. ownedFamiliars (below) is indexed directly by FamiliarName/FamiliarNameCount - adding a
	// new species is just adding it to that enum (PublicVariables.h) and its FamiliarType entries
	// to familiarTypeList just below; ownedFamiliars resizes itself automatically.
	// This already fits the planned fusion system too: fusing consumes two owned species and
	// grants a new one (e.g. Time + Darkness -> Chaos), and a lost/fused-away species has to be
	// found again (as a wild egg) before it can be owned again - so the player can never own two
	// instances of the same species at once, which is exactly what "one slot per species" assumes.

	// List of all available familiar types, for later use in the ShopTab and FamiliarTab
	std::vector<FamiliarType> familiarTypeList =
	{
		// Baby Fire Dragon: a single ranged auto-attack mixing physical + fire damage into one fireball
		FamiliarType(FireDragon, "Baby Fire Dragon", "Fire", Age::Baby,
		"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_idle.png", 0.15f, {96, 96},
		{
			FamiliarAttack{
				.name = "Fireball",
				.animationKey = "attack1",
				.animationAddress = "Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_red_attack1.png",
				.animationFrameTimerMax = 0.10f,
				.baseCooldown = 3.0f,
				.minCooldownLimit = 0.5f,
				.isAutoAttack = true,
				.usesPhysicalDamage = true,
				.basePhysicalPower = BigNumber(5),
				.usesElementalDamage = true,
				.baseElementalPower = ElementalPower().set(Fire, BigNumber(3)),
				.isRanged = true,
				.projectileAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/red/fire_bolt_small/fire_bolt_small.png",
				.projectileSpriteSize = {60, 60},
				.projectileFrameTimerMax = 0.10f,
				.projectileFrameCount = 6, // sheet is 360x60px = 6 frames of 60x60
				.projectileScale = 2.5f,
				.projectileTravelDuration = 1.8f,
				.activeFrame = 5,
				.projectileSpawnOffset = {60.0f, -4.0f}
			}
		}),

			// Fire Dragon: melee physical-only auto-attack (claws, no fireball) + a manually-activated big fireball, pure fire damage
			FamiliarType(FireDragon, "Fire Dragon", "Fire", Age::Juvenile, // same species (FireDragon) as the Baby entry above - Age::Juvenile is what distinguishes this stage
			"Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_idle.png", 0.15f, {240, 224},
			{
				FamiliarAttack{
					.name = "Claw Swipe",
					.animationKey = "attack1",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_attack1.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 7.0f,
					.minCooldownLimit = 1.0f,
					.isAutoAttack = true,
					.usesPhysicalDamage = true,
					.basePhysicalPower = BigNumber(1500), // higher than the baby stage's, since this stage's base stats are stronger
					.usesElementalDamage = false,
					.baseElementalPower = ElementalPower(), // unused, claws carry no element
					.isRanged = false, // melee, so every projectile field above stays at its default
					.activeFrame = 7 // damage lands on frame 7, not on activation
				},
				FamiliarAttack{
					.name = "Fireball",
					.animationKey = "attack2",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/FireDragon_red_attack2.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 10.0f,
					.minCooldownLimit = 0.5f, // unused, bonus cooldown reduction never applies to a manual ability
					.isAutoAttack = false, // manually activated
					.usesPhysicalDamage = false,
					.basePhysicalPower = BigNumber(0), // unused
					.usesElementalDamage = true,
					.baseElementalPower = ElementalPower().set(Fire, BigNumber(5000)),
					.isRanged = true,
					.projectileAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/red/fire_bolt_small/fire_bolt_small.png",
					.projectileSpriteSize = {60, 60},
					.projectileFrameTimerMax = 0.10f,
					.projectileFrameCount = 6,
					.projectileScale = 5.0f, // bigger than the auto-attack fireball used to be, it's the "big fireball" now
					.projectileTravelDuration = 1.8f,
					.activeFrame = 6,
					.projectileSpawnOffset = {244.0f, 16.0f},
					.unlockFeatureKey = "FireDragonFireballUnlocked" // gated: hidden from the HUD until FamiliarTab's purchase button sets this (see FamiliarTab.cpp, and the "Juvenile Flame" achievement in AchievementManager.cpp that unlocks that purchase button in the first place)
				}
			}),

			// Baby Water Dragon: a single ranged auto-attack that consists of water damage into one fireball
			FamiliarType(WaterDragon, "Baby Water Dragon", "Water", Age::Baby,
			"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_blue_idle.png", 0.15f, {96, 96},
			{
				FamiliarAttack{
					.name = "Water Bubbles",
					.animationKey = "attack1",
					.animationAddress = "Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_blue_attack1.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 3.0f,
					.minCooldownLimit = 0.5f,
					.isAutoAttack = true,
					.usesPhysicalDamage = false,
					.basePhysicalPower = BigNumber(),
					.usesElementalDamage = true,
					.baseElementalPower = ElementalPower().set(Water, BigNumber(1)),
					.isRanged = true,
					.projectileAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/blue/fire_bolt_small/fire_bolt_small.png",
					.projectileSpriteSize = {60, 60},
					.projectileFrameTimerMax = 0.10f,
					.projectileFrameCount = 6, // sheet is 360x60px = 6 frames of 60x60
					.projectileScale = 2.5f,
					.projectileTravelDuration = 1.8f,
					.projectileMotion = ProjectileMotion::Bobbing,
					.projectileMotionAmplitude = 8.0f,
					.activeFrame = 5,       // first bubble
					.projectileSpawnOffset = {60.0f, -4.0f},
					.projectileCount = 5,   // 5 bubbles total
					.loopStartFrame = 7,    // frames 7-10 repeat - one more bubble each pass - until all 5 have fired
					.loopEndFrame = 10
					// still using the fireball placeholder sprite and straight-line travel - real
					// bubble art and the up/down bobbing motion (point a. in your message) are later
					// steps. projectileSpawnOffset left at default pending testing, same as before.
				}
			}),

			// Water Dragon: melee physical-only auto-attack (claws, no fireball) + a manually-activated big fireball, pure fire damage
			FamiliarType(WaterDragon, "Water Dragon", "Water", Age::Juvenile, // same species (WaterDragon) as the Baby entry above - Age::Juvenile is what distinguishes this stage
			"Textures/Familiars/Dragons/sprites/adultdragon/sheet/WaterDragon_blue_idle.png", 0.15f, {240, 224},
			{
				FamiliarAttack{
					.name = "Claw Swipe",
					.animationKey = "attack1",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/WaterDragon_blue_attack1.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 7.0f,
					.minCooldownLimit = 1.0f,
					.isAutoAttack = true,
					.usesPhysicalDamage = true,
					.basePhysicalPower = BigNumber(1500), // higher than the baby stage's, since this stage's base stats are stronger
					.usesElementalDamage = false,
					.baseElementalPower = ElementalPower(), // unused, claws carry no element
					.isRanged = false, // melee, so every projectile field above stays at its default
					.activeFrame = 7 // damage lands on frame 7, not on activation
				},
				FamiliarAttack{
					.name = "Bubble Stream",
					.animationKey = "attack2",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/WaterDragon_blue_attack2.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 15.0f,
					.minCooldownLimit = 0.5f, // unused, bonus cooldown reduction never applies to a manual ability
					.isAutoAttack = false, // manually activated
					.usesPhysicalDamage = false,
					.basePhysicalPower = BigNumber(0), // unused
					.usesElementalDamage = true,
					.baseElementalPower = ElementalPower().set(Water, BigNumber(5000)),
					.isRanged = true,
					.projectileAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/blue/fire_bolt_small/fire_bolt_small.png",
					.projectileSpriteSize = {60, 60},
					.projectileFrameTimerMax = 0.10f,
					.projectileFrameCount = 6,
					.projectileScale = 5.0f, // bigger than the auto-attack fireball used to be, it's the "big fireball" now
					.projectileTravelDuration = 1.8f,
					.projectileMotion = ProjectileMotion::Bobbing,
					.projectileMotionAmplitude = 15.0f,
					.activeFrame = 9,       // first projectile, right as the loop begins
					.projectileSpawnOffset = {242.0f, -50.0f},
					.projectileCount = 10,  // 10 bubbles total
					.loopStartFrame = 9,    // frames 9-12 repeat - one more bubble each pass - until all 10 have fired
					.loopEndFrame = 12,
					.riseStartFrame = 5,
					.riseEndFrame = 8,
					.descendStartFrame = 13,
					.descendEndFrame = 16,
					.liftHeight = 40.0f,
					.unlockFeatureKey = "WaterDragonBubbleStreamUnlocked" // gated: hidden from the HUD until FamiliarTab's purchase button sets this (see FamiliarTab.cpp, and the "Juvenile Flame" achievement in AchievementManager.cpp that unlocks that purchase button in the first place)
					// still a fireball placeholder, straight-line travel - the dragon does leave
					// the ground now though, per point c. in your message. Tune liftHeight if the
					// leap looks too high/low once you test it.
				}
			}),

			// Baby Wind Dragon: a single ranged auto-attack that consists of a combination of physical and wind damage into one wind gust
			FamiliarType(WindDragon, "Baby Wind Dragon", "Wind", Age::Baby,
			"Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_bronze_idle.png", 0.15f, { 96, 96 },
			{
				FamiliarAttack{
					.name = "Wing Gust",
					.animationKey = "attack1",
					.animationAddress = "Textures/Familiars/Dragons/sprites/babydragon_sheets/babydragon_bronze_attack1.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 3.0f,
					.minCooldownLimit = 0.5f,
					.isAutoAttack = true,
					.usesPhysicalDamage = false,
					.basePhysicalPower = BigNumber(3),
					.usesElementalDamage = true,
					.baseElementalPower = ElementalPower().set(Wind, BigNumber(5)),
					.isRanged = true,
					.projectileAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/green/fire_bolt_small/fire_bolt_small.png",
					.projectileSpriteSize = {60, 60},
					.projectileFrameTimerMax = 0.10f,
					.projectileFrameCount = 6, // sheet is 360x60px = 6 frames of 60x60
					.projectileScale = 2.5f,
					.projectileTravelDuration = 1.8f,
					.projectileMotion = ProjectileMotion::GroundArc,
					.projectileMotionAmplitude = 30.0f,
					.activeFrame = 5,
					.projectileSpawnOffset = {60.0f, -4.0f}
					// still using the fireball placeholder sprite (point e. in your message) -
					// only the motion is real tornado-style curving so far, no tornado art yet
				}
			}),

			// Wind Dragon: melee physical-only auto-attack (claws, no fireball) + a manually-activated multi-hitting wind slashes, wind and physical damage
			FamiliarType(WindDragon, "Wind Dragon", "Wind", Age::Juvenile, // same species (WindDragon) as the Baby entry above - Age::Juvenile is what distinguishes this stage
			"Textures/Familiars/Dragons/sprites/adultdragon/sheet/WindDragon_bronze_idle.png", 0.15f, { 240, 224 },
			{
				FamiliarAttack{
					.name = "Claw Swipe",
					.animationKey = "attack1",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/WindDragon_bronze_attack1.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 7.0f,
					.minCooldownLimit = 1.0f,
					.isAutoAttack = true,
					.usesPhysicalDamage = true,
					.basePhysicalPower = BigNumber(1500), // higher than the baby stage's, since this stage's base stats are stronger
					.usesElementalDamage = false,
					.baseElementalPower = ElementalPower(), // unused, claws carry no element
					.isRanged = false, // melee, so every projectile field above stays at its default
					.activeFrame = 7 // damage lands on frame 7, not on activation
				},
				FamiliarAttack{
					.name = "Wind Swipes",
					.animationKey = "attack2",
					.animationAddress = "Textures/Familiars/Dragons/sprites/adultdragon/sheet/WindDragon_bronze_attack2.png",
					.animationFrameTimerMax = 0.10f,
					.baseCooldown = 15.0f,
					.minCooldownLimit = 0.5f, // unused, bonus cooldown reduction never applies to a manual ability
					.isAutoAttack = false, // manually activated
					.usesPhysicalDamage = true,
					.basePhysicalPower = BigNumber(500),
					.usesElementalDamage = true,
					.baseElementalPower = ElementalPower().set(Wind, BigNumber(500)),
					.isRanged = false, // instant wind-slash hits, not traveling projectiles - see spawnsEnemyEffect below
					.activeFrame = 12,      // first slash
					.projectileCount = 10,  // 10 slashes total
					.loopStartFrame = 9,
					.loopEndFrame = 15,
					.loopActiveFrame = 12,  // where in the loop each repeat slash lands
					.riseStartFrame = 5,
					.riseEndFrame = 8,
					.descendStartFrame = 16,
					.descendEndFrame = 19,
					.liftHeight = 40.0f,
					.spawnsEnemyEffect = true,
					.enemyEffectAnimationAddress = "Textures/Familiars/Dragons/sprites/effect/fire_bolt/green/fire_bolt_small/fire_bolt_small.png",
					.enemyEffectSpriteSize = {60, 60},
					.enemyEffectFrameTimerMax = 0.10f,
					.enemyEffectFrameCount = 6,
					.enemyEffectScale = 2.0f,
					.enemyEffectDuration = 0.3f,
					.unlockFeatureKey = "WindDragonWindSwipesUnlocked" // gated: hidden from the HUD until FamiliarTab's purchase button sets this (see FamiliarTab.cpp, and the "Juvenile Flame" achievement in AchievementManager.cpp that unlocks that purchase button in the first place)
					// still using the fireball placeholder art for the enemy-overlay slash effect
					// (no wind-slice sfx sprites yet, per your note) - only the mechanism (instant
					// multi-hit + overlay on the enemy, hover self-movement) is real. Total damage
					// is now ~10x what it was as a single hit - tell me if you want it rebalanced.
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
	// One persistent Familiar instance per SPECIES, indexed directly by FamiliarName - e.g.
	// ownedFamiliars[WaterDragon] is the player's Water Dragon, whatever evolution stage it's
	// currently at (see Familiar::getAge()). Index 0 (None) is always unused/nullptr. Every real
	// species is always built - even before it's ever unlocked - the exact same way the original
	// single `familiar` used to be: Familiar::isUnlocked() gates everything else, the object
	// itself just exists locked until purchased. Owned by Game (unlike shopTab/optionsTab, these
	// aren't TabContents).
	std::array<Familiar*, static_cast<size_t>(FamiliarNameCount)> ownedFamiliars{};

	// Convenience alias for ownedFamiliars[FireDragon] specifically. Fire is the one species with
	// still-special-cased tutorial behavior (ShopTab's unlock button, AchievementManager's
	// constructor) that hasn't been generalized onto the roster yet - keeping this name around
	// means none of that existing wiring needs to change. Always points at the same object as
	// ownedFamiliars[FireDragon] once initFamiliars() has run; will be retired once those
	// remaining call sites are migrated onto the roster.
	Familiar* familiar = nullptr;
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
	void initFamiliars(); // constructs one locked Familiar object per species into ownedFamiliars (each starting at its baby stage) - called once
	int findFamiliarTypeIndex(FamiliarName name, Age age) const; // index into familiarTypeList for that EXACT (species, stage) pair, or -1 if it isn't defined there (yet) - used by loadFromFile()/evolve handling to resolve a Familiar's saved/next stage via Familiar::evolveTo(), without reconstructing the Familiar object itself (which would orphan every pointer already handed out to it - ShopTab, AchievementManager, its damage/resolver callbacks)

	// Stable, human-readable save-file identifiers for a species ("FireDragon"/"WaterDragon"/...) -
	// used for both the per-species [Familiar:X] save sections and FamiliarSlots' occupant string,
	// instead of the enum's raw ordinal, which isn't safe to persist (reordering FamiliarName
	// later would silently remap every existing save file to the wrong species).
	// familiarNameFromSaveName returns None for an empty string (an empty slot) or any name this
	// build doesn't recognize (e.g. a future species an older save predates).
	// NOTE: add a case to both whenever a new species is added.
	static std::string familiarSaveName(FamiliarName name);
	static FamiliarName familiarNameFromSaveName(const std::string& saveName);

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

	// --- Active (slotted) familiar DPS ---
	// Each slotted familiar tracks its OWN rolling DPS (Familiar::getFamiliarDPS()), but they used
	// to each draw their own on-screen text at this exact same anchored spot - with 2+ familiars
	// slotted, that just overlapped into unreadable stacked text instead of one meaningful number.
	// Same fix as passiveFamiliarDPSText above: one combined line, owned by Game, summed fresh
	// every frame from whichever familiars are currently in familiarSlots.
	Text* activeFamiliarDPSText = nullptr;
	void initActiveFamiliarDPSText(); // creates activeFamiliarDPSText; called once from Game::Game()
	void updateActiveFamiliarDPSText(); // refreshes activeFamiliarDPSText's string + position from the current familiarSlots; called every frame from update()

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

	int getUnlockedFamiliarCount() const;       // how many familiars are currently unlocked, in total, across every lineage (ownership counts, not slotting - see familiarSlots' comment above)
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
	std::vector<Familiar*> familiarSlots; // size == number of unlocked slots; nullptr = empty, non-null = points at an owned familiar from ownedFamiliars (any lineage)

	void onFamiliarSlotPurchased();                                // adds one new, empty slot
	int getFamiliarSlotCount() const;
	std::string getFamiliarSlotLabel(int slotIndex) const;         // occupant's display label (from familiarTypeList, like getFamiliarSlotLabel()'s sibling helpers above), or "" if that slot is empty
	std::vector<Familiar*> getAvailableFamiliarsForSlotting() const; // every owned, unlocked, currently-unassigned familiar across all lineages, in ownedFamiliars' fixed order - the single source both of the two below derive from, so a pickedIndex always means the same familiar in both
	std::vector<std::string> getAvailableFamiliarNamesForSlotting() const; // display label of every familiar getAvailableFamiliarsForSlotting() returns, same order
	void assignFamiliarToSlot(int slotIndex, int pickedIndex);     // pickedIndex indexes into getAvailableFamiliarsForSlotting()/getAvailableFamiliarNamesForSlotting(); no-op if either index is invalid or the slot's already occupied
	void unassignFamiliarSlot(int slotIndex);
	bool isFamiliarInAnySlot(const Familiar* f) const;

	// Re-numbers every currently-OCCUPIED slot densely (0, 1, 2, ... skipping empty slots) and
	// tells each occupant its row and the current stack size via Familiar::setSlotPosition(), so
	// up to 5 familiars stack cleanly on the left side regardless of which slot indices happen to
	// be empty (e.g. 2 familiars in slots 0 and 3 still end up stacked as rows 0 and 1, not 0 and
	// 3 with empty gaps between them). Call this whenever occupancy changes - assign, unassign, or
	// restoring saved slots on load.
	void repositionSlottedFamiliars();

	// Polls raw game state that no achievement's own condition() can see on its own (an
	// achievement only has access to hero/statistics/familiar/itemManager - not familiarSlots),
	// and flips on the UnlockManager feature key(s) gating an achievement's visibility once that
	// state becomes true. unlockManager->unlock() is a std::set insert, so calling this every
	// frame is cheap even once the key is already set - see the .cpp for the current key(s).
	void updateFamiliarFeatureGates();

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
