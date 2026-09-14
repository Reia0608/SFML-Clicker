#pragma once
#include "stdafx.h"
#include "Anchor.h"

class LayoutManager
{
private:
	static sf::Vector2f originForAnchor(Anchor anchor, sf::FloatRect bounds)
	{
		float left = bounds.position.x;
		float right = bounds.position.x + bounds.size.x;
		float centerX = bounds.position.x + bounds.size.x / 2.0f;
		float top = bounds.position.y;
		float bottom = bounds.position.y + bounds.size.y;
		float centerY = bounds.position.y + bounds.size.y / 2.0f;

		switch (anchor)
		{
		case Anchor::TopLeft:      return { left, top };
		case Anchor::TopCenter:    return { centerX, top };
		case Anchor::TopRight:     return { right, top };
		case Anchor::MiddleLeft:   return { left, centerY };
		case Anchor::MiddleCenter: return { centerX, centerY };
		case Anchor::MiddleRight:  return { right, centerY };
		case Anchor::BottomLeft:   return { left, bottom };
		case Anchor::BottomCenter: return { centerX, bottom };
		case Anchor::BottomRight:  return { right, bottom };
		}
		return { left, top };
	}

	static sf::Vector2f anchorPointInContainer(Anchor anchor, sf::Vector2f containerOrigin, sf::Vector2f containerSize)
	{
		float left = containerOrigin.x;
		float right = containerOrigin.x + containerSize.x;
		float centerX = containerOrigin.x + containerSize.x / 2.0f;
		float top = containerOrigin.y;
		float bottom = containerOrigin.y + containerSize.y;
		float centerY = containerOrigin.y + containerSize.y / 2.0f;

		switch (anchor)
		{
		case Anchor::TopLeft:      return { left, top };
		case Anchor::TopCenter:    return { centerX, top };
		case Anchor::TopRight:     return { right, top };
		case Anchor::MiddleLeft:   return { left, centerY };
		case Anchor::MiddleCenter: return { centerX, centerY };
		case Anchor::MiddleRight:  return { right, centerY };
		case Anchor::BottomLeft:   return { left, bottom };
		case Anchor::BottomCenter: return { centerX, bottom };
		case Anchor::BottomRight:  return { right, bottom };
		}
		return { left, top };
	}

public:
	// T has to have getLocalBounds(), setOrigin() and setPosition() — filled by sf::Text AND sf::Shape-Departments (e.g. sf::RectangleShape)
	template<typename T>
	static void applyAnchor(T& element, Anchor anchor, sf::Vector2f offset,
		sf::Vector2f containerOrigin, sf::Vector2f containerSize)
	{
		sf::FloatRect bounds = element.getLocalBounds();
		element.setOrigin(originForAnchor(anchor, bounds));

		sf::Vector2f target = anchorPointInContainer(anchor, containerOrigin, containerSize) + offset;
		element.setPosition(target);
	}
};