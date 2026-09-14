#pragma once
#include "stdafx.h"
#include "TabContent.h"
#include "UIButton.h"
#include "IWindowController.h"
#include "PublicVariables.h"
#include "ISaveable.h"

class OptionsTab : public TabContent
{
	private:
		IWindowController* windowController;   // non-owning
		ISaveable* saveable;                 // non-owning

		UIButton* windowModeButton;
		UIButton* resolutionButton;
		UIButton* deathAnimationsButton;
		UIButton* saveButton;
		UIButton* loadButton;

		void updateWindowModeLabel();
		void updateResolutionLabel();
		void updateDeathAnimationsLabel();

		void onWindowModeClicked();
		void onResolutionClicked();
		void onDeathAnimationsClicked();
		void onSaveClicked();
		void onLoadClicked();

	public:
		OptionsTab(IWindowController* windowController, ISaveable* saveable);
		~OptionsTab();

		void onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize) override;
		void render(sf::RenderWindow* window) override;
		bool handleClick(sf::Vector2f pos) override;
		void enableLoadButton(bool enable);
		void refreshLabels();
};