#include "stdafx.h"
#include "UIButton.h"
#include "LayoutManager.h"
#include <algorithm>
#include <sstream>

void UIButton::updatePressAnimation()
{
	if (pressed && !sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) pressed = false; // detects LMB release without needing a dedicated event
	float dt = animationClock.restart().asSeconds();
	float step = dt / pressAnimationDuration;
	if (pressed) pressAnimation = std::min(1.0f, pressAnimation + step);
	else pressAnimation = std::max(0.0f, pressAnimation - step);
}

sf::Color UIButton::blendColor(sf::Color from, sf::Color to, float t)
{
	auto lerp = [t](std::uint8_t a, std::uint8_t b) { return static_cast<std::uint8_t>(a + (b - a) * t); };
	return sf::Color(lerp(from.r, to.r), lerp(from.g, to.g), lerp(from.b, to.b), from.a);
}

void UIButton::layoutIcon()
{
	sf::Vector2f buttonSize = background.getSize();

	// The currently VISIBLE region's own size - for a plain icon button this already equals the
	// whole image (a freshly constructed sf::Sprite's texture rect defaults to the whole texture),
	// so this is unchanged for every icon button that existed before the sourceRect overload. For
	// a cropped sprite-sheet icon (see createIconButton's sourceRect overload) this is just that
	// one frame's size, which is what actually needs to fit/scale into the button, not the sheet.
	sf::Vector2i frameSize = iconSprite->getTextureRect().size;

	float availableWidth = buttonSize.x - iconPadding * 2.0f;
	float availableHeight = buttonSize.y - iconPadding * 2.0f;
	float scale = std::max(0.0f, std::min(availableWidth / frameSize.x, availableHeight / frameSize.y));

	iconSprite->setScale({ scale, scale });
	LayoutManager::applyAnchor(*iconSprite, Anchor::MiddleCenter, { 0.0f, 0.0f }, background.getPosition(), buttonSize);
}

std::string UIButton::wrapText(const std::string& text, float maxWidth)
{
	std::istringstream words(text);
	std::string word;
	std::vector<std::string> lines;
	std::string currentLine;

	while (words >> word)
	{
		std::string candidate = currentLine.empty() ? word : currentLine + " " + word;
		label->setString(candidate); // measuring tool: borrow the real label to get the real pixel width for this font/size
		float width = label->getLocalBounds().size.x;

		if (!currentLine.empty() && width > maxWidth)
		{
			lines.push_back(currentLine); // "word" doesn't fit next to what's already on this line...
			currentLine = word;           // ...so close the line out and start the next one with it
		}
		else
		{
			currentLine = candidate; // fits (or it's a lone word too wide to split further either way)
		}
	}
	if (!currentLine.empty()) lines.push_back(currentLine);

	std::string wrapped;
	for (size_t i = 0; i < lines.size(); i++)
	{
		if (i > 0) wrapped += "\n";
		wrapped += lines[i];
	}
	return wrapped;
}

void UIButton::relayoutLabel()
{
	if (!label) // icon button - nothing to wrap, no height to grow
	{
		background.setPosition(layoutPosition);
		background.setSize(layoutSize);
		layoutIcon();
		return;
	}

	float maxTextWidth = layoutSize.x - labelPadding * 2.0f;

	// setLayout() runs every frame for any button repositioned while its tab is scrolled (see
	// TabbedPanel::render()), not just on an actual resize or a real label change - wrapText()
	// measures word-by-word via setString()/getLocalBounds(), which isn't free, so skip it unless
	// the text or the width it has to fit actually changed since the last time it ran (this was a
	// second, separate source of the same framerate issue AchievementsTab's own wrap had - see its
	// relayoutLines() comment, 2026-11-04).
	if (labelText != lastWrappedText || maxTextWidth != lastWrapMaxWidth)
	{
		label->setString(wrapText(labelText, maxTextWidth));
		lastWrappedText = labelText;
		lastWrapMaxWidth = maxTextWidth;
	}

	float wrappedHeight = label->getLocalBounds().size.y + labelPadding * 2.0f;
	sf::Vector2f finalSize = { layoutSize.x, std::max(layoutSize.y, wrappedHeight) }; // never smaller than what was asked for, only taller if the text needs it

	background.setPosition(layoutPosition);
	background.setSize(finalSize);

	LayoutManager::applyAnchor(*label, Anchor::MiddleCenter, { 0.0f, 0.0f }, layoutPosition, finalSize);
}

UIButton::UIButton(std::string content, std::function<void()> onClickCallback, Kind kind, sf::IntRect sourceRect)
	: onClick(onClickCallback)
{
	background.setFillColor(sf::Color(70, 100, 140));
	background.setOutlineColor(sf::Color::Black);
	background.setOutlineThickness(1.0f);

	if (kind == Kind::Text)
	{
		labelText = content;

		label = new Text(content, "Fonts/arial.ttf");
		label->setFillColor(sf::Color::White);
		label->setCharacterSize(14);

		iconTexture = nullptr;
		iconSprite = nullptr;
	}
	else // Kind::Icon
	{
		label = nullptr;

		iconTexture = new sf::Texture();
		if (!iconTexture->loadFromFile(content))
		{
			std::cerr << "ERROR::UIBUTTON::COULD NOT LOAD ICON: " << content << std::endl;
		}
		iconTexture->setSmooth(false); // keeps the pixel-art icon crisp instead of blurring it when scaled down

		iconSprite = new sf::Sprite(*iconTexture);

		// A zero-sized sourceRect (the default) means "show the whole image" - iconSprite already
		// defaults to that on construction, so there's nothing to do in that case. A real rect
		// crops it down to one frame of a sprite sheet - see this constructor's own comment in the header.
		if (sourceRect.size.x > 0 && sourceRect.size.y > 0) iconSprite->setTextureRect(sourceRect);
	}
}

UIButton::UIButton(std::string labelText, std::function<void()> onClickCallback)
	: UIButton(labelText, onClickCallback, Kind::Text)
{
}

UIButton* UIButton::createIconButton(std::string iconImagePath, std::function<void()> onClickCallback)
{
	return new UIButton(iconImagePath, onClickCallback, Kind::Icon);
}

UIButton* UIButton::createIconButton(std::string iconImagePath, sf::IntRect sourceRect, std::function<void()> onClickCallback)
{
	return new UIButton(iconImagePath, onClickCallback, Kind::Icon, sourceRect);
}

UIButton::~UIButton()
{
	delete label;
	delete iconSprite;
	delete iconTexture;
}

void UIButton::setLayout(sf::Vector2f position, sf::Vector2f size)
{
	layoutPosition = position;
	layoutSize = size;
	relayoutLabel();
}

void UIButton::setLabelText(std::string text)
{
	if (!label) return; // icon buttons have no text label to set

	labelText = text;
	relayoutLabel();
}

bool UIButton::handleClick(sf::Vector2f pos)
{
	// If the button is disabled, ignore clicks
	if (!enabled) return false;

	if (background.getGlobalBounds().contains(pos))
	{
		pressed = true; // starts the "pressed in" animation, released automatically once LMB is no longer held
		if (onClick)
		{
			onClick();
		}
		return true;
	}
	return false;
}

void UIButton::render(sf::RenderWindow* window)
{
	// if the button is disabled, it is not rendered (invisible)
	if (!enabled) return;

	updatePressAnimation();

	sf::Color baseColor = background.getFillColor();
	if (notifying)
	{
		float t = notifyClock.getElapsedTime().asSeconds();
		float phase = (std::sin(t * 5.0f) + 1.0f) / 2.0f; // 0..1 pulse
		baseColor = blendColor(baseColor, sf::Color(255, 215, 60), phase * 0.6f); // pulse toward gold
	}

	float inset = pressInset * pressAnimation;
	sf::RectangleShape visual = background; // local copy, so "background" (used for hit-testing) stays untouched
	visual.setPosition(background.getPosition() + sf::Vector2f(inset, inset));
	visual.setSize(background.getSize() - sf::Vector2f(inset * 2.0f, inset * 2.0f));
	visual.setFillColor(blendColor(baseColor, sf::Color(45, 65, 95), pressAnimation)); // darkens while pressed, on top of the pulse

	window->draw(visual);

	// Only the background pulses/darkens above - the icon or label is drawn on top last, unaffected,
	// exactly as requested (icon itself doesn't flash, just the background behind it).
	if (iconSprite) window->draw(*iconSprite);
	else window->draw(*label);
}

void UIButton::setEnabled(bool isEnabled)
{
	enabled = isEnabled;
}

bool UIButton::isEnabled() const
{
	return enabled;
}

void UIButton::setNotifying(bool isNotifying)
{
	if (isNotifying && !notifying) notifyClock.restart(); // start each pulse cycle fresh
	notifying = isNotifying;
}
