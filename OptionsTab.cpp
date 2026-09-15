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
	if (windowController->getWindowMode() == WindowMode::Fullscreen) return;   // Auflösung im Fullscreen nicht wählbar

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

void OptionsTab::onSaveClicked()
{
	saveable->saveToFile(kSaveFilePath);
}

void OptionsTab::onLoadClicked()
{
	if (!std::filesystem::exists(kSaveFilePath))
	{
		// z.B. Button deaktiviert lassen, Hinweistext anzeigen, etc.
		return;
	}

	saveable->loadFromFile(kSaveFilePath);
	refreshLabels();
}

OptionsTab::OptionsTab(IWindowController* windowController, ISaveable* saveable) : windowController(windowController), saveable(saveable)
{
	windowModeButton = new UIButton("", [this]() { onWindowModeClicked(); });
	resolutionButton = new UIButton("", [this]() { onResolutionClicked(); });
	deathAnimationsButton = new UIButton("", [this]() { onDeathAnimationsClicked(); });
	saveButton = new UIButton("Save Game", [this]() { onSaveClicked(); });
	loadButton = new UIButton("Load Game", [this]() { onLoadClicked(); });

	loadButton->setEnabled(std::filesystem::exists(kSaveFilePath)); // hidden by default, visible once a save exists

	updateWindowModeLabel();
	updateResolutionLabel();
	updateDeathAnimationsLabel();
}

OptionsTab::~OptionsTab()
{
	delete windowModeButton;
	delete resolutionButton;
	delete deathAnimationsButton;
	delete saveButton;
	delete loadButton;
}

void OptionsTab::onResize(sf::Vector2f contentAreaOrigin, sf::Vector2f contentAreaSize)
{
	const float buttonHeight = 50.0f;
	const float margin = 10.0f;
	const float spacing = 10.0f;

	windowModeButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	resolutionButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	deathAnimationsButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 2 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	saveButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 3 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	loadButton->setLayout(
		{ contentAreaOrigin.x + margin, contentAreaOrigin.y + margin + (buttonHeight + spacing) * 4 },
		{ contentAreaSize.x - margin * 2.0f, buttonHeight }
	);

	// total height of the button stack in its unscrolled layout (5 buttons), so TabbedPanel can enable scrolling if it overflows
	contentHeight = margin * 2.0f + 5 * (buttonHeight + spacing) - spacing;
}

void OptionsTab::render(sf::RenderWindow* window)
{
	windowModeButton->render(window);
	resolutionButton->render(window);
	deathAnimationsButton->render(window);
	saveButton->render(window);
	loadButton->render(window);
}

bool OptionsTab::handleClick(sf::Vector2f pos)
{
	if (windowModeButton->handleClick(pos)) return true;
	if (resolutionButton->handleClick(pos)) return true;
	if (deathAnimationsButton->handleClick(pos)) return true;
	if (saveButton->handleClick(pos)) return true;
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
}
