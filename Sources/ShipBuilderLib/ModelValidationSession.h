/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-07-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Model.h"
#include "ModelValidationResults.h"

#include <Core/Buffer2D.h>
#include <Core/Finalizer.h>
#include <Core/GameTypes.h>

#include <functional>
#include <optional>
#include <vector>

namespace ShipBuilder {

class ModelValidationSession final
{
public:

    ModelValidationSession(
        Model const & model,
        Finalizer && finalizer);

    size_t GetNumberOfSteps()
    {
        if (mValidationSteps.empty())
        {
            InitializeValidationSteps();
        }

        return mValidationSteps.size();
    }

    std::optional<ModelValidationResults> DoNext();

private:

    Model const & mModel;
    Finalizer mFinalizer;

    //
    // State
    //

    std::vector<std::function<void()>> mValidationSteps;

    size_t mCurrentStep;

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

    // Initialize validations (can't do in cctor)
    void InitializeValidationSteps();

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