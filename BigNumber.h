#pragma once
#include "stdafx.h"
#include <array>
#include <sstream>
#include <iomanip>
#include <cmath>

struct BigNumber
{
	double mantissa = 0.0;
	int tier = 0;

	static constexpr std::array<const char*, 9> suffixes = { "", "K", "Mil", "Bil", "Tril", "Quad", "Quint", "Sext", "Sept"};

	BigNumber() = default;
	explicit BigNumber(double value)
	{
		mantissa = value;
		tier = 0;
		normalize();
	}

	void normalize()
	{
		while (mantissa >= 1000.0 && tier < (int)suffixes.size() - 1)
		{
			mantissa /= 1000.0;
			tier++;
		}
		while (mantissa < 1.0 && mantissa > 0.0 && tier > 0)
		{
			mantissa *= 1000.0;
			tier--;
		}
	}

	BigNumber operator+(const BigNumber& other) const
	{
		BigNumber result = *this;
		BigNumber added = other;

		// bring to the same tier before addition
		while (added.tier < result.tier)
		{
			added.mantissa /= 1000.0;
			added.tier++;
		}
		while (result.tier < added.tier)
		{
			result.mantissa /= 1000.0;
			result.tier++;
		}

		result.mantissa += added.mantissa;
		result.normalize();
		return result;
	}

	BigNumber operator-(const BigNumber& other) const
	{
		BigNumber result = *this;
		BigNumber subtracted = other;

		while (subtracted.tier < result.tier)
		{
			subtracted.mantissa /= 1000.0;
			subtracted.tier++;
		}
		while (result.tier < subtracted.tier)
		{
			result.mantissa /= 1000.0;
			result.tier++;
		}

		result.mantissa -= subtracted.mantissa;
		result.normalize();
		return result;
	}

	BigNumber operator*(const BigNumber& other) const
	{
		BigNumber result;
		result.mantissa = mantissa * other.mantissa;
		result.tier = tier + other.tier;
		result.normalize();
		return result;
	}

	double toApproxDouble() const
	{
		return mantissa * std::pow(1000.0, tier);
	}

	BigNumber operator/(double divisor) const
	{
		BigNumber result = *this;
		result.mantissa /= divisor;
		result.normalize();
		return result;
	}

	bool operator<=(const BigNumber& other) const
	{
		return toApproxDouble() <= other.toApproxDouble();
	}

	bool operator>=(const BigNumber& other) const
	{
		return toApproxDouble() >= other.toApproxDouble();
	}

	bool operator>(const BigNumber& other) const
	{
		return toApproxDouble() > other.toApproxDouble();
	}

	bool operator<(const BigNumber& other) const
	{
		return toApproxDouble() < other.toApproxDouble();
	}

	std::string toString() const
	{
		double sign = (mantissa < 0.0) ? -1.0 : 1.0;
		double absMantissa = std::abs(mantissa);
		int displayTier = tier;

		int decimals = (displayTier == 0) ? 0 : (absMantissa < 10.0) ? 2 : (absMantissa < 100.0) ? 1 : 0;

		double roundingFactor = std::pow(10.0, decimals);
		double rounded = std::round(absMantissa * roundingFactor) / roundingFactor;

		if (rounded >= 1000.0 && displayTier < (int)suffixes.size() - 1)
		{
			absMantissa = rounded / 1000.0;
			displayTier++;
			decimals = (absMantissa < 10.0) ? 2 : (absMantissa < 100.0) ? 1 : 0;
		}
		else
		{
			absMantissa = rounded;
		}

		std::ostringstream out;
		out << std::fixed << std::setprecision(decimals) << (sign * absMantissa) << suffixes[displayTier];
		return out.str();
	}

	BigNumber scaleBy(double factor, int times) const
	{
		// needs to be optimized later, maybe with mantissa^factor
		BigNumber result = *this;
		BigNumber factorBN(factor);
		for (int i = 0; i < times; i++)
		{
			result = result * factorBN;
		}
		return result;
	}
};