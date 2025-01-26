/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-07-20
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <chrono>
#include <cstdint>

/*
 * A portable arithmetic representation of timestamps,
 * at an arbitrary granularity and with an arbitrary epoch.
 */
class PortableTimepoint final
{
public:

	using value_type = std::uint64_t;

	PortableTimepoint(value_type ticks)
		: mTicks(ticks)
	{
	}

	static PortableTimepoint Now();

	template<typename TTime>
	static PortableTimepoint FromTime(TTime const & time)
	{
		// Convert to system clock (warning: approx)
		auto const systemClockTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			(time - TTime::clock::now())
			+ std::chrono::system_clock::now());

		return PortableTimepoint(systemClockTime);
	}

	value_type Value() const
	{
		return mTicks;
	}

	bool operator==(PortableTimepoint const & other) const
	{
		return mTicks == other.mTicks;
	}

	bool operator!=(PortableTimepoint const & other) const
	{
		return !(mTicks == other.mTicks);
	}

	bool operator<(PortableTimepoint const & other) const
	{
		return mTicks < other.mTicks;
	}

	bool operator<=(PortableTimepoint const & other) const
	{
		return (*this < other) || (*this == other);
	}

	bool operator>(PortableTimepoint const & other) const
	{
		return mTicks > other.mTicks;
	}

	bool operator>=(PortableTimepoint const & other) const
	{
		return (*this > other) || (*this == other);
	}

private:

	PortableTimepoint(std::chrono::system_clock::time_point const & systemClockTimepoint)
		: PortableTimepoint(ToTicks(systemClockTimepoint))
	{}

	static value_type ToTicks(std::chrono::system_clock::time_point const & systemClockTimepoint);

	value_type mTicks;
};
