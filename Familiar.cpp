#include "stdafx.h"
#include "Familiar.h"
#include "PublicVariables.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>


void Familiar::initSprites(std::string idleAnimationAddress, float idleFrameTimerMax, sf::Vector2u spriteSize, const std::vector<FamiliarAttack>& attackList)
{
	sprites = new Sprites();
	sprites->addAnimation("idle", idleAnimationAddress, spriteSize, idleFrameTimerMax);
	for (const auto& attack : attackList)
	{
		sprites->addAnimation(attack.animationKey, attack.animationAddress, spriteSize, attack.animationFrameTimerMax);
	}
	applyStackedPosition(); // left side of the game area, facing the enemy - defaults to centered (alone in a stack of 1) until Game calls setSlotPosition() once this familiar is actually assigned to a slot
	sprites->playAnimation("idle");
}

void Familiar::setupAttacks(std::vector<FamiliarAttack> attackList)
{
	for (auto& attack : attackList)
	{
		attack.cooldownRemaining = 0.0f; // start ready
		if (attack.isRanged)
		{
			// frameCount passed explicitly for clarity/robustness - Textures::Textures() auto-detects it
			// from sheet width / frame width when 0 is passed, but being explicit avoids relying on that
			// for sheets that might later include padding frames not meant for this animation.
			attack.projectileTexture = new Textures(attack.projectileAnimationAddress, attack.projectileSpriteSize, attack.projectileFrameCount);
			attack.projectileTexture->setFrameTimerMax(attack.projectileFrameTimerMax);
			attack.projectileTexture->setFrameTimer(attack.projectileFrameTimerMax);
			attack.projectileTexture->setSmooth(false);
		}
		if (attack.spawnsEnemyEffect)
		{
			attack.enemyEffectTexture = new Textures(attack.enemyEffectAnimationAddress, attack.enemyEffectSpriteSize, attack.enemyEffectFrameCount);
			attack.enemyEffectTexture->setFrameTimerMax(attack.enemyEffectFrameTimerMax);
			attack.enemyEffectTexture->setFrameTimer(attack.enemyEffectFrameTimerMax);
			attack.enemyEffectTexture->setSmooth(false);
		}
	}
	attacks = std::move(attackList);
}

void Familiar::buildRingArc(sf::VertexArray& target, sf::Vector2f center, float innerRadius, float outerRadius, float startAngleDeg, float sweepAngleDeg, int maxSegments, sf::Color color)
{
	target.clear();
	target.setPrimitiveType(sf::PrimitiveType::TriangleStrip);
	if (sweepAngleDeg <= 0.0f) return;

	int segments = std::max(1, static_cast<int>(maxSegments * (sweepAngleDeg / 360.0f))); // fewer segments for a smaller slice - cheap and still smooth
	const float pi = 3.14159265f;

	for (int i = 0; i <= segments; i++)
	{
		float t = static_cast<float>(i) / static_cast<float>(segments);
		float angleRad = (startAngleDeg + sweepAngleDeg * t) * pi / 180.0f;
		sf::Vector2f dir(std::cos(angleRad), std::sin(angleRad)); // screen Y is down, so increasing angle sweeps clockwise

		sf::Vertex outer;
		outer.position = center + dir * outerRadius;
		outer.color = color;

		sf::Vertex inner;
		inner.position = center + dir * innerRadius;
		inner.color = color;

		target.append(outer);
		target.append(inner);
	}
}

void Familiar::initCooldownIndicator()
{
	cooldownText = new Text("", "Fonts/arial.ttf");
	cooldownText->setCharacterSize(14);
	cooldownText->setFillColor(sf::Color::White);
	cooldownText->setOutlineColor(sf::Color::Black);
	cooldownText->setOutlineThickness(1.0f);

	sf::Vector2f center = restPosition + sf::Vector2f(0.0f, cooldownRingOffsetY);
	buildRingArc(cooldownRingTrack, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f, cooldownRingSegments, sf::Color(50, 50, 50, 200));
}

float Familiar::getEffectiveCooldown(const FamiliarAttack& attack) const
{
	if (!attack.isAutoAttack) return attack.baseCooldown; // bonusCooldownReduction (ShopTab) only ever applies to auto-attacks
	return std::max(attack.minCooldownLimit, attack.baseCooldown - bonusCooldownReduction);
}

void Familiar::updateCooldownIndicator()
{
	auto it = std::find_if(attacks.begin(), attacks.end(), [](const FamiliarAttack& a) { return a.isAutoAttack; });
	if (it == attacks.end()) return; // this familiar has no auto-attack to show - ring stays at its static "track" appearance

	// Anchored to restPosition rather than sprites->getPosition() - the cooldown indicator
	// shouldn't follow the familiar's sprite when self-movement (e.g. Bubble Stream's leap)
	// lifts it off the ground; it stays put at the familiar's normal spot.
	sf::Vector2f center = restPosition + sf::Vector2f(0.0f, cooldownRingOffsetY);

	float fullCooldown = getEffectiveCooldown(*it);
	float progress = fullCooldown > 0.0f ? 1.0f - (it->cooldownRemaining / fullCooldown) : 1.0f; // 0 = just attacked, 1 = ready
	progress = std::clamp(progress, 0.0f, 1.0f);

	buildRingArc(cooldownRingProgress, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f * progress, cooldownRingSegments, sf::Color(240, 140, 50));

	std::ostringstream out;
	out << std::fixed << std::setprecision(1) << std::max(0.0f, it->cooldownRemaining);
	cooldownText->setString(out.str());

	sf::FloatRect textBounds = cooldownText->getLocalBounds();
	cooldownText->setOrigin({ textBounds.position.x + textBounds.size.x / 2.0f, textBounds.position.y + textBounds.size.y / 2.0f });
	cooldownText->setPosition(center);
}

void Familiar::syncDPSTrackerWindow()
{
	auto it = std::find_if(attacks.begin(), attacks.end(), [](const FamiliarAttack& a) { return a.isAutoAttack; });
	familiarDPSTracker.setWindowDuration(it != attacks.end() ? getEffectiveCooldown(*it) : 5.0f); // fallback window for a familiar with only manual attacks
}

Familiar::Familiar(FamiliarType familiarType)
{
	name = familiarType.name;
	currentAge = familiarType.age;
	label = familiarType.label;
	type = familiarType.type;

	initSprites(familiarType.idleAnimationAddress, familiarType.idleFrameTimerMax, familiarType.spriteSize, familiarType.attacks);
	setupAttacks(familiarType.attacks);
	syncDPSTrackerWindow();

	initCooldownIndicator();
}

Familiar::~Familiar()
{
	delete sprites;
	delete cooldownText;
	for (auto& attack : attacks) { delete attack.projectileTexture; delete attack.enemyEffectTexture; }
}

void Familiar::applyStackedPosition()
{
	// Splits the window height into currentTotalSlots equal bands and centers this familiar
	// within its own band - with 1 slot that's exactly the old hardcoded "centered vertically"
	// position; with more, each familiar gets an even share of the left side instead of
	// overlapping. std::max guards against a div-by-zero if this is ever somehow called with 0.
	float bandHeight = windowSize.y / static_cast<float>(std::max(1, currentTotalSlots));
	float y = bandHeight * (static_cast<float>(currentSlotIndex) + 0.5f);
	restPosition = { 60.0f, y };
	sprites->setPosition(restPosition);
}

void Familiar::setSlotPosition(int slotIndex, int totalActiveSlots)
{
	currentSlotIndex = slotIndex;
	currentTotalSlots = totalActiveSlots;
	applyStackedPosition();

	// The cooldown ring is drawn relative to restPosition (see cooldownRingOffsetY), so
	// it needs rebuilding at its new center too - same as onResize() already does below.
	sf::Vector2f center = restPosition + sf::Vector2f(0.0f, cooldownRingOffsetY);
	buildRingArc(cooldownRingTrack, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f, cooldownRingSegments, sf::Color(50, 50, 50, 200));
}

void Familiar::onResize()
{
	applyStackedPosition(); // re-applies the SAME row/stack-size Game last told us via setSlotPosition() - the window changing size doesn't change where in the stack this familiar belongs

	sf::Vector2f center = restPosition + sf::Vector2f(0.0f, cooldownRingOffsetY);
	buildRingArc(cooldownRingTrack, center, cooldownRingInnerRadius, cooldownRingOuterRadius, -90.0f, 360.0f, cooldownRingSegments, sf::Color(50, 50, 50, 200));
}

void Familiar::startAttack(FamiliarAttack& attack, sf::Vector2f targetPosition)
{
	if (!attack.isAutoAttack)
	{
		attack.resetCooldown(getEffectiveCooldown(attack)); // manual attacks go on cooldown the instant they're activated
	}
	attack.beginActivation();

	// See the comment on this method in Familiar.h for the full priority rule: a manual attack
	// may preempt a currently-playing AUTO-attack's animation, but nothing may ever preempt a
	// currently-playing MANUAL attack's animation, and an auto-attack never preempts anything.
	bool spriteIsFree = !sprites->isOneShotPlaying();
	bool manualPreemptsAuto = !attack.isAutoAttack && !currentOneShotIsManual;

	if (spriteIsFree || manualPreemptsAuto)
	{
		sprites->playAnimation(attack.animationKey, true, "idle");
		currentOneShotIsManual = !attack.isAutoAttack;

		if (attack.hasLoop())
		{
			// Armed from the very start of playback - the loop only actually takes effect once
			// the animation naturally advances past loopEndFrame for the first time (see
			// Textures::frameRight()), so configuring it now is safe even though frame 1 is
			// nowhere near the loop range yet.
			sprites->setAnimationLoopRange(attack.animationKey, attack.loopStartFrame, attack.loopEndFrame);
			sprites->setAnimationLooping(attack.animationKey, true);
		}
	}
}

void Familiar::resolveAttack(FamiliarAttack& attack, sf::Vector2f targetPosition)
{
	bool isFirstShot = (attack.shotsFiredThisActivation == 0);

	// Bonus stats (purchased via ShopTab) only ever apply to auto-attacks - a manually
	// activated ability runs on its own base stats alone.
	BigNumber physical = attack.usesPhysicalDamage
		? attack.basePhysicalPower + (attack.isAutoAttack ? bonusPhysicalPower : BigNumber(0))
		: BigNumber(0);

	// addMatching() only folds the bonus into elements THIS attack's base power already carries -
	// e.g. a bonus purchased while this familiar's auto-attack was Fire-only won't leak into a
	// different, Water-only ability this same familiar might also have.
	ElementalPower elemental = attack.usesElementalDamage
		? (attack.isAutoAttack ? attack.baseElementalPower.addMatching(bonusElementalPower) : attack.baseElementalPower)
		: ElementalPower();

	if (attack.isRanged)
	{
		sf::Vector2f spawnPosition = sprites->getPosition() + attack.projectileSpawnOffset;
		activeProjectiles.emplace_back(spawnPosition, targetPosition, attack.projectileTravelDuration,
			physical, elemental, *attack.projectileTexture, attack.projectileScale,
			attack.projectileMotion, attack.projectileMotionAmplitude, attack.projectileMotionCycles);
	}
	else
	{
		// melee - resolved against whatever's on the field right now, same as the hero's own
		// click damage. computeDamage() resolves each active element's own multiplier
		// independently (no averaging across elements) - see ElementalPower::computeDamage.
		BigNumber elementalDamage = elemental.computeDamage(elementalMultiplierResolver);
		BigNumber finalDamage = physical + elementalDamage;
		familiarDPSTracker.recordDamage(finalDamage);
		if (onDamageDealt) onDamageDealt(finalDamage, elemental.getMask());
	}

	if (attack.spawnsEnemyEffect)
	{
		sf::Vector2f effectPosition = targetPosition + attack.enemyEffectSpawnOffset;
		activeEffects.emplace_back(effectPosition, effectPosition, attack.enemyEffectDuration,
			BigNumber(0), ElementalPower(), *attack.enemyEffectTexture, attack.enemyEffectScale);
	}

	if (attack.customEffect) attack.customEffect(*this, attack, targetPosition);

	if (isFirstShot && attack.isAutoAttack)
	{
		// The cooldown ring reaches 0 exactly as this shot lands - see the lead-in windup
		// trigger in update(), which starts this attack's animation early enough that its
		// active frame arrives right when the cooldown would otherwise hit 0.
		attack.resetCooldown(getEffectiveCooldown(attack));
		attack.windupStarted = false; // allow the next cycle's windup to begin
	}

	// registerShotFired() decides whether this activation still owes more shots (and arms the
	// timing for the next one) - once it says no, let this attack's loop (if it has one) finish
	// its current pass and continue on rather than wrap around again.
	if (!attack.registerShotFired() && attack.hasLoop())
	{
		sprites->setAnimationLooping(attack.animationKey, false);
	}
}

void Familiar::update(float dt, sf::Vector2f targetPosition)
{
	if (!unlocked) return;

	sprites->animate(dt);
	familiarDPSTracker.update(dt);

	for (auto& attack : attacks)
	{
		attack.tick(dt);
		if (attack.isAutoAttack && !attack.windupStarted)
		{
			float leadTime = std::min(attack.getActiveFrameDelay(), getEffectiveCooldown(attack));
			if (attack.cooldownRemaining <= leadTime)
			{
				attack.windupStarted = true;
				startAttack(attack, targetPosition);
			}
		}

		if (!attack.hasPendingShot()) continue;

		if (sprites->isAnimationCurrentlyShowing(attack.animationKey))
		{
			// This attack's own animation is the one actually playing - fire the instant its
			// real displayed frame becomes the one we're waiting for. There's only one clock
			// here (the sprite's own), so this can never drift from what's on screen.
			attack.fallbackArmed = false; // clears any earlier fallback wait, in case this attack loses visibility again before its next shot
			unsigned int newFrame;
			if (sprites->consumeFrameChange(attack.animationKey, newFrame) &&
				newFrame == static_cast<unsigned int>(attack.pendingShotTargetFrame))
			{
				resolveAttack(attack, targetPosition);
			}
		}
		else
		{
			// A manual attack preempted this one's animation before it got there - it's frozen
			// now and won't resume this activation, so fall back to a plain elapsed-time
			// countdown instead, seeded once from wherever it got frozen, so the shot still
			// lands exactly on schedule even though it's no longer visible (see the comment on
			// fallbackCountdown in FamiliarAttack.h).
			if (!attack.fallbackArmed)
			{
				attack.fallbackFrame = sprites->getCurrentFrame(attack.animationKey);
				attack.fallbackCountdown = attack.framesToSeconds(attack.fallbackFrame, static_cast<unsigned int>(attack.pendingShotTargetFrame));
				attack.fallbackArmed = true;
			}

			attack.fallbackCountdown -= dt;
			if (attack.fallbackCountdown <= 0.0f)
			{
				attack.fallbackFrame = static_cast<unsigned int>(attack.pendingShotTargetFrame);
				resolveAttack(attack, targetPosition);
				if (attack.hasPendingShot())
				{
					attack.fallbackCountdown = attack.framesToSeconds(attack.fallbackFrame, static_cast<unsigned int>(attack.pendingShotTargetFrame));
				}
			}
		}
	}

	for (auto activeProjectile = activeProjectiles.begin(); activeProjectile != activeProjectiles.end(); )
	{
		bool arrived = activeProjectile->update(dt);
		if (arrived)
		{
			// Resolve weakness/resistance now, against whatever enemy is actually on the field
			// at the moment of impact - not against whatever was there when the projectile
			// launched. computeDamage() resolves each element this projectile carries against
			// its own multiplier independently, with no averaging.
			BigNumber finalDamage = activeProjectile->getPhysicalDamage() + activeProjectile->getElementalPower().computeDamage(elementalMultiplierResolver);

			familiarDPSTracker.recordDamage(finalDamage);
			if (onDamageDealt) onDamageDealt(finalDamage, activeProjectile->getElementalPower().getMask()); // Game applies the damage to whatever is currently at that spot
			activeProjectile = activeProjectiles.erase(activeProjectile);
		}
		else
		{
			++activeProjectile;
		}
	}

	for (auto activeEffect = activeEffects.begin(); activeEffect != activeEffects.end(); )
	{
		if (activeEffect->update(dt)) activeEffect = activeEffects.erase(activeEffect);
		else ++activeEffect;
	}

	updateSelfMovement();
}

void Familiar::updateSelfMovement()
{
	float lift = 0.0f;
	for (const auto& attack : attacks)
	{
		if (attack.hasSelfMovement() && sprites->isAnimationCurrentlyShowing(attack.animationKey))
		{
			lift = attack.getSelfMovementLift(sprites->getCurrentFrame(attack.animationKey));
			break; // only one animation can be showing at a time
		}
	}
	sprites->setPosition(restPosition - sf::Vector2f(0.0f, lift)); // screen Y grows downward - subtracting lift moves it up
}

void Familiar::draw(sf::RenderWindow* window)
{
	if (!unlocked) return;
	window->draw(*sprites);

	for (const auto& projectile : activeProjectiles)
	{
		projectile.draw(window);
	}

	for (const auto& effect : activeEffects)
	{
		effect.draw(window);
	}
}

void Familiar::drawHUD(sf::RenderWindow* window)
{
	if (!unlocked) return;

	updateCooldownIndicator();
	window->draw(cooldownRingTrack);
	window->draw(cooldownRingProgress);
	window->draw(*cooldownText);
}

bool Familiar::isUnlocked() const
{
	return unlocked;
}

void Familiar::unlock()
{
	unlocked = true;
}

size_t Familiar::getAttackCount() const
{
	return attacks.size();
}

const FamiliarAttack& Familiar::getAttack(size_t index) const
{
	return attacks[index];
}

bool Familiar::isManualAttackReady(size_t attackIndex) const
{
	if (!unlocked || attackIndex >= attacks.size()) return false;
	const FamiliarAttack& attack = attacks[attackIndex];
	return !attack.isAutoAttack && attack.isReady();
}

bool Familiar::activateManualAttack(size_t attackIndex, sf::Vector2f targetPosition)
{
	if (!isManualAttackReady(attackIndex)) return false;
	startAttack(attacks[attackIndex], targetPosition);
	return true;
}

void Familiar::setOnDamageDealtCallback(std::function<void(BigNumber damage, ElementMask mask)> callback)
{
	onDamageDealt = std::move(callback);
}

void Familiar::setElementalMultiplierResolver(std::function<double(Elements)> resolver)
{
	elementalMultiplierResolver = std::move(resolver);
}

BigNumber Familiar::rollDamage() const
{
	// A rough "power level" preview of the primary auto-attack's base + bonus stats - no
	// weakness/resistance applied. Manual abilities aren't represented here since bonuses never
	// apply to them anyway.
	auto it = std::find_if(attacks.begin(), attacks.end(), [](const FamiliarAttack& a) { return a.isAutoAttack; });
	if (it == attacks.end()) return BigNumber(0);

	BigNumber physical = it->usesPhysicalDamage ? it->basePhysicalPower + bonusPhysicalPower : BigNumber(0);
	BigNumber elemental = it->usesElementalDamage ? it->baseElementalPower.addMatching(bonusElementalPower).getTotal() : BigNumber(0);
	return physical + elemental;
}

BigNumber Familiar::getBonusPhysicalPower() const
{
	return bonusPhysicalPower;
}

BigNumber Familiar::getBonusElementalPower(Elements element) const
{
	return bonusElementalPower.get(element);
}

void Familiar::increasePhysicalAttack(BigNumber amount)
{
	bonusPhysicalPower = bonusPhysicalPower + amount;
}

void Familiar::increaseElementalAttack(BigNumber amount)
{
	// Applied to every element the primary auto-attack's base power carries - covers today's
	// single-element case and a future multi-element auto-attack (e.g. Fire+Water) alike,
	// without ShopTab needing to know which element(s) are involved.
	auto it = std::find_if(attacks.begin(), attacks.end(), [](const FamiliarAttack& a) { return a.isAutoAttack && a.usesElementalDamage; });
	if (it == attacks.end()) return; // this familiar's auto-attack doesn't use elemental damage (yet) - nothing to upgrade

	ElementMask mask = it->baseElementalPower.getMask();
	for (int i = 0; i < static_cast<int>(ElementCount); i++)
	{
		Elements element = static_cast<Elements>(i);
		if (mask & elementBit(element)) bonusElementalPower.add(element, amount);
	}
}

void Familiar::increaseCooldownReduction(float amount)
{
	bonusCooldownReduction += amount;
	syncDPSTrackerWindow(); // the DPS averaging window tracks the primary auto-attack's effective cooldown, which just changed
}

bool Familiar::isAutoAttackCooldownAtMinimum() const
{
	for (const auto& attack : attacks)
	{
		if (attack.isAutoAttack && getEffectiveCooldown(attack) > attack.minCooldownLimit) return false;
	}
	return true; // true if every auto-attack (if any) is already capped at its own floor
}

void Familiar::setName(FamiliarName name)
{
	this->name = name;
}

FamiliarName Familiar::getName() const
{
	return name;
}

void Familiar::setAge(Age age)
{
	currentAge = age;
}

Age Familiar::getAge() const
{
	return currentAge;
}

void Familiar::setBonusPhysicalPower(BigNumber value)
{
	bonusPhysicalPower = value;
}

void Familiar::setBonusElementalPower(Elements element, BigNumber value)
{
	bonusElementalPower.set(element, value);
}

void Familiar::setBonusCooldownReduction(float value)
{
	bonusCooldownReduction = value;
	syncDPSTrackerWindow();
}

float Familiar::getBonusCooldownReduction() const
{
	return bonusCooldownReduction;
}

float Familiar::getAttackCooldown(size_t attackIndex) const
{
	return attackIndex < attacks.size() ? getEffectiveCooldown(attacks[attackIndex]) : 0.0f;
}

void Familiar::evolveTo(FamiliarType newType)
{
	name = newType.name; // should already equal the current species - evolving never changes WHICH species this is, only its stage
	currentAge = newType.age;
	label = newType.label;
	type = newType.type;

	delete sprites;
	initSprites(newType.idleAnimationAddress, newType.idleFrameTimerMax, newType.spriteSize, newType.attacks);

	for (auto& attack : attacks) { delete attack.projectileTexture; delete attack.enemyEffectTexture; } // free the outgoing stage's projectile/effect textures
	attacks.clear();
	setupAttacks(newType.attacks);
	syncDPSTrackerWindow();

	// projectiles already in flight were built from the old sprite/element - let them finish
	// as the previous stage's attack rather than mid-air become the new dragon's
	activeProjectiles.clear();

	// bonusPhysicalPower, bonusElementalPower, bonusCooldownReduction and unlocked are
	// deliberately left untouched here - evolving keeps purchased upgrade progress, it doesn't
	// reset it. `attacks` above was just rebuilt from newType's BASE stats only, so the new
	// stage's effective stats/cooldown are naturally base(new stage) combined with these
	// unchanged bonuses the next time they're read (see getEffectiveCooldown/resolveAttack) -
	// no separate "carry the old cooldown reduction over" step is needed.
}

BigNumber Familiar::getFamiliarDPS() const
{
	return familiarDPSTracker.getDPS();
}

float Familiar::getAttackCooldownRemaining(size_t attackIndex) const
{
	return attackIndex < attacks.size() ? attacks[attackIndex].cooldownRemaining : 0.0f;
}

Familiar::State Familiar::captureState() const
{
	State state;
	state.bonusPhysicalPower = bonusPhysicalPower;
	state.bonusElementalPower = bonusElementalPower;
	state.bonusCooldownReduction = bonusCooldownReduction;
	state.attacks = attacks;
	state.activeProjectiles = activeProjectiles;
	state.activeEffects = activeEffects;
	state.currentOneShotIsManual = currentOneShotIsManual;
	state.dpsTrackerState = familiarDPSTracker.captureState();
	state.spritesState = sprites->captureState();
	return state;
}

void Familiar::restoreState(const State& state)
{
	bonusPhysicalPower = state.bonusPhysicalPower;
	bonusElementalPower = state.bonusElementalPower;
	bonusCooldownReduction = state.bonusCooldownReduction;
	attacks = state.attacks;
	activeProjectiles = state.activeProjectiles;
	activeEffects = state.activeEffects;
	currentOneShotIsManual = state.currentOneShotIsManual;
	familiarDPSTracker.restoreState(state.dpsTrackerState);
	sprites->restoreState(state.spritesState);

	updateCooldownIndicator();
}
