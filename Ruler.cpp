#include "stdafx.h"
#include "Ruler.h"
#include "PublicVariables.h"
#include <sstream>

Ruler::Ruler()
{
	fonts = new Fonts("Fonts/arial.ttf");

	mousePosText = new Text("", fonts);
	mousePosText->setCharacterSize(14);
	mousePosText->setFillColor(sf::Color::White);
	mousePosText->setOutlineColor(sf::Color::Black);
	mousePosText->setOutlineThickness(2.0f);
}

Ruler::~Ruler()
{
	delete mousePosText;
	delete fonts;
}

void Ruler::toggle()
{
	visible = !visible;
	if (visible) rebuild(); // pick up any size change that happened while hidden
}

bool Ruler::isVisible() const
{
	return visible;
}

void Ruler::onResize()
{
	if (visible) rebuild();
}

void Ruler::rebuild()
{
	gridLines.clear();
	labels.clear();

	const sf::Color lineColor(0, 255, 255, 90); // translucent cyan - visible over both light and dark backgrounds without hiding what's underneath

	for (float x = 0.0f; x <= windowSize.x; x += gridSpacing)
	{
		gridLines.append(sf::Vertex{ { x, 0.0f }, lineColor });
		gridLines.append(sf::Vertex{ { x, windowSize.y }, lineColor });

		auto label = std::make_unique<Text>(std::to_string(static_cast<int>(x)), fonts);
		label->setCharacterSize(12);
		label->setFillColor(sf::Color::White);
		label->setOutlineColor(sf::Color::Black);
		label->setOutlineThickness(1.0f);
		label->setPosition({ x + 2.0f, 2.0f });
		labels.push_back(std::move(label));
	}

	for (float y = 0.0f; y <= windowSize.y; y += gridSpacing)
	{
		gridLines.append(sf::Vertex{ { 0.0f, y }, lineColor });
		gridLines.append(sf::Vertex{ { windowSize.x, y }, lineColor });

		auto label = std::make_unique<Text>(std::to_string(static_cast<int>(y)), fonts);
		label->setCharacterSize(12);
		label->setFillColor(sf::Color::White);
		label->setOutlineColor(sf::Color::Black);
		label->setOutlineThickness(1.0f);
		label->setPosition({ 20.0f, y + 2.0f }); // nudged right of the left edge so it doesn't sit exactly under the x-axis labels at the (0,0) corner
		labels.push_back(std::move(label));
	}
}

void Ruler::render(sf::RenderWindow* window)
{
	window->draw(gridLines);
	for (auto& label : labels)
	{
		window->draw(*label);
	}
}

void Ruler::renderCrosshair(sf::RenderWindow* window, sf::Vector2f mouseWorldPos)
{
	const sf::Color crosshairColor(255, 200, 0, 200); // solid-ish amber - stands out against the static grid's translucent cyan

	crosshairLines.clear();
	crosshairLines.append(sf::Vertex{ { 0.0f, mouseWorldPos.y }, crosshairColor });
	crosshairLines.append(sf::Vertex{ { windowSize.x, mouseWorldPos.y }, crosshairColor });
	crosshairLines.append(sf::Vertex{ { mouseWorldPos.x, 0.0f }, crosshairColor });
	crosshairLines.append(sf::Vertex{ { mouseWorldPos.x, windowSize.y }, crosshairColor });
	window->draw(crosshairLines);

	std::ostringstream out;
	out << "(" << static_cast<int>(mouseWorldPos.x) << ", " << static_cast<int>(mouseWorldPos.y) << ")";
	mousePosText->setString(out.str());
	mousePosText->setPosition({ mouseWorldPos.x + 14.0f, mouseWorldPos.y + 14.0f }); // offset so the label doesn't sit directly under the cursor
	window->draw(*mousePosText);
}
