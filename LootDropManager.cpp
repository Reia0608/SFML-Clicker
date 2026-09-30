#include "LootDropManager.h"
#include "PublicVariables.h"
#include "Randomize.h"
#include <algorithm>
#include <cmath>

// NOTE: adjust this path if the coin spin sheet ends up somewhere else in the project. It's
// expected to sit next to emblem.png (see ItemManager::ItemManager()) at 16x16 per frame, laid
// out as a single row - coinsanimated.png is 112x16, i.e. 7 frames.
static const std::string coinSpriteSheetPath = "Textures/PNG/Items/coinsanimated.png";

LootDropManager::LootDropManager(Hero* hero, ItemBar* itemBar, ItemManager* itemManager)
	: hero(hero), itemBar(itemBar), itemManager(itemManager)
{
	if (!coinTexture.loadFromFile(coinSpriteSheetPath))
	{
		std::cerr << "ERROR::LOOTDROPMANAGER::COULD NOT LOAD FILE!" << coinSpriteSheetPath << std::endl;
	}
	coinTexture.setSmooth(false);
	coinFrameCount = std::max(1, static_cast<int>(coinTexture.getSize().x) / static_cast<int>(coinFrameSize.x));

	drops.reserve(maxSimultaneousDrops);
}

LootDropManager::~LootDropManager()
{
}

sf::Vector2f LootDropManager::randomLowerHalfLandingPoint() const
{
	const float margin = 40.0f;
	float minX = margin;
	float maxX = std::max(minX, gameAreaWidth - margin);
	float minY = windowSize.y / 2.0f;
	float maxY = std::max(minY, windowSize.y - margin);

	return { Randomize::returnRandomizeFloat(minX, maxX), Randomize::returnRandomizeFloat(minY, maxY) };
}

void LootDropManager::onGoldDropped(BigNumber amount, sf::Vector2f position)
{
	if (drops.size() >= maxSimultaneousDrops)
	{
		// Too many drops animating already - grant the reward instantly rather than losing it or
		// letting the backlog grow unbounded (same safety-net idea as Enemy::showDamage's cap).
		if (onGoldCollectedCallback) onGoldCollectedCallback(amount);
		return;
	}

	LootDrop drop(coinTexture, DropType::Gold, position);
	drop.goldAmount = amount;
	drop.landPos = randomLowerHalfLandingPoint();
	drop.coinFrameTimer = coinFrameTimerMax;

	drop.sprite.setTextureRect(sf::IntRect({ 0, 0 }, sf::Vector2i(coinFrameSize)));
	drop.sprite.setOrigin({ coinFrameSize.x / 2.0f, coinFrameSize.y / 2.0f });
	drop.sprite.setScale({ coinScale, coinScale });
	drop.sprite.setPosition(position);

	drops.push_back(std::move(drop));
}

void LootDropManager::onItemDropped(int itemIndex, sf::Vector2f position)
{
	if (drops.size() >= maxSimultaneousDrops)
	{
		if (onItemCollectedCallback) onItemCollectedCallback(itemIndex);
		return;
	}

	sf::Vector2u frameSize = itemManager->getIconFrameSize();
	int col = itemManager->getItemDefinition(itemIndex).columnIndex;

	LootDrop drop(*itemManager->getIconSheet(), DropType::Item, position);
	drop.itemIndex = itemIndex;
	drop.landPos = randomLowerHalfLandingPoint();
	drop.shinePhase = Randomize::returnRandomizeFloat(0.0f, 6.28318f);

	drop.sprite.setTextureRect(sf::IntRect({ col * static_cast<int>(frameSize.x), 0 }, sf::Vector2i(frameSize)));
	drop.sprite.setOrigin({ frameSize.x / 2.0f, frameSize.y / 2.0f });
	drop.sprite.setScale({ itemScale, itemScale });
	drop.sprite.setPosition(position);

	drops.push_back(std::move(drop));
}

void LootDropManager::updateDrop(LootDrop& drop, float dt)
{
	switch (drop.phase)
	{
	case DropPhase::Landing:
	{
		drop.phaseElapsed += dt;
		float t = std::min(drop.phaseElapsed / landingDuration, 1.0f);
		float easedT = 1.0f - (1.0f - t) * (1.0f - t); // ease-out

		sf::Vector2f pos = drop.startPos + (drop.landPos - drop.startPos) * easedT;
		pos.y -= std::sin(t * 3.14159265f) * landingHopHeight; // small upward pop while falling
		drop.sprite.setPosition(pos);

		if (t >= 1.0f)
		{
			drop.phase = DropPhase::Lingering;
			drop.phaseElapsed = 0.0f;
			drop.sprite.setPosition(drop.landPos);
		}
		break;
	}
	case DropPhase::Lingering:
	{
		drop.phaseElapsed += dt;
		float lingerDuration = (drop.type == DropType::Gold) ? lingerDurationGold : lingerDurationItem;

		if (drop.type == DropType::Gold)
		{
			drop.coinFrameTimer -= dt;
			if (drop.coinFrameTimer <= 0.0f)
			{
				drop.coinFrame = (drop.coinFrame + 1) % coinFrameCount;
				drop.sprite.setTextureRect(sf::IntRect({ drop.coinFrame * static_cast<int>(coinFrameSize.x), 0 }, sf::Vector2i(coinFrameSize)));
				drop.coinFrameTimer = coinFrameTimerMax;
			}
		}
		else
		{
			drop.shinePhase += dt * shinePulseSpeed;
			float pulse = 1.0f + shinePulseStrength * std::sin(drop.shinePhase);
			drop.sprite.setScale({ itemScale * pulse, itemScale * pulse });
		}

		if (drop.phaseElapsed >= lingerDuration)
		{
			drop.phase = DropPhase::FlyingToTarget;
			drop.phaseElapsed = 0.0f;
			drop.flyStartPos = drop.sprite.getPosition();
		}
		break;
	}
	case DropPhase::FlyingToTarget:
	{
		drop.phaseElapsed += dt;
		float t = std::min(drop.phaseElapsed / flyDuration, 1.0f);
		float easedT = t * t; // ease-in - accelerates as if being pulled in

		sf::Vector2f target = (drop.type == DropType::Gold)
			? hero->getCurrencyTextPosition()
			: itemBar->getSlotPosition(drop.itemIndex);

		sf::Vector2f pos = drop.flyStartPos + (target - drop.flyStartPos) * easedT;
		drop.sprite.setPosition(pos);

		float baseScale = (drop.type == DropType::Gold) ? coinScale : itemScale;
		float shrink = 1.0f - 0.7f * easedT;
		drop.sprite.setScale({ baseScale * shrink, baseScale * shrink });

		if (t >= 1.0f)
		{
			drop.phase = DropPhase::Done;
		}
		break;
	}
	default:
		break;
	}
}

void LootDropManager::drawItemShine(sf::RenderWindow* window, const LootDrop& drop) const
{
	// Cheap procedural "shine" since emblem.png has no glow frames yet (per-item glow animation
	// frames are a planned future addition - see ItemManager): two thin diamonds crossed into a
	// 4-point sparkle, pulsing opposite the item's own scale pulse so the glint flashes brightest
	// as the icon shrinks slightly.
	float pulse = 0.5f + 0.5f * std::sin(drop.shinePhase + 3.14159265f);
	std::uint8_t alpha = static_cast<std::uint8_t>(180.0f * pulse);
	if (alpha < 10) return;

	sf::Vector2f center = drop.sprite.getPosition();
	float size = 14.0f + 4.0f * pulse;

	sf::ConvexShape sparkleVertical(4);
	sparkleVertical.setPoint(0, { 0.0f, -size });
	sparkleVertical.setPoint(1, { size * 0.25f, 0.0f });
	sparkleVertical.setPoint(2, { 0.0f, size });
	sparkleVertical.setPoint(3, { -size * 0.25f, 0.0f });
	sparkleVertical.setFillColor(sf::Color(255, 255, 255, alpha));
	sparkleVertical.setPosition(center);
	window->draw(sparkleVertical);

	sf::ConvexShape sparkleHorizontal(4);
	sparkleHorizontal.setPoint(0, { -size, 0.0f });
	sparkleHorizontal.setPoint(1, { 0.0f, size * 0.25f });
	sparkleHorizontal.setPoint(2, { size, 0.0f });
	sparkleHorizontal.setPoint(3, { 0.0f, -size * 0.25f });
	sparkleHorizontal.setFillColor(sf::Color(255, 255, 255, alpha));
	sparkleHorizontal.setPosition(center);
	window->draw(sparkleHorizontal);
}

void LootDropManager::update(float dt)
{
	for (auto& drop : drops)
	{
		updateDrop(drop, dt);
	}

	for (auto it = drops.begin(); it != drops.end(); )
	{
		if (it->phase == DropPhase::Done)
		{
			if (it->type == DropType::Gold)
			{
				if (onGoldCollectedCallback) onGoldCollectedCallback(it->goldAmount);
			}
			else
			{
				if (onItemCollectedCallback) onItemCollectedCallback(it->itemIndex);
			}
			it = drops.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void LootDropManager::render(sf::RenderWindow* window)
{
	for (auto& drop : drops)
	{
		if (drop.type == DropType::Item && drop.phase == DropPhase::Lingering)
		{
			drawItemShine(window, drop);
		}
		window->draw(drop.sprite);
	}
}

void LootDropManager::setOnGoldCollectedCallback(std::function<void(BigNumber amount)> callback)
{
	onGoldCollectedCallback = std::move(callback);
}

void LootDropManager::setOnItemCollectedCallback(std::function<void(int itemIndex)> callback)
{
	onItemCollectedCallback = std::move(callback);
}

void LootDropManager::setLandingDuration(float seconds)
{
	landingDuration = std::max(0.01f, seconds);
}

void LootDropManager::setLingerDurations(float goldSeconds, float itemSeconds)
{
	lingerDurationGold = std::max(0.0f, goldSeconds);
	lingerDurationItem = std::max(0.0f, itemSeconds);
}

void LootDropManager::setFlyDuration(float seconds)
{
	flyDuration = std::max(0.01f, seconds);
}

void LootDropManager::setMaxSimultaneousDrops(size_t count)
{
	maxSimultaneousDrops = count;
}