#include "stdafx.h"
#include "OptionsTab.h"
#include <filesystem>

namespace
{
	std::string windowModeToString(WindowMode mode)
	{
		switch (mode)
		{
		case WindowMode::Windowed:   return "Windowed";
		case WindowMode::Borderless: return "Borderless";
		case WindowMode::Fullscreen: return "Fullscreen";
		}
		return "Unknown";
	}

	const std::string kSaveFilePath = "Save/savegame.json"; // keep in sync with Game::saveFilePath
}

void OptionsTab::relayoutButtons() // mirrors ShopTab/FamiliarTab's own version - positions only currently-visible buttonEntries, so a hidden one (autoAttackButton, before it's unlocked) leaves no gap behind it
{
	const float buttonHeight = 50.0f;
	const float margin = 10.0f;
	const float spacing = 10.0f;

	float y = lastContentAreaOrigin.y + margin;
	for (auto& entry : buttonEntries)
	{
		bool visible = entry.isVisible();
		entry.button->setEnabled(visible); // UIButton already skips render()/handleClick() while disabled

		if (!visible) continue;

		entry.button->setLayout(
			{ lastContentAreaOrigin.x + margin, y },
			{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
		);
		y += buttonHeight + spacing;
	}

	// loadButton sits right after the dynamic stack above, keeping the exact same independently-
	// enabled behavior it always had (see enableLoadButton()) - it's deliberately kept out of
	// buttonEntries, since its enabled state is driven from outside (Game, whenever a save does or
	// doesn't exist yet), not from a visibility rule relayoutButtons() should ever overwrite.
	loadButton->setLayout(
		{ lastContentAreaOrigin.x + margin, y },
		{ lastContentAreaSize.x - margin * 2.0f, buttonHeight }
	);
	y += buttonHeight + spacing;

	contentHeight = (y - lastContentAreaOrigin.y) - spacing + margin;
}

void OptionsTab::updateWindowModeLabel()
{
	windowModeButton->setLabelText("Window Mode: " + windowModeToString(windowController->getWindowMode()) + " >");
}

void OptionsTab::updateResolutionLabel()
{
	if (windowController->getWindowMode() == WindowMode::Fullscreen)
	{
		resolutionButton->setLabelText("Resolution: Native (Fullscreen)");
	}
	else
	{
		sf::Vector2u res = windowController->getAvailableResolutions()[windowController->getResolutionIndex()];
		resolutionButton->setLabelText("Current Resolution: " + std::to_string(res.x) + "x" + std::to_string(res.y) + " >");
	}
}

void OptionsTab::updateDeathAnimationsLabel()
{
	deathAnimationsButton->setLabelText(std::string("Death Animations: ") + (deathAnimationsEnabled ? "ON" : "OFF"));
}

void OptionsTab::updateAutoAttackLabel()
{
	autoAttackButton->setLabelText(std::string("Hero Auto-Attack (A): ") + (hero->isAutoAttackActive() ? "ON" : "OFF"));
}

void OptionsTab::updateTextSizeLabel()
{
	textSizeButton->setLabelText("Tab Text Size: " + std::to_string(tabBodyTextSize) + "px >");
}

void OptionsTab::onWindowModeClicked()
{
	WindowMode current = windowController->getWindowMode();
	WindowMode next;
	switch (current)
	{
	case WindowMode::Windowed:   next = WindowMode::Borderless; break;
	case WindowMode::Borderless: next = WindowMode::Fullscreen; break;
	default:                     next = WindowMode::Windowed;  break;
	}
	windowController->setWindowMode(next);
	updateWindowModeLabel();
	updateResolutionLabel();
}

void OptionsTab::onResolutionClicked()
{
	if (windowController->getWindowMode() == WindowMode::Fullscreen) return;   // resolution isn't selectable while in Fullscreen

	int count = static_cast<int>(windowController->getAvailableResolutions().size());
	int nextIndex = (windowController->getResolutionIndex() + 1) % count;
	windowController->setResolutionIndex(nextIndex);
	updateResolutionLabel();
}

void OptionsTab::onDeathAnimationsClicked()
{
	deathAnimationsEnabled = !deathAnimationsEnabled;
	updateDeathAnimationsLabel();
}

void OptionsTab::onAutoAttackClicked()
{
	hero->toggleAutoAttack();
	updateAutoAttackLabel();
}

void OptionsTab::onTextSizeClicked()
{
	tabBodyTextSize++;
	if (tabBodyTextSize > tabBodyTextSizeMax) tabBodyTextSize = tabBodyTextSizeMin;
	updateTextSizeLabel();
	if (onTabTextSizeChanged) onTabTextSizeChanged(); // lets Game re-lay-out every tab now, instead of waiting for the next time AchievementsTab/StatisticsTab happen to be re-laid-out on their own
}

void OptionsTab::onSaveClicked()
{
	saveable->saveToFile(kSaveFilePath);
}

void OptionsTab::onLoadClicked()
{
	if (!std::filesystem::exists(kSaveFilePath))
	{
		// e.g. leave the button disabled, show a hint text, etc.
		return;
	}

	saveable->loadFromFile(kSaveFilePath);
	refreshLabels();
}

OptionsTab::OptionsTab(IWindowController* windowController, ISaveable* saveable, UnlockManager* unlockManager, Hero* hero)
	: windowController(windowController), saveable(saveable), unlockManager(unlockManager), hero(hero)
{
	windowModeButton = new UIButton("", [this]() { onWindowModeClicked(); });
	resolutionButton = new UIButton("", [this]() { onResolutionClicked(); });
	deathAnimationsButton = new UIButton("", [this]() { onDeathAnimationsClicked(); });
	autoAttackButton = new UIButton("", [this]() { onAutoAttackClicked(); });
	textSizeButton = new UIButton("", [this]() { onTextSizeClicked(); });
	saveButton = new UIButton("Save Game", [this]() { onSaveClicked(); });
	loadButton = new UIButton("Load Game", [this]() { onLoadClicked(); });

	loadButton->setEnabled(std::filesystem::exists(kSaveFilePath)); // hidden by default, visible once a save exists

	updateWindowModeLabel();
	updateResolutionLabel();
	updateDeathAnimationsLabel();
	updateAutoAttackLabel();
	updateTextSizeLabel();

	buttonEntries =
	{
		{ windowModeButton,      [this]() { return true; } },
		{ resolutionButton,      [this]() { return true; } },
		{ deathAnimationsButton, [this]() { return true; } },
		{ autoAttackButton,      [this]() { return this->unlockManager->isUnlocked("HeroAutoAttackUnlocked"); } },
		{ textSizeButton,        [this]() { return true; } },
		{ saveButton,            [this]() { return true; } },
	};
}

OptionsTab::~OptionsTab()
{
	delete windowModeButton;
	delete resolutionButton;
	delete deathAnimationsButton;
	delete autoAttackButton;
	delete textSizeButton;
	delete saveButton;
	delete loadButton;
}

void OptionsTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	lastContentAreaOrigin = contentAreaOrigin;
	lastContentAreaSize = contentAreaSize;
	relayoutButtons();
}

void OptionsTab::render(sf::RenderWindow* window)
{
	hasUnseenUnlock = false; // being viewed right now - stop pulsing, same as FamiliarTab::render()/AchievementsTab::render()

	for (auto& entry : buttonEntries)
	{
		entry.button->render(window); // internally a no-op while disabled
	}
	loadButton->render(window);
}

bool OptionsTab::handleClick(sf::Vector2f pos)
{
	for (auto& entry : buttonEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
	if (loadButton->handleClick(pos)) return true;
	return false;
}

void OptionsTab::enableLoadButton(bool enable)
{
	loadButton->setEnabled(enable);
}

void OptionsTab::refreshLabels()
{
	updateWindowModeLabel();
	updateResolutionLabel();
	updateDeathAnimationsLabel();
	updateAutoAttackLabel();
	updateTextSizeLabel();
	relayoutButtons(); // picks up autoAttackButton's visibility the instant it unlocks, same as ShopTab/FamiliarTab's own refreshLabels()
}
