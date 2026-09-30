#pragma once

#include "stdafx.h"
#include "Sprites.h"
#include "Text.h"
#include <functional>
#include <random>
#include <unordered_map>

// Describes one tile variant: its texture, and which of its 4 corners (NW/NE/SW/SE) are dirt vs
// grass (true = dirt, false = grass). Tiles are matched purely by comparing the corner points
// where they touch - two tiles fit side by side only if the shared corners agree. This works
// perfectly as long as tileDefs contains all 16 possible corner combinations exactly once (see
// the note in Stage::initTileTextures) - every possible pair of neighbors then always has a
// valid answer, with no manual per-tile adjacency lists to keep consistent by hand.
struct TileDefinition
{
	std::string name; // human-readable, used only for console diagnostics
	std::string texturePath;
	bool cornerNW; // true = dirt, false = grass
	bool cornerNE;
	bool cornerSW;
	bool cornerSE;
};

class Stage
{
private:
	int stageNumber;
	int mapSizeX;
	int mapSizeY;
	float tileSize = 64.0f;

	std::vector<std::vector<sf::RectangleShape>> tileMap;
	std::vector<std::vector<int>> tileIndices;              // which tile each cell uses - derived from cornerGrid, cached for rendering
	std::vector<std::vector<bool>> cornerGrid;              // one dirt/grass value per grid VERTEX - (mapSizeY+1) x (mapSizeX+1), persisted across resizes
	std::vector<TileDefinition> tileDefs;                   // every tile variant and its 4-corner signature
	std::vector<sf::Texture*> tileTextures;                 // loaded textures, index-aligned with tileDefs
	struct Decoration
	{
		int tileX;        // grid cell this decoration is anchored to (its base sits at the cell's center)
		int tileY;
		int textureIndex;
	};
	std::vector<sf::Texture*> decorationTextures;            // textures for the second background layer (sprite_0002 to sprite_0018)
	std::vector<bool> decorationIsDirt;                      // index-aligned with decorationTextures - true if this item belongs on dirt, false if it belongs on grass
	std::vector<Decoration> decorations;                     // current active decorations on the map
	std::unordered_map<int, int> cornerPatternToTileIndex;  // 4-bit corner pattern -> tileDefs index
	std::mt19937 rng{ std::random_device{}() };
	float dirtPatchChance = 0.18f; // rough proportion of brand-new area that starts as dirt - tweak to taste
	float familiarZoneFraction = 0.30f;          // left strip reserved for familiars, as a fraction of the play area width - widen this as more familiars line up on the left
	float centerBandHalfWidthFraction = 0.15f;   // half-width of the middle band kept clear of decorations (hero/enemy fight zone), as a fraction of the play area width
	Text* stageText;
	Text* enemyInfoText; // shown just below stageText: current enemy's name + progress (or "Boss: <name>")

	void initStageNumber();
	void initStageText();
	void initEnemyInfoText();
	void initMapSize();
	void initTileTextures();
	void initDecorations();
	void initTileMap();

	void growCornerGrid();
	int cornerPattern(bool nw, bool ne, bool sw, bool se) const;
	void rebuildTileGrid();
	void updateStageText();
	void drawDecoration(size_t index, sf::RenderWindow* window); // draws one decoration from the decorations vector (used by drawMap's depth merge)

	// One drawable "actor" that stands on the ground (hero, familiar, enemy). Stage::drawMap interleaves these with
	// the decorations back-to-front by their ground-Y position, so a tree below an actor covers its feet while a tree
	// above it stays behind. Each actor is one depth unit - everything attached to it (health bars, damage text,
	// cooldown ring, projectiles) moves and draws together at that character's ground-Y.
public:
	struct DepthActor
	{
		float groundY;              // Y of the actor's feet on the ground - smaller = further back = drawn first
		std::function<void()> draw; // draws the whole actor (sprite + attached labels) at its current position
	};

	// A HUD element for the play area - always drawn last, on top of tiles, decorations, and
	// actor sprites, regardless of world position. Use for trackers (DPS/damage/gold text,
	// cooldown rings, etc.) that must never be occluded by a decoration sorting in front of them.
	struct HudElement
	{
		std::function<void()> draw;
	};

	Stage();
	virtual ~Stage();

	int getStageNumber() const;
	int getMapSizeX();
	int getMapSizeY();

	void setStageNumber(int newStageNumber);

	void onResize();
	void drawMap(sf::RenderWindow* window, const std::vector<DepthActor>& actors = {}, const std::vector<HudElement>& hudElements = {}); // draws tiles, then decorations and actors interleaved back-to-front by ground-Y

	void updateTexts();
	void setEnemyInfoText(const std::string& text); // Game calls this every frame with the current enemy's name/progress, or "" when no enemy is on the field
};