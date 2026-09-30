#include "stdafx.h"
#include "HUD.h"
#include "PublicVariables.h"
#include <sstream>
#include <iomanip>

HUD::HUD(std::function<sf::Vector2f()> getTargetPosition)
	: getTargetPosition(std::move(getTargetPosition))
{
}

bool HUD::matchesCurrentSlots(const std::vector<Familiar*>& familiarSlots) const
{
	size_t entryIndex = 0;
	for (Familiar* f : familiarSlots)
	{
		if (!f) continue;
		for (size_t i = 0; i < f->getAttackCount(); i++)
		{
			if (f->getAttack(i).isAutoAttack) continue;
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
}

void HUD::onResize()
{
	layoutManualAttackEntries();
}