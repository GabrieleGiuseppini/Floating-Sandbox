/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-07-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PortableTimepoint.h"

#include <chrono>
#include <ctime>

PortableTimepoint PortableTimepoint::Now()
{
	return PortableTimepoint(std::chrono::system_clock::now());
}

PortableTimepoint PortableTimepoint::FromLastWriteTime(std::filesystem::path const & filePath)
{
	auto const fileLastWriteTime = std::filesystem::last_write_time(filePath);

	// Convert to system clock (warning: approx)
	auto const systemClockFileLastWriteTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileLastWriteTime - decltype(fileLastWriteTime)::clock::now()
		+ std::chrono::system_clock::now());

	return PortableTimepoint(systemClockFileLastWriteTime);
}

PortableTimepoint::value_type PortableTimepoint::ToTicks(std::chrono::system_clock::time_point const & systemClockTimepoint)
{
	// Convert system clock to time_t
	std::time_t const tt = std::chrono::system_clock::to_time_t(systemClockTimepoint);

	// Convert time_t to tm 
	std::tm const * cal = std::gmtime(&tt);

	// Calculate seconds
	value_type ticks = std::max(cal->tm_year - 2000, 0);
	ticks = ticks * 12 + cal->tm_mon;
	ticks = ticks * 31 + (cal->tm_mday - 1); // Yeah! we don't care
	ticks = ticks * 24 + cal->tm_hour;
	ticks = ticks * 60 + cal->tm_min;
	ticks = ticks * 60 + cal->tm_sec;

	return ticks;
}
