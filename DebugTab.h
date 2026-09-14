#pragma once
#include "TabContent.h"
#include "UIButton.h"
#include "TextInputField.h"
#include "IDebugController.h"
#include "PublicVariables.h"
#include "BigNumber.h"
#include "Hero.h"
#include "Familiar.h"
#include "Statistics.h"


class DebugTab : public TabContent
{
	private:
		IDebugController* debugController; // non-owning

		TextInputField* attackDamageInput;
		TextInputField* goldInput;
		TextInputField* enemyHealthInput;
		TextInputField* dropChanceInput;
		TextInputField* critChanceInput;
		TextInputField* critDamageInput;

		UIButton* setAttackDamage;
		UIButton* setGold;
		UIButton* setEnemyHealth;
		UIButton* dropChance;
		UIButton* critChance;
		UIButton* critDamage;
		UIButton* resetStats;
		UIButton* resetItems;
		UIButton* resetPrices;

		BigNumber attackDamageValue = BigNumber(1);
		BigNumber goldValue = BigNumber(10);
		BigNumber enemyHealthValue = BigNumber(10);
		float dropChanceValue = 0.1f;
		float critChanceValue = 0.1f;
		int critDamageValue = 150;

		void onSetAttackDamageClicked();
		void onSetGoldClicked();
		void onSetEnemyHealthClicked();
		void onSetDropChanceClicked();
		void onSetCritChanceClicked();
		void onSetCritDamageClicked();
		void onResetStatsClicked();
		void onResetItemsClicked();
		void onResetPricesClicked();

	public:
		DebugTab(IDebugController* debugController);
		~DebugTab();

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;
		void handleTextEntered(char32_t unicode);
		void handleBackspace();
};