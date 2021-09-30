/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2021-09-30
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <algorithm>
#include <cstdint>

template<typename T>
class BigEndian
{
public:

    static T Read(unsigned char const * ptr) noexcept;

    static void Write(T const & value, unsigned char * ptr) noexcept;

private:

    static bool ShouldSwap()
    {
        static const uint16_t swapTest = 1;
        return (*((char *)&swapTest) == 1);
    }
};

template<>
uint16_t BigEndian<std::uint16_t>::Read(unsigned char const * ptr) noexcept
{
    if (ShouldSwap())
    {
        return (static_cast<std::uint16_t>(ptr[0]) << 8) | static_cast<std::uint16_t>(ptr[1]);
    }
    else
    {
        return *(reinterpret_cast<std::uint16_t const *>(ptr));
    }
}

template<>
void BigEndian<std::uint16_t>::Write(std::uint16_t const & value, unsigned char * ptr) noexcept
{
    if (ShouldSwap())
    {
        unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
        ptr[0] = vPtr[1];
        ptr[1] = vPtr[0];
    }
    else
    {
        *reinterpret_cast<std::uint16_t *>(ptr) = value;
    }
}

template<>
uint32_t BigEndian<std::uint32_t>::Read(unsigned char const * ptr) noexcept
{
    if (ShouldSwap())
    {
        return (static_cast<std::uint32_t>(ptr[0]) << 24) | (static_cast<std::uint32_t>(ptr[1]) << 16)
            | (static_cast<std::uint32_t>(ptr[2]) << 8) | static_cast<std::uint32_t>(ptr[3]);
    }
    else
    {
        return *(reinterpret_cast<std::uint32_t const *>(ptr));
    }
}

template<>
void BigEndian<std::uint32_t>::Write(std::uint32_t const & value, unsigned char * ptr) noexcept
{
    if (ShouldSwap())
    {
        unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
        ptr[0] = vPtr[3];
        ptr[1] = vPtr[2];
        ptr[2] = vPtr[1];
        ptr[3] = vPtr[0];
    }
    else
    {
        *reinterpret_cast<std::uint32_t *>(ptr) = value;
    }
}

template<>
float BigEndian<float>::Read(unsigned char const * ptr) noexcept
{
    if (ShouldSwap())
    {
        unsigned char buffer[4];
        buffer[0] = ptr[3];
        buffer[1] = ptr[2];
        buffer[2] = ptr[1];
        buffer[3] = ptr[0];
        return *(reinterpret_cast<float const *>(buffer));
    }
    else
    {
        return *(reinterpret_cast<float const *>(ptr));
    }
}

template<>
void BigEndian<float>::Write(float const & value, unsigned char * ptr) noexcept
{
    if (ShouldSwap())
    {
        unsigned char const * vPtr = reinterpret_cast<unsigned char const *>(&value);
        ptr[0] = vPtr[3];
        ptr[1] = vPtr[2];
        ptr[2] = vPtr[1];
        ptr[3] = vPtr[0];
    }
    else
    {
        *reinterpret_cast<float *>(ptr) = value;
    }
}