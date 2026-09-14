#include "stdafx.h"
#include "Randomize.h"

int Randomize::returnRandomizeInt(int min, int max)
{
	static std::mt19937 engine(std::random_device{}());
	std::uniform_int_distribution<int> dist(min, max);
	return dist(engine);
}

float Randomize::returnRandomizeFloat(float min, float max)
{
	static std::mt19937 engine(std::random_device{}());
	std::uniform_real_distribution<float> dist(min, max);
	return dist(engine);
}