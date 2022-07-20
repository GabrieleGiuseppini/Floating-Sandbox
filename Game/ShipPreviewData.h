/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <GameCore/GameTypes.h>
#include <GameCore/PortableTimepoint.h>

#include <filesystem>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreviewData
{
public:

    std::filesystem::path PreviewFilePath;
    ShipSpaceSize ShipSize;
    ShipMetadata Metadata;
    bool IsHD;
    bool HasElectricals;
    PortableTimepoint LastWriteTime;

    ShipPreviewData(
        std::filesystem::path const & previewFilePath,
        ShipSpaceSize const & shipSize,
        ShipMetadata const & metadata,
        bool isHD,
        bool hasElectricals,
        PortableTimepoint lastWriteTime)
        : PreviewFilePath(previewFilePath)
        , ShipSize(shipSize)
        , Metadata(metadata)
        , IsHD(isHD)
        , HasElectricals(hasElectricals)
        , LastWriteTime(lastWriteTime)
    {
    }

    ShipPreviewData(ShipPreviewData && other) = default;
    ShipPreviewData & operator=(ShipPreviewData && other) = default;
};
