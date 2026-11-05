#include "stdafx.h"
#include "FpsCounter.h"
#include <cmath>

FpsCounter::FpsCounter()
{
	fpsText = new Text("FPS: 0", "Fonts/arial.ttf");
	fpsText->setCharacterSize(16);
	fpsText->setFillColor(sf::Color::Yellow); // stands out clearly against the play area's green background
	fpsText->setOutlineColor(sf::Color::Black);
	fpsText->setOutlineThickness(2.0f);
	fpsText->setPosition({ 10.0f, 10.0f }); // top-left corner of the screen
}

FpsCounter::~FpsCounter()
{
	delete fpsText;
}

void FpsCounter::toggle()
{
	visible = !visible;
}

bool FpsCounter::isVisible() const
{
	return visible;
}

void FpsCounter::update(float dt)
{
	frameCount++;
	elapsedSinceRefresh += dt;

	if (elapsedSinceRefresh >= refreshInterval)
	{
		float fps = static_cast<float>(frameCount) / elapsedSinceRefresh;
		fpsText->setString("FPS: " + std::to_string(static_cast<int>(std::round(fps))));

		frameCount = 0;
		elapsedSinceRefresh = 0.0f;
	}
}

void FpsCounter::render(sf::RenderWindow* window)
{
	window->draw(*fpsText);
}
