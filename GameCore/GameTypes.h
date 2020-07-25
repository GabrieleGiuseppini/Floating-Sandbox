/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////
// Geometry
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Integral point's coordinates.
 */
struct IntegralPoint
{
    int X;
    int Y;

    IntegralPoint(
        int x,
        int y)
        : X(x)
        , Y(y)
    {}

    static IntegralPoint FromFlippedY(
        int x,
        int y,
        int height)
    {
        return IntegralPoint(x, height - 1 - y);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << X << ", " << Y << ")";
        return ss.str();
    }
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, IntegralPoint const & p)
{
    os << p.ToString();
    return os;
}

/*
 * Octants, i.e. the direction of a spring connecting two neighbors.
 *
 * Octant 0 is E, octant 1 is SE, ..., Octant 7 is NE.
 */
using Octant = std::int32_t;

////////////////////////////////////////////////////////////////////////////////////////////////
// Data Structures
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
 * IDs (sequential) of electrical elements that have an identity.
 *
 * Comparable and ordered. Start from 0.
 */
using ElectricalElementInstanceIndex = std::uint8_t; // Max 255 instances
static constexpr ElectricalElementInstanceIndex NoneElectricalElementInstanceIndex = std::numeric_limits<ElectricalElementInstanceIndex>::max();

/*
 * Various other identifiers.
 */
using LocalBombId = std::uint32_t;

/*
 * Object ID's, identifying objects of ships across ships.
 *
 * An ObjectId is unique only in the context in which it's used; for example,
 * a bomb might have the same object ID as a switch. That's where the type tag
 * comes from.
 *
 * Not comparable, not ordered.
 */
template<typename TLocalObjectId, typename TTypeTag>
struct ObjectId
{
    using LocalObjectId = TLocalObjectId;

    ObjectId(
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

template<typename TLocalObjectId, typename TTypeTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, ObjectId<TLocalObjectId, TTypeTag> const & oid)
{
    os << oid.ToString();
    return os;
}

namespace std {

    template <typename TLocalObjectId, typename TTypeTag>
    struct hash<ObjectId<TLocalObjectId, TTypeTag>>
    {
        std::size_t operator()(ObjectId<TLocalObjectId, TTypeTag> const & objectId) const
        {
            return std::hash<ShipId>()(static_cast<uint16_t>(objectId.GetShipId()))
                ^ std::hash<typename ObjectId<TLocalObjectId, TTypeTag>::LocalObjectId>()(objectId.GetLocalObjectId());
        }
    };

}

// Generic ID for generic elements (points, springs, etc.)
using ElementId = ObjectId<ElementIndex, struct ElementTypeTag>;

// ID for a bomb
using BombId = ObjectId<LocalBombId, struct BombTypeTag>;

// ID for electrical elements (switches, probes, etc.)
using ElectricalElementId = ObjectId<ElementIndex, struct ElectricalElementTypeTag>;

/*
 * A sequence number which is never zero.
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

    inline SequenceNumber & operator=(SequenceNumber const & other)
    {
        mValue = other.mValue;

        return *this;
    }

    SequenceNumber & operator++()
    {
        ++mValue;
        if (0 == mValue)
            mValue = 1;

        return *this;
    }

    inline bool operator==(SequenceNumber const & other) const
    {
        return mValue == other.mValue;
    }

    inline bool operator!=(SequenceNumber const & other) const
    {
        return !(*this == other);
    }

    inline operator bool() const
    {
        return (*this) != None();
    }

    inline bool IsStepOf(std::uint32_t step, std::uint32_t period)
    {
        return step == (mValue % period);
    }

private:

    std::uint32_t mValue;
};

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
 * Types of explosions (duh).
 */
enum class ExplosionType
{
    Combustion,
    Deflagration
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
 * Generic duration enum - short and long.
 */
enum class DurationShortLongType
{
    Short,
    Long
};

DurationShortLongType StrToDurationShortLongType(std::string const & str);

/*
 * Information (layout, etc.) for an element in the electrical panel.
 */
struct ElectricalPanelElementMetadata
{
    IntegralPoint PanelCoordinates;
    std::string Label;
    bool IsHidden;

    ElectricalPanelElementMetadata(
        IntegralPoint panelCoordinates,
        std::string const & label,
        bool isHidden)
        : PanelCoordinates(panelCoordinates)
        , Label(label)
        , IsHidden(isHidden)
    {}
};

/*
 * Repair session IDs and step IDs in a session.
 *
 * Comparable and ordered.
 */
using RepairSessionId = std::uint32_t;
using RepairSessionStepId = std::uint64_t;

/*
 * HeatBlaster action.
 */
enum class HeatBlasterActionType
{
    Heat,
    Cool
};

////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The different auto-texturization modes for ships that don't have a texture layer.
 */
enum class ShipAutoTexturizationModeType : std::int64_t
{
    FlatStructure = 1,      // Builds texture using structural materials' RenderColor
    MaterialTextures = 2    // Builds texture using materials' "Bump Maps"
};

/*
 * Ship auto-texturization settings.
 */
struct ShipAutoTexturizationSettings
{
    ShipAutoTexturizationModeType Mode;
    float MaterialTextureMagnification;
    float MaterialTextureTransparency;

    ShipAutoTexturizationSettings(
        ShipAutoTexturizationModeType mode,
        float materialTextureMagnification,
        float materialTextureTransparency)
        : Mode(mode)
        , MaterialTextureMagnification(materialTextureMagnification)
        , MaterialTextureTransparency(materialTextureTransparency)
    {}

    static ShipAutoTexturizationSettings FromJSON(picojson::object const & jsonObject);
    picojson::object ToJSON() const;
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
 * The debug ways in which ships may be rendered.
 */
enum class DebugShipRenderModeType
{
    None,
    Wireframe,
    Points,
    Springs,
    EdgeSprings,
    Decay,
    Structure
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
 * The different ways in which the ocean floor may be rendered.
 */
enum class LandRenderModeType
{
    Texture,
    Flat
};

/*
 * The different vector fields that may be rendered.
 */
enum class VectorFieldRenderModeType
{
    None,
    PointVelocity,
    PointForce,
    PointWaterVelocity,
    PointWaterMomentum
};

/*
 * The different ways of rendering ship flames.
 */
enum class ShipFlameRenderModeType
{
    Mode1,
    Mode2,
    Mode3,
    NoDraw
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
