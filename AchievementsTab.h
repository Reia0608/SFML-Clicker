#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "AchievementManager.h"
#include "UIButton.h"
#include "Text.h"

class AchievementsTab : public TabContent
{
	private:
		AchievementManager* achievementManager;   // non-owning
		bool hasUnseenUnlock = false;
		float contentHeight = 0.0f; // total height of the unscrolled layout, measured in onResize (see getContentHeight)

		struct AchievementLine
		{
			Text* nameText;
			Text* descriptionText;
			UIButton* claimButton; // only visible while unlocked && !claimed (see UIButton::setEnabled)
		};

		std::vector<AchievementLine> lines;
		
		void rebuildLines();
		void refreshClaimButtons(); // updates each claim button's visibility/enabled state to match unlocked/claimed


	public:
		AchievementsTab(AchievementManager* achievementManager);
		~AchievementsTab();

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;

		float getContentHeight() const override { return contentHeight; }

		bool hasPendingNotification() const override { return hasUnseenUnlock; }
		void markPending() { hasUnseenUnlock = true; } // call from AchievementManager::onAchievementUnlocked
};