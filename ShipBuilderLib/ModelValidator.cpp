/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-07-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ModelValidator.h"

#include <queue>

namespace ShipBuilder {

ModelValidationResults ModelValidator::ValidateModel(Model const & model)
{
    std::vector<ModelValidationIssue> issues;

    //
    // Visit structural layer
    //

    assert(model.HasLayer(LayerType::Structural));

    StructuralLayerData const & structuralLayer = model.GetStructuralLayer();

    size_t structuralParticlesCount = 0;

    for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
    {
        for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
        {
            if (structuralLayer.Buffer[{x, y}].Material != nullptr)
            {
                ++structuralParticlesCount;
            }
        }
    }

    //
    // Check: empty structural layer
    //

    issues.emplace_back(
        ModelValidationIssue::CheckClassType::EmptyStructuralLayer,
        (structuralParticlesCount == 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);

    if (structuralParticlesCount != 0)
    {
        //
        // Check: structure too large
        //

        size_t constexpr MaxStructuralParticles = 100000;

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::StructureTooLarge,
            (structuralParticlesCount > MaxStructuralParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
    }

    if (model.HasLayer(LayerType::Electrical))
    {
        //
        // Visit electrical layer
        //

        ElectricalLayerData const & electricalLayer = model.GetElectricalLayer();

        size_t electricalParticlesWithNoStructuralSubstratumCount = 0;
        size_t lightEmittingParticlesCount = 0;

        assert(structuralLayer.Buffer.Size == electricalLayer.Buffer.Size);
        for (int y = 0; y < structuralLayer.Buffer.Size.height; ++y)
        {
            for (int x = 0; x < structuralLayer.Buffer.Size.width; ++x)
            {
                auto const coords = ShipSpaceCoordinates(x, y);
                auto const electricalMaterial = electricalLayer.Buffer[coords].Material;
                if (electricalMaterial != nullptr
                    && structuralLayer.Buffer[coords].Material == nullptr)
                {
                    ++electricalParticlesWithNoStructuralSubstratumCount;

                    if (electricalMaterial->Luminiscence != 0.0f)
                    {
                        ++lightEmittingParticlesCount;
                    }
                }
            }
        }

        //
        // Check: connectivity
        //

        ValidateElectricalConnectivity(electricalLayer, issues);

        //
        // Check: electrical substratum
        //

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::MissingElectricalSubstratum,
            (electricalParticlesWithNoStructuralSubstratumCount > 0) ? ModelValidationIssue::SeverityType::Error : ModelValidationIssue::SeverityType::Success);

        //
        // Check: too many lights
        //

        size_t constexpr MaxLightEmittingParticles = 5000;

        issues.emplace_back(
            ModelValidationIssue::CheckClassType::TooManyLights,
            (lightEmittingParticlesCount > MaxLightEmittingParticles) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
    }

    return ModelValidationResults(std::move(issues));
}

void ModelValidator::ValidateElectricalConnectivity(
    ElectricalLayerData const & electricalLayer,
    std::vector<ModelValidationIssue> & issues)
{
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

            issues.emplace_back(
                ModelValidationIssue::CheckClassType::UnpoweredElectricalComponent,
                (unpoweredElectricalComponentCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }

        // Electrical sources not connected to any consumers 
        {
            size_t unconsumedElectricalSourceCount = CountElectricallyUnconnected(
                electricalConsumers,
                electricalSources,
                electricalConnectivityVisitBuffer);

            issues.emplace_back(
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

            issues.emplace_back(
                ModelValidationIssue::CheckClassType::UnpoweredEngineComponent,
                (unpoweredEngineComponentCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }

        // Engine sources not connected to any consumers 
        {
            size_t unconsumedEngineSourceCount = CountElectricallyUnconnected(
                engineConsumers,
                engineSources,
                engineConnectivityVisitBuffer);

            issues.emplace_back(
                ModelValidationIssue::CheckClassType::UnconsumedEngineSource,
                (unconsumedEngineSourceCount > 0) ? ModelValidationIssue::SeverityType::Warning : ModelValidationIssue::SeverityType::Success);
        }
    }
}

size_t ModelValidator::CountElectricallyUnconnected(
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

}