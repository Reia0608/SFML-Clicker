#pragma once
#include "stdafx.h"
#include "Text.h"
#include "Fonts.h"
#include <vector>
#include <memory>

// Debug-only overlay: draws a pixel grid across the whole window with coordinate labels along
// the top and left edges, toggled by Game's rulerKey, so sprites/UI elements can be positioned
// against known pixel values instead of guessing. Purely visual - never touches gameplay state.
// While the game is also paused, renderCrosshair() additionally draws a live crosshair + exact
// coordinate readout at the mouse position - see Game::render().
class Ruler
{
private:
	static constexpr float gridSpacing = 50.0f; // pixels between gridlines - fine enough to place things precisely without cluttering the screen

	bool visible = false;
	Fonts* fonts;
	sf::VertexArray gridLines{ sf::PrimitiveType::Lines };
	std::vector<std::unique_ptr<Text>> labels;

	sf::VertexArray crosshairLines{ sf::PrimitiveType::Lines };
	Text* mousePosText;

	void rebuild(); // re-lays-out gridLines/labels for the current windowSize - called when toggled on, and on every resize while visible

public:
	Ruler();
	~Ruler();

	void toggle();
	bool isVisible() const;
	void onResize(); // call whenever the window size changes
	void render(sf::RenderWindow* window);

	// Draws a full-window crosshair through mouseWorldPos, plus its exact coordinates next to it.
	// Game only calls this while paused (see Game::render()) - there's no reason to track the
	// mouse in real time during normal play, only while frozen and actually comparing it to something.
	void renderCrosshair(sf::RenderWindow* window, sf::Vector2f mouseWorldPos);
};
