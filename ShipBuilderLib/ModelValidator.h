/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ModelValidationResults.h"

#include <GameCore/Buffer2D.h>
#include <GameCore/GameTypes.h>

#include <vector>

namespace ShipBuilder {

class ModelValidator final
{
public:

	static ModelValidationResults ValidateModel(Model const & model);

private:

    struct ConnectivityFlags
    {
        unsigned isConductive : 1;
        unsigned isVisited : 1;
    };

    union CVElement
    {
        std::uint8_t wholeElement;
        ConnectivityFlags flags;
    };

private:

	static void ValidateElectricalConnectivity(
		ElectricalLayerData const & electricalLayer,
		std::vector<ModelValidationIssue> & issues);

    static size_t CountElectricallyUnconnected(
        std::vector<ShipSpaceCoordinates> const & propagationSources,
        std::vector<ShipSpaceCoordinates> const & propagationTargets,
        Buffer2D<CVElement, ShipSpaceTag> & connectivityVisitBuffer);
};

}