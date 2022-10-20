/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ModelValidationResults.h"

#include <GameCore/Buffer2D.h>
#include <GameCore/Finalizer.h>
#include <GameCore/GameTypes.h>

#include <optional>
#include <vector>

namespace ShipBuilder {

class ModelValidationSession final
{
private:

    static unsigned int constexpr NumberOfSteps = 2;

public:

    ModelValidationSession(
        Model const & model,
        Finalizer && finalizer);

    unsigned int GetNumberOfSteps() const
    {
        return NumberOfSteps;
    }

    std::optional<ModelValidationResults> DoNext();

private:

    Model const & mModel;
    Finalizer mFinalizer;

    //
    // State
    //

    unsigned int mCurrentStep;

    size_t mStructuralParticlesCount;
    size_t mElectricalParticlesWithNoStructuralSubstratumCount;
    size_t mLightEmittingParticlesCount;
    size_t mVisibleElectricalPanelElementsCount;

    ModelValidationResults mResults;

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

    void PrevisitStructuralLayer();

    void CheckEmptyStructuralLayer();

    void CheckStructureTooLarge();

    void PrevisitElectricalLayer();

    void CheckElectricalSubstratum();

    void CheckTooManyLights();

    void CheckTooManyElectricalPanelElements();

	void ValidateElectricalConnectivity();

    static size_t CountElectricallyUnconnected(
        std::vector<ShipSpaceCoordinates> const & propagationSources,
        std::vector<ShipSpaceCoordinates> const & propagationTargets,
        Buffer2D<CVElement, ShipSpaceTag> & connectivityVisitBuffer);
};

}