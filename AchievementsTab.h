#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "AchievementManager.h"
#include "UIButton.h"
#include "Text.h"
#include <array>

class AchievementsTab : public TabContent
{
private:
	AchievementManager* achievementManager;   // non-owning
	bool hasUnseenUnlock = false;
	float contentHeight = 0.0f; // total height of the unscrolled layout, measured in relayoutLines() (see getContentHeight)

	struct AchievementLine
	{
		Text* nameText;
		Text* descriptionText;
		std::string rawDescription; // un-wrapped text, kept so relayoutLines() can re-wrap it from scratch whenever the content area width changes
		UIButton* claimButton; // only visible while unlocked && !claimed (see UIButton::setEnabled)
	};

	std::vector<AchievementLine> lines;

	// lines[i] is always built from lastBuiltOrder[i] - NOT from achievementManager->getVisibleAchievements()[i]
	// directly, since display order is grouped (see buildGroupedAchievements() in the .cpp), not manager order.
	std::vector<const Achievement*> lastBuiltOrder;

	// One reusable header per display group - index is the achievementGroupRank() the .cpp sorts by:
	// 0 "Unclaimed" (can be claimed right now), 1 "Locked" (not yet reached), 2 "Claimed" (reward collected).
	Text* groupTitleTexts[3];
	sf::Texture* groupIconTextures[3]; // padlock_unlocked/padlock_locked/success_check.png, same index as groupTitleTexts - drawn to the left of each header
	sf::Sprite* groupIconSprites[3];
	std::array<bool, 3> groupTitleVisible{}; // which of the headers above actually has >=1 achievement under it this layout - an empty group shows no header, same "hidden takes no space" rule as the rest of the UI

	sf::Vector2f lastContentAreaOrigin; // cached so relayoutLines() can be called outside onResize - same pattern as FamiliarTab::lastContentAreaOrigin
	sf::Vector2f lastContentAreaSize;
	float lastWrapWidth = -1.0f; // descriptionWidth as of the last actual re-wrap - lets relayoutLines() skip wrapTextToWidth()'s expensive per-word measuring when only the scroll offset changed, not the available width or tabBodyTextSize (see its own comment in the .cpp)
	int lastWrapFontSize = -1; // tabBodyTextSize as of the last actual re-wrap - a font-size change (Options tab) also invalidates the current wrap, same as a width change

	std::vector<const Achievement*> buildGroupedAchievements() const; // achievementManager->getVisibleAchievements(), stable-sorted into the Unclaimed/Locked/Claimed display groups (see achievementGroupRank() in the .cpp) - achievements keep their original relative order within a group
	void rebuildLines();
	void relayoutLines(); // positions every current line and recomputes contentHeight, using the cached area above - called from onResize() AND from rebuildLines() itself, since a line list can now also be rebuilt at runtime (render() noticing a previously-hidden achievement became visible, or a claim/unlock moving one into a different group) with no onResize() call coming afterward to position it
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
