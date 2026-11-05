#pragma once
#include "stdafx.h"
#include "Text.h"

// Debug-only overlay: shows the current frames-per-second in the top-left corner, toggled by
// Game's fpsCounterKey - same toggle/update/render pattern as Ruler. update() is called every
// frame regardless of pause state, with the same real frame time Game's own Times gives it, so
// this still reflects actual rendering frame drops even while the game's own simulation is
// paused. The displayed number only refreshes once per refreshInterval rather than every single
// frame, since a raw per-frame 1/dt reading jitters too fast to read.
class FpsCounter
{
private:
	static constexpr float refreshInterval = 0.5f;

	bool visible = false;
	int frameCount = 0;
	float elapsedSinceRefresh = 0.0f;
	Text* fpsText;

public:
	FpsCounter();
	~FpsCounter();

	void toggle();
	bool isVisible() const;
	void update(float dt);
	void render(sf::RenderWindow* window);
};
