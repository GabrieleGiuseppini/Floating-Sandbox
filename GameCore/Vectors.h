/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "SysSpecifics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ostream>
#include <string>

struct vec2i;

#pragma pack(push, 1)

struct vec2f
{
public:

    float x;
    float y;

    static constexpr vec2f zero()
    {
        return vec2f();
    }

    inline static vec2f fromPolar(
        float magnitude,
        float angle) // Angle is CW, starting FROM E (1.0, 0.0); angle 0.0 <=> (1.0, 0.0); angle +PI/2 <=> (0.0, -1.0)
    {
        return vec2f(
            magnitude * std::cos(angle),
            -magnitude * std::sin(angle)); // Angle is CW and our positive points up
    }

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

    inline vec2f operator+(vec2f const & other) const
    {
        return vec2f(
            x + other.x,
            y + other.y);
    }

    inline vec2f operator-(vec2f const & other) const
    {
        return vec2f(
            x - other.x,
            y - other.y);
    }

    inline vec2f operator-() const
    {
        return vec2f(
            -x,
            -y);
    }

    inline vec2f operator*(vec2f const & other) const
    {
        return vec2f(
            x * other.x,
            y * other.y);
    }

    inline vec2f operator*(float other) const
    {
        return vec2f(
            x * other,
            y * other);
    }

    inline vec2f operator/(float other) const
    {
        return vec2f(
            x / other,
            y / other);
    }

    inline vec2f & operator+=(vec2f const & other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    inline vec2f & operator-=(vec2f const & other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline vec2f & operator*=(float other)
    {
        x *= other;
        y *= other;
        return *this;
    }

    inline vec2f & operator/=(float other)
    {
        x /= other;
        y /= other;
        return *this;
    }

    inline vec2f clamp(float minX, float maxX, float minY, float maxY) const
    {
        return vec2f(
            Clamp(x, minX, maxX),
            Clamp(y, minY, maxY));
    }

    inline vec2f & clamp(float minX, float maxX, float minY, float maxY)
    {
        x = Clamp(x, minX, maxX);
        y = Clamp(y, minY, maxY);
        return *this;
    }

    inline bool operator==(vec2f const & other) const
    {
        return x == other.x && y == other.y;
    }

    inline bool operator!=(vec2f const & other) const
    {
        return !(*this == other);
    }

    // (lexicographic comparison only)
    inline bool operator<(vec2f const & other) const
    {
        return x < other.x || (x == other.x && y < other.y);
    }

    inline float dot(vec2f const & other) const noexcept
    {
        return x * other.x + y * other.y;
    }

    /*
     * When >= 0, then this is to the right of other
     * When >= 0, angle between other and this is < PI
     */
    inline float cross(vec2f const & other) const noexcept
    {
        return x * other.y - y * other.x;
    }

    inline float length() const noexcept
    {
        return std::sqrt(x * x + y * y);
    }

    inline float squareLength() const noexcept
    {
        return x * x + y * y;
    }

    inline vec2f normalise() const noexcept
    {
        float const squareLength = x * x + y * y;
        if (squareLength != 0)
        {
            return (*this) / std::sqrt(squareLength);
        }
        else
        {
            return vec2f(0.0f, 0.0f);
        }
    }

    inline vec2f normalise_approx() const noexcept
    {
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
        // SSE version is 15% faster than "normal" vec.normalize()
        __m128 _x = _mm_load_ss(&x);
        __m128 _y = _mm_load_ss(&y);

        __m128 const _sqrArg = _mm_add_ss(
            _mm_mul_ss(_x, _x),
            _mm_mul_ss(_y, _y));

        __m128 const _validMask = _mm_cmpneq_ss(_sqrArg, _mm_setzero_ps());

        __m128 const _invLen_or_zero = _mm_and_ps(
            _mm_rsqrt_ss(_sqrArg),
            _validMask);

        _x = _mm_mul_ss(_x, _invLen_or_zero);
        _y = _mm_mul_ss(_y, _invLen_or_zero);

        return vec2f(_mm_cvtss_f32(_x), _mm_cvtss_f32(_y));
#else
        return normalise();
#endif
    }

    inline vec2f normalise(float length) const noexcept
    {
        if (length != 0)
        {
            return (*this) / length;
        }
        else
        {
            return vec2f(0.0f, 0.0f);
        }
    }

    inline vec2f normalise_approx(float length) const noexcept
    {
#if FS_IS_ARCHITECTURE_X86_32() || FS_IS_ARCHITECTURE_X86_64()
        // SSE version is 5% faster than "normal" vec.normalize(length)
        __m128 _x = _mm_load_ss(&x);
        __m128 _y = _mm_load_ss(&y);

        __m128 const _length = _mm_load_ss(&length);

        __m128 const _validMask = _mm_cmpneq_ss(_length, _mm_setzero_ps());

        __m128 const _invLen_or_zero = _mm_and_ps(
            _mm_rcp_ss(_length),
            _validMask);

        _x = _mm_mul_ss(_x, _invLen_or_zero);
        _y = _mm_mul_ss(_y, _invLen_or_zero);

        return vec2f(_mm_cvtss_f32(_x), _mm_cvtss_f32(_y));
#else
        return normalise(length);
#endif
    }

    inline vec2f square() const noexcept
    {
        // |vector|^2 * normal
        return (*this) * this->length();
    }

    /*
     * Returns the CW angle between other and this; angle is positive when other
     * is CW wrt this (up to PI), and then becomes -PI at 180 degrees and decreases towards -0.
     */
    inline float angleCw(vec2f const & other) const
    {
        return -std::atan2(
            x * other.y - y * other.x,
            x * other.x + y * other.y);
    }

    /*
     * Returns the CW angle between this vector and (1.0, 0.0); angle is positive when this is
     * CW wrt (1.0, 0.0) (up to PI), and then becomes -PI at 180 degrees and decreases towards -0.
     */
    inline float angleCw() const
    {
        return -atan2f(
            y,
            x);
    }

    /*
     * Returns the vector rotated by PI/2.
     */
    inline vec2f to_perpendicular() const noexcept
    {
        return vec2f(-y, x);
    }

    /*
     * Rotates the vector by the specified angle (radians, CCW, starting at E).
     */
    inline vec2f rotate(float angle) const noexcept
    {
        float const cosAngle = std::cos(angle);
        float const sinAngle = std::sin(angle);

        return vec2f(
            x * cosAngle - y * sinAngle,
            x * sinAngle + y * cosAngle);
    }

    inline vec2f scale(vec2f const & multiplier) const noexcept
    {
        return vec2f(
            x * multiplier.x,
            y * multiplier.y);
    }

    std::string toString() const;

    vec2i to_vec2i_round() const;
};

#pragma pack(pop)

static_assert(offsetof(vec2f, x) == 0);
static_assert(offsetof(vec2f, y) == sizeof(float));
static_assert(sizeof(vec2f) == 2 * sizeof(float));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, vec2f const & v)
{
    os << v.toString();
    return os;
}

#pragma pack(push, 1)

struct vec3f
{
public:

    float x;
    float y;
    float z;

    static constexpr vec3f zero()
    {
        return vec3f();
    }

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

    inline constexpr vec3f(
        vec2f const & vec2,
        float _z)
        : x(vec2.x)
        , y(vec2.y)
        , z(_z)
    {
    }

    inline vec3f operator+(vec3f const & other) const
    {
        return vec3f(
            x + other.x,
            y + other.y,
            z + other.z);
    }

    inline vec3f operator-(vec3f const & other) const
    {
        return vec3f(
            x - other.x,
            y - other.y,
            z - other.z);
    }

    inline vec3f operator-() const
    {
        return vec3f(
            -x,
            -y,
            -z);
    }

    inline vec3f operator*(float other) const
    {
        return vec3f(
            x * other,
            y * other,
            z * other);
    }

    inline vec3f operator/(float other) const
    {
        return vec3f(
            x / other,
            y / other,
            z / other);
    }

    inline vec3f & operator+=(vec3f const & other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    inline vec3f & operator-=(vec3f const & other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    inline vec3f & operator*=(float other)
    {
        x *= other;
        y *= other;
        z *= other;
        return *this;
    }

    inline vec3f & operator/=(float other)
    {
        x /= other;
        y /= other;
        z /= other;
        return *this;
    }

    inline bool operator==(vec3f const & other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }

    inline bool operator!=(vec3f const & other) const
    {
        return !(*this == other);
    }

    // (lexicographic comparison only)
    inline bool operator<(vec3f const & other) const
    {
        return x < other.x || (x == other.x && (y < other.y || (y == other.y && z < other.z)));
    }

    float dot(vec3f const & other) const noexcept
    {
        return x * other.x + y * other.y + z * other.z;
    }

    float length() const noexcept
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    float squareLength() const noexcept
    {
        return x * x + y * y + z * z;
    }

    vec3f normalise() const noexcept
    {
        float const squareLength = x * x + y * y + z * z;
        if (squareLength != 0)
        {
            return (*this) / std::sqrt(squareLength);
        }
        else
        {
            return vec3f(0.0f, 0.0f, 0.0f);
        }
    }

    vec3f abs() const noexcept
    {
        return vec3f(
            std::abs(x),
            std::abs(y),
            std::abs(z));
    }

    std::string toString() const;
};

#pragma pack(pop)

static_assert(offsetof(vec3f, x) == 0);
static_assert(offsetof(vec3f, y) == sizeof(float));
static_assert(offsetof(vec3f, z) == 2 * sizeof(float));
static_assert(sizeof(vec3f) == 3 * sizeof(float));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, vec3f const & v)
{
    os << v.toString();
    return os;
}

#pragma pack(push, 1)

struct vec4f
{
public:

    float x;
    float y;
    float z;
    float w;

    static constexpr vec4f zero()
    {
        return vec4f();
    }

    inline constexpr vec4f()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    inline constexpr vec4f(
        float _x,
        float _y,
        float _z,
        float _w)
        : x(_x)
        , y(_y)
        , z(_z)
        , w(_w)
    {
    }

    inline constexpr vec4f(
        vec3f const & _xyz,
        float _w)
        : x(_xyz.x)
        , y(_xyz.y)
        , z(_xyz.z)
        , w(_w)
    {
    }

    inline vec4f operator+(vec4f const & other) const
    {
        return vec4f(
            x + other.x,
            y + other.y,
            z + other.z,
            w + other.w);
    }

    inline vec4f operator-(vec4f const & other) const
    {
        return vec4f(
            x - other.x,
            y - other.y,
            z - other.z,
            w - other.w);
    }

    inline vec4f operator-() const
    {
        return vec4f(
            -x,
            -y,
            -z,
            -w);
    }

    inline vec4f operator*(float other) const
    {
        return vec4f(
            x * other,
            y * other,
            z * other,
            w * other);
    }

    inline vec4f operator/(float other) const
    {
        return vec4f(
            x / other,
            y / other,
            z / other,
            w / other);
    }

    inline vec4f & operator+=(vec4f const & other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }

    inline vec4f & operator-=(vec4f const & other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }

    inline vec4f & operator*=(float other)
    {
        x *= other;
        y *= other;
        z *= other;
        w *= other;
        return *this;
    }

    inline vec4f & operator/=(float other)
    {
        x /= other;
        y /= other;
        z /= other;
        w /= other;
        return *this;
    }

    inline bool operator==(vec4f const & other) const
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    inline bool operator!=(vec4f const & other) const
    {
        return !(*this == other);
    }

    std::string toString() const;
};

#pragma pack(pop)

static_assert(offsetof(vec4f, x) == 0);
static_assert(offsetof(vec4f, y) == sizeof(float));
static_assert(offsetof(vec4f, z) == 2 * sizeof(float));
static_assert(offsetof(vec4f, w) == 3 * sizeof(float));
static_assert(sizeof(vec4f) == 4 * sizeof(float));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, vec4f const & v)
{
    os << v.toString();
    return os;
}

#pragma pack(push, 1)

struct vec2i
{
public:

    int x;
    int y;

    static constexpr vec2i zero()
    {
        return vec2i();
    }

    inline constexpr vec2i()
        : x(0)
        , y(0)
    {
    }

    inline constexpr vec2i(
        int _x,
        int _y)
        : x(_x)
        , y(_y)
    {
    }

    inline vec2i operator+(vec2i const & other) const
    {
        return vec2i(
            x + other.x,
            y + other.y);
    }

    inline vec2i operator-(vec2i const & other) const
    {
        return vec2i(
            x - other.x,
            y - other.y);
    }

    inline vec2i operator-() const
    {
        return vec2i(
            -x,
            -y);
    }

    inline vec2i & operator+=(vec2i const & other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    inline vec2i & operator-=(vec2i const & other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    inline vec2i clamp(int minX, int maxX, int minY, int maxY) const
    {
        return vec2i(
            Clamp(x, minX, maxX),
            Clamp(y, minY, maxY));
    }

    inline vec2i & clamp(int minX, int maxX, int minY, int maxY)
    {
        x = Clamp(x, minX, maxX);
        y = Clamp(y, minY, maxY);
        return *this;
    }

    inline bool operator==(vec2i const & other) const
    {
        return x == other.x && y == other.y;
    }

    inline bool operator!=(vec2i const & other) const
    {
        return !(*this == other);
    }

    // (lexicographic comparison only)
    inline bool operator<(vec2i const & other) const
    {
        return x < other.x || (x == other.x && y < other.y);
    }

    /*
     * Returns the vector rotated by PI/2.
     */
    inline vec2i to_perpendicular() const noexcept
    {
        return vec2i(-y, x);
    }

    template<typename TSize>
    bool IsInSize(TSize const & size) const
    {
        return x >= 0 && x < size.width && y >= 0 && y < size.height;
    }

    std::string toString() const;
};

#pragma pack(pop)

static_assert(offsetof(vec2i, x) == 0);
static_assert(offsetof(vec2i, y) == sizeof(int));
static_assert(sizeof(vec2i) == 2 * sizeof(int));

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, vec2i const & v)
{
    os << v.toString();
    return os;
}

inline vec2i vec2f::to_vec2i_round() const
{
    return vec2i(
        static_cast<int>(std::round(x)),
        static_cast<int>(std::round(y)));
}
