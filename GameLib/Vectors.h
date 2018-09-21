/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cmath>
#include <cstddef>
#include <string>

struct vec2f
{
public:
	
    float x, y;

    static constexpr const vec2f zero() 
    { 
        return vec2f(); 
    }

	inline vec2f operator+(vec2f const & rhs) const
	{
		return vec2f(
			x + rhs.x, 
			y + rhs.y);
	}

	inline vec2f operator-(vec2f const & rhs) const
	{
		return vec2f(
			x - rhs.x, 
			y - rhs.y);
	}

    inline vec2f operator-() const
    {
        return vec2f(
            -x,
            -y);
    }

	inline vec2f operator*(float rhs) const
	{
		return vec2f(
			x * rhs,
			y * rhs);
	}

	inline vec2f operator/(float rhs) const
	{
		return vec2f(
			x / rhs,
			y / rhs);
	}

	inline vec2f & operator+=(vec2f const & rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	inline vec2f & operator-=(vec2f const & rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	inline vec2f & operator*=(float rhs)
	{
		x *= rhs;
		y *= rhs;
		return *this;
	}

	inline vec2f& operator/=(float rhs)
	{
		x /= rhs;
		y /= rhs;
		return *this;
	}

	inline bool operator==(vec2f const & rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}

    inline bool operator!=(vec2f const & rhs) const
    {
        return !(*this == rhs);
    }

	// (lexicographic comparison only)
	inline bool operator<(vec2f const & rhs) const
	{
		return x < rhs.x || (x == rhs.x && y < rhs.y);
	}

	inline float dot(vec2f const & rhs) const
	{
		return x * rhs.x + y * rhs.y;
	}

    inline float cross(vec2f const & rhs) const
    {
        return x * rhs.y - y * rhs.x;
    }

	inline float length() const
	{
        return sqrtf(x * x + y * y);
	}

    inline float squareLength() const
    {
        return x * x + y * y;
    }

	inline vec2f normalise() const
	{
        float const squareLength = x * x + y * y;
        if (squareLength > 0)
        {
            return (*this) / sqrtf(squareLength);
        }
        else
        {
            return vec2f(0.0f, 0.0f);
        }
	}

    inline vec2f normalise(float length) const
    {
        if (length > 0)
        {
            return (*this) / length;
        }
        else
        {
            return vec2f(0.0f, 0.0f);
        }
    }

    inline float angle(vec2f const & rhs) const
    {
        return -atan2f(
            x * rhs.y - y * rhs.x,
            x * rhs.x + y * rhs.y);
    }

    std::string toString() const;

    inline constexpr vec2f()
        : x(0.0f)
        , y(0.0f)
    {
    }

	inline constexpr vec2f(
		float _x,
		float _y)
		: x(_x)
		, y(_y)
	{
	}
};

static_assert(offsetof(vec2f, x) == 0);
static_assert(offsetof(vec2f, y) == sizeof(float));
static_assert(sizeof(vec2f) == 2 * sizeof(float));

using vec2 = vec2f;

struct vec3f
{
public:
	
    float x, y, z;

    static constexpr const vec3f zero() { return vec3f(); }

	inline vec3f operator+(vec3f const & rhs) const
	{
		return vec3f(
			x + rhs.x,
			y + rhs.y,
			z + rhs.z);
	}

    inline vec3f operator-(vec3f const & rhs) const
	{
		return vec3f(
			x - rhs.x,
			y - rhs.y,
			z - rhs.z);
	}

    inline vec3f operator-() const
    {
        return vec3f(
            -x,
            -y,
            -z);
    }

	inline vec3f operator*(float rhs) const
	{
		return vec3f(
			x * rhs,
			y * rhs,
			z * rhs);
	}

	inline vec3f operator/(float rhs) const
	{
		return vec3f(
			x / rhs,
			y / rhs,
			z / rhs);
	}

	inline vec3f & operator+=(vec3f const & rhs)
	{
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}

	inline vec3f & operator-=(vec3f const & rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		z -= rhs.z;
		return *this;
	}

	inline vec3f & operator*=(float rhs)
	{
		x *= rhs;
		y *= rhs;
		z *= rhs;
		return *this;
	}

	inline vec3f & operator/=(float rhs)
	{
		x /= rhs;
		y /= rhs;
		z /= rhs;
		return *this;
	}

	inline bool operator==(vec3f const & rhs) const
	{
		return x == rhs.x && y == rhs.y && z == rhs.z;
	}

    inline bool operator!=(vec3f const & rhs) const
    {
        return !(*this == rhs);
    }

	// (lexicographic comparison only)
	inline bool operator<(vec3f const & rhs) const
	{
		return x < rhs.x || (x == rhs.x && (y < rhs.y || (y == rhs.y && z < rhs.z)));
	}
    
	float dot(vec3f const & rhs) const
	{
		return x * rhs.x + y * rhs.y + z * rhs.z;
	}
    
	float length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

    float squareLength() const
    {
        return x * x + y * y + z * z;
    }

	vec3f normalise() const
	{
        float const squareLength = x * x + y * y + z * z;
        if (squareLength > 0)
        {
            return (*this) / sqrtf(squareLength);
        }
        else
        {
            return vec3f(0.0f, 0.0f, 0.0f);
        }
	}

    std::string toString() const;

    inline constexpr vec3f()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
    {
    }

	inline constexpr vec3f(
		float _x,
		float _y,
		float _z)
		: x(_x)
		, y(_y)
		, z(_z)
	{
	}
};

static_assert(offsetof(vec3f, x) == 0);
static_assert(offsetof(vec3f, y) == sizeof(float));
static_assert(offsetof(vec3f, z) == 2 * sizeof(float));
static_assert(sizeof(vec3f) == 3 * sizeof(float));

using vec3 = vec3f;
