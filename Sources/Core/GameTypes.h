/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "BarycentricCoords.h"
#include "Colors.h"
#include "EnumFlags.h"
#include "GameMath.h"
#include "SysSpecifics.h"
#include "Vectors.h"

#include <picojson.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////
// Basics
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * These types define the cardinality of elements in the ElementContainer.
 *
 * Indices are equivalent to pointers in OO terms. Given that we don't believe
 * we'll ever have more than 4 billion elements, a 32-bit integer suffices.
 *
 * This also implies that where we used to store one pointer, we can now store two indices,
 * resulting in even better data locality.
 */
using ElementCount = std::uint32_t;
using ElementIndex = std::uint32_t;
static ElementIndex constexpr NoneElementIndex = std::numeric_limits<ElementIndex>::max();

/*
 * Ship identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ShipId = std::uint32_t;
static ShipId constexpr NoneShipId = std::numeric_limits<ShipId>::max();

/*
 * Connected component identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ConnectedComponentId = std::uint32_t;
static ConnectedComponentId constexpr NoneConnectedComponentId = std::numeric_limits<ConnectedComponentId>::max();

/*
 * Plane (depth) identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using PlaneId = std::uint32_t;
static PlaneId constexpr NonePlaneId = std::numeric_limits<PlaneId>::max();

/*
 * IDs (sequential) of electrical elements that have an identity.
 *
 * Comparable and ordered. Start from 0.
 */
using ElectricalElementInstanceIndex = std::uint16_t;
static ElectricalElementInstanceIndex constexpr NoneElectricalElementInstanceIndex = std::numeric_limits<ElectricalElementInstanceIndex>::max();

/*
 * Frontier identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using FrontierId = std::uint32_t;
static FrontierId constexpr NoneFrontierId = std::numeric_limits<FrontierId>::max();

/*
 * NPC identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using NpcId = std::uint32_t;
static NpcId constexpr NoneNpcId = std::numeric_limits<NpcId>::max();

/*
 * Gadget identifiers.
 */
using GadgetId = std::uint32_t;
static GadgetId constexpr NoneGadgetId = std::numeric_limits<GadgetId>::max();

/*
 * Object ID's, identifying objects of ships across ships.
 *
 * A GlobalObjectId is unique only in the context in which it's used; for example,
 * a gadget might have the same object ID as a switch. That's where the type tag
 * comes from.
 *
 * Not comparable, not ordered.
 */
template<typename TLocalObjectId, typename TTypeTag>
struct GlobalObjectId
{
    using LocalObjectId = TLocalObjectId;

    GlobalObjectId(
        ShipId shipId,
        LocalObjectId localObjectId)
        : mShipId(shipId)
        , mLocalObjectId(localObjectId)
    {}

    inline ShipId GetShipId() const noexcept
    {
        return mShipId;
    };

    inline LocalObjectId GetLocalObjectId() const noexcept
    {
        return mLocalObjectId;
    }

    GlobalObjectId & operator=(GlobalObjectId const & other) = default;

    inline bool operator==(GlobalObjectId const & other) const
    {
        return this->mShipId == other.mShipId
            && this->mLocalObjectId == other.mLocalObjectId;
    }

    inline bool operator<(GlobalObjectId const & other) const
    {
        return this->mShipId < other.mShipId
            || (this->mShipId == other.mShipId && this->mLocalObjectId < other.mLocalObjectId);
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss << static_cast<int>(mShipId) << ":" << static_cast<int>(mLocalObjectId);

        return ss.str();
    }

private:

    ShipId mShipId;
    LocalObjectId mLocalObjectId;
};

template<typename TLocalObjectId, typename TTypeTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, GlobalObjectId<TLocalObjectId, TTypeTag> const & oid)
{
    os << oid.ToString();
    return os;
}

namespace std {

template <typename TLocalObjectId, typename TTypeTag>
struct hash<GlobalObjectId<TLocalObjectId, TTypeTag>>
{
    std::size_t operator()(GlobalObjectId<TLocalObjectId, TTypeTag> const & objectId) const
    {
        return std::hash<ShipId>()(static_cast<uint16_t>(objectId.GetShipId()))
            ^ std::hash<typename GlobalObjectId<TLocalObjectId, TTypeTag>::LocalObjectId>()(objectId.GetLocalObjectId());
    }
};

}

// Generic ID for generic elements (points, springs, etc.)
using GlobalElementId = GlobalObjectId<ElementIndex, struct ElementTypeTag>;

// ID for a ship's connected component
using GlobalConnectedComponentId = GlobalObjectId<ConnectedComponentId, struct ConnectedComponentTypeTag>;

// ID for a gadget
using GlobalGadgetId = GlobalObjectId<GadgetId, struct GadgetTypeTag>;

// ID for electrical elements (switches, probes, etc.)
using GlobalElectricalElementId = GlobalObjectId<ElementIndex, struct ElectricalElementTypeTag>;

/*
 * A sequence number which is never zero.
 *
 * Assuming an increment at each frame, this sequence will wrap
 * every ~700 days.
 */
struct SequenceNumber
{
public:

    static constexpr SequenceNumber None()
    {
        return SequenceNumber();
    }

    inline constexpr SequenceNumber()
        : mValue(0)
    {}

    inline SequenceNumber & operator=(SequenceNumber const & other) = default;

    SequenceNumber & operator++()
    {
        ++mValue;
        if (0 == mValue)
            mValue = 1;

        return *this;
    }

    SequenceNumber Previous() const
    {
        SequenceNumber res = *this;
        --res.mValue;
        if (res.mValue == 0)
            res.mValue = std::numeric_limits<std::uint32_t>::max();

        return res;
    }

    inline bool operator==(SequenceNumber const & other) const
    {
        return mValue == other.mValue;
    }

    inline bool operator!=(SequenceNumber const & other) const
    {
        return !(*this == other);
    }

    inline explicit operator bool() const
    {
        return (*this) != None();
    }

    inline bool IsStepOf(std::uint32_t step, std::uint32_t period)
    {
        return step == (mValue % period);
    }

private:

    friend std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, SequenceNumber const & s);

    std::uint32_t mValue;
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, SequenceNumber const & s)
{
    os << s.mValue;

    return os;
}

/*
 * Session identifiers. The main usecase is tool interactions.
 *
 * Never equal to itself, globally. Starts from 0.
 */
struct SessionId
{
public:

    static SessionId New();

    SessionId()
        : mValue(0)
    {}

    inline bool operator==(SessionId const & other) const
    {
        return mValue == other.mValue;
    }

    inline bool operator!=(SessionId const & other) const
    {
        return !(*this == other);
    }

private:

    explicit SessionId(std::uint64_t value)
        : mValue(value)
    {}

    std::uint64_t mValue;
};

// Password hash
using PasswordHash = std::uint64_t;

// Variable-length 16-bit unsigned integer
struct var_uint16_t
{
public:

    static std::uint16_t constexpr MaxValue = 0x03fff;

    std::uint16_t value() const
    {
        return mValue;
    }

    var_uint16_t() = default;

    constexpr explicit var_uint16_t(std::uint16_t value)
        : mValue(value)
    {
        assert(value <= MaxValue);
    }

private:

    std::uint16_t mValue;
};

namespace std {
    template<> class numeric_limits<var_uint16_t>
    {
    public:
        static constexpr var_uint16_t min() { return var_uint16_t(0); };
        static constexpr var_uint16_t max() { return var_uint16_t(var_uint16_t::MaxValue); };
        static constexpr var_uint16_t lowest() { return min(); };
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Geometry
////////////////////////////////////////////////////////////////////////////////////////////////

using ProjectionMatrix = float[4][4];

/*
 * Octants, i.e. the direction of a spring connecting two neighbors.
 *
 * Octant 0 is E, octant 1 is SE, ..., Octant 7 is NE.
 */
using Octant = std::int32_t;

/*
 * Our local circular order (clockwise, starting from E), indexes by Octant.
 * Note: cardinal directions are labeled according to x growing to the right and y growing upwards
 */
extern int const TessellationCircularOrderDirections[8][2];

/*
 * Generic directions.
 */
enum class DirectionType
{
    Horizontal = 1,
    Vertical = 2
};

template <> struct is_flag<DirectionType> : std::true_type {};

/*
 * Generic rotation directions.
 */
enum class RotationDirectionType
{
    Clockwise,
    CounterClockwise
};

/*
 * Integral system
 */

#pragma pack(push, 1)

template<typename TIntegralTag>
struct _IntegralSize
{
    using integral_type = int;

    integral_type width;
    integral_type height;

    constexpr _IntegralSize(
        integral_type _width,
        integral_type _height)
        : width(_width)
        , height(_height)
    {}

    static _IntegralSize<TIntegralTag> FromFloatRound(vec2f const & vec)
    {
        return _IntegralSize<TIntegralTag>(
            static_cast<integral_type>(std::round(vec.x)),
            static_cast<integral_type>(std::round(vec.y)));
    }

    static _IntegralSize<TIntegralTag> FromFloatFloor(vec2f const & vec)
    {
        return _IntegralSize<TIntegralTag>(
            static_cast<integral_type>(std::floor(vec.x)),
            static_cast<integral_type>(std::floor(vec.y)));
    }

    inline bool operator==(_IntegralSize<TIntegralTag> const & other) const
    {
        return this->width == other.width
            && this->height == other.height;
    }

    inline bool operator!=(_IntegralSize<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    inline _IntegralSize<TIntegralTag> operator+(_IntegralSize<TIntegralTag> const & sz) const
    {
        return _IntegralSize<TIntegralTag>(
            this->width + sz.width,
            this->height + sz.height);
    }

    inline void operator+=(_IntegralSize<TIntegralTag> const & sz)
    {
        this->width += sz.width;
        this->height += sz.height;
    }

    inline _IntegralSize<TIntegralTag> operator*(integral_type factor) const
    {
        return _IntegralSize<TIntegralTag>(
            this->width * factor,
            this->height * factor);
    }

    inline _IntegralSize<TIntegralTag> operator*(float factor) const
    {
        return _IntegralSize<TIntegralTag>(
            static_cast<int>(std::roundf(static_cast<float>(this->width) * factor)),
            static_cast<int>(std::roundf(static_cast<float>(this->height) * factor)));
    }

    inline size_t GetLinearSize() const
    {
        return this->width * this->height;
    }

    inline void Rotate90()
    {
        std::swap(width, height);
    }

    inline _IntegralSize<TIntegralTag> Union(_IntegralSize<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            std::max(this->width, other.width),
            std::max(this->height, other.height));
    }

    inline _IntegralSize<TIntegralTag> Intersection(_IntegralSize<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            std::min(this->width, other.width),
            std::min(this->height, other.height));
    }

    inline _IntegralSize<TIntegralTag> ScaleToWidth(int finalWidth) const
    {
        return _IntegralSize<TIntegralTag>(
            finalWidth,
            static_cast<int>(
                std::roundf(
                    static_cast<float>(this->height)
                    / static_cast<float>(this->width)
                    * static_cast<float>(finalWidth))));
    }

    inline _IntegralSize<TIntegralTag> ShrinkToFit(_IntegralSize<TIntegralTag> const & maxSize) const
    {
        float wShrinkFactor = static_cast<float>(maxSize.width) / static_cast<float>(this->width);
        float hShrinkFactor = static_cast<float>(maxSize.height) / static_cast<float>(this->height);
        float shrinkFactor = std::min(
            std::min(wShrinkFactor, hShrinkFactor),
            1.0f);

        return _IntegralSize<TIntegralTag>(
            static_cast<int>(std::roundf(static_cast<float>(this->width) * shrinkFactor)),
            static_cast<int>(std::roundf(static_cast<float>(this->height) * shrinkFactor)));
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    template<typename TCoordsRatio>
    vec2f ToFractionalCoords(TCoordsRatio const & coordsRatio) const
    {
        assert(coordsRatio.inputUnits != 0.0f);

        return vec2f(
            static_cast<float>(width) / coordsRatio.inputUnits * coordsRatio.outputUnits,
            static_cast<float>(height) / coordsRatio.inputUnits * coordsRatio.outputUnits);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << width << " x " << height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

template<typename TTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, _IntegralSize<TTag> const & is)
{
    os << is.ToString();
    return os;
}

using IntegralRectSize = _IntegralSize<struct IntegralTag>;
using ImageSize = _IntegralSize<struct ImageTag>;
using ShipSpaceSize = _IntegralSize<struct ShipSpaceTag>;
using DisplayLogicalSize = _IntegralSize<struct DisplayLogicalTag>;
using DisplayPhysicalSize = _IntegralSize<struct DisplayPhysicalTag>;

#pragma pack(push, 1)

template<typename TIntegralTag>
struct _IntegralCoordinates
{
    using integral_type = int;

    integral_type x;
    integral_type y;

    constexpr _IntegralCoordinates(
        integral_type _x,
        integral_type _y)
        : x(_x)
        , y(_y)
    {}

    static _IntegralCoordinates<TIntegralTag> FromFloatRound(vec2f const & vec)
    {
        return _IntegralCoordinates<TIntegralTag>(
            static_cast<integral_type>(std::round(vec.x)),
            static_cast<integral_type>(std::round(vec.y)));
    }

    static _IntegralCoordinates<TIntegralTag> FromFloatFloor(vec2f const & vec)
    {
        return _IntegralCoordinates<TIntegralTag>(
            static_cast<integral_type>(std::floor(vec.x)),
            static_cast<integral_type>(std::floor(vec.y)));
    }

    inline bool operator==(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return this->x == other.x
            && this->y == other.y;
    }

    inline bool operator!=(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    inline _IntegralCoordinates<TIntegralTag> operator+(_IntegralSize<TIntegralTag> const & sz) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x + sz.width,
            this->y + sz.height);
    }

    inline void operator+=(_IntegralSize<TIntegralTag> const & sz)
    {
        this->x += sz.width;
        this->y += sz.height;
    }

    inline _IntegralCoordinates<TIntegralTag> operator-() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            -this->x,
            -this->y);
    }

    inline _IntegralSize<TIntegralTag> operator-(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            this->x - other.x,
            this->y - other.y);
    }

    inline _IntegralCoordinates<TIntegralTag> operator-(_IntegralSize<TIntegralTag> const & offset) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x - offset.width,
            this->y - offset.height);
    }

    inline _IntegralCoordinates<TIntegralTag> operator*(float factor) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            static_cast<int>(std::roundf(static_cast<float>(this->x) * factor)),
            static_cast<int>(std::roundf(static_cast<float>(this->y) * factor)));
    }

    inline _IntegralCoordinates<TIntegralTag> scale(_IntegralCoordinates<TIntegralTag> const & multiplier) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x * multiplier.x,
            this->y * multiplier.y);
    }

    template<typename TSize>
    bool IsInSize(TSize const & size) const
    {
        return x >= 0 && x < size.width && y >= 0 && y < size.height;
    }

    template<typename TRect>
    bool IsInRect(TRect const & rect) const
    {
        return x >= rect.origin.x && x < rect.origin.x + rect.size.width
            && y >= rect.origin.y && y < rect.origin.y + rect.size.height;
    }

    template<typename TSize>
    _IntegralCoordinates<TIntegralTag> Clamp(TSize const & size) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            ::Clamp(x, 0, size.width),
            ::Clamp(y, 0, size.height));
    }

    _IntegralCoordinates<TIntegralTag> FlipX(integral_type width) const
    {
        assert(width > x);
        return _IntegralCoordinates<TIntegralTag>(width - 1 - x, y);
    }

    _IntegralCoordinates<TIntegralTag> FlipY(integral_type height) const
    {
        assert(height > y);
        return _IntegralCoordinates<TIntegralTag>(x, height - 1 - y);
    }

    // Returns coords of this point after being rotated (and assuming
    // the size will also get rotated).
    template<RotationDirectionType TDirection>
    _IntegralCoordinates<TIntegralTag> Rotate90(_IntegralSize<TIntegralTag> const & sz) const
    {
        if constexpr (TDirection == RotationDirectionType::Clockwise)
        {
            return _IntegralCoordinates<TIntegralTag>(
                this->y,
                sz.width - 1 - this->x);
        }
        else
        {
            static_assert(TDirection == RotationDirectionType::CounterClockwise);

            return _IntegralCoordinates<TIntegralTag>(
                sz.height - 1 - this->y,
                this->x);
        }
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(x),
            static_cast<float>(y));
    }

    template<typename TCoordsRatio>
    vec2f ToFractionalCoords(TCoordsRatio const & coordsRatio) const
    {
        assert(coordsRatio.inputUnits != 0.0f);

        return vec2f(
            static_cast<float>(x) / coordsRatio.inputUnits * coordsRatio.outputUnits,
            static_cast<float>(y) / coordsRatio.inputUnits * coordsRatio.outputUnits);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }
};

#pragma pack(pop)

template<typename TTag>
inline bool operator<(_IntegralCoordinates<TTag> const & l, _IntegralCoordinates<TTag> const & r)
{
    return l.x < r.x
        || (l.x == r.x && l.y < r.y);
}

template<typename TTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, _IntegralCoordinates<TTag> const & p)
{
    os << p.ToString();
    return os;
}

using IntegralCoordinates = _IntegralCoordinates<struct IntegralTag>; // Generic integer
using ImageCoordinates = _IntegralCoordinates<struct ImageTag>; // Image
using ShipSpaceCoordinates = _IntegralCoordinates<struct ShipSpaceTag>; // Y=0 at bottom
using DisplayLogicalCoordinates = _IntegralCoordinates<struct DisplayLogicalTag>; // Y=0 at top
using DisplayPhysicalCoordinates = _IntegralCoordinates<struct DisplayPhysicalTag>; // Y=0 at top

#pragma pack(push)

template<typename TIntegralTag>
struct _IntegralRect
{
    _IntegralCoordinates<TIntegralTag> origin;
    _IntegralSize<TIntegralTag> size;

    constexpr _IntegralRect()
        : origin(0,0 )
        , size(0, 0)
    {}

    constexpr _IntegralRect(
        _IntegralCoordinates<TIntegralTag> const & _origin,
        _IntegralSize<TIntegralTag> const & _size)
        : origin(_origin)
        , size(_size)
    {}

    explicit constexpr _IntegralRect(_IntegralCoordinates<TIntegralTag> const & _origin)
        : origin(_origin)
        , size(1, 1)
    {}

    constexpr _IntegralRect(
        _IntegralCoordinates<TIntegralTag> const & _origin,
        _IntegralCoordinates<TIntegralTag> const & _oppositeCorner)
        : origin(
            std::min(_origin.x, _oppositeCorner.x),
            std::min(_origin.y, _oppositeCorner.y))
        , size(
            std::abs(_oppositeCorner.x - _origin.x),
            std::abs(_oppositeCorner.y - _origin.y))
    {
    }

    /*
     * Makes a rectangle from {0, 0} of the specified size.
     */
    explicit constexpr _IntegralRect(_IntegralSize<TIntegralTag> const & _size)
        : origin(0, 0)
        , size(_size)
    {}

    _IntegralCoordinates<TIntegralTag> MinMin() const
    {
        return origin;
    }

    _IntegralCoordinates<TIntegralTag> MaxMin() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            origin.x + size.width,
            origin.y);
    }

    _IntegralCoordinates<TIntegralTag> MaxMax() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            origin.x + size.width,
            origin.y + size.height);
    }

    _IntegralCoordinates<TIntegralTag> MinMax() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            origin.x,
            origin.y + size.height);
    }

    _IntegralCoordinates<TIntegralTag> Center() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            origin.x + size.width / 2,
            origin.y + size.height / 2);
    }

    inline bool operator==(_IntegralRect<TIntegralTag> const & other) const
    {
        return origin == other.origin
            && size == other.size;
    }

    inline bool operator!=(_IntegralRect<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    bool IsEmpty() const
    {
        return size.width == 0 || size.height == 0;
    }

    bool IsContainedInRect(_IntegralRect<TIntegralTag> const & container) const
    {
        return origin.x >= container.origin.x
            && origin.y >= container.origin.y
            && origin.x + size.width <= container.origin.x + container.size.width
            && origin.y + size.height <= container.origin.y + container.size.height;
    }

    void UnionWith(_IntegralCoordinates<TIntegralTag> const & other)
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::min(origin.x, other.x),
            std::min(origin.y, other.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::max(origin.x + size.width, other.x + 1) - newOrigin.x,
            std::max(origin.y + size.height, other.y + 1) - newOrigin.y);

        assert(newSize.width >= 0 && newSize.height >= 0);

        origin = newOrigin;
        size = newSize;
    }

    void UnionWith(_IntegralRect<TIntegralTag> const & other)
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::min(origin.x, other.origin.x),
            std::min(origin.y, other.origin.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::max(origin.x + size.width, other.origin.x + other.size.width) - newOrigin.x,
            std::max(origin.y + size.height, other.origin.y + other.size.height) - newOrigin.y);

        assert(newSize.width >= 0 && newSize.height >= 0);

        origin = newOrigin;
        size = newSize;
    }

    std::optional<_IntegralRect<TIntegralTag>> MakeIntersectionWith(_IntegralRect<TIntegralTag> const & other) const
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::max(origin.x, other.origin.x),
            std::max(origin.y, other.origin.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::min(size.width - (newOrigin.x - origin.x), other.size.width - (newOrigin.x - other.origin.x)),
            std::min(size.height - (newOrigin.y - origin.y), other.size.height - (newOrigin.y - other.origin.y)));

        if (newSize.width <= 0 || newSize.height <= 0)
        {
            return std::nullopt;
        }
        else
        {
            return _IntegralRect<TIntegralTag>(
                newOrigin,
                newSize);
        }
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << origin.x << ", " << origin.y << " -> " << size.width << " x " << size.height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

template<typename TTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, _IntegralRect<TTag> const & p)
{
    os << p.origin.ToString() << "x" << p.size.ToString();
    return os;
}

using IntegralRect = _IntegralRect<struct IntegralTag>;
using ImageRect = _IntegralRect<struct ImageTag>;
using ShipSpaceRect = _IntegralRect<struct ShipSpaceTag>;
using DisplayPhysicalRect = _IntegralRect<struct DisplayPhysicalTag>; // Y=0 at top

template<typename TIntegralTag>
struct _IntegralCoordsRatio
{
    float inputUnits; // i.e. how many integral units
    float outputUnits; // i.e. how many float units

    constexpr _IntegralCoordsRatio(
        float _inputUnits,
        float _outputUnits)
        : inputUnits(_inputUnits)
        , outputUnits(_outputUnits)
    {}

    inline bool operator==(_IntegralCoordsRatio<TIntegralTag> const & other) const
    {
        return inputUnits == other.inputUnits
            && outputUnits == other.outputUnits;
    }

    inline bool operator!=(_IntegralCoordsRatio<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }
};

using ShipSpaceToWorldSpaceCoordsRatio = _IntegralCoordsRatio<struct ShipSpaceTag>;

/*
 * Generic quad (not necessarily square), intrinsics-friendly
 */

#pragma pack(push, 1)

union Quad final
{
    struct
    {
        vec2f TopLeft;
        vec2f BottomLeft;
        vec2f TopRight;
        vec2f BottomRight;
    } V;

    float fptr[8];

    Quad & operator=(Quad const & other) = default;
};

#pragma pack(pop)

/*
 * Float size.
 */

#pragma pack(push)

struct FloatSize
{
    float width;
    float height;

    constexpr FloatSize(
        float const & _width,
        float const & _height)
        : width(_width)
        , height(_height)
    {}

    static constexpr FloatSize zero()
    {
        return FloatSize(0.0f, 0.0f);
    }

    inline bool operator==(FloatSize const & other) const
    {
        return width == other.width
            && height == other.height;
    }

    inline constexpr FloatSize operator+(FloatSize const & other) const
    {
        return FloatSize(
            width + other.width,
            height + other.height);
    }

    inline constexpr FloatSize operator/(float scale) const
    {
        return FloatSize(
            width / scale,
            height / scale);
    }

    inline vec2i to_vec2i_round() const
    {
        return vec2i(
            static_cast<int>(std::roundf(width)),
            static_cast<int>(std::roundf(height)));
    }

    std::string toString() const
    {
        std::stringstream ss;
        ss << std::setprecision(12) << "(" << width << " x " << height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, FloatSize const & fs)
{
    os << fs.toString();
    return os;
}

/*
 * Float rectangle.
 */

#pragma pack(push)

struct FloatRect
{
    vec2f origin;
    FloatSize size;

    constexpr FloatRect()
        : origin(vec2f::zero())
        , size(FloatSize::zero())
    {}

    constexpr FloatRect(
        vec2f const & _origin,
        FloatSize const & _size)
        : origin(_origin)
        , size(_size)
    {}

    inline bool operator==(FloatRect const & other) const
    {
        return origin == other.origin
            && size == other.size;
    }

    float const CalculateRightX() const
    {
        return origin.x + size.width;
    }

    vec2f const CalculateCenter() const
    {
        return vec2f(
            origin.x + size.width / 2.0f,
            origin.y + size.height / 2.0f);
    }

    bool Contains(vec2f const & pos) const
    {
        return pos.x >= origin.x && pos.x <= origin.x + size.width
               && pos.y >= origin.y && pos.y <= origin.y + size.height;
    }

    bool IsContainedInRect(FloatRect const & container) const
    {
        return origin.x >= container.origin.x
            && origin.y >= container.origin.y
            && origin.x + size.width <= container.origin.x + container.size.width
            && origin.y + size.height <= container.origin.y + container.size.height;
    }

    void UnionWith(FloatRect const & other)
    {
        auto const newOrigin = vec2f(
            std::min(origin.x, other.origin.x),
            std::min(origin.y, other.origin.y));

        auto const newSize = FloatSize(
            std::max(origin.x + size.width, other.origin.x + other.size.width) - newOrigin.x,
            std::max(origin.y + size.height, other.origin.y + other.size.height) - newOrigin.y);

        assert(newSize.width >= 0 && newSize.height >= 0);

        origin = newOrigin;
        size = newSize;
    }

    std::optional<FloatRect> MakeIntersectionWith(FloatRect const & other) const
    {
        auto const newOrigin = vec2f(
            std::max(origin.x, other.origin.x),
            std::max(origin.y, other.origin.y));

        auto const newSize = FloatSize(
            std::min(size.width - (newOrigin.x - origin.x), other.size.width - (newOrigin.x - other.origin.x)),
            std::min(size.height - (newOrigin.y - origin.y), other.size.height - (newOrigin.y - other.origin.y)));

        if (newSize.width <= 0 || newSize.height <= 0)
        {
            return std::nullopt;
        }
        else
        {
            return FloatRect(
                newOrigin,
                newSize);
        }
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << origin.x << ", " << origin.y << " -> " << size.width << " x " << size.height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

/*
 * Identifies the edge of a triangle among all edges on a ship.
 */
struct TriangleAndEdge
{
    ElementIndex TriangleElementIndex;
    int EdgeOrdinal;

    TriangleAndEdge() = default;

    TriangleAndEdge(
        ElementIndex triangleElementIndex,
        int edgeOrdinal)
        : TriangleElementIndex(triangleElementIndex)
        , EdgeOrdinal(edgeOrdinal)
    {
        assert(triangleElementIndex != NoneElementIndex);
        assert(edgeOrdinal >= 0 && edgeOrdinal < 3);
    }
};

/*
 * Barycentric coordinates in a specific triangle.
 */
struct AbsoluteTriangleBCoords
{
    ElementIndex TriangleElementIndex;
    bcoords3f BCoords;

    AbsoluteTriangleBCoords() = default;

    AbsoluteTriangleBCoords(
        ElementIndex triangleElementIndex,
        bcoords3f bCoords)
        : TriangleElementIndex(triangleElementIndex)
        , BCoords(bCoords)
    {
        assert(triangleElementIndex != NoneElementIndex);
    }

    bool operator==(AbsoluteTriangleBCoords const & other) const
    {
        return this->TriangleElementIndex == other.TriangleElementIndex
            && this->BCoords == other.BCoords;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << TriangleElementIndex << ":" << BCoords;
        return ss.str();
    }
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, AbsoluteTriangleBCoords const & is)
{
    os << is.ToString();
    return os;
}

/*
 * Definition of the visible portion of the world.
 */
struct VisibleWorld
{
    vec2f Center;

    float Width;
    float Height;

    vec2f TopLeft;
    vec2f BottomRight;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Computation
////////////////////////////////////////////////////////////////////////////////////////////////

enum class SpringRelaxationParallelComputationModeType
{
    StepByStep,
    FullSpeed,
    Hybrid
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Game
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The color key of materials.
 */
using MaterialColorKey = rgbColor;

static MaterialColorKey constexpr EmptyMaterialColorKey = MaterialColorKey(255, 255, 255);

/*
 * The different layers.
 */
enum class LayerType : std::uint32_t
{
    Structural = 0,
    Electrical = 1,
    Ropes = 2,
    ExteriorTexture = 3,
    InteriorTexture = 4
};

/*
 * The different material layers.
 */
enum class MaterialLayerType
{
    Structural,
    Electrical
};

/*
 * Top level of NPC type hierarchy.
 */
enum class NpcKindType
{
    Furniture,
    Human
};

std::string NpcKindTypeToStr(NpcKindType npcKind);

/*
 * Second level of NPC type hierarchy; domain is open
 * as it may be expanded after compile time, via NPC packs.
 * The unique identifier of an NPC kind is the whole
 * <NpcKindType,NpcSubKindIdType> tuple; so, for example,
 * NpcSubKindIdType=X means one thing for Humans and another
 * thing for Furniture.
 */
using NpcSubKindIdType = std::uint32_t;

/*
 * Roles for humans.
 */
enum class NpcHumanRoleType : std::uint32_t
{
    Captain = 0,
    Crew = 1,
    Passenger = 2,
    Other = 3,

    _Last = Other
};

NpcHumanRoleType StrToNpcHumanRoleType(std::string const & str);

/*
 * Roles for furniture.
 */
enum class NpcFurnitureRoleType : std::uint32_t
{
    Furniture = 0,
    Other = 1,

    _Last = Other
};

NpcFurnitureRoleType StrToNpcFurnitureRoleType(std::string const & str);

/*
 * Return type of picking an NPC.
 */
struct PickedNpc
{
    NpcId Id;
    int ParticleOrdinal;
    vec2f WorldOffset;

    PickedNpc(
        NpcId id,
        int particleOrdinal,
        vec2f const & worldOffset)
        : Id(id)
        , ParticleOrdinal(particleOrdinal)
        , WorldOffset(worldOffset)
    {}
};

/*
 * Reasons for NPC placement failure.
 */
enum class NpcPlacementFailureReasonType
{
    Success,
    TooManyNpcs,
    TooManyCaptains
};

/*
 * Return type of attempting to place an NPC.
 */
struct NpcPlacementOutcome
{
    std::optional<PickedNpc> Npc;
    NpcPlacementFailureReasonType FailureReason;
};

enum class NpcFloorKindType
{
    NotAFloor,
    DefaultFloor // Futurework: areas, etc.
};

enum class NpcFloorGeometryDepthType
{
    NotAFloor,
    Depth1, // Main depth: H-V
    Depth2 // Staircases: S-S
};

enum class NpcFloorGeometryType
{
    NotAFloor,
    // Depth 1: main depth
    Depth1H,
    Depth1V,
    // Depth 2: staircases
    Depth2S1,
    Depth2S2
};

inline NpcFloorGeometryDepthType NpcFloorGeometryDepth(NpcFloorGeometryType geometry)
{
    switch (geometry)
    {
        case NpcFloorGeometryType::NotAFloor:
        {
            return NpcFloorGeometryDepthType::NotAFloor;
        }

        case NpcFloorGeometryType::Depth1H:
        case NpcFloorGeometryType::Depth1V:
        {
            return NpcFloorGeometryDepthType::Depth1;
        }

        case NpcFloorGeometryType::Depth2S1:
        case NpcFloorGeometryType::Depth2S2:
        {
            return NpcFloorGeometryDepthType::Depth2;
        }
    }

    assert(false);
    return NpcFloorGeometryDepthType::NotAFloor;
}

/*
 * Types of frontiers (duh).
 */
enum class FrontierType
{
    External,
    Internal
};

/*
 * Types of gadgets.
 */
enum class GadgetType
{
    AntiMatterBomb,
    FireExtinguishingBomb,
    ImpactBomb,
    PhysicsProbe,
    RCBomb,
    TimerBomb
};

/*
 * Types of explosions.
 */
enum class ExplosionType
{
    Combustion,
    Deflagration,
    FireExtinguishing,
    Sodium
};

/*
 * Types of electrical switches.
 */
enum class SwitchType
{
    InteractiveToggleSwitch,
    InteractivePushSwitch,
    AutomaticSwitch,
    ShipSoundSwitch
};

/*
 * Types of power probes.
 */
enum class PowerProbeType
{
    PowerMonitor,
    Generator
};

/*
 * Electrical states.
 */
enum class ElectricalState : bool
{
    Off = false,
    On = true
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, ElectricalState const & s)
{
    if (s == ElectricalState::On)
    {
        os << "ON";
    }
    else
    {
        assert(s == ElectricalState::Off);
        os << "OFF";
    }

    return os;
}

/*
 * Unit systems.
 *
 * Note: values of this enum are saved in preferences.
 */
enum class UnitsSystem
{
    SI_Kelvin,
    SI_Celsius,
    USCS
};

/*
 * Generic duration enum - short and long.
 */
enum class DurationShortLongType
{
    Short,
    Long
};

DurationShortLongType StrToDurationShortLongType(std::string const & str);

/*
 * HeatBlaster action.
 */
enum class HeatBlasterActionType
{
    Heat,
    Cool
};

/*
 * Location that a tool is applied to.
 */
enum class ToolApplicationLocus
{
    World = 1,
    Ship = 2,

    AboveWater = 4,
    UnderWater = 8
};

template <> struct is_flag<ToolApplicationLocus> : std::true_type {};

enum class NoiseType : std::uint32_t
{
    Gross = 0,
    Fine = 1,
    Perlin_4_32_043 = 2,
    Perlin_8_1024_073 = 3
};

/*
 * Parameters of a gripped move.
 */
struct GrippedMoveParameters
{
    // All in world coords
    vec2f GripCenter;
    float GripRadius;
    vec2f MoveOffset;
    vec2f InertialVelocity;
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////////////////////////////////////

struct TextureCoordinatesQuad
{
    float LeftX;
    float RightX;
    float BottomY;
    float TopY;

    TextureCoordinatesQuad FlipH() const
    {
        return TextureCoordinatesQuad({
            RightX,
            LeftX,
            BottomY,
            TopY });
    }
};

/*
 * A color together with a progress float.
 *
 * Used as-is in shaders.
 */
#pragma pack(push, 1)
struct ColorWithProgress
{
    vec3f BaseColor;
    float Progress;

    ColorWithProgress(
        vec3f baseColor,
        float progress)
        : BaseColor(baseColor)
        , Progress(progress)
    {}
};
#pragma pack(pop)

/*
 * The positions at which UI elements may be anchored.
 */
enum class AnchorPositionType
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

/*
 * The different ship views.
 */
enum class ShipViewModeType
{
    Exterior,
    Interior
};

/*
 * The different auto-texturization modes for ships that don't have a texture layer.
 *
 * Note: enum value are serialized in ship files, do not change.
 */
enum class ShipAutoTexturizationModeType : std::uint32_t
{
    FlatStructure = 1,      // Builds texture using structural materials' RenderColor
    MaterialTextures = 2    // Builds texture using materials' "Bump Maps"
};

/*
 * The different visual ways in which we render highlights.
 */
enum class HighlightModeType : size_t
{
    Circle = 0,
    ElectricalElement,

    _Last = ElectricalElement
};

/*
 * The ways in which heat may be rendered.
 */
enum class HeatRenderModeType
{
    None,
    Incandescence,
    HeatOverlay
};

/*
 * The ways in which stress may be rendered.
 */
enum class StressRenderModeType
{
    None,
    StressOverlay,
    TensionOverlay
};

/*
 * The ways in which ships particles (points) may be rendered.
 */
enum class ShipParticleRenderModeType
{
    Fragment,
    Particle
};

/*
 * The debug ways in which ships may be rendered.
 */
enum class DebugShipRenderModeType
{
    None,
    Wireframe,
    Points,
    Springs,
    EdgeSprings,
    Structure,
    Decay,
    InternalPressure,
    Strength
};

/*
 * The different levels of detail with which clouds may be rendered.
 */
enum class CloudRenderDetailType
{
    Basic,
    Detailed
};

/*
 * The different ways in which the ocean may be rendered.
 */
enum class OceanRenderModeType
{
    Texture,
    Depth,
    Flat
};

/*
 * The different levels of detail with which the ocean may be rendered.
 */
enum class OceanRenderDetailType
{
    Basic,
    Detailed
};

/*
 * The different ways in which the ocean floor may be rendered.
 */
enum class LandRenderModeType
{
    Texture,
    Flat
};

/*
 * The different levels of detail with which the land may be rendered.
 */
enum class LandRenderDetailType
{
    Basic,
    Detailed
};

/*
 * The different types in which NPCs (humans and furniture) may be rendered.
 */
enum class NpcRenderModeType
{
    Texture,
    QuadWithRoles,
    QuadFlat
};

/*
 * The different vector fields that may be rendered.
 */
enum class VectorFieldRenderModeType
{
    None,
    PointVelocity,
    PointStaticForce,
    PointDynamicForce,
    PointWaterVelocity,
    PointWaterMomentum
};

/*
 * The possible targets of auto-focus.
 */
enum class AutoFocusTargetKindType
{
    Ship,
    SelectedNpc
};

/*
 * The index of a single texture frame in a group of textures.
 */
using TextureFrameIndex = std::uint16_t;

/*
 * The global identifier of a single texture frame.
 *
 * The identifier of a frame is hierarchical:
 * - A group, identified by a value of the enum that
 *   this identifier is templated on
 * - The index of the frame in that group
 */
template <typename TextureGroups>
struct TextureFrameId
{
    TextureGroups Group;
    TextureFrameIndex FrameIndex;

    TextureFrameId(
        TextureGroups group,
        TextureFrameIndex frameIndex)
        : Group(group)
        , FrameIndex(frameIndex)
    {}

    TextureFrameId & operator=(TextureFrameId const & other) = default;

    inline bool operator==(TextureFrameId const & other) const
    {
        return this->Group == other.Group
            && this->FrameIndex == other.FrameIndex;
    }

    inline bool operator!=(TextureFrameId const & other) const
    {
        return !(*this == other);
    }

    inline bool operator<(TextureFrameId const & other) const
    {
        return this->Group < other.Group
            || (this->Group == other.Group && this->FrameIndex < other.FrameIndex);
    }

    inline bool operator>(TextureFrameId const & other) const
    {
        return (*this != other) && !(*this < other);
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss << static_cast<int>(Group) << ":" << static_cast<int>(FrameIndex);

        return ss.str();
    }
};

namespace std {

    template <typename TextureGroups>
    struct hash<TextureFrameId<TextureGroups>>
    {
        std::size_t operator()(TextureFrameId<TextureGroups> const & frameId) const
        {
            return std::hash<uint16_t>()(static_cast<uint16_t>(frameId.Group))
                ^ std::hash<TextureFrameIndex>()(frameId.FrameIndex);
        }
    };

}
