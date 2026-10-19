#include "Stage.h"
#include "Game.h"
#include "PublicVariables.h"
#include "LayoutManager.h"
#include <algorithm>
#include <random>

void Stage::initStageNumber()
{
	stageNumber = 1;
}

void Stage::initStageText()
{
	stageText = new Text("Stage: " + std::to_string(stageNumber), "Fonts/arial.ttf");
	stageText->setCharacterSize(24);
	stageText->setFillColor(sf::Color::White);
	stageText->setOutlineColor(sf::Color::Black);
	stageText->setOutlineThickness(1.0f);
	stageText->setPosition({ (windowSize.x - uiPanelWidth) / 2.0f, 20.0f });
	this->updateStageText();
}

void Stage::initEnemyInfoText()
{
	enemyInfoText = new Text("", "Fonts/arial.ttf");
	enemyInfoText->setCharacterSize(18);
	enemyInfoText->setFillColor(sf::Color::White);
	enemyInfoText->setOutlineColor(sf::Color::Black);
	enemyInfoText->setOutlineThickness(1.0f);
	enemyInfoText->setPosition({ (windowSize.x - uiPanelWidth) / 2.0f, 50.0f });
	this->setEnemyInfoText("");
}

void Stage::initMapSize()
{
	// Not accurate, but it works for now. Adding 1 to the size to account for the last tile that is not fully visible on the screen.
	this->mapSizeX = 1 + static_cast<int>(gameAreaWidth / tileSize);
	this->mapSizeY = 1 + static_cast<int>(windowSize.y / tileSize);

	// Safety net against negative or NULL values
	if (mapSizeX < 1) mapSizeX = 1;
	if (mapSizeY < 1) mapSizeY = 1;
}

void Stage::initTileTextures()
{
	// Tileset from: https://gametoolkit.itch.io/top-down-forest-tileset
	//
	// Each tile is fully described by which of its 4 corners (NW/NE/SW/SE) are dirt vs grass.
	// Two tiles are compatible wherever the corner points where they touch agree - e.g. a tile
	// can sit to the right of another only if its NW/SW corners match that tile's NE/SE corners.
	// This is a standard "dual-grid" / corner-based autotile set: as long as all 16 possible
	// corner combinations are covered exactly once (as below), every possible combination of
	// neighbors always has a valid answer, so there's no adjacency list to maintain and no
	// fallback/relaxation logic needed - see rebuildTileGrid().
	tileDefs =
	{
		// name            texture                                                NW      NE      SW      SE
		{ "Grass",         "Textures/SlicedWoods/Grass_Dirt/sprite_0024.png",      false,  false,  false,  false },
		{ "Dirt",          "Textures/SlicedWoods/Grass_Dirt/sprite_0009.png",      true,   true,   true,   true  },
		{ "Grass8Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0001.png",      false,  false,  true,   true  }, // dirt on south edge
		{ "Grass2Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0017.png",      true,   true,   false,  false }, // dirt on north edge
		{ "Grass4Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0008.png",      false,  true,   false,  true  }, // dirt on east edge
		{ "Grass6Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0010.png",      true,   false,  true,   false }, // dirt on west edge
		{ "Grass7Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0000.png",      false,  false,  false,  true  }, // small dirt nub, SE corner
		{ "Grass9Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0002.png",      false,  false,  true,   false }, // small dirt nub, SW corner
		{ "Grass1Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0016.png",      false,  true,   false,  false }, // small dirt nub, NE corner
		{ "Grass3Dirt",    "Textures/SlicedWoods/Grass_Dirt/sprite_0018.png",      true,   false,  false,  false }, // small dirt nub, NW corner
		{ "RoundGrass7",   "Textures/SlicedWoods/Grass_Dirt/sprite_0003.png",      false,  true,   true,   true  }, // large rounded grass corner, NW
		{ "RoundGrass9",   "Textures/SlicedWoods/Grass_Dirt/sprite_0004.png",      true,   false,  true,   true  }, // large rounded grass corner, NE
		{ "RoundGrass1",   "Textures/SlicedWoods/Grass_Dirt/sprite_0011.png",      true,   true,   false,  true  }, // large rounded grass corner, SW
		{ "RoundGrass3",   "Textures/SlicedWoods/Grass_Dirt/sprite_0012.png",      true,   true,   true,   false }, // large rounded grass corner, SE
		{ "SplitGrass73",  "Textures/SlicedWoods/Grass_Dirt/sprite_0019.png",      false,  true,   true,   false }, // diagonal dirt nubs, NE + SW
		{ "SplitGrass91",  "Textures/SlicedWoods/Grass_Dirt/sprite_0020.png",      true,   false,  false,  true  }, // diagonal dirt nubs, NW + SE
	};

	cornerPatternToTileIndex.clear();
	for (int i = 0; i < static_cast<int>(tileDefs.size()); ++i)
	{
		const auto& def = tileDefs[i];
		int pattern = cornerPattern(def.cornerNW, def.cornerNE, def.cornerSW, def.cornerSE);

		auto existing = cornerPatternToTileIndex.find(pattern);
		if (existing != cornerPatternToTileIndex.end())
		{
			// Two tiles claiming the same corner pattern means one of them can never be picked -
			// almost certainly a copy/paste mistake in the corner values above.
			std::cerr << "WARNING::STAGE::INITTILETEXTURES::" << def.name << " HAS THE SAME CORNER PATTERN AS "
				<< tileDefs[existing->second].name << " - " << def.name << " WILL NEVER BE PICKED" << std::endl;
		}
		cornerPatternToTileIndex[pattern] = i;
	}

	for (const auto& def : tileDefs)
	{
		sf::Texture* tex = new sf::Texture();
		if (tex->loadFromFile(def.texturePath))
		{
			tex->setSmooth(false);
			tileTextures.push_back(tex);
		}
		else
		{
			std::cerr << "ERROR::STAGE::INITTILETEXTURES::COULD NOT LOAD FILE!" << def.texturePath << std::endl;
			delete tex;
			tileTextures.push_back(nullptr); // keep indices aligned with tileDefs even if a load fails
		}
	}

	// Load decoration textures - pool is sprite_0002..sprite_0018 (sprite_0000, sprite_0001 and sprite_0019 are excluded).
	// The ground type each item belongs on (index 0 = sprite_0002) - true = dirt base, false = grass base.
	static const bool decorationGroundIsDirt[] = {
		false, // sprite_0002 - grass
		false, // sprite_0003 - grass
		false, // sprite_0004 - grass
		false, // sprite_0005 - grass
		true,  // sprite_0006 - dirt
		false, // sprite_0007 - grass
		false, // sprite_0008 - grass
		false, // sprite_0009 - grass
		true,  // sprite_0010 - dirt
		false, // sprite_0011 - grass
		true,  // sprite_0012 - dirt
		false, // sprite_0013 - grass
		true,  // sprite_0014 - dirt
		true,  // sprite_0015 - dirt
		true,  // sprite_0016 - dirt
		true,  // sprite_0017 - dirt
		false, // sprite_0018 - grass
	};

	for (int i = 2; i <= 18; ++i)
	{
		sf::Texture* tex = new sf::Texture();
		char filename[64];
		sprintf_s(filename, "Textures/SlicedWoods/sprite_%04d.png", i);
		if (tex->loadFromFile(filename))
		{
			tex->setSmooth(false);
			decorationTextures.push_back(tex);
		}
		else
		{
			std::cerr << "ERROR::STAGE::INITTILETEXTURES::COULD NOT LOAD DECORATION FILE!" << filename << std::endl;
			delete tex;
			decorationTextures.push_back(nullptr);
		}
		decorationIsDirt.push_back(decorationGroundIsDirt[i - 2]); // keep the ground type index-aligned with decorationTextures
	}
}

void Stage::initDecorations()
{
	decorations.clear();
	std::uniform_int_distribution<int> distX(0, mapSizeX - 1);
	std::uniform_int_distribution<int> distY(0, mapSizeY - 1);
	std::uniform_int_distribution<int> countDist(3, 4);

	// Split the pool into grass-based and dirt-based items (skipping any texture that failed to load),
	// so placement only ever picks from the items whose base matches the ground type of the chosen tile.
	std::vector<int> grassIndices;
	std::vector<int> dirtIndices;
	for (int i = 0; i < static_cast<int>(decorationTextures.size()); ++i)
	{
		if (!decorationTextures[i]) continue; // failed load - never place this one
		(decorationIsDirt[i] ? dirtIndices : grassIndices).push_back(i);
	}

	int numDecorations = countDist(rng);
	int placed = 0;
	int attempts = 0;
	const int maxAttempts = numDecorations * 100; // safety net so a tiny window can never spin us forever
	while (placed < numDecorations && attempts < maxAttempts)
	{
		++attempts;
		int x = distX(rng);
		int y = distY(rng);

		// Work in pixel space so the exclusion zones stay correct at any window size.
		const float cellCenterX = (x + 0.5f) * tileSize;

		// Left strip is reserved for familiars (they line up along the left edge of the play area).
		if (cellCenterX < gameAreaWidth * familiarZoneFraction) continue;

		// The middle band of the play area is where the hero and enemy fight - keep it clear of decorations.
		const float center = gameAreaWidth / 2.0f;
		const float halfBand = gameAreaWidth * centerBandHalfWidthFraction;
		if (cellCenterX > center - halfBand && cellCenterX < center + halfBand) continue;

		// Determine if it's dirt or grass based on the tile at (x, y)
		int tileIdx = tileIndices[y][x];
		const auto& def = tileDefs[tileIdx];
		bool isDirt = (def.cornerNW + def.cornerNE + def.cornerSW + def.cornerSE) >= 2;

		// Pick a texture from the pool that matches this ground type - each item's grass/dirt base is listed in decorationIsDirt.
		const std::vector<int>& candidates = isDirt ? dirtIndices : grassIndices;
		if (candidates.empty()) continue; // no loadable texture for this ground type - try another spot

		int texIdx = candidates[std::uniform_int_distribution<int>(0, static_cast<int>(candidates.size()) - 1)(rng)];

		Decoration dec;
		dec.tileX = x;
		dec.tileY = y;
		dec.textureIndex = texIdx;
		decorations.push_back(dec);
		placed++;
	}

	// Painter's algorithm: sort back-to-front (top of screen first) so decorations lower on the
	// screen are drawn last and correctly overlap the ones above them. All bases sit at their
	// tile center, so ordering by tileY is the same as ordering by pixel Y.
	std::stable_sort(decorations.begin(), decorations.end(),
		[](const Decoration& a, const Decoration& b) { return a.tileY < b.tileY; });
}

void Stage::initTileMap()
{
	initTileTextures();
	rebuildTileGrid();
	initDecorations();
}

int Stage::cornerPattern(bool nw, bool ne, bool sw, bool se) const
{
	return (nw ? 8 : 0) | (ne ? 4 : 0) | (sw ? 2 : 0) | (se ? 1 : 0);
}

void Stage::growCornerGrid()
{
	// The corner grid holds one dirt/grass value per grid VERTEX, not per cell - a mapSizeX x
	// mapSizeY grid of cells has (mapSizeX+1) x (mapSizeY+1) corner points, since each interior
	// point is shared by the 4 cells that touch it. Growing the map only ever adds new vertices
	// along the new bottom/right edge; every existing vertex - and therefore every already-visible
	// tile, since a tile is 100% determined by its 4 corners - is left completely untouched.
	int neededRows = mapSizeY + 1;
	int neededCols = mapSizeX + 1;

	std::vector<std::vector<bool>> newGrid(neededRows, std::vector<bool>(neededCols, false));
	std::uniform_real_distribution<float> chance(0.0f, 1.0f);

	for (int y = 0; y < neededRows; ++y)
	{
		for (int x = 0; x < neededCols; ++x)
		{
			bool alreadyExisted = y < static_cast<int>(cornerGrid.size()) && x < static_cast<int>(cornerGrid[y].size());
			if (alreadyExisted)
			{
				newGrid[y][x] = cornerGrid[y][x]; // keep exactly as it was
				continue;
			}

			// Lean toward whatever the already-placed neighbors are doing, so dirt forms loose
			// clumps/patches instead of single-vertex speckles. Filled top-to-bottom, left-to-right,
			// so up/left/up-left/up-right are always already assigned (old data or earlier this pass).
			int knownDirt = 0;
			int knownTotal = 0;
			auto sample = [&](int sy, int sx)
				{
					if (sy < 0 || sy >= neededRows || sx < 0 || sx >= neededCols) return;
					bool fromOld = sy < static_cast<int>(cornerGrid.size()) && sx < static_cast<int>(cornerGrid[sy].size());
					bool value = fromOld ? cornerGrid[sy][sx] : newGrid[sy][sx];
					++knownTotal;
					if (value) ++knownDirt;
				};
			sample(y - 1, x);
			sample(y, x - 1);
			sample(y - 1, x - 1);
			sample(y - 1, x + 1);

			float dirtChance = dirtPatchChance;
			if (knownTotal > 0)
			{
				float neighborRatio = static_cast<float>(knownDirt) / static_cast<float>(knownTotal);
				dirtChance = 0.3f * dirtPatchChance + 0.7f * neighborRatio; // weighted mostly toward neighbors
			}

			newGrid[y][x] = chance(rng) < dirtChance;
		}
	}

	cornerGrid = std::move(newGrid);
}

void Stage::rebuildTileGrid()
{
	growCornerGrid();

	tileIndices.assign(mapSizeY, std::vector<int>(mapSizeX, 0));
	tileMap.assign(mapSizeY, std::vector<sf::RectangleShape>(mapSizeX));

	for (int y = 0; y < mapSizeY; ++y)
	{
		for (int x = 0; x < mapSizeX; ++x)
		{
			bool nw = cornerGrid[y][x];
			bool ne = cornerGrid[y][x + 1];
			bool sw = cornerGrid[y + 1][x];
			bool se = cornerGrid[y + 1][x + 1];

			int pattern = cornerPattern(nw, ne, sw, se);
			int tileIndex = 0;
			auto it = cornerPatternToTileIndex.find(pattern);
			if (it != cornerPatternToTileIndex.end())
			{
				tileIndex = it->second;
			}
			else
			{
				// Should be unreachable as long as tileDefs covers all 16 corner combinations.
				std::cerr << "ERROR::STAGE::REBUILDTILEGRID::NO TILE DEFINED FOR CORNER PATTERN " << pattern << std::endl;
			}
			tileIndices[y][x] = tileIndex;

			tileMap[y][x].setSize(sf::Vector2f(tileSize, tileSize));
			tileMap[y][x].setPosition(sf::Vector2f((x * tileSize), (y * tileSize)));

			if (!tileTextures.empty())
			{
				sf::Texture* tex = tileTextures[tileIndex];
				if (!tex)
				{
					// This tile's texture failed to load - draw any loaded texture rather than a blank square.
					for (auto* t : tileTextures)
					{
						if (t) { tex = t; break; }
					}
				}
				tileMap[y][x].setTexture(tex);
			}
		}
	}
}

void Stage::updateStageText()
{
	stageText->setString("Stage: " + std::to_string(stageNumber));

	LayoutManager::applyAnchor(*stageText, Anchor::TopCenter, { 0.0f, 20.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

void Stage::setEnemyInfoText(const std::string& text)
{
	enemyInfoText->setString(text);
	LayoutManager::applyAnchor(*enemyInfoText, Anchor::TopCenter, { 0.0f, 50.0f }, { 0.0f, 0.0f }, { gameAreaWidth, windowSize.y });
}

Stage::Stage()
{
	this->initStageNumber();
	this->initStageText();
	this->initEnemyInfoText();
	this->initMapSize();
	this->initTileMap();
}

Stage::~Stage()
{
	for (auto* tex : tileTextures)
	{
		if (tex) delete tex;
	}
	for (auto* tex : decorationTextures)
	{
		if (tex) delete tex;
	}
	delete stageText;
	delete enemyInfoText;
}

int Stage::getStageNumber() const
{
	return stageNumber;
}

int Stage::getMapSizeX()
{
	return mapSizeX;
}

int Stage::getMapSizeY()
{
	return mapSizeY;
}

void Stage::setStageNumber(int newStageNumber)
{
	stageNumber = newStageNumber;
	this->updateStageText();
}

void Stage::onResize()
{
	initMapSize();       // recalculate mapSizeX/mapSizeY using the new gameAreaWidth/windowSize.y
	rebuildTileGrid();   // extends the corner grid and re-derives tiles - existing corners (and therefore existing tiles) are untouched
}

void Stage::drawDecoration(size_t index, sf::RenderWindow* window)
{
	const Decoration& dec = decorations[index];
	if (!decorationTextures[dec.textureIndex]) return; // texture failed to load - skip this decoration

	sf::Sprite sprite(*decorationTextures[dec.textureIndex]); // sf::Sprite requires a texture at construction in this SFML version

	// Same global scale as the hero/enemy/familiar sprites - this brings the tallest decorations (trees) to roughly the hero's on-screen height while keeping small props proportionally smaller.
	sprite.setScale({ spriteScale, spriteScale });

	// Anchor the decoration's base at the center of its tile so scaled-up sprites stay visually on the tile they were placed for (and keep their dirt/grass association).
	const sf::Vector2u texSize = decorationTextures[dec.textureIndex]->getSize();
	const float scaledW = static_cast<float>(texSize.x) * spriteScale;
	const float scaledH = static_cast<float>(texSize.y) * spriteScale;
	sprite.setPosition({ (dec.tileX + 0.5f) * tileSize - scaledW / 2.0f,
					(dec.tileY + 0.5f) * tileSize - scaledH });

	window->draw(sprite);
}

void Stage::drawMap(sf::RenderWindow* window, const std::vector<DepthActor>& actors, const std::vector<HudElement>& hudElements)
{
	// Draw the tile map (always at the very back)
	for (int y = 0; y < mapSizeY; ++y)
	{
		for (int x = 0; x < mapSizeX; ++x)
		{
			window->draw(tileMap[y][x]);
		}
	}

	// Depth-sort everything that stands on the ground: decorations AND actors (hero/familiar/enemy).
	// Both lists are ordered back-to-front by their ground-Y position, so a two-pointer merge can draw
	// whichever is further back first. That's what makes a tree below the hero cover his feet while a tree
	// above him stays behind - and keeps decorations overlapping each other correctly too.
	std::vector<DepthActor> actorsSorted(actors); // copy, so the caller's vector is left untouched (and std::function copies are cheap here)
	std::stable_sort(actorsSorted.begin(), actorsSorted.end(),
		[](const DepthActor& a, const DepthActor& b) { return a.groundY < b.groundY; }); // stable so equal-depth actors keep their given order

	size_t actorIndex = 0;
	size_t decorationIndex = 0;
	while (actorIndex < actorsSorted.size() && decorationIndex < decorations.size())
	{
		// A decoration "stands" where its base sits - the center of its tile.
		const float decorationGroundY = (decorations[decorationIndex].tileY + 0.5f) * tileSize;

		if (actorsSorted[actorIndex].groundY < decorationGroundY)
		{
			// The actor is further back - draw it first, then move on to the next actor.
			actorsSorted[actorIndex++].draw();
		}
		else
		{
			// The decoration is further back (or at exactly the same depth - in that case we still draw the
			// decoration first, so a character standing right on top of it appears in front).
			drawDecoration(decorationIndex++, window);
		}
	}

	// Draw whatever is left over from either list.
	while (actorIndex < actorsSorted.size())
	{
		actorsSorted[actorIndex++].draw();
	}
	while (decorationIndex < decorations.size())
	{
		drawDecoration(decorationIndex++, window);
	}

	// HUD layer, drawn last so nothing from the world layers above can ever cover it. The stage
	// name/enemy label used to be drawn right here too - they now come in through hudElements
	// (see Stage::drawHUD and Game::render), alongside the enemy health bar/boss timer/damage
	// numbers, so a foreground decoration can never occlude any of them anymore.
	for (const auto& hud : hudElements)
	{
		hud.draw();
	}
}

void Stage::drawHUD(sf::RenderWindow* window)
{
	window->draw(*stageText);
	window->draw(*enemyInfoText);
}

void Stage::updateTexts()
{
	this->updateStageText();
}