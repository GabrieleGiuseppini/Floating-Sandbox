/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

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
static constexpr ElementIndex NoneElementIndex = std::numeric_limits<ElementIndex>::max();

/*
 * Ship identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ShipId = std::uint32_t;
static constexpr ShipId NoneShip = std::numeric_limits<ShipId>::max();

/*
 * Connected component identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ConnectedComponentId = std::uint32_t;
static constexpr ConnectedComponentId NoneConnectedComponentId = std::numeric_limits<ConnectedComponentId>::max();

/*
 * Plane (depth) identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using PlaneId = std::uint32_t;
static constexpr PlaneId NonePlaneId = std::numeric_limits<PlaneId>::max();

/*
 * Object ID's, identifying objects of ships across ships.
 *
 * An ObjectId is unique only in the context in which it's used; for example,
 * a bomb might have the same object ID as a switch.
 *
 * Not comparable, not ordered.
 */
struct ObjectId
{
    using LocalObjectId = uint32_t;

    ObjectId(
        ShipId shipId,
        LocalObjectId localObjectId)
        : mShipId(shipId)
        , mLocalObjectId(localObjectId)
    {}

    inline ShipId GetShipId() const
    {
        return mShipId;
    };

    inline LocalObjectId GetLocalObjectId() const
    {
        return mLocalObjectId;
    }

    ObjectId & operator=(ObjectId const & other) = default;

    inline bool operator==(ObjectId const & other) const
    {
        return this->mShipId == other.mShipId
            && this->mLocalObjectId == other.mLocalObjectId;
    }

    inline bool operator<(ObjectId const & other) const
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

namespace std {

template <>
struct hash<ObjectId>
{
    std::size_t operator()(ObjectId const & objectId) const
    {
        return std::hash<ShipId>()(static_cast<uint16_t>(objectId.GetShipId()))
            ^ std::hash<typename ObjectId::LocalObjectId>()(objectId.GetLocalObjectId());
    }
};

}

/*
 * Graph visit sequence numbers.
 *
 * Equatable. Never zero.
 */
using VisitSequenceNumber = std::uint32_t;
static constexpr VisitSequenceNumber NoneVisitSequenceNumber = 0;

/*
 * Types of bombs (duh).
 */
enum class BombType
{
    AntiMatterBomb,
    ImpactBomb,
    RCBomb,
    TimerBomb
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

////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The different ways in which ships may be rendered.
 */
enum class ShipRenderMode
{
    Structure,
    Texture
};

/*
 * The debug ways in which ships may be rendered.
 */
enum class DebugShipRenderMode
{
    None,
    Wireframe,
    Points,
    Springs,
    EdgeSprings
};

/*
 * The different ways in which water may be rendered.
 */
enum class WaterRenderMode
{
    Texture,
    Depth,
    Flat
};

/*
 * The different vector fields that may be rendered.
 */
enum class VectorFieldRenderMode
{
    None,
    PointVelocity,
    PointForce,
    PointWaterVelocity,
    PointWaterMomentum
};

/*
 * The texture groups we support.
 */
enum class TextureGroupType : uint16_t
{
    AirBubble = 0,
    AntiMatterBombArmor,
    AntiMatterBombSphere,
    AntiMatterBombSphereCloud,
    Cloud,
    ImpactBomb,
    Land,
    PinnedPoint,
    RcBomb,
    RcBombExplosion,
    RcBombPing,
    SawSparkle,
    TimerBomb,
    TimerBombDefuse,
    TimerBombExplosion,
    TimerBombFuse,
    Water,

    _Last = Water
};

TextureGroupType StrToTextureGroupType(std::string const & str);

/*
 * The index of a single texture frame in a group of textures.
 */
using TextureFrameIndex = std::uint16_t;

/*
 * The global identifier of a single texture frame.
 */
struct TextureFrameId
{
    TextureGroupType Group;
    TextureFrameIndex FrameIndex;

    TextureFrameId(
        TextureGroupType group,
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

    inline bool operator<(TextureFrameId const & other) const
    {
        return this->Group < other.Group
            || (this->Group == other.Group && this->FrameIndex < other.FrameIndex);
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss << static_cast<int>(Group) << ":" << static_cast<int>(FrameIndex);

        return ss.str();
    }
};

namespace std {

    template <>
    struct hash<TextureFrameId>
    {
        std::size_t operator()(TextureFrameId const & frameId) const
        {
            return std::hash<uint16_t>()(static_cast<uint16_t>(frameId.Group))
                ^ std::hash<TextureFrameIndex>()(frameId.FrameIndex);
        }
    };

}

/*
 * The different fonts available.
 */
enum class FontType
{
    // Indices must match suffix of filename
    StatusText = 0
};

/*
 * The positions at which text may be rendered.
 */
enum class TextPositionType
{
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight
};

/*
 * The handle to "sticky" rendered text.
 */
using RenderedTextHandle = uint32_t;
static constexpr RenderedTextHandle NoneRenderedTextHandle = std::numeric_limits<RenderedTextHandle>::max();
