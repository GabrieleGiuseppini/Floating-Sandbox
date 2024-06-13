/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2024-02-09
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <array>
#include <cassert>
#include <optional>
#include <ostream>
#include <string>

#pragma pack(push, 1)

struct bcoords3f
{
public:

    std::array<float, 3> coords;

    static constexpr bcoords3f zero()
    {
        return bcoords3f(0.0f, 0.0f, 0.0f);
    }

    inline constexpr bcoords3f()
        : coords({ 0.0f, 0.0f, 0.0f })
    {
    }

    inline constexpr bcoords3f(float a, float b, float c)
        : coords({a, b, c})
    {
    }

    float const & operator[](int index) const
    {
        assert(index >= 0 && index < 3);
        return coords[index];
    }

    float & operator[](int index)
    {
        assert(index >= 0 && index < 3);
        return coords[index];
    }

	inline bool operator==(bcoords3f const & other) const
	{
		return coords == other.coords;
	}

    inline bool operator!=(bcoords3f const & other) const
    {
        return !(*this == other);
    }

    inline bcoords3f operator+(bcoords3f const & other) const
    {
        return bcoords3f(
            coords[0] + other.coords[0],
            coords[1] + other.coords[1],
            coords[2] + other.coords[2]);
    }

    inline bcoords3f operator-(bcoords3f const & other) const
    {
        return bcoords3f(
            coords[0] - other.coords[0],
            coords[1] - other.coords[1],
            coords[2] - other.coords[2]);
    }

    inline bcoords3f operator*(float value) const
    {
        return bcoords3f(
            coords[0] * value,
            coords[1] * value,
            coords[2] * value);
    }

    bool is_on_edge() const
    {
        return coords[0] == 0.0f
            || coords[1] == 0.0f
            || coords[2] == 0.0f;
    }

    // Returns the index of the vertex we're on, if we're on a vertex
    std::optional<int> try_get_vertex() const
    {
        if (coords[0] == 0.0f)
        {
            if (coords[1] == 0.0f)
            {
                if (coords[2] == 1.0f)
                {
                    return 2;
                }
            }
            else
            {
                if (coords[1] == 1.0f && coords[2] == 0.0f)
                {
                    return 1;
                }
            }
        }
        else
        {
            if (coords[0] == 1.0f)
            {
                if (coords[1] == 0.0f && coords[2] == 0.0f)
                {
                    return 0;
                }
            }
        }

        return std::nullopt;
    }

    bool is_on_edge_or_internal() const
    {
        return
            coords[0] >= 0.0f && coords[0] <= 1.0f
            && coords[1] >= 0.0f && coords[1] <= 1.0f
            && coords[2] >= 0.0f && coords[2] <= 1.0f;
    }

    std::string toString() const;
};

#pragma pack(pop)

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, bcoords3f const & v)
{
    os << v.toString();
    return os;
}
