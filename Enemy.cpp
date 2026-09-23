#include "Enemy.h"
#include "EnemyType.h"
#include "Text.h"
#include "Randomize.h"
#include "PublicVariables.h"
#include <sstream>
#include <iomanip>

void Enemy::initHealth(BigNumber maxHealth)
{
	health = maxHealth;
};

void Enemy::initHealthBar()
{
	const float barWidth = 80.0f;
	const float barHeight = 10.0f;

	healthBarBackground.setSize({ barWidth, barHeight });
	healthBarBackground.setFillColor(sf::Color(60, 60, 60));
	healthBarBackground.setOutlineColor(sf::Color::Black);
	healthBarBackground.setOutlineThickness(1.0f);
	healthBarBackground.setOrigin({ barWidth / 2.0f, 0.0f });

	healthBarFill.setSize({ barWidth, barHeight });
	healthBarFill.setFillColor(sf::Color::Red);
	healthBarFill.setOrigin({ 0.0f, 0.0f });

	healthText = new Text(health.toString(), fonts);
	healthText->setFillColor(sf::Color::White);
	healthText->setOutlineColor(sf::Color::Black);
	healthText->setOutlineThickness(2.0f);
	healthText->setCharacterSize(14);
};

void Enemy::updateHealthBar()
{
	sf::FloatRect bounds = sprites->getGlobalBounds();
	const float margin = 10.0f;
	const float barWidth = 80.0f;
	const float barHeight = 10.0f;

	float centerX = bounds.position.x + bounds.size.x / 2.0f;
	float barY = bounds.position.y + bounds.size.y + margin;

	healthBarBackground.setPosition({ centerX, barY });

	double ratio = health.toApproxDouble() / maxHealth.toApproxDouble();
	if (ratio < 0.0) ratio = 0.0;
	if (ratio > 1.0) ratio = 1.0;

	healthBarFill.setSize({ barWidth * static_cast<float>(ratio), barHeight });
	healthBarFill.setPosition({ centerX - barWidth / 2.0f, barY });

	healthText->setString(health.toString());
	sf::FloatRect textBounds = healthText->getLocalBounds();
	healthText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f });
	healthText->setPosition({ centerX, barY + barHeight / 2.0f });
};

void Enemy::initFonts()
{
	fonts = new Fonts("Fonts/arial.ttf");
};

void Enemy::showDamage(BigNumber damage, bool isCritical)
{
	sf::FloatRect bounds = sprites->getGlobalBounds();

	float leftX = bounds.position.x;
	float rightX = bounds.position.x + bounds.size.x;
	float topY = bounds.position.y - 2.0f;
	float randomX = Randomize::returnRandomizeFloat(leftX, rightX);

	Text* newText = new Text(damage.toString() + (isCritical ? "!" : ""), fonts);
	newText->setFillColor(isCritical ? sf::Color(255, 165, 0, 255) : sf::Color::Red);
	newText->setOutlineColor(sf::Color::Black);
	newText->setOutlineThickness(2.0f);
	newText->setOrigin(newText->getGlobalBounds().size / 2.0f);
	newText->setPosition({ randomX, topY });

	damageNumbers.push_back({ newText, 0.0f });

	// Safety net against Autoclicker: prevents unlimited growth
	const size_t maxSimultaneousNumbers = 40;
	if (damageNumbers.size() > maxSimultaneousNumbers)
	{
		delete damageNumbers.front().text;
		damageNumbers.erase(damageNumbers.begin());
	}
};

void Enemy::initBossTimer()
{
	bossTimeRemaining = bossTimeLimit;

	const float barWidth = 100.0f;
	const float barHeight = 10.0f;

	timerBarBackground.setSize({ barWidth, barHeight });
	timerBarBackground.setFillColor(sf::Color(60, 60, 60));
	timerBarBackground.setOutlineColor(sf::Color::Black);
	timerBarBackground.setOutlineThickness(1.0f);
	timerBarBackground.setOrigin({ barWidth / 2.0f, 0.0f });

	timerBarFill.setSize({ barWidth, barHeight });
	timerBarFill.setFillColor(sf::Color(220, 180, 40));
	timerBarFill.setOrigin({ 0.0f, 0.0f });

	timerText = new Text("15.0", fonts);
	timerText->setFillColor(sf::Color::White);
	timerText->setOutlineColor(sf::Color::Black);
	timerText->setOutlineThickness(2.0f);
	timerText->setCharacterSize(14);
};

void Enemy::updateTimerBar()
{
	sf::FloatRect bounds = sprites->getGlobalBounds();
	const float margin = 30.0f;   // a somewhat bigger margin so that Timer-Bar and Health-Bar don't overlap
	const float barWidth = 100.0f;
	const float barHeight = 10.0f;

	float centerX = bounds.position.x + bounds.size.x / 2.0f;
	float barY = bounds.position.y + bounds.size.y + margin;

	timerBarBackground.setPosition({ centerX, barY });

	double ratio = bossTimeRemaining / bossTimeLimit;
	if (ratio < 0.0) ratio = 0.0;

	timerBarFill.setSize({ barWidth * static_cast<float>(ratio), barHeight });
	timerBarFill.setPosition({ centerX - barWidth / 2.0f, barY });

	std::ostringstream out;
	out << std::fixed << std::setprecision(1) << bossTimeRemaining;
	timerText->setString(out.str());

	sf::FloatRect textBounds = timerText->getLocalBounds();
	timerText->setOrigin({ textBounds.position.x + textBounds.size.x + 8.0f, textBounds.position.y + textBounds.size.y / 2.0f });
	timerText->setPosition({ centerX - barWidth / 2.0f, barY + barHeight / 2.0f });   // left next to the bar
};

Enemy::Enemy(std::string spriteAddress, sf::Vector2u spriteSize, BigNumber maxHealth, float frameTimerMax,
	std::string hurtSpriteAddress, sf::Vector2u hurtSpriteSize, float hurtFrameTimerMax,
	std::string deathSpriteAddress, sf::Vector2u deathSpriteSize, float deathFrameTimerMax, BigNumber gold,
	bool isBoss, ElementalAffinity elementalAffinity)
	: EnemyType(spriteAddress, spriteSize, maxHealth, frameTimerMax,
		hurtSpriteAddress, hurtSpriteSize, hurtFrameTimerMax,
		deathSpriteAddress, deathSpriteSize, deathFrameTimerMax, gold), isBoss(isBoss)
{
	this->elementalAffinity = elementalAffinity; // EnemyType's own constructor has no affinity param, so apply it here instead
	initHealth(maxHealth);
	initFonts();
	initHealthBar();

	sprites = new Sprites();
	sprites->addAnimation("idle", spriteAddress, spriteSize, frameTimerMax);
	sprites->addAnimation("hurt", hurtSpriteAddress, hurtSpriteSize, hurtFrameTimerMax);
	sprites->addAnimation("death", deathSpriteAddress, deathSpriteSize, deathFrameTimerMax);
	sprites->playAnimation("idle");

	if (!isBoss)
	{
		sprites->setScale({ spriteScale, spriteScale });
	}
	else
	{
		initBossTimer();
		sprites->setScale({ 5.0f, 5.0f });
	}
};

Enemy::~Enemy()
{
	delete sprites;
	for (auto& dmg : damageNumbers)
	{
		delete dmg.text;
	}
	delete fonts;
	delete healthText;
	if (timerText != nullptr)
	{
		delete timerText;
	}
};

BigNumber Enemy::getHealth() const
{
	return health;
};

const ElementalAffinity& Enemy::getElementalAffinity() const
{
	return elementalAffinity; // inherited from EnemyType
};

bool Enemy::isClicked(sf::Vector2f mousePos) const
{
	return sprites->getGlobalBounds().contains(mousePos);
}

bool Enemy::isDead() const
{
	return health <= BigNumber(0);
};

bool Enemy::isDeathAnimationFinished() const
{
	return deathAnimationStarted && !sprites->isOneShotPlaying();
};

bool Enemy::isShowingDamageText() const
{
	return !damageNumbers.empty();
};

Sprites* Enemy::getSprites() const
{
	return sprites;
};

bool Enemy::getIsDeathAnimationStarted() const
{
	return deathAnimationStarted;
};

void Enemy::takeDamage(BigNumber damage, bool isCritical)
{
	health = health - damage;
	this->showDamage(damage, isCritical);
	updateHealthBar();
	sprites->playAnimation("hurt", true, "idle"); // plays once, then back to idle state
};

void Enemy::setHealth(BigNumber newHealth)
{
	health = newHealth;
	if (health < BigNumber(0)) health = BigNumber(0);
	updateHealthBar();
};

void Enemy::updateDamageText(float dt)
{
	for (auto it = damageNumbers.begin(); it != damageNumbers.end(); )
	{
		it->elapsed += dt;
		float t = it->elapsed / damageTextDuration;

		if (t >= 1.0f)
		{
			delete it->text;
			it = damageNumbers.erase(it);
			continue;
		}

		sf::Vector2f pos = it->text->getPosition();
		pos.y -= 30.0f * dt;
		it->text->setPosition(pos);

		std::uint8_t alpha = static_cast<std::uint8_t>(255 * (1.0f - t));

		sf::Color fillColor = it->text->getFillColor();
		fillColor.a = alpha;
		it->text->setFillColor(fillColor);

		sf::Color outlineColor = it->text->getOutlineColor();
		outlineColor.a = alpha;
		it->text->setOutlineColor(outlineColor);

		++it;
	}
};

void Enemy::drawDamageText(sf::RenderWindow* window)
{
	for (auto& dmg : damageNumbers)
	{
		window->draw(*dmg.text);
	}
};

void Enemy::drawHealthBar(sf::RenderWindow* window)
{
	window->draw(healthBarBackground);
	window->draw(healthBarFill);
	window->draw(*healthText);
};

void Enemy::refreshHealthBar()
{
	updateHealthBar();
};

void Enemy::loadDeathAnimation()
{
	if (!deathAnimationStarted)
	{
		sprites->playAnimation("death", true, "idle"); // plays once, then back to idle state
		deathAnimationStarted = true;
	}
};

bool Enemy::getIsBoss() const
{
	return isBoss;
};

void Enemy::updateBossTimer(float dt)
{
	if (!isBoss || deathAnimationStarted) return;

	bossTimeRemaining -= dt;
	if (bossTimeRemaining < 0.0f)
	{
		bossTimeRemaining = 0.0f;
	}

	updateTimerBar();
};

bool Enemy::isBossTimeUp() const
{
	return isBoss && bossTimeRemaining <= 0.0f;
};

void Enemy::drawBossTimer(sf::RenderWindow* window)
{
	if (!isBoss) return;

	window->draw(timerBarBackground);
	window->draw(timerBarFill);
	window->draw(*timerText);
};

float Enemy::getBossTimeElapsed() const
{
	return bossTimeLimit - bossTimeRemaining;
};