#pragma once
#include "stdafx.h"
#include <functional>
#include "TabContent.h"
#include "Text.h"
#include "Hero.h"
#include "Statistics.h"
#include "UnlockManager.h"
#include <vector>

class Familiar; // forward declaration only - this header just stores pointers (see familiars below), the full definition is only needed in StatisticsTab.cpp where those pointers are actually used - same reasoning as FamiliarTab.h's own forward declaration

class StatisticsTab : public TabContent
{
	private:
		Hero* hero;             // non-owning
		Statistics* statistics; // non-owning
		UnlockManager* unlockManager; // non-owning - checks whether a familiar's current passive is actually active yet (see refreshFamiliarBlocks)

		// Every OWNED familiar slot (one per species), whether unlocked yet or not - Game passes
		// this once at construction, same roster ownedFamiliars already holds. A locked familiar
		// is simply skipped when building familiarBlocks below, so this tab only ever shows one
		// once it's actually been bought - same "hidden takes no space" rule the rest of the UI
		// already follows.
		std::vector<Familiar*> familiars;

		std::vector<Text*> lines;
		void refreshValues();

		// One stat block per currently-UNLOCKED familiar, rebuilt (see refreshFamiliarBlocks)
		// whenever which familiars are unlocked - or whether any of their current passives just
		// became an active, damage-dealing one - changes.
		struct FamiliarStatBlock
		{
			Familiar* familiar; // non-owning - whose stats this block displays
			Text* headerText;   // this familiar's current display label + evolution stage
			Text* physicalText;
			Text* elementalText;
			Text* dpsText;
			Text* passiveText; // nullptr unless this familiar's CURRENT passive deals damage and has been purchased (see hasActiveDamagingPassive in StatisticsTab.cpp)
		};
		std::vector<FamiliarStatBlock> familiarBlocks;

		// Rebuilds familiarBlocks from scratch whenever the set of unlocked familiars, or which of
		// them currently have an active damage-dealing passive, has changed since the last check -
		// same "detect a change, then rebuild" idiom AchievementsTab::render() already uses for its
		// own dynamically-growing line list. Safe to call every render(): with only a handful of
		// familiars ever existing, comparing against the last-seen snapshot first is far cheaper
		// than the Text allocations a real rebuild costs, so this only actually rebuilds on a
		// genuine change.
		void refreshFamiliarBlocks();
		void relayoutFamiliarBlocks(bool forceFontSize = false); // positions every current block's texts below the fixed lines above, and recomputes contentHeight. forceFontSize: true right after refreshFamiliarBlocks() rebuilds the block list, so the brand new Text objects get their size set at least once even if the panel width itself didn't change
		void refreshFamiliarValues();  // updates every block's displayed numbers in place (no allocation) - called every render()

		bool hasUnseenUnlock = false; // flags a newly-unlocked familiar (or a newly-active damage passive) so the tab button pulses until viewed - same pattern as AchievementsTab::hasUnseenUnlock
		float contentHeight = 0.0f;
		sf::Vector2f lastContentAreaOrigin;
		sf::Vector2f lastContentAreaSize;
		int lastAppliedFontSize = -1; // tabBodyTextSize as of the last actual setCharacterSize() pass - lets onResize()/relayoutFamiliarBlocks() skip re-setting every text's size (not free - forces SFML to rebuild that text's glyph layout) on the frames TabbedPanel re-lays-out this tab purely to track a scroll offset, not an actual change to the setting

	public:
		StatisticsTab(Hero* hero, Statistics* statistics, UnlockManager* unlockManager, std::vector<Familiar*> familiars);
		~StatisticsTab();

		// Live bonus from a currently-active passive (e.g. Wind Dragon's Keen Gale) - added on top
		// of hero->getCritChance()/getCritDamageMultiplier() for display only, so this tab always
		// shows the Hero's REAL effective crit odds, not just what's been purchased. Wired by Game
		// in initUI(); left unset (and treated as 0) if Game never wires them, so this tab works
		// the same as before for anyone who doesn't need this.
		std::function<double()> getCritChanceBonus;
		std::function<double()> getCritDamageBonus;

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;

		// Now grows with the familiar roster below the fixed lines, so this tab needs to report
		// its real height for TabbedPanel to know when to enable scrolling (same reason
		// AchievementsTab/FamiliarTab override this already) - previously always 0 (the default),
		// which was harmless only because the 10 fixed lines never grew taller than the visible area.
		float getContentHeight() const override { return contentHeight; }

		// Makes the tab button pulse until this tab is actually viewed - same pattern as
		// AchievementsTab/FamiliarTab's own hasPendingNotification()/markPending(). Call from Game
		// whenever a familiar is newly unlocked, or a damage-dealing passive (e.g. Burn) is newly
		// purchased - see FamiliarTab::onFamiliarUnlocked / onDamagingPassiveUnlocked.
		bool hasPendingNotification() const override { return hasUnseenUnlock; }
		void markPending() { hasUnseenUnlock = true; }
};
