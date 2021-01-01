/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-12-28
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <istream>
#include <ostream>

template <typename TValue, typename TTag>
struct StrongTypeDef
{
	using ValueType = TValue;

	explicit constexpr StrongTypeDef(TValue const & value)
		: Value(value)
	{
	}

	explicit constexpr StrongTypeDef(TValue && value)
		: Value(std::move(value))
	{
	}

	constexpr StrongTypeDef()
		: Value()
	{}

	StrongTypeDef(StrongTypeDef const & other) = default;
	StrongTypeDef(StrongTypeDef && other ) = default;
	StrongTypeDef & operator=(StrongTypeDef const & rhs) = default;
	StrongTypeDef & operator=(StrongTypeDef && rhs) = default;

	StrongTypeDef & operator=(TValue const & rhs)
	{
		Value = rhs;
		return *this;
	}

	StrongTypeDef & operator=(TValue && rhs)
	{
		Value = std::move(rhs);
		return *this;
	}

	explicit operator const TValue & () const
	{
		return Value;
	}

	explicit operator TValue & ()
	{
		return Value;
	}

	bool operator==(StrongTypeDef const & rhs) const { return Value == rhs.Value; }
	bool operator!=(StrongTypeDef const & rhs) const { return Value != rhs.Value; }
	bool operator<(StrongTypeDef const & rhs) const { return Value < rhs.Value; }
	bool operator>(StrongTypeDef const & rhs) const { return Value > rhs.Value; }
	bool operator<=(StrongTypeDef const & rhs) const { return Value <= rhs.Value; }
	bool operator>=(StrongTypeDef const & rhs) const { return Value >= rhs.Value; }

	bool operator==(TValue const & rhs) const { return Value == rhs; }
	bool operator!=(TValue const & rhs) const { return Value != rhs; }
	bool operator<(TValue const & rhs) const { return Value < rhs; }
	bool operator>(TValue const & rhs) const { return Value > rhs; }
	bool operator<=(TValue const & rhs) const { return Value <= rhs; }
	bool operator>=(TValue const & rhs) const { return Value >= rhs; }

	TValue Value;
};

template<typename TValue, typename TTag>
bool operator==(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs == rhs.Value; }

template<typename TValue, typename TTag>
bool operator!=(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs != rhs.Value; }

template<typename TValue, typename TTag>
bool operator<(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs < rhs.Value; }

template<typename TValue, typename TTag>
bool operator>(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs > rhs.Value; }

template<typename TValue, typename TTag>
bool operator<=(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs <= rhs.Value; }

template<typename TValue, typename TTag>
bool operator>=(TValue const & lhs, const StrongTypeDef<TValue, TTag>& rhs) { return lhs >= rhs.Value; }

template <typename TValue, typename TTag>
inline std::ostream & operator<<(std::ostream & os, StrongTypeDef<TValue, TTag> const & v)
{
	return os << static_cast<TValue const &>(v);
}

template<typename TValue, typename TTag>
inline std::istream & operator>>(std::istream & stream, StrongTypeDef<TValue, TTag> & d)
{
	stream >> d.Value;
	return stream;
}

/*
 * Specializations for "named booleans". Example usage:
 *
 * void MyFunction(Common::Bool<struct IsBlocking> isBlocking, Common::Bool<struct UseMagic> useMagic);
 *
 * MyFunction(Common::True<IsBlocking>, Common::False<UseMagic>);
 *
 */

template<typename TTag>
using StrongTypedBool = StrongTypeDef<bool, TTag>;

template<typename TTag>
constexpr StrongTypedBool<TTag> StrongTypedTrue(true);

template<typename TTag>
constexpr StrongTypedBool<TTag> StrongTypedFalse(false);