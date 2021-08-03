/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-06-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <type_traits>

template <typename T, bool = std::is_enum<T>::value>
struct is_flag;

template <typename T>
struct is_flag<T, false> : std::false_type { };

template <typename T, typename std::enable_if<is_flag<T>::value>::type* = nullptr>
constexpr T operator |(T lhs, T rhs)
{
    using u_t = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<u_t>(lhs) | static_cast<u_t>(rhs));
}

template <typename T, typename std::enable_if<is_flag<T>::value>::type* = nullptr>
constexpr T operator &(T lhs, T rhs)
{
    using u_t = typename std::underlying_type<T>::type;
    return static_cast<T>(static_cast<u_t>(lhs) & static_cast<u_t>(rhs));
}

template <typename T, typename std::enable_if<is_flag<T>::value>::type* = nullptr>
constexpr bool operator !(T val)
{
    using u_t = typename std::underlying_type<T>::type;
    return static_cast<u_t>(val) == u_t(0);
}
