/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-07-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelValidationSession.h"

#include "WorkbenchState.h"

#include <cassert>
#include <queue>

namespace ShipBuilder {

ModelValidationSession::ModelValidationSession(
    Model const & model,
    Finalizer && finalizer)
    : mModel(model)
    , mFinalizer(std::move(finalizer))
    , mValidationSteps()
    , mCurrentStep(0)
{
}

std::optional<ModelValidationResults> ModelValidationSession::DoNext()
{
    if (mValidationSteps.empty())
    {
        InitializeValidationSteps();
    }

    if (mCurrentStep < mValidationSteps.size())
    {
        mValidationSteps[mCurrentStep]();
        ++mCurrentStep;
        return std::nullopt;
    }
    else
    {
        // We're done
        return mResults;
    }
}

/////////////////////////////////////////////////////////////////

void ModelValidationSession::InitializeValidationSteps()
{
    // Initialize validations (can't do in cctor)
    assert(mValidationSteps.empty());
    if (mModel.HasLayer(LayerType::Structural))
    {
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::PrevisitStructuralLayer, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckEmptyStructuralLayer, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckStructureTooLarge, this));
    }
    if (mModel.HasLayer(LayerType::Electrical))
    {
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::PrevisitElectricalLayer, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckElectricalSubstratum, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckTooManyLights, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckTooManyElectricalPanelElements, this));
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::ValidateElectricalConnectivity, this));
    }
    if (mModel.HasLayer(LayerType::ExteriorTexture))
    {
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckExteriorLayerTextureSize, this));
    }
    if (mModel.HasLayer(LayerType::InteriorTexture))
    {
        mValidationSteps.emplace_back(std::bind(&ModelValidationSession::CheckInteriorLayerTextureSize, this));
    }
}

void ModelValidationSession::PrevisitStructuralLayer()
{
    assert(mModel.HasLayer(LayerType::Structural));

    StructuralLayerData const & structuralLayer = mModel.GetStructuralLayer();

    mStructuralParticlesCount = 0;

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            if (structuralLayer.Buffer[{x, y}].Material != nullptr)
            {
                ++mStructuralParticlesCount;
            }
        }
    }
}

void ModelValidationSession::CheckEmptyStructuralLayer()
{
    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::EmptyStructuralLayer,
        (mStructuralParticlesCount == 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);
}

void ModelValidationSession::CheckStructureTooLarge()
{
    if (mStructuralParticlesCount != 0)
    {
        size_t constexpr MaxStructuralParticles = 100000;

        mResults.AddIssue(
            ModelValidationIssue::CheckClassType::StructureTooLarge,
            (mStructuralParticlesCount > MaxStructuralParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
    }
}

void ModelValidationSession::PrevisitElectricalLayer()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mElectricalParticlesWithNoStructuralSubstratumCount = 0;
    mLightEmittingParticlesCount = 0;
    mVisibleElectricalPanelElementsCount = 0;

    //
    // Visit electrical layer
    //

    StructuralLayerData const & structuralLayer = mModel.GetStructuralLayer();
    ElectricalLayerData const & electricalLayer = mModel.GetElectricalLayer();
    std::optional<RopesLayerData> const & ropesLayer = mModel.HasLayer(LayerType::Ropes)
        ? mModel.GetRopesLayer()
        : std::optional<RopesLayerData>();

    assert(structuralLayer.Buffer.Size == electricalLayer.Buffer.Size);
    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            auto const coords = ShipSpaceCoordinates(x, y);
            auto const electricalMaterial = electricalLayer.Buffer[coords].Material;
            if (electricalMaterial != nullptr)
            {
                if (structuralLayer.Buffer[coords].Material == nullptr
                    && (!ropesLayer.has_value() || !ropesLayer->Buffer.HasEndpointAt(coords)))
                {
                    ++mElectricalParticlesWithNoStructuralSubstratumCount;
                }

                if (electricalMaterial->Luminiscence != 0.0f)
                {
                    ++mLightEmittingParticlesCount;
                }

                if (electricalMaterial->IsInstanced)
                {
                    assert(electricalLayer.Buffer[coords].InstanceIndex != NoneElectricalElementInstanceIndex);
                    if (auto const searchIt = electricalLayer.Panel.Find(electricalLayer.Buffer[coords].InstanceIndex);
                        searchIt == electricalLayer.Panel.end() || !searchIt->second.IsHidden)
                    {
                        ++mVisibleElectricalPanelElementsCount;
                    }
                }
            }
        }
    }
}

void ModelValidationSession::CheckElectricalSubstratum()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::MissingElectricalSubstratum,
        (mElectricalParticlesWithNoStructuralSubstratumCount > 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);
}

void ModelValidationSession::CheckTooManyLights()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    size_t constexpr MaxLightEmittingParticles = 5000;

    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::TooManyLights,
        (mLightEmittingParticlesCount > MaxLightEmittingParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
}

void ModelValidationSession::CheckTooManyElectricalPanelElements()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    size_t constexpr MaxVisibleElectricalPanelElements = 22;

    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::TooManyVisibleElectricalPanelElements,
        (mVisibleElectricalPanelElementsCount > MaxVisibleElectricalPanelElements) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
}


void ModelValidationSession::ValidateElectricalConnectivity()
{
    assert(mModel.HasLayer(LayerType::Electrical));

    ElectricalLayerData const & electricalLayer = mModel.GetElectricalLayer();

    //
    // Pass 1: create collections of categories, and connectivity visit buffers
    //

    Buffer2D<CVElement, ShipSpaceTag> electricalConnectivityVisitBuffer(electricalLayer.Buffer.Size);
    Buffer2D<CVElement, ShipSpaceTag> engineConnectivityVisitBuffer(electricalLayer.Buffer.Size);

    std::vector<ShipSpaceCoordinates> electricalSources;
    std::vector<ShipSpaceCoordinates> electricalComponents; // Anything that needs to be connected to a source; includes all consumers
    std::vector<ShipSpaceCoordinates> electricalConsumers;
    std::vector<ShipSpaceCoordinates> engineSources;
    std::vector<ShipSpaceCoordinates> engineComponents; // Anything that needs to be connected to a source; incldues all consumers
    std::vector<ShipSpaceCoordinates> engineConsumers;

    bool hasElectricals = false;
    bool hasEngines = false;

    for (int y = 0; y < electricalLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < electricalLayer.Buffer.Size.width; ++x)
        {
            ShipSpaceCoordinates coords{ x, y };

            // Initialize flags
            electricalConnectivityVisitBuffer[coords].wholeElement = 0;
            engineConnectivityVisitBuffer[coords].wholeElement = 0;

            // Inspect material
            auto const electricalMaterial = electricalLayer.Buffer[coords].Material;
            if (electricalMaterial)
            {
                switch (electricalMaterial->ElectricalType)
                {
                    case ElectricalMaterial::ElectricalElementType::Cable:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::Engine:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = true; // Engines may be electrically conductive when they're working
                        engineConnectivityVisitBuffer[coords].flags.isConductive = true;
                        engineComponents.push_back(coords);
                        engineConsumers.push_back(coords);

                        hasEngines = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::EngineController:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = true;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords); // Controllers need electricity
                        engineSources.push_back(coords);

                        hasElectricals = true;
                        hasEngines = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::EngineTransmission:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = true;
                        engineComponents.push_back(coords);

                        hasEngines = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::Generator:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalSources.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::InteractiveSwitch:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = true;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::Lamp:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;

                        if (!electricalMaterial->IsSelfPowered)
                        {
                            electricalComponents.push_back(coords);
                            electricalConsumers.push_back(coords);
                        }

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::OtherSink:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::PowerMonitor:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::ShipSound:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = true; // Acts as a switch
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::SmokeEmitter:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::TimerSwitch:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = true; // Acts as a switch
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::WaterPump:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::WaterSensingSwitch:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = true; // Acts as a switch
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);

                        hasElectricals = true;
                        break;
                    }

                    case ElectricalMaterial::ElectricalElementType::WatertightDoor:
                    {
                        electricalConnectivityVisitBuffer[coords].flags.isConductive = electricalMaterial->ConductsElectricity;
                        engineConnectivityVisitBuffer[coords].flags.isConductive = false;
                        electricalComponents.push_back(coords);
                        electricalConsumers.push_back(coords);

                        hasElectricals = true;
                        break;
                    }
                }
            }
        }
    }

    //
    // Pass 2: do checks
    //

    if (hasElectricals)
    {
        // Electrical components not connected to sources
        {
            size_t unpoweredElectricalComponentCount = CountElectricallyUnconnected(
                electricalSources,
                electricalComponents,
                electricalConnectivityVisitBuffer);

            mResults.AddIssue(
                ModelValidationIssue::CheckClassType::UnpoweredElectricalComponent,
                (unpoweredElectricalComponentCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }

        // Electrical sources not connected to any consumers
        {
            size_t unconsumedElectricalSourceCount = CountElectricallyUnconnected(
                electricalConsumers,
                electricalSources,
                electricalConnectivityVisitBuffer);

            mResults.AddIssue(
                ModelValidationIssue::CheckClassType::UnconsumedElectricalSource,
                (unconsumedElectricalSourceCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }
    }

    if (hasEngines)
    {
        // Engine components not connected to sources
        {
            size_t unpoweredEngineComponentCount = CountElectricallyUnconnected(
                engineSources,
                engineComponents,
                engineConnectivityVisitBuffer);

            mResults.AddIssue(
                ModelValidationIssue::CheckClassType::UnpoweredEngineComponent,
                (unpoweredEngineComponentCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }

        // Engine sources not connected to any consumers
        {
            size_t unconsumedEngineSourceCount = CountElectricallyUnconnected(
                engineConsumers,
                engineSources,
                engineConnectivityVisitBuffer);

            mResults.AddIssue(
                ModelValidationIssue::CheckClassType::UnconsumedEngineSource,
                (unconsumedEngineSourceCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }
    }
}

///////////////////////////////////////////

size_t ModelValidationSession::CountElectricallyUnconnected(
    std::vector<ShipSpaceCoordinates> const & propagationSources,
    std::vector<ShipSpaceCoordinates> const & propagationTargets,
    Buffer2D<CVElement, ShipSpaceTag> & connectivityVisitBuffer)
{
    // Clear visit
    for (int y = 0; y < connectivityVisitBuffer.Size.height; ++y)
    {
        for (int x = 0; x < connectivityVisitBuffer.Size.width; ++x)
        {
            connectivityVisitBuffer[{x, y}].flags.isVisited = false;
        }
    }

    // Do visit
    std::queue<ShipSpaceCoordinates> coordsToVisit;
    for (auto const & sourceCoords : propagationSources)
    {
        // Make sure we haven't visited it already
        if (!connectivityVisitBuffer[sourceCoords].flags.isVisited)
        {
            //
            // Flood graph
            //

            // Mark starting point as visited
            connectivityVisitBuffer[sourceCoords].flags.isVisited = true;

            // Add source to queue
            assert(coordsToVisit.empty());
            coordsToVisit.push(sourceCoords);

            // Visit all elements reachable from this source
            while (!coordsToVisit.empty())
            {
                auto const coords = coordsToVisit.front();
                coordsToVisit.pop();

                // Already marked as visited
                assert(connectivityVisitBuffer[coords].flags.isVisited == true);

                // Visit neighbors
                for (int yn = coords.y - 1; yn <= coords.y + 1; ++yn)
                {
                    for (int xn = coords.x - 1; xn <= coords.x + 1; ++xn)
                    {
                        ShipSpaceCoordinates const neighborCoordinates{ xn, yn };
                        if (neighborCoordinates.IsInSize(connectivityVisitBuffer.Size)
                            && connectivityVisitBuffer[neighborCoordinates].flags.isConductive
                            && !connectivityVisitBuffer[neighborCoordinates].flags.isVisited)
                        {
                            // Mark it as visited
                            connectivityVisitBuffer[neighborCoordinates].flags.isVisited = true;

                            // Add to queue
                            coordsToVisit.push(neighborCoordinates);
                        }
                    }
                }
            }
        }
    }

    // Count unconnected
    size_t unconnectedComponentsCount = 0;
    for (auto const & targetCoords : propagationTargets)
    {
        if (!connectivityVisitBuffer[targetCoords].flags.isVisited)
        {
            ++unconnectedComponentsCount;
        }
    }

    return unconnectedComponentsCount;
}

void ModelValidationSession::CheckExteriorLayerTextureSize()
{
    assert(mModel.HasLayer(LayerType::ExteriorTexture));

    auto const & exteriorLayer = mModel.GetExteriorTextureLayer();
    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::ExteriorLayerTextureTooLarge,
        (exteriorLayer.Buffer.Size.width > WorkbenchState::GetMaxTextureDimension() || exteriorLayer.Buffer.Size.height > WorkbenchState::GetMaxTextureDimension()) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
}

void ModelValidationSession::CheckInteriorLayerTextureSize()
{
    assert(mModel.HasLayer(LayerType::InteriorTexture));

    auto const & interiorLayer = mModel.GetInteriorTextureLayer();
    mResults.AddIssue(
        ModelValidationIssue::CheckClassType::InteriorLayerTextureTooLarge,
        (interiorLayer.Buffer.Size.width > WorkbenchState::GetMaxTextureDimension() || interiorLayer.Buffer.Size.height > WorkbenchState::GetMaxTextureDimension()) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
}

}