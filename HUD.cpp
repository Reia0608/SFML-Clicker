#include "stdafx.h"
#include "HUD.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include "Anchor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace
{
	// Duplicated from Game.cpp's own file-local helper (same reason the AchievementsTab/UIButton
	// word-wrap helpers are duplicated rather than shared - see LayoutManager.h's encoding note).
	sf::FloatRect unionRects(const sf::FloatRect& a, const sf::FloatRect& b)
	{
		float left = std::min(a.position.x, b.position.x);
		float top = std::min(a.position.y, b.position.y);
		float right = std::max(a.position.x + a.size.x, b.position.x + b.size.x);
		float bottom = std::max(a.position.y + a.size.y, b.position.y + b.size.y);
		return sf::FloatRect({ left, top }, { right - left, bottom - top });
	}
}

HUD::HUD(std::function<sf::Vector2f()> getTargetPosition, UnlockManager* unlockManager)
	: getTargetPosition(std::move(getTargetPosition)), unlockManager(unlockManager)
{
	pausedText = new Text("Paused", "Fonts/arial.ttf");
	pausedText->setCharacterSize(48);
	pausedText->setFillColor(sf::Color::White);
	pausedText->setOutlineColor(sf::Color::Black);
	pausedText->setOutlineThickness(3.0f);
	layoutPausedText();

	initStageText();
	initEnemyInfoText();

	initHUDStatPanel();
	initPassiveFamiliarDPSText();
	initActiveFamiliarDPSText();

	initCurrencyText();
	initHeroDPSText();
	initClickDamageText();
}

HUD::~HUD()
{
	delete pausedText;
	delete stageText;
	delete enemyInfoText;
	delete passiveFamiliarDPSText;
	delete activeFamiliarDPSText;
	delete currencyText;
	delete dpsText;
	delete clickDamageText;
}

bool HUD::isAttackAvailable(const FamiliarAttack& attack) const
{
	return attack.unlockFeatureKey.empty() || unlockManager->isUnlocked(attack.unlockFeatureKey);
}

bool HUD::matchesCurrentSlots(const std::vector<Familiar*>& familiarSlots) const
{
	size_t entryIndex = 0;
	for (Familiar* f : familiarSlots)
	{
		if (!f) continue;
		for (size_t i = 0; i < f->getAttackCount(); i++)
		{
			const FamiliarAttack& attack = f->getAttack(i);
			if (attack.isAutoAttack) continue;
			if (!isAttackAvailable(attack)) continue; // gated and not purchased yet - treated as not existing, same as an auto-attack. Once purchased this flips, so the next sync() call naturally detects the mismatch below and triggers a rebuild - no extra signal needed from FamiliarTab.
			if (entryIndex >= manualAttackEntries.size()) return false;
			if (manualAttackEntries[entryIndex].familiar != f || manualAttackEntries[entryIndex].attackIndex != i) return false;
			entryIndex++;
		}
	}
	return entryIndex == manualAttackEntries.size();
}

void HUD::rebuildManualAttackEntries(const std::vector<Familiar*>& familiarSlots)
{
	manualAttackEntries.clear();

	for (Familiar* f : familiarSlots)
	{
		if (!f) continue;
		for (size_t i = 0; i < f->getAttackCount(); i++)
		{
			if (f->getAttack(i).isAutoAttack) continue;
			if (!isAttackAvailable(f->getAttack(i))) continue; // gated and not purchased yet - see matchesCurrentSlots()

			ManualAttackEntry entry;
			entry.familiar = f;
			entry.attackIndex = i;
			entry.button = std::make_unique<UIButton>(f->getAttack(i).name, [this, f, i]()
				{
					if (f->isManualAttackReady(i)) f->activateManualAttack(i, getTargetPosition());
				});
			manualAttackEntries.push_back(std::move(entry));
		}
	}

	layoutManualAttackEntries();
}

void HUD::layoutManualAttackEntries()
{
	const sf::Vector2f buttonSize{ 70.0f, 50.0f };
	const float spacing = 10.0f;
	float x = 20.0f;
	float y = windowSize.y - buttonSize.y - 20.0f;

	for (auto& entry : manualAttackEntries)
	{
		entry.button->setLayout({ x, y }, buttonSize);
		x += buttonSize.x + spacing;
	}
}

void HUD::layoutPausedText()
{
	// Centered over the play area (everything left of the UI panel), not the whole window - same
	// "playArea" bounds Game::pollEvents() already uses to gate a click (clickPos.x < gameAreaWidth).
	LayoutManager::applyAnchor(*pausedText, Anchor::MiddleCenter, { 0.0f, 0.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::initStageText()
{
	stageText = new Text("Stage: " + std::to_string(stageNumber), "Fonts/arial.ttf");
	stageText->setCharacterSize(24);
	stageText->setFillColor(sf::Color::White);
	stageText->setOutlineColor(sf::Color::Black);
	stageText->setOutlineThickness(1.0f);
	layoutStageText();
}

void HUD::initEnemyInfoText()
{
	enemyInfoText = new Text("", "Fonts/arial.ttf");
	enemyInfoText->setCharacterSize(18);
	enemyInfoText->setFillColor(sf::Color::White);
	enemyInfoText->setOutlineColor(sf::Color::Black);
	enemyInfoText->setOutlineThickness(1.0f);
	layoutEnemyInfoText();
}

void HUD::layoutStageText()
{
	LayoutManager::applyAnchor(*stageText, Anchor::TopCenter, { 0.0f, 20.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::layoutEnemyInfoText()
{
	LayoutManager::applyAnchor(*enemyInfoText, Anchor::TopCenter, { 0.0f, 50.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::setStageNumber(int newStageNumber)
{
	stageNumber = newStageNumber;
	stageText->setString("Stage: " + std::to_string(stageNumber));
	layoutStageText();
}

void HUD::setEnemyInfoText(const std::string& text)
{
	enemyInfoText->setString(text);
	layoutEnemyInfoText();
}

void HUD::renderStageInfo(sf::RenderWindow* window)
{
	window->draw(*stageText);
	window->draw(*enemyInfoText);
}

void HUD::sync(const std::vector<Familiar*>& familiarSlots)
{
	if (!matchesCurrentSlots(familiarSlots)) rebuildManualAttackEntries(familiarSlots);
}

void HUD::update()
{
	for (auto& entry : manualAttackEntries)
	{
		const FamiliarAttack& attack = entry.familiar->getAttack(entry.attackIndex);
		bool ready = entry.familiar->isManualAttackReady(entry.attackIndex);

		if (ready)
		{
			entry.button->setLabelText(attack.name);
		}
		else
		{
			float remaining = entry.familiar->getAttackCooldownRemaining(entry.attackIndex);
			std::ostringstream out;
			out << std::fixed << std::setprecision(1) << remaining;
			entry.button->setLabelText(out.str());
		}
		entry.button->setNotifying(ready); // pulses gold once it's off cooldown
	}
}

bool HUD::handleClick(sf::Vector2f pos)
{
	for (auto& entry : manualAttackEntries)
	{
		if (entry.button->handleClick(pos)) return true;
	}
	return false;
}

void HUD::render(sf::RenderWindow* window)
{
	for (auto& entry : manualAttackEntries) entry.button->render(window);

	if (paused)
	{
		window->draw(*pausedText);
	}
}

void HUD::onResize()
{
	layoutManualAttackEntries();
	layoutPausedText(); // gameAreaWidth/windowSize may have changed
	layoutStageText();
	layoutEnemyInfoText();
	layoutCurrencyText();
	layoutHeroDPSText();
	layoutClickDamageText();
}

void HUD::togglePause()
{
	paused = !paused;
}

bool HUD::isPaused() const
{
	return paused;
}

void HUD::initHUDStatPanel()
{
	hudStatPanel.setFillColor(sf::Color(10, 8, 6, 140)); // dark, semi-transparent - same style as TabbedPanel's own background rectangle
	hudStatPanel.setOutlineColor(sf::Color::Black);
	hudStatPanel.setOutlineThickness(1.0f);
}

void HUD::initPassiveFamiliarDPSText()
{
	passiveFamiliarDPSText = new Text("Passive Familiar DPS: " + BigNumber(0).toString(), "Fonts/arial.ttf");
	passiveFamiliarDPSText->setFillColor(passiveDPSColor);
	passiveFamiliarDPSText->setOutlineColor(sf::Color::Black);
	passiveFamiliarDPSText->setOutlineThickness(2.0f);
	layoutPassiveFamiliarDPSText();
}

void HUD::initActiveFamiliarDPSText()
{
	activeFamiliarDPSText = new Text("Familiar DPS: " + BigNumber(0).toString(), "Fonts/arial.ttf");
	// Pale silver-blue - same source-identity color every slotted familiar's own DPS text used to
	// use individually (Familiar::dpsColor) - now shared by the one combined line that replaces
	// all of them, so floating damage numbers on the enemy still match this HUD line's color.
	activeFamiliarDPSText->setFillColor(Familiar::dpsColor);
	activeFamiliarDPSText->setOutlineColor(sf::Color::Black);
	activeFamiliarDPSText->setOutlineThickness(2.0f);
	layoutActiveFamiliarDPSText();
}

void HUD::layoutPassiveFamiliarDPSText()
{
	// -120: sits below activeFamiliarDPSText's -160 - passive damage unlocks as soon as any
	// familiar is unlocked, while Familiar DPS additionally requires that familiar to be slotted
	// (a later step), so this line appears first/lower in the stack.
	LayoutManager::applyAnchor(*passiveFamiliarDPSText, Anchor::BottomRight, { -10.0f, -120.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::layoutActiveFamiliarDPSText()
{
	// -160: top of the BottomRight stat stack - this requires a familiar to be actively slotted
	// (not just unlocked), which happens later than everything below it, including
	// passiveFamiliarDPSText's passive DPS line at -120 (unlocks as soon as any familiar is
	// unlocked, no slotting needed).
	LayoutManager::applyAnchor(*activeFamiliarDPSText, Anchor::BottomRight, { -10.0f, -160.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::setPassiveFamiliarDPS(const BigNumber& dps)
{
	passiveFamiliarDPSText->setString("Passive Familiar DPS: " + dps.toString());
	layoutPassiveFamiliarDPSText();
}

void HUD::setActiveFamiliarDPS(const BigNumber& dps)
{
	activeFamiliarDPSText->setString("Familiar DPS: " + dps.toString());
	layoutActiveFamiliarDPSText();
}

void HUD::updateStatPanel(bool includePassiveFamiliarDPS, bool includeActiveFamiliarDPS)
{
	sf::FloatRect bounds = unionRects(currencyText->getGlobalBounds(), dpsText->getGlobalBounds());
	bounds = unionRects(bounds, clickDamageText->getGlobalBounds());

	if (includePassiveFamiliarDPS)
	{
		bounds = unionRects(bounds, passiveFamiliarDPSText->getGlobalBounds());
	}
	if (includeActiveFamiliarDPS)
	{
		bounds = unionRects(bounds, activeFamiliarDPSText->getGlobalBounds());
	}

	hudStatPanel.setPosition({ bounds.position.x - hudStatPanelPadding, bounds.position.y - hudStatPanelPadding });
	hudStatPanel.setSize({ bounds.size.x + hudStatPanelPadding * 2.0f, bounds.size.y + hudStatPanelPadding * 2.0f });
}

void HUD::renderStatPanel(sf::RenderWindow* window)
{
	window->draw(hudStatPanel);
}

void HUD::renderPassiveFamiliarDPS(sf::RenderWindow* window)
{
	window->draw(*passiveFamiliarDPSText);
}

void HUD::renderActiveFamiliarDPS(sf::RenderWindow* window)
{
	window->draw(*activeFamiliarDPSText);
}

void HUD::initCurrencyText()
{
	currencyText = new Text(BigNumber(0).toString(), "Fonts/arial.ttf");
	currencyText->setFillColor(sf::Color::Yellow);
	currencyText->setOutlineColor(sf::Color::Black);
	currencyText->setOutlineThickness(2.0f);
	layoutCurrencyText();
}

void HUD::initHeroDPSText()
{
	dpsText = new Text("DPS: 0", "Fonts/arial.ttf");
	// Pale gold/ivory - source-identity color for "damage dealt by the Hero", deliberately
	// desaturated so it reads as a stat readout rather than an elemental effect color (see
	// activeFamiliarDPSText/passiveFamiliarDPSText above for the other two of this trio).
	// clickDamageText below stays pure Red on purpose - it's not part of this scheme, it just
	// shows current attack power and already matches the Enemy's Red/Orange floating hit numbers.
	dpsText->setFillColor(sf::Color(255, 235, 200));
	dpsText->setOutlineColor(sf::Color::Black);
	dpsText->setOutlineThickness(2.0f);
	layoutHeroDPSText();
}

void HUD::initClickDamageText()
{
	clickDamageText = new Text("Click Damage: " + BigNumber(0).toString(), "Fonts/arial.ttf");
	clickDamageText->setFillColor(sf::Color::Red);
	clickDamageText->setOutlineColor(sf::Color::Black);
	clickDamageText->setOutlineThickness(2.0f);
	layoutClickDamageText();
}

void HUD::layoutCurrencyText()
{
	LayoutManager::applyAnchor(*currencyText, Anchor::BottomRight, { -10.0f, -10.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::layoutHeroDPSText()
{
	LayoutManager::applyAnchor(*dpsText, Anchor::BottomRight, { -10.0f, -50.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::layoutClickDamageText()
{
	LayoutManager::applyAnchor(*clickDamageText, Anchor::BottomRight, { -10.0f, -80.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void HUD::setCurrency(const BigNumber& currency)
{
	currencyText->setString("Gold: " + currency.toString());
	layoutCurrencyText();
}

void HUD::triggerCurrencyPulse()
{
	currencyPulseElapsed = 0.0f;
	currencyPulseActive = true;
}

void HUD::updateCurrencyPulse(float dt)
{
	if (!currencyPulseActive) return;

	currencyPulseElapsed += dt;
	float t = currencyPulseElapsed / currencyPulseDuration;

	if (t >= 1.0f)
	{
		currencyText->setScale({ 1.0f, 1.0f });
		currencyPulseActive = false;
		return;
	}

	const float pi = 3.14159265f;
	float scale = 1.0f + (currencyPulseMaxScale - 1.0f) * std::sin(t * pi);
	currencyText->setScale({ scale, scale });
}

void HUD::setHeroDPS(const BigNumber& dps)
{
	dpsText->setString("DPS: " + dps.toString());
	layoutHeroDPSText();
}

void HUD::setClickDamage(const BigNumber& attackPower)
{
	clickDamageText->setString("Click Damage: " + attackPower.toString());
	layoutClickDamageText();
}

sf::Vector2f HUD::getCurrencyTextPosition() const
{
	return currencyText->getPosition();
}

void HUD::renderHeroStats(sf::RenderWindow* window)
{
	window->draw(*currencyText);
	window->draw(*dpsText);
	window->draw(*clickDamageText);
}