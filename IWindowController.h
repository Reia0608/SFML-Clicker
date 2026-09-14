#pragma once
#include "stdafx.h"
#include "WindowMode.h"

class IWindowController
{
public:
	virtual ~IWindowController() {}

	virtual void setWindowMode(WindowMode mode) = 0;
	virtual void setResolutionIndex(int index) = 0;
	virtual WindowMode getWindowMode() const = 0;
	virtual int getResolutionIndex() const = 0;
	virtual const std::vector<sf::Vector2u>& getAvailableResolutions() const = 0;
};