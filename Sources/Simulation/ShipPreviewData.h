/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-01-26
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipMetadata.h"

#include <Core/GameTypes.h>

/*
* A partial ship definition, suitable for a preview of the ship.
*/
struct ShipPreviewData
{
public:

    ShipSpaceSize ShipSize;
    ShipMetadata Metadata;
    bool IsHD;
    bool HasElectricals;

    ShipPreviewData(
        ShipSpaceSize const & shipSize,
        ShipMetadata const & metadata,
        bool isHD,
        bool hasElectricals)
        : ShipSize(shipSize)
        , Metadata(metadata)
        , IsHD(isHD)
        , HasElectricals(hasElectricals)
    {
    }

    ShipPreviewData(ShipPreviewData && other) = default;
    ShipPreviewData & operator=(ShipPreviewData && other) = default;
};
