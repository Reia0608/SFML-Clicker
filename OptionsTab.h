#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "UIButton.h"
#include "IWindowController.h"
#include "PublicVariables.h"
#include "ISaveable.h"
#include "UnlockManager.h"
#include "Hero.h"
#include <functional>

class OptionsTab : public TabContent
{
	private:
		IWindowController* windowController;   // non-owning
		ISaveable* saveable;                 // non-owning
		UnlockManager* unlockManager;         // non-owning - gates autoAttackButton's visibility (see buttonEntries)
		Hero* hero;                          // non-owning - read/toggled by the Auto-Attack button

		UIButton* windowModeButton;
		UIButton* resolutionButton;
		UIButton* deathAnimationsButton;
		UIButton* autoAttackButton;
		UIButton* textSizeButton;
		UIButton* saveButton;
		UIButton* loadButton; // kept out of buttonEntries below - see relayoutButtons()

		bool hasUnseenUnlock = false; // flags autoAttackButton just becoming available (via the "Autopilot" achievement) so the tab button pulses until viewed - same pattern as FamiliarTab::hasUnseenUnlock

		// Same pattern as ShopTab/FamiliarTab's own button-entry list: pairs each button with the
		// rule that decides whether it currently takes up UI space, so a newly-unlocked option (like
		// autoAttackButton) can appear/disappear without every other button needing to shift by hand,
		// and the next gated option OptionsTab gets doesn't need this rewritten again.
		struct OptionsButtonEntry
		{
			UIButton* button;
			std::function<bool()> isVisible;
		};
		std::vector<OptionsButtonEntry> buttonEntries; // ordered top-to-bottom, built once in the constructor
		sf::Vector2f lastContentAreaOrigin; // cached so relayoutButtons() can be called outside onResize (e.g. the moment auto-attack unlocks mid-game)
		sf::Vector2f lastContentAreaSize;

		float contentHeight = 0.0f; // total height of the button stack in its unscrolled layout, measured in relayoutButtons (see getContentHeight)

		void relayoutButtons(); // positions only currently-visible entries from buttonEntries, then loadButton - called from onResize() and whenever a button's visibility could have changed

		void updateWindowModeLabel();
		void updateResolutionLabel();
		void updateDeathAnimationsLabel();
		void updateAutoAttackLabel();
		void updateTextSizeLabel();

		void onWindowModeClicked();
		void onResolutionClicked();
		void onDeathAnimationsClicked();
		void onAutoAttackClicked();
		void onTextSizeClicked();
		void onSaveClicked();
		void onLoadClicked();

	public:
		OptionsTab(IWindowController* windowController, ISaveable* saveable, UnlockManager* unlockManager, Hero* hero);
		~OptionsTab();

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;

		float getContentHeight() const override { return contentHeight; }

		void enableLoadButton(bool enable);

		// Fires whenever textSizeButton changes tabBodyTextSize, so Game can re-lay-out every tab
		// (tabbedPanel->onResize()) and have the new size actually show up right away - AchievementsTab/
		// StatisticsTab only pick up a changed tabBodyTextSize the next time THEY are re-laid-out, which
		// doesn't happen on its own while they're not the active+scrolled tab (see TabbedPanel::render()).
		std::function<void()> onTabTextSizeChanged;

		// Re-reads every label AND re-checks autoAttackButton's visibility - call after anything that
		// could have changed either: loading a save, claiming the "Autopilot" achievement, or the "A"
		// hotkey toggling auto-attack directly (see Game.cpp) - same as ShopTab/FamiliarTab's own
		// refreshLabels(), which also re-checks visibility.
		void refreshLabels();

		// Makes the tab button pulse (same visual as a tab that just became available) until this tab
		// is actually viewed - see render() below, which clears the flag, same pattern as
		// FamiliarTab::hasPendingNotification()/markPending().
		bool hasPendingNotification() const override { return hasUnseenUnlock; }
		void markPending() { hasUnseenUnlock = true; } // call from Game when the "Autopilot" achievement is claimed, revealing autoAttackButton
};
