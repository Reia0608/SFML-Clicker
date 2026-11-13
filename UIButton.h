#pragma once
#include "stdafx.h"
#include "Text.h"
#include <functional>

class UIButton
{
private:
	sf::RectangleShape background;
	Text* label; // nullptr for icon buttons (they show a picture instead of words)

	sf::Texture* iconTexture; // nullptr for ordinary text buttons
	sf::Sprite* iconSprite;   // only exists alongside iconTexture

	std::function<void()> onClick;

	bool enabled = true;
	bool firstTimePurchase = false;

	bool pressed = false;
	float pressAnimation = 0.0f; // 0 = resting, 1 = fully pressed in; animated between the two

	bool notifying = false; // when true, button pulses to flag unseen content
	sf::Clock notifyClock;  // independent time base for the pulse, unaffected by press animation

	sf::Clock animationClock; // measures dt between render() calls, no external update() needed

	// The box setLayout() was last asked for, and the text setLabelText() was last asked to show -
	// kept separately from "background" because the button is allowed to grow taller than
	// layoutSize when the label wraps to multiple lines (see relayoutLabel()). Re-deriving the
	// actual drawn size from these two every time (instead of growing "background" in place)
	// means a button that goes back to a shorter label later shrinks back down correctly too.
	sf::Vector2f layoutPosition{};
	sf::Vector2f layoutSize{};
	std::string labelText; // unused on an icon button - it has no text label

	// The text/width relayoutLabel() last actually re-wrapped for - lets it skip wrapText()'s
	// expensive per-word measuring when setLayout() is just repositioning this button (e.g.
	// tracking a scroll offset) with the same text at the same width as before.
	std::string lastWrappedText;
	float lastWrapMaxWidth = -1.0f;

	static constexpr float pressAnimationDuration = 0.08f; // seconds to fully press/release

	static constexpr float pressInset = 3.0f; // pixels the button shrinks/shifts when fully pressed
	static constexpr float iconPadding = 4.0f; // empty space kept between the icon and the button's edges
	static constexpr float labelPadding = 4.0f; // empty space kept between wrapped label text and the button's edges

	// Distinguishes the two private-constructor call shapes below ("picture" vs. "words") since
	// both would otherwise take the same (string, callback) parameters and be ambiguous to overload.
	enum class Kind { Text, Icon };

	// sourceRect: only meaningful for Kind::Icon. Default (zero-sized) means "show the whole
	// loaded image", same as before this parameter existed. A non-empty rect crops the sprite to
	// just that sub-region instead - used for a sprite SHEET where only one frame (e.g. a
	// familiar's idle animation's first frame) should ever be shown as this button's icon, never
	// the whole strip. Uses the image's own pixel coordinates, same convention as Sprites/Textures.
	UIButton(std::string content, std::function<void()> onClickCallback, Kind kind, sf::IntRect sourceRect = {});

	void updatePressAnimation();
	static sf::Color blendColor(sf::Color from, sf::Color to, float t); // New
	void layoutIcon(); // recomputes the icon's scale/position to fit the button's current size

	// Breaks "text" into lines (separated by '\n') so that none of them is wider than maxWidth,
	// measured with the label's actual font/size. Breaks only between words - a single word
	// wider than maxWidth is left on its own line rather than being cut into pieces.
	std::string wrapText(const std::string& text, float maxWidth);

	// Recomputes everything that depends on layoutPosition/layoutSize/labelText: wraps the label
	// text to fit the requested width, grows the button's actual height to fit the wrapped text
	// (but never shrinks it below layoutSize.y), and re-centers the label. For icon buttons there's
	// no text to wrap, so this just applies layoutSize directly and re-fits the icon.
	void relayoutLabel();

public:
	UIButton(std::string labelText, std::function<void()> onClickCallback);

	// Builds a button that shows an image (from iconImagePath) instead of text - used for the tab
	// buttons. Everything else about it (press animation, the notification pulse) works the same.
	static UIButton* createIconButton(std::string iconImagePath, std::function<void()> onClickCallback);

	// Same as above, but crops the icon down to just `sourceRect` of the loaded image instead of
	// showing the whole file - see the private constructor's sourceRect comment above. Used for
	// the familiar slot/picker tiles (FamiliarTab), which all share one sprite sheet per familiar
	// stage and only ever want its idle animation's very first frame.
	static UIButton* createIconButton(std::string iconImagePath, sf::IntRect sourceRect, std::function<void()> onClickCallback);

	~UIButton();

	void setLayout(sf::Vector2f position, sf::Vector2f size);
	void setLabelText(std::string text); // no-op on an icon button - it has no text label to set
	bool handleClick(sf::Vector2f pos);
	void render(sf::RenderWindow* window);

	// button visibility and interactivity
	void setEnabled(bool isEnabled);
	bool isEnabled() const;

	void setNotifying(bool isNotifying); // starts/stops the pulse; resets phase when turning on
};
