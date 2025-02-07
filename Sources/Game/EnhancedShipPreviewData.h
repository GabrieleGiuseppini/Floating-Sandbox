/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2025-02-07
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Simulation/ShipPreviewData.h>

#include <Core/PortableTimepoint.h>

#include <filesystem>

struct EnhancedShipPreviewData final : ShipPreviewData
{
    std::filesystem::path PreviewFilePath;
    PortableTimepoint LastWriteTime;

    EnhancedShipPreviewData(
        std::filesystem::path const & previewFilePath,
        ShipSpaceSize const & shipSize,
        ShipMetadata const & metadata,
        bool isHD,
        bool hasElectricals,
        PortableTimepoint lastWriteTime)
        : ShipPreviewData(
            shipSize,
            metadata,
            isHD,
            hasElectricals)
        , PreviewFilePath(previewFilePath)
        , LastWriteTime(lastWriteTime)
    {
    }

    EnhancedShipPreviewData(EnhancedShipPreviewData && other) = default;
    EnhancedShipPreviewData & operator=(EnhancedShipPreviewData && other) = default;
};
