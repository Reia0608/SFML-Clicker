#pragma once
#include "stdafx.h"

class Times
{
private:
	sf::Clock clock;
	float deltaTime;

public:
	Times();
	~Times();

	void nextDeltaTime();
	float getDeltaTime() const;
	int getDtEpoch();
}; 
