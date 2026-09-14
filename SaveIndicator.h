#pragma once
#include "stdafx.h"

// Zeigt kurzzeitig ein kleines "Saving..." Icon (oben links in der Game-Area) an.
// Wird von Game bei jedem saveToFile() (manuell per Options-Tab oder Autosave) aufgerufen.
class SaveIndicator
{
private:
	sf::Text label;
	sf::RectangleShape diskBody;
	sf::RectangleShape diskLabel;
	sf::RectangleShape diskNotch;

	float visibleDuration; // Zeit, in der das Icon voll sichtbar bleibt
	float fadeDuration;    // Zeit, in der es danach ausgeblendet wird
	float timer;           // verstrichene Zeit seit show()
	bool active;

	void applyAlpha(std::uint8_t alpha);

public:
	explicit SaveIndicator(const sf::Font& font);

	void show();                            // (re)startet die Anzeige
	void update(float dt);                  // in Game::update() aufrufen
	void render(sf::RenderWindow* window);   // in Game::render() aufrufen, zuletzt (oben drauf)
	void onResize();                         // Position neu berechnen bei Fenster-/Layoutänderung

	bool isActive() const { return active; }
};