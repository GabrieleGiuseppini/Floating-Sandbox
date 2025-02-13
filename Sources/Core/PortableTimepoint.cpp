/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-07-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PortableTimepoint.h"

#include <algorithm>
#include <chrono>
#include <ctime>

PortableTimepoint PortableTimepoint::Now()
{
	return PortableTimepoint(std::chrono::system_clock::now());
}

PortableTimepoint::value_type PortableTimepoint::ToTicks(std::chrono::system_clock::time_point const & systemClockTimepoint)
{
	// Convert system clock to time_t
	std::time_t const tt = std::chrono::system_clock::to_time_t(systemClockTimepoint);

	// Convert time_t to tm
	std::tm const * cal = std::gmtime(&tt);

	// Calculate seconds
	value_type ticks = std::max(cal->tm_year - 100, 0);
	ticks = ticks * 12 + cal->tm_mon;
	ticks = ticks * 31 + (cal->tm_mday - 1); // Yeah, 31! we don't care
	ticks = ticks * 24 + cal->tm_hour;
	ticks = ticks * 60 + cal->tm_min;
	ticks = ticks * 60 + cal->tm_sec;

	return ticks;
}
