/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-01
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/SysSpecifics.h>
#include <Core/Utils.h>

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

enum class SoundChannelModeType
{
    Mono,
    Stereo
};

struct LoopPointsType
{
    std::int32_t Start; // In-sound frame index
    std::int32_t End; // In-sound frame index; excluded

    LoopPointsType(
        std::int32_t start,
        std::int32_t end)
        : Start(start)
        , End(end)
    {
        // Prevent zero-size loops from existing
        assert(end > start);
    }

    picojson::value Serialize() const
    {
        picojson::object root;

        root.emplace("start", picojson::value(static_cast<std::int64_t>(Start)));
        root.emplace("end", picojson::value(static_cast<std::int64_t>(End)));

        return picojson::value(root);
    }

    static LoopPointsType Deserialize(picojson::object const & object)
    {
        return LoopPointsType(
            Utils::GetMandatoryJsonMember<std::int32_t>(object, "start"),
            Utils::GetMandatoryJsonMember<std::int32_t>(object, "end"));
    }
};

struct SoundAssetProperties
{
    std::string Name;

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

    picojson::value Serialize() const
    {
        picojson::object root;

        if (LoopPoints.has_value())
        {
            root.emplace("loop_points", LoopPoints->Serialize());
        }

        root.emplace("volume", picojson::value(static_cast<double>(Volume)));

        return picojson::value(root);
    }

    static SoundAssetProperties Deserialize(
        std::string const & name,
        picojson::value const & value)
    {
        auto const & rootObject = Utils::GetJsonValueAsObject(value, "SoundAssetProperties");

        std::optional<LoopPointsType> loopPoints;

        auto const loopPointsJsonObject = Utils::GetOptionalJsonObject(rootObject, "loop_points");
        if (loopPointsJsonObject.has_value())
        {
            loopPoints = LoopPointsType::Deserialize(*loopPointsJsonObject);
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
    std::int32_t Offset; // Number of frames
    std::int32_t Size; // Number of frames

    SoundAssetBuffer(
        std::int32_t offset,
        std::int32_t size)
        : Offset(offset)
        , Size(size)
    {
        // Prevent zero-size buffers from existing
        assert(size > 0);
    }

    picojson::value Serialize() const
    {
        picojson::object root;

        root.emplace("offset", picojson::value(static_cast<std::int64_t>(Offset)));
        root.emplace("size", picojson::value(static_cast<std::int64_t>(Size)));

        return picojson::value(root);
    }

    static SoundAssetBuffer Deserialize(picojson::value const & value)
    {
        auto const & rootObject = Utils::GetJsonValueAsObject(value, "SoundAssetBuffer");

        return SoundAssetBuffer(
            Utils::GetMandatoryJsonMember<std::int32_t>(rootObject, "offset"),
            Utils::GetMandatoryJsonMember<std::int32_t>(rootObject, "size"));
    }
};