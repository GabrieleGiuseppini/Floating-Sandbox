/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/SysSpecifics.h>
#include <Core/Utils.h>

#include <picojson.h>

#include <optional>
#include <string>

enum class SoundChannelModeType
{
    Mono,
    Stero
};

struct SoundAssetProperties
{
    std::string Name;

    struct LoopPointsType
    {
        size_t Start; // Frame index
        size_t End; // Frame index; excluded

        LoopPointsType(
            size_t start,
            size_t end)
            : Start(start)
            , End(end)
        { }

        static LoopPointsType Deserialize(picojson::object const & object)
        {
            return LoopPointsType(
                Utils::GetMandatoryJsonMember<size_t>(object, "start"),
                Utils::GetMandatoryJsonMember<size_t>(object, "end"));
        }
    };

    std::optional<LoopPointsType> LoopPoints; // If set, it's looping sound

    float Volume; // Asset volume

    SoundAssetProperties(
        std::string name,
        std::optional<LoopPointsType> loopPoints,
        float volume)
        : Name(std::move(name))
        , LoopPoints(std::move(loopPoints))
        , Volume(volume)
    { }

    static SoundAssetProperties Deserialize(
        std::string const & name,
        picojson::value const & value)
    {
        auto const & rootObject = Utils::GetJsonValueAsObject(value, "SoundAssetProperties");

        std::optional<SoundAssetProperties::LoopPointsType> loopPoints;

        auto const loopPointsJsonObject = Utils::GetOptionalJsonObject(rootObject, "loop_points");
        if (loopPointsJsonObject.has_value())
        {
            loopPoints = SoundAssetProperties::LoopPointsType::Deserialize(*loopPointsJsonObject);
        }

        auto const volumeJsonValue = Utils::GetOptionalJsonMember<float>(rootObject, "volume");

        return SoundAssetProperties(
            name,
            loopPoints,
            volumeJsonValue.value_or(1.0f));
    }
};

struct SoundAssetBuffer
{
    float * restrict Ptr;
    size_t Size; // Number of frames

    SoundAssetBuffer(
        float * restrict ptr,
        size_t size)
        : Ptr(ptr)
        , Size(size)
    {
    }
};