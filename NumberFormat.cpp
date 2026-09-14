#include "stdafx.h"
#include "NumberFormat.h"
#include <sstream>
#include <iomanip>
#include <array>

std::string NumberFormat::format(long long value)
{
	static const std::array<std::string, 8> suffixes = { "", "K", "Mil", "Bil", "Tril", "Quad", "Quint", "Sext" };

	if (value < 1000)
	{
		return std::to_string(value);
	}

	double reduced = static_cast<double>(value);
	size_t tier = 0;
	while (reduced >= 1000.0 && tier < suffixes.size() - 1)
	{
		reduced /= 1000.0;
		tier++;
	}

	int decimals = (reduced < 10.0) ? 2 : (reduced < 100.0) ? 1 : 0;

	std::ostringstream out;
	out << std::fixed << std::setprecision(decimals) << reduced << suffixes[tier];
	return out.str();
}