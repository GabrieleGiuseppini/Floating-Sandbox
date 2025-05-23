﻿/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipFactory.h"

#include "Physics/Formulae.h"
#include "ShipFloorplanizer.h"

#include <Core/GameChronometer.h>
#include <Core/GameDebug.h>
#include <Core/GameMath.h>
#include <Core/ImageTools.h>
#include <Core/Log.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <optional>
#include <sstream>
#include <utility>

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData, RgbaImageData> ShipFactory::Create(
    ShipId shipId,
    World & parentWorld,
    ShipDefinition && shipDefinition,
    ShipLoadOptions const & shipLoadOptions,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    ShipStrengthRandomizer const & shipStrengthRandomizer,
    SimulationEventDispatcher & simulationEventDispatcher,
    IAssetManager const & assetManager,
    SimulationParameters const & simulationParameters)
{
    auto const totalStartTime = GameChronometer::Now();

    //
    // Process load options
    //

    auto shipSize = shipDefinition.Layers.Size;

    if (shipLoadOptions.FlipHorizontally)
    {
        shipDefinition.Layers.Flip(DirectionType::Horizontal);
    }

    if (shipLoadOptions.FlipVertically)
    {
        shipDefinition.Layers.Flip(DirectionType::Vertical);
    }

    if (shipLoadOptions.Rotate90CW)
    {
        shipSize.Rotate90();
        shipDefinition.Layers.Rotate90(RotationDirectionType::Clockwise);
    }

    //
    // Process structural ship layer and:
    // - Create ShipFactoryPoint's for each particle, including ropes' endpoints
    // - Build a 2D matrix containing indices to the particles
    //

    assert(shipDefinition.Layers.StructuralLayer);
    auto const & structuralLayerBuffer = shipDefinition.Layers.StructuralLayer->Buffer;

    float const halfShipWidth = static_cast<float>(shipSize.width) / 2.0f;
    float const shipSpaceToWorldSpaceFactor = shipDefinition.Metadata.Scale.outputUnits / shipDefinition.Metadata.Scale.inputUnits;

    // ShipFactoryPoint's
    std::vector<ShipFactoryPoint> pointInfos1;

    // Matrix of points - we allocate 2 extra dummy rows and cols - around - to avoid checking for boundaries
    ShipFactoryPointIndexMatrix pointIndexMatrix(shipSize.width + 2, shipSize.height + 2);

    // Region of actual content
    int minX = shipSize.width;
    int maxX = 0;
    int minY = shipSize.height;
    int maxY = 0;

    // Visit all columns
    for (int x = 0; x < shipSize.width; ++x)
    {
        // From bottom to top
        for (int y = 0; y < shipSize.height; ++y)
        {
            ShipSpaceCoordinates const coords = ShipSpaceCoordinates(x, y);

            // Get structural material properties

            StructuralMaterial const * structuralMaterial = structuralLayerBuffer[coords].Material;

            rgbaColor structuralMaterialRenderColor = (structuralMaterial != nullptr)
                ? structuralMaterial->RenderColor
                : rgbaColor::zero();

            bool isStructuralMaterialRope = (structuralMaterial != nullptr)
                ? structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope)
                : false;

            bool isStructuralMaterialLeaking = (structuralMaterial != nullptr)
                ? structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope)
                : false;

            // Check if there's a rope endpoint here
            if (shipDefinition.Layers.RopesLayer)
            {
                auto const ropeSearchIt = std::find_if(
                    shipDefinition.Layers.RopesLayer->Buffer.cbegin(),
                    shipDefinition.Layers.RopesLayer->Buffer.cend(),
                    [&coords](RopeElement const & e)
                    {
                        return e.StartCoords == coords || e.EndCoords == coords;
                    });

                if (ropeSearchIt != shipDefinition.Layers.RopesLayer->Buffer.cend())
                {
                    //
                    // There is a rope endpoint here
                    //

                    if (structuralMaterial == nullptr)
                    {
                        // Make a structural element for this endpoint
                        structuralMaterial = ropeSearchIt->Material;
                        assert(structuralMaterial != nullptr);
                        isStructuralMaterialLeaking = true; // Ropes leak by default
                    }

                    // Change endpoint's color to match the rope's - or else the spring will look bad
                    structuralMaterialRenderColor = ropeSearchIt->RenderColor;

                    // Make it a rope point so that the first spring segment is a rope spring
                    isStructuralMaterialRope = true;
                }
            }

            // Check if there's a structural element here
            if (nullptr != structuralMaterial)
            {
                //
                // Transform water point to air point + water
                //

                float water = 0.0f;
                if (structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Water))
                {
                    structuralMaterial = &(materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Air));
                    water = 1.0f;
                }

                //
                // Make a point
                //

                ElementIndex const pointIndex = static_cast<ElementIndex>(pointInfos1.size());

                pointIndexMatrix[{x + 1, y + 1}] = static_cast<ElementIndex>(pointIndex);

                vec2f const worldCoords =
                    vec2f(
                        static_cast<float>(x) - halfShipWidth,
                        static_cast<float>(y)) * shipSpaceToWorldSpaceFactor
                    + shipDefinition.PhysicsData.Offset;

                pointInfos1.emplace_back(
                    coords,
                    worldCoords,
                    MakeTextureCoordinates(x, y, shipSize),
                    structuralMaterialRenderColor,
                    *structuralMaterial,
                    isStructuralMaterialRope,
                    isStructuralMaterialLeaking,
                    structuralMaterial->Strength,
                    water);

                // Eventually decorate with electrical layer information
                if (shipDefinition.Layers.ElectricalLayer && shipDefinition.Layers.ElectricalLayer->Buffer[coords].Material != nullptr)
                {
                    pointInfos1.back().ElectricalMtl = shipDefinition.Layers.ElectricalLayer->Buffer[coords].Material;
                    pointInfos1.back().ElectricalElementInstanceIdx = shipDefinition.Layers.ElectricalLayer->Buffer[coords].InstanceIndex;
                }

                //
                // Update min/max coords
                //

                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                minY = std::min(minY, y);
                maxY = std::max(maxY, y);
            }
            else
            {
                // Just ignore this pixel
            }
        }
    }

    //
    // Process the rope endpoints and:
    // - Fill-in points between the endpoints, creating additional ShipFactoryPoint's for them
    // - Fill-in springs between each pair of points in the rope, creating ShipFactorySpring's for them
    //      - And populating the point pair -> spring index 1 map
    //

    std::vector<ShipFactorySpring> springInfos1;

    ShipFactoryPointPairToIndexMap pointPairToSpringIndex1Map;

    if (shipDefinition.Layers.RopesLayer)
    {
        AppendRopes(
            shipDefinition.Layers.RopesLayer->Buffer,
            shipSize,
            pointIndexMatrix,
            pointInfos1,
            springInfos1,
            pointPairToSpringIndex1Map);
    }

    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded ShipFactoryPoint's as "leaking"
    //  - Detect springs and create ShipFactorySpring's for them (additional to ropes)
    //      - And populate the point pair -> spring index 1 map
    //  - Do tessellation and create ShipFactoryTriangle's
    //

    std::vector<ShipFactoryTriangle> triangleInfos;

    size_t leakingPointsCount;

    CreateShipElementInfos(
        pointIndexMatrix,
        pointInfos1,
        springInfos1,
        pointPairToSpringIndex1Map,
        triangleInfos,
        leakingPointsCount);

    //
    // Filter out redundant triangles
    //

    triangleInfos = FilterOutRedundantTriangles(
        triangleInfos,
        pointInfos1,
        springInfos1);

    //
    // Connect points to triangles
    //

    ConnectPointsToTriangles(
        pointInfos1,
        triangleInfos);

    //
    // Optimize order of ShipFactoryPoint's and ShipFactorySpring's for our spring
    // relaxation algorithm - and hopefully to improve cache hits
    //

    auto [pointInfos2, pointIndexRemap, springInfos2, springIndexRemap, perfectSquareCount] = OptimizeLayout(
        pointIndexMatrix,
        pointInfos1,
        springInfos1);

    // Note: we don't optimize triangles, as tests indicate that performance gets (marginally) worse,
    // and at the same time, it makes sense to use the natural order of the triangles as it ensures
    // that higher elements in the ship cover lower elements when they are semi-detached.

    //
    // Associate all springs with the triangles that run through them (supertriangles)
    //

    ConnectSpringsAndTriangles(
        springInfos2,
        triangleInfos,
        pointIndexRemap);

    //
    // Create frontiers
    //

    std::vector<ShipFactoryFrontier> shipFactoryFrontiers = CreateShipFrontiers(
        pointIndexMatrix,
        pointIndexRemap,
        pointInfos2,
        springInfos2,
        pointPairToSpringIndex1Map,
        springIndexRemap);

    //
    // Randomize strength
    //

    shipStrengthRandomizer.RandomizeStrength(
        pointIndexMatrix,
        vec2i(minX, minY) + vec2i(1, 1), // Image -> PointIndexMatrix
        vec2i(maxX - minX + 1, maxY - minY + 1),
        pointInfos2,
        pointIndexRemap,
        springInfos2,
        triangleInfos,
        shipFactoryFrontiers);

    //
    // Create floorplan
    //

    ShipFloorplanizer shipFloorplanizer;

    ShipFactoryFloorPlan floorPlan2 = shipFloorplanizer.BuildFloorplan(
        pointIndexMatrix,
        pointInfos2,
        pointIndexRemap,
        springInfos2);

    //
    // Visit all ShipFactoryPoint's and create Points, i.e. the entire set of points
    //

    auto [points, allElectricalElementInstanceIndices] = CreatePoints(
        pointInfos2,
        parentWorld,
        materialDatabase,
        simulationEventDispatcher,
        simulationParameters,
        shipDefinition.PhysicsData);

    //
    // Create Springs for all ShipFactorySpring's
    //

    Springs springs = CreateSprings(
        springInfos2,
        perfectSquareCount,
        points,
        parentWorld,
        simulationEventDispatcher,
        simulationParameters);

    //
    // Create Triangles for all ShipFactoryTriangle's
    //

    Triangles triangles = CreateTriangles(
        triangleInfos,
        points,
        pointIndexRemap,
        springInfos2,
        floorPlan2);

    //
    // Create Electrical Elements
    //

    ElectricalElements electricalElements = CreateElectricalElements(
        points,
        pointInfos2,
        allElectricalElementInstanceIndices,
        shipDefinition.Layers.ElectricalLayer
            ? shipDefinition.Layers.ElectricalLayer->Panel
            : ElectricalPanel(),
        shipLoadOptions.FlipHorizontally,
        shipLoadOptions.FlipVertically,
        shipLoadOptions.Rotate90CW,
        shipId,
        parentWorld,
        simulationEventDispatcher,
        simulationParameters);

    //
    // Create frontiers
    //

    Frontiers frontiers = CreateFrontiers(
        shipFactoryFrontiers,
        points,
        springs);

    //
    // Create exterior texture
    //

    RgbaImageData exteriorTextureImage = shipDefinition.Layers.ExteriorTextureLayer
        ? std::move(shipDefinition.Layers.ExteriorTextureLayer->Buffer) // Use provided texture
        : shipTexturizer.MakeAutoTexture(
            *shipDefinition.Layers.StructuralLayer,
            shipDefinition.AutoTexturizationSettings, // Auto-texturize
            ShipTexturizer::MaxHighDefinitionTextureSize,
            assetManager);

    //
    // Create interior texture
    //

    RgbaImageData interiorTextureImage = shipDefinition.Layers.InteriorTextureLayer
        ? std::move(shipDefinition.Layers.InteriorTextureLayer->Buffer) // Use provided texture
        : shipTexturizer.MakeAutoTexture(
            *shipDefinition.Layers.StructuralLayer,
            ShipAutoTexturizationSettings( // Custom
                ShipAutoTexturizationModeType::MaterialTextures,
                0.15f,
                0.65f),
            ShipTexturizer::MaxHighDefinitionTextureSize,
            assetManager);

    // Whiteout
    ImageTools::BlendWithColor(
        interiorTextureImage,
        rgbColor(rgbColor::data_type_max, rgbColor::data_type_max, rgbColor::data_type_max),
        0.5f);

    //
    // Create interior view
    //

    RgbaImageData interiorViewImage = shipTexturizer.MakeInteriorViewTexture(
        triangles,
        points,
        shipSize,
        interiorTextureImage);

    //
    // We're done!
    //

#ifdef _DEBUG
    VerifyShipInvariants(
        points,
        springs,
        triangles);
#endif

    LogMessage("ShipFactory: Created ship: W=", shipSize.width, ", H=", shipSize.height, ", ",
        points.GetRawShipPointCount(), "raw/", points.GetBufferElementCount(), "buf points, ",
        springs.GetElementCount(), " springs (", perfectSquareCount, " perfect squares, ", perfectSquareCount * 4 * 100 / std::max(1u, springs.GetElementCount()), "%), ",
        triangles.GetElementCount(), " triangles, ",
        electricalElements.GetElementCount(), " electrical elements (", electricalElements.GetLampCount(), " lamps), ",
        frontiers.GetElementCount(), " frontiers.");

    auto ship = std::make_unique<Ship>(
        shipId,
        parentWorld,
        materialDatabase,
        simulationEventDispatcher,
        std::move(points),
        std::move(springs),
        std::move(triangles),
        std::move(electricalElements),
        std::move(frontiers),
        std::move(interiorTextureImage));

    LogMessage("ShipFactory: Create() took ",
        std::chrono::duration_cast<std::chrono::microseconds>(GameChronometer::Now() - totalStartTime).count(), "us");

    return std::make_tuple(
        std::move(ship),
        std::move(exteriorTextureImage),
        std::move(interiorViewImage));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Building helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

void ShipFactory::AppendRopes(
    RopeBuffer const & ropeBuffer,
    ShipSpaceSize const & shipSize,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::vector<ShipFactoryPoint> & pointInfos1,
    std::vector<ShipFactorySpring> & springInfos1,
    ShipFactoryPointPairToIndexMap & pointPairToSpringIndex1Map)
{
    //
    // - Fill-in points between each pair of endpoints, creating additional ShipFactoryPoint's for them
    // - Fill-in springs between each pair of points in the rope, creating ShipFactorySpring's for them
    //

    // Visit all RopeElement's
    for (auto const & ropeElement : ropeBuffer)
    {
        assert(pointIndexMatrix[vec2i(ropeElement.StartCoords.x + 1, ropeElement.StartCoords.y + 1)].has_value());
        ElementIndex const pointAIndex1 = *pointIndexMatrix[vec2i(ropeElement.StartCoords.x + 1, ropeElement.StartCoords.y + 1)];

        assert(pointIndexMatrix[vec2i(ropeElement.EndCoords.x + 1, ropeElement.EndCoords.y + 1)].has_value());
        ElementIndex const pointBIndex1 = *pointIndexMatrix[vec2i(ropeElement.EndCoords.x + 1, ropeElement.EndCoords.y + 1)];

        // No need to lay a rope if the points are adjacent - as there will be a rope anyway
        if (pointInfos1[pointAIndex1].DefinitionCoordinates.has_value()
            && pointInfos1[pointBIndex1].DefinitionCoordinates.has_value())
        {
            if (abs(pointInfos1[pointAIndex1].DefinitionCoordinates->x - pointInfos1[pointBIndex1].DefinitionCoordinates->x) <= 1
                && abs(pointInfos1[pointAIndex1].DefinitionCoordinates->y - pointInfos1[pointBIndex1].DefinitionCoordinates->y) <= 1)
            {
                // No need to lay a rope
                continue;
            }
        }

        // Get endpoint (world) positions
        vec2f const startPos = pointInfos1[pointAIndex1].Position;
        vec2f const endPos = pointInfos1[pointBIndex1].Position;

        // Get endpoint electrical materials - if any

        ElectricalMaterial const * startElectricalMaterial = nullptr;
        if (auto const pointAElectricalMaterial = pointInfos1[pointAIndex1].ElectricalMtl;
            nullptr != pointAElectricalMaterial
            && (pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Cable
                || pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Generator
                || pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Lamp)
            && !pointAElectricalMaterial->IsInstanced)
            startElectricalMaterial = pointAElectricalMaterial;

        ElectricalMaterial const * endElectricalMaterial = nullptr;
        if (auto const pointBElectricalMaterial = pointInfos1[pointBIndex1].ElectricalMtl;
            nullptr != pointBElectricalMaterial
            && (pointBElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Cable
                || pointBElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Generator
                || pointBElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Lamp)
            && !pointBElectricalMaterial->IsInstanced)
            endElectricalMaterial = pointBElectricalMaterial;

        //
        // "Draw" line from start position to end position
        //
        // Go along widest of Dx and Dy, in steps of 1.0, until we're very close to end position
        //

        // W = wide, N = narrow

        float const dx = endPos.x - startPos.x;
        float const dy = endPos.y - startPos.y;
        bool widestIsX;
        float slope;
        float startW, startN;
        float endW;
        float stepW; // +1.0/-1.0
        if (fabs(dx) > fabs(dy))
        {
            widestIsX = true;
            slope = dy / dx;
            startW = startPos.x;
            startN = startPos.y;
            endW = endPos.x;
            stepW = dx / fabs(dx);
        }
        else
        {
            widestIsX = false;
            slope = dx / dy;
            startW = startPos.y;
            startN = startPos.x;
            endW = endPos.y;
            stepW = dy / fabs(dy);
        }

        // Calculate spring directions
        int factoryDirectionStart, factoryDirectionEnd;
        if (dx > 0)
        {
            // West->East
            if (dy > 0)
            {
                // South->North
                factoryDirectionStart = 3; // SW
                factoryDirectionEnd = 7; // NE
            }
            else
            {
                // North->South
                factoryDirectionStart = 5; // NW
                factoryDirectionEnd = 1; // SE
            }
        }
        else
        {
            // East->West
            if (dy > 0)
            {
                // South->North
                factoryDirectionStart = 1; // SE
                factoryDirectionEnd = 5; // NW
            }
            else
            {
                // North-South
                factoryDirectionStart = 7; // NE
                factoryDirectionEnd = 3; // SW
            }
        }

        float curW = startW;
        float curN = startN;
        float const halfW = fabs(endW - curW) / 2.0f;

        auto curStartPointIndex1 = pointAIndex1;
        while (true)
        {
            curW += stepW;
            curN += slope * stepW;

            if (fabs(endW - curW) <= 0.5f)
            {
                // Reached destination
                break;
            }

            bool const isFirstHalf = fabs(curW - startW) <= halfW;

            // Create position
            vec2f newPosition;
            if (widestIsX)
            {
                newPosition = vec2f(curW, curN);
            }
            else
            {
                newPosition = vec2f(curN, curW);
            }

            auto const newPointIndex1 = static_cast<ElementIndex>(pointInfos1.size());

            // Add ShipFactorySpring
            ElementIndex const springIndex1 = static_cast<ElementIndex>(springInfos1.size());
            springInfos1.emplace_back(
                curStartPointIndex1,
                factoryDirectionEnd,
                newPointIndex1,
                factoryDirectionStart);

            // Add spring to point pair map
            auto const [_, isInserted] = pointPairToSpringIndex1Map.try_emplace({ curStartPointIndex1 , newPointIndex1 }, springIndex1);
            assert(isInserted);
            (void)isInserted;

            // Add ShipFactoryPoint
            assert(ropeElement.Material != nullptr);
            pointInfos1.emplace_back(
                std::nullopt,
                newPosition,
                MakeTextureCoordinates(newPosition.x, newPosition.y, shipSize),
                ropeElement.RenderColor,
                *ropeElement.Material,
                true, // IsRope
                true, // Ropes leak by default
                ropeElement.Material->Strength,
                0.0f); // Water

            // Set electrical material
            pointInfos1.back().ElectricalMtl = isFirstHalf
                ? startElectricalMaterial // First half
                : endElectricalMaterial; // Second half

            // Connect points to spring
            pointInfos1[curStartPointIndex1].AddConnectedSpring1(springIndex1);
            pointInfos1[newPointIndex1].AddConnectedSpring1(springIndex1);

            // Advance
            curStartPointIndex1 = newPointIndex1;
        }

        // Add last ShipFactorySpring (no ShipFactoryPoint as the endpoint has already a ShipFactoryPoint)
        ElementIndex const lastSpringIndex1 = static_cast<ElementIndex>(springInfos1.size());
        springInfos1.emplace_back(
            curStartPointIndex1,
            factoryDirectionEnd,
            pointBIndex1,
            factoryDirectionStart);

        // Add spring to point pair map
        auto const [_, isInserted] = pointPairToSpringIndex1Map.try_emplace(
            { curStartPointIndex1, pointBIndex1 },
            lastSpringIndex1);
        assert(isInserted);
        (void)isInserted;

        // Connect points to spring
        pointInfos1[curStartPointIndex1].AddConnectedSpring1(lastSpringIndex1);
        pointInfos1[pointBIndex1].AddConnectedSpring1(lastSpringIndex1);
    }
}

void ShipFactory::CreateShipElementInfos(
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::vector<ShipFactoryPoint> & pointInfos1,
    std::vector<ShipFactorySpring> & springInfos1,
    ShipFactoryPointPairToIndexMap & pointPairToSpringIndex1Map,
    std::vector<ShipFactoryTriangle> & triangleInfos1,
    size_t & leakingPointsCount)
{
    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded PointInfos as "leaking"
    //  - Detect springs and create ShipFactorySpring's for them (additional to ropes)
    //  - Do tessellation and create ShipFactoryTriangle's
    //

    // Initialize count of leaking points
    leakingPointsCount = 0;

    // From bottom to top - excluding extras at boundaries
    for (int y = 1; y < pointIndexMatrix.height - 1; ++y)
    {
        // We're starting a new row, so we're not in a ship now
        bool isRowInShip = false;

        // From left to right - excluding extras at boundaries
        for (int x = 1; x < pointIndexMatrix.width - 1; ++x)
        {
            if (!!pointIndexMatrix[{x, y}])
            {
                //
                // A point exists at these coordinates
                //

                ElementIndex pointIndex1 = *pointIndexMatrix[{x, y}];

                // If a non-hull node has empty space on one of its four sides, it is leaking.
                // Check if a is leaking; a is leaking if:
                // - a is not hull, AND
                // - there is at least a hole at E, S, W, N
                if (!pointInfos1[pointIndex1].StructuralMtl.IsHull)
                {
                    if (!pointIndexMatrix[{x + 1, y}]
                        || !pointIndexMatrix[{x, y + 1}]
                        || !pointIndexMatrix[{x - 1, y}]
                        || !pointIndexMatrix[{x, y - 1}])
                    {
                        pointInfos1[pointIndex1].IsLeaking = true;
                        ++leakingPointsCount;
                    }
                }

                //
                // Springs
                //

                // First four directions out of 8: from 0 deg (+x) through to 225 deg (-x -y),
                // i.e. E, SE, S, SW - this covers each pair of points in each direction
                for (int i = 0; i < 4; ++i)
                {
                    int adjx1 = x + TessellationCircularOrderDirections[i][0];
                    int adjy1 = y + TessellationCircularOrderDirections[i][1];

                    if (!!pointIndexMatrix[{adjx1, adjy1}])
                    {
                        // This point is adjacent to the first point at one of E, SE, S, SW

                        //
                        // Create ShipFactorySpring
                        //

                        ElementIndex const otherEndpointIndex1 = *pointIndexMatrix[{adjx1, adjy1}];

                        // Add spring to spring infos
                        ElementIndex const springIndex1 = static_cast<ElementIndex>(springInfos1.size());
                        springInfos1.emplace_back(
                            pointIndex1,
                            i,
                            otherEndpointIndex1,
                            (i + 4) % 8);

                        // Add spring to point pair map
                        auto [_, isInserted] = pointPairToSpringIndex1Map.try_emplace(
                            { pointIndex1, otherEndpointIndex1 },
                            springIndex1);
                        assert(isInserted);
                        (void)isInserted;

                        // Add the spring to its endpoints
                        pointInfos1[pointIndex1].AddConnectedSpring1(springIndex1);
                        pointInfos1[otherEndpointIndex1].AddConnectedSpring1(springIndex1);
                    }
                }

                //
                // Triangles
                //

                //              P
                //  W (4) o --- * --- o  E (0)
                //            / | \
                //           /  |  \
                //          /   |   \
                //  SW (3) o    o    o SE (1)
                //             S (2)
                //

                // - If this is the first point in the row that is in a ship, we check from E CW all the way up to SW;
                // - Else, we check only up to S, so to avoid covering areas already covered by the triangulation
                //   at the previous point
                //

                //
                // Quad: P - E - SE - S
                //

                auto const pointECoordinates = vec2i(x + TessellationCircularOrderDirections[0][0], y + TessellationCircularOrderDirections[0][1]);
                auto const & pointE = pointIndexMatrix[pointECoordinates];
                auto const pointSECoordinates = vec2i(x + TessellationCircularOrderDirections[1][0], y + TessellationCircularOrderDirections[1][1]);
                auto const & pointSE = pointIndexMatrix[pointSECoordinates];
                auto const pointSCoordinates = vec2i(x + TessellationCircularOrderDirections[2][0], y + TessellationCircularOrderDirections[2][1]);
                auto const & pointS = pointIndexMatrix[pointSCoordinates];

                if (pointE.has_value())
                {
                    if (pointSE.has_value())
                    {
                        if (pointS.has_value())
                        {
                            //
                            // We can choose if two triangles along P-SE diagonal, or two triangles along S-E diagonal;
                            // we prioritize the one that is hull, so we honor hull edges for NPC floors (since floors
                            // may only exist on hull springs)
                            //

                            bool const isP_SE_hull = pointInfos1[pointIndex1].StructuralMtl.IsHull && pointInfos1[*pointSE].StructuralMtl.IsHull;
                            bool const isS_E_hull = pointInfos1[*pointS].StructuralMtl.IsHull && pointInfos1[*pointE].StructuralMtl.IsHull;

                            if (isS_E_hull)
                            {
                                if (isP_SE_hull)
                                {
                                    // Both are hull - the one with the most "continuations" wins

                                    // S-E
                                    int seCount = 0;
                                    // S.SW
                                    auto contCoord = vec2i(pointSCoordinates.x + TessellationCircularOrderDirections[3][0], pointSCoordinates.y + TessellationCircularOrderDirections[3][1]);
                                    if (pointIndexMatrix[contCoord].has_value()
                                        && pointInfos1[*pointIndexMatrix[contCoord]].StructuralMtl.IsHull)
                                        ++seCount;
                                    // E.NE
                                    contCoord = vec2i(pointECoordinates.x + TessellationCircularOrderDirections[7][0], pointECoordinates.y + TessellationCircularOrderDirections[7][1]);
                                    if (pointIndexMatrix[contCoord].has_value()
                                        && pointInfos1[*pointIndexMatrix[contCoord]].StructuralMtl.IsHull)
                                        ++seCount;

                                    // P-SE
                                    int pseCount = 0;
                                    // P.NW
                                    contCoord = vec2i(x + TessellationCircularOrderDirections[5][0], y + TessellationCircularOrderDirections[5][1]);
                                    if (pointIndexMatrix[contCoord].has_value()
                                        && pointInfos1[*pointIndexMatrix[contCoord]].StructuralMtl.IsHull)
                                        ++pseCount;
                                    // SE.SE
                                    contCoord = vec2i(pointSECoordinates.x + TessellationCircularOrderDirections[1][0], pointSECoordinates.y + TessellationCircularOrderDirections[1][1]);
                                    if (pointIndexMatrix[contCoord].has_value()
                                        && pointInfos1[*pointIndexMatrix[contCoord]].StructuralMtl.IsHull)
                                        ++pseCount;

                                    if (pseCount >= seCount)
                                    {
                                        // P - E - SE

                                        //
                                        // Create ShipFactoryTriangle
                                        //

                                        triangleInfos1.emplace_back(
                                            std::array<ElementIndex, 3>( // Points are in CW order
                                                {
                                                    pointIndex1,
                                                    *pointE,
                                                    *pointSE
                                                }));

                                        // P - SE - S

                                        //
                                        // Create ShipFactoryTriangle
                                        //

                                        triangleInfos1.emplace_back(
                                            std::array<ElementIndex, 3>( // Points are in CW order
                                                {
                                                    pointIndex1,
                                                    *pointSE,
                                                    *pointS
                                                }));

                                    }
                                    else
                                    {
                                        // P - E - S

                                        //
                                        // Create ShipFactoryTriangle
                                        //

                                        triangleInfos1.emplace_back(
                                            std::array<ElementIndex, 3>( // Points are in CW order
                                                {
                                                    pointIndex1,
                                                    *pointE,
                                                    *pointS
                                                }));

                                        // S - E - SE

                                        //
                                        // Create ShipFactoryTriangle
                                        //

                                        triangleInfos1.emplace_back(
                                            std::array<ElementIndex, 3>( // Points are in CW order
                                                {
                                                    *pointS,
                                                    *pointE,
                                                    *pointSE
                                                }));
                                    }
                                }
                                else
                                {
                                    // Only S-E is hull

                                    // P - E - S

                                    //
                                    // Create ShipFactoryTriangle
                                    //

                                    triangleInfos1.emplace_back(
                                        std::array<ElementIndex, 3>( // Points are in CW order
                                            {
                                                pointIndex1,
                                                *pointE,
                                                *pointS
                                            }));

                                    // S - E - SE

                                    //
                                    // Create ShipFactoryTriangle
                                    //

                                    triangleInfos1.emplace_back(
                                        std::array<ElementIndex, 3>( // Points are in CW order
                                            {
                                                *pointS,
                                                *pointE,
                                                *pointSE
                                            }));
                                }
                            }
                            else
                            {
                                // Only P-SE is hull or neither is hull; in the last case P-SE wins arbitrarily

                                // P - E - SE

                                //
                                // Create ShipFactoryTriangle
                                //

                                triangleInfos1.emplace_back(
                                    std::array<ElementIndex, 3>( // Points are in CW order
                                        {
                                            pointIndex1,
                                            *pointE,
                                            *pointSE
                                        }));

                                // P - SE - S

                                //
                                // Create ShipFactoryTriangle
                                //

                                triangleInfos1.emplace_back(
                                    std::array<ElementIndex, 3>( // Points are in CW order
                                        {
                                            pointIndex1,
                                            *pointSE,
                                            *pointS
                                        }));
                            }
                        }
                        else
                        {
                            // P - E - SE

                            //
                            // Create ShipFactoryTriangle
                            //

                            triangleInfos1.emplace_back(
                                std::array<ElementIndex, 3>( // Points are in CW order
                                    {
                                        pointIndex1,
                                        *pointE,
                                        *pointSE
                                    }));
                        }
                    }
                    else if (pointS.has_value())
                    {
                        // P - E - S

                        //
                        // Create ShipFactoryTriangle
                        //

                        triangleInfos1.emplace_back(
                            std::array<ElementIndex, 3>( // Points are in CW order
                                {
                                    pointIndex1,
                                    *pointE,
                                    *pointS
                                }));
                    }
                }
                else if (pointSE.has_value() && pointS.has_value())
                {
                    // P - SE - S

                    //
                    // Create ShipFactoryTriangle
                    //

                    triangleInfos1.emplace_back(
                        std::array<ElementIndex, 3>( // Points are in CW order
                            {
                                pointIndex1,
                                *pointSE,
                                *pointS
                            }));
                }

                //
                // Triangle: P - S - SW
                //

                if (!isRowInShip)
                {
                    auto const & pointSW = pointIndexMatrix[{x + TessellationCircularOrderDirections[3][0], y + TessellationCircularOrderDirections[3][1]}];

                    if (pointS.has_value() && pointSW.has_value())
                    {
                        //
                        // Create ShipFactoryTriangle
                        //

                        triangleInfos1.emplace_back(
                            std::array<ElementIndex, 3>( // Points are in CW order
                                {
                                    pointIndex1,
                                    *pointS,
                                    *pointSW
                                }));
                    }
                }

                // Remember now that we're in a ship
                isRowInShip = true;
            }
            else
            {
                //
                // No point exists at these coordinates
                //

                // From now on we're not in a ship anymore
                isRowInShip = false;
            }
        }
    }
}

std::vector<ShipFactoryTriangle> ShipFactory::FilterOutRedundantTriangles(
    std::vector<ShipFactoryTriangle> const & triangleInfos,
    std::vector<ShipFactoryPoint> & pointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1)
{
    // Remove:
    //  - Those triangles whose vertices are all rope points, of which at least one is connected exclusively
    //    to rope points (these would be knots "sticking out" of the structure)
    //      - This happens when two or more rope endpoints - from the structural layer - are next to each other

    std::vector<ShipFactoryTriangle> newTriangleInfos;
    newTriangleInfos.reserve(triangleInfos.size());

    for (ElementIndex t = 0; t < triangleInfos.size(); ++t)
    {
        if (pointInfos1[triangleInfos[t].PointIndices1[0]].IsRope
            && pointInfos1[triangleInfos[t].PointIndices1[1]].IsRope
            && pointInfos1[triangleInfos[t].PointIndices1[2]].IsRope)
        {
            // Do not add triangle if at least one vertex is connected to rope points only
            if (!IsConnectedToNonRopePoints(triangleInfos[t].PointIndices1[0], pointInfos1, springInfos1)
                || !IsConnectedToNonRopePoints(triangleInfos[t].PointIndices1[1], pointInfos1, springInfos1)
                || !IsConnectedToNonRopePoints(triangleInfos[t].PointIndices1[2], pointInfos1, springInfos1))
            {
                continue;
            }
        }

        // Remember to create this triangle
        newTriangleInfos.push_back(triangleInfos[t]);
    }

    return newTriangleInfos;
}

void ShipFactory::ConnectPointsToTriangles(
    std::vector<ShipFactoryPoint> & pointInfos1,
    std::vector<ShipFactoryTriangle> const & triangleInfos1)
{
    for (ElementIndex t = 0; t < triangleInfos1.size(); ++t)
    {
        // Add triangle to its endpoints
        pointInfos1[triangleInfos1[t].PointIndices1[0]].ConnectedTriangles1.emplace_back(t);
        pointInfos1[triangleInfos1[t].PointIndices1[1]].ConnectedTriangles1.emplace_back(t);
        pointInfos1[triangleInfos1[t].PointIndices1[2]].ConnectedTriangles1.emplace_back(t);
    }
}

ShipFactory::LayoutOptimizationResults ShipFactory::OptimizeLayout(
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1)
{
    IndexRemap optimalPointRemap(pointInfos1.size());
    IndexRemap optimalSpringRemap(springInfos1.size());

    std::vector<bool> remappedPointMask(pointInfos1.size(), false);
    std::vector<bool> remappedSpringMask(springInfos1.size(), false);
    std::vector<bool> springFlipMask(springInfos1.size(), false);

    // Build Point Pair (Old) -> Spring Index (Old) table
    ShipFactoryPointPairToIndexMap pointPair1ToSpringIndex1Map;
    for (ElementIndex s = 0; s < springInfos1.size(); ++s)
    {
        pointPair1ToSpringIndex1Map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springInfos1[s].PointAIndex, springInfos1[s].PointBIndex),
            std::forward_as_tuple(s));
    }

    //
    // 1. Find all "complete squares" from left-bottom
    //
    // A complete square looks like:
    //
    //  If A is "even":
    //
    //  D  C
    //  |\/|
    //  |/\|
    //  A  B
    //
    // Else (A is "odd"):
    //
    //  D--C
    //   \/
    //   /\
    //  A--B
    //
    // For each perfect square, we re-order springs and their endpoints of each spring so that:
    //  - The first two springs of the perfect square are the cross springs
    //  - The endpoints A's of the cross springs are to be connected, and likewise
    //    the endpoint B's
    //

    ElementCount perfectSquareCount = 0;

    for (int y = 0; y < pointIndexMatrix.height; ++y)
    {
        for (int x = 0; x < pointIndexMatrix.width; ++x)
        {
            // Check if this is vertex A of a square
            if (pointIndexMatrix[{x, y}]
                && x < pointIndexMatrix.width - 1 && pointIndexMatrix[{x + 1, y}]
                && y < pointIndexMatrix.height - 1 && pointIndexMatrix[{x + 1, y + 1}]
                && pointIndexMatrix[{x, y + 1}])
            {
                ElementIndex const a = *pointIndexMatrix[{x, y}];
                ElementIndex const b = *pointIndexMatrix[{x + 1, y}];
                ElementIndex const c = *pointIndexMatrix[{x + 1, y + 1}];
                ElementIndex const d = *pointIndexMatrix[{x, y + 1}];

                // Check existence - and availability - of all springs now

                ElementIndex crossSpringACIndex;
                if (auto const springIt = pointPair1ToSpringIndex1Map.find({ a, c });
                    springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                {
                    crossSpringACIndex = springIt->second;
                }
                else
                {
                    continue;
                }

                ElementIndex crossSpringBDIndex;
                if (auto const springIt = pointPair1ToSpringIndex1Map.find({ b, d });
                    springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                {
                    crossSpringBDIndex = springIt->second;
                }
                else
                {
                    continue;
                }

                if ((x + y) % 2 == 0)
                {
                    // Even: check AD, BC

                    ElementIndex sideSpringADIndex;
                    if (auto const springIt = pointPair1ToSpringIndex1Map.find({ a, d });
                        springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringADIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    ElementIndex sideSpringBCIndex;
                    if (auto const springIt = pointPair1ToSpringIndex1Map.find({ b, c });
                        springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringBCIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    // It'a a perfect square

                    // Re-order springs and make sure they have the right directions:
                    //  A->C
                    //  B->D
                    //  A->D
                    //  B->C

                    optimalSpringRemap.AddOld(crossSpringACIndex);
                    remappedSpringMask[crossSpringACIndex] = true;
                    if (springInfos1[crossSpringACIndex].PointBIndex != c)
                    {
                        assert(springInfos1[crossSpringACIndex].PointBIndex == a);
                        springFlipMask[crossSpringACIndex] = true;
                    }

                    optimalSpringRemap.AddOld(crossSpringBDIndex);
                    remappedSpringMask[crossSpringBDIndex] = true;
                    if (springInfos1[crossSpringBDIndex].PointBIndex != d)
                    {
                        assert(springInfos1[crossSpringBDIndex].PointBIndex == b);
                        springFlipMask[crossSpringBDIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringADIndex);
                    remappedSpringMask[sideSpringADIndex] = true;
                    if (springInfos1[sideSpringADIndex].PointBIndex != d)
                    {
                        assert(springInfos1[sideSpringADIndex].PointBIndex == a);
                        springFlipMask[sideSpringADIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringBCIndex);
                    remappedSpringMask[sideSpringBCIndex] = true;
                    if (springInfos1[sideSpringBCIndex].PointBIndex != c)
                    {
                        assert(springInfos1[sideSpringBCIndex].PointBIndex == b);
                        springFlipMask[sideSpringBCIndex] = true;
                    }
                }
                else
                {
                    // Odd: check AB, CD

                    ElementIndex sideSpringABIndex;
                    if (auto const springIt = pointPair1ToSpringIndex1Map.find({ a, b });
                        springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringABIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    ElementIndex sideSpringCDIndex;
                    if (auto const springIt = pointPair1ToSpringIndex1Map.find({ c, d });
                        springIt != pointPair1ToSpringIndex1Map.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringCDIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    // It'a a perfect square

                    // Re-order springs abd make sure they have the right directions:
                    //  A->C
                    //  D->B
                    //  A->B
                    //  D->C

                    optimalSpringRemap.AddOld(crossSpringACIndex);
                    remappedSpringMask[crossSpringACIndex] = true;
                    if (springInfos1[crossSpringACIndex].PointBIndex != c)
                    {
                        assert(springInfos1[crossSpringACIndex].PointBIndex == a);
                        springFlipMask[crossSpringACIndex] = true;
                    }

                    optimalSpringRemap.AddOld(crossSpringBDIndex);
                    remappedSpringMask[crossSpringBDIndex] = true;
                    if (springInfos1[crossSpringBDIndex].PointBIndex != b)
                    {
                        assert(springInfos1[crossSpringBDIndex].PointBIndex == d);
                        springFlipMask[crossSpringBDIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringABIndex);
                    remappedSpringMask[sideSpringABIndex] = true;
                    if (springInfos1[sideSpringABIndex].PointBIndex != b)
                    {
                        assert(springInfos1[sideSpringABIndex].PointBIndex == a);
                        springFlipMask[sideSpringABIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringCDIndex);
                    remappedSpringMask[sideSpringCDIndex] = true;
                    if (springInfos1[sideSpringCDIndex].PointBIndex != c)
                    {
                        assert(springInfos1[sideSpringCDIndex].PointBIndex == d);
                        springFlipMask[sideSpringCDIndex] = true;
                    }
                }

                // If we're here, this was a perfect square

                // Remap points

                if (!remappedPointMask[a])
                {
                    optimalPointRemap.AddOld(a);
                    remappedPointMask[a] = true;
                }

                if (!remappedPointMask[b])
                {
                    optimalPointRemap.AddOld(b);
                    remappedPointMask[b] = true;
                }

                if (!remappedPointMask[c])
                {
                    optimalPointRemap.AddOld(c);
                    remappedPointMask[c] = true;
                }

                if (!remappedPointMask[d])
                {
                    optimalPointRemap.AddOld(d);
                    remappedPointMask[d] = true;
                }

                ++perfectSquareCount;
            }
        }
    }

    //
    // Map leftovers now
    //

    LogMessage("LayoutOptimizer: ", perfectSquareCount, " perfect squares, ", std::count(remappedPointMask.cbegin(), remappedPointMask.cend(), false), " leftover points, ",
        std::count(remappedSpringMask.cbegin(), remappedSpringMask.cend(), false), " leftover springs");

    for (ElementIndex p = 0; p < pointInfos1.size(); ++p)
    {
        if (!remappedPointMask[p])
        {
            optimalPointRemap.AddOld(p);
        }
    }

    for (ElementIndex s = 0; s < springInfos1.size(); ++s)
    {
        if (!remappedSpringMask[s])
        {
            optimalSpringRemap.AddOld(s);
        }
    }

    //
    // Remap
    //

    // Remap point info's

    std::vector<ShipFactoryPoint> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());
    for (ElementIndex oldP : optimalPointRemap.GetOldIndices())
    {
        pointInfos2.emplace_back(pointInfos1[oldP]);
    }

    // Remap spring info's

    std::vector<ShipFactorySpring> springInfos2;
    springInfos2.reserve(springInfos1.size());
    for (ElementIndex oldS : optimalSpringRemap.GetOldIndices())
    {
        springInfos2.emplace_back(springInfos1[oldS]);

        springInfos2.back().PointAIndex = optimalPointRemap.OldToNew(springInfos2.back().PointAIndex);
        springInfos2.back().PointBIndex = optimalPointRemap.OldToNew(springInfos2.back().PointBIndex);

        if (springFlipMask[oldS])
        {
            springInfos2.back().SwapEndpoints();
        }
    }

    return std::make_tuple(
        std::move(pointInfos2),
        std::move(optimalPointRemap),
        std::move(springInfos2),
        std::move(optimalSpringRemap),
        std::move(perfectSquareCount));
}

void ShipFactory::ConnectSpringsAndTriangles(
    std::vector<ShipFactorySpring> & springInfos2,
    std::vector<ShipFactoryTriangle> & triangleInfos2,
    IndexRemap const & pointIndexRemap)
{
    //
    // 1. Build Point Pair (Old) -> Spring (New) table
    //

    ShipFactoryPointPairToIndexMap pointPair1ToSpring2Map;

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        pointPair1ToSpring2Map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(pointIndexRemap.NewToOld(springInfos2[s].PointAIndex), pointIndexRemap.NewToOld(springInfos2[s].PointBIndex)),
            std::forward_as_tuple(s));
    }

    //
    // 2. Visit all triangles and connect them to their springs
    //

    for (ElementIndex t = 0; t < triangleInfos2.size(); ++t)
    {
        for (size_t p = 0; p < triangleInfos2[t].PointIndices1.size(); ++p)
        {
            ElementIndex const endpointIndex1 = triangleInfos2[t].PointIndices1[p];

            ElementIndex const nextEndpointIndex1 =
                p < triangleInfos2[t].PointIndices1.size() - 1
                ? triangleInfos2[t].PointIndices1[p + 1]
                : triangleInfos2[t].PointIndices1[0];

            // Lookup spring for this pair
            auto const springIt = pointPair1ToSpring2Map.find({ endpointIndex1, nextEndpointIndex1 });
            assert(springIt != pointPair1ToSpring2Map.end());

            ElementIndex const springIndex2 = springIt->second;

            // Tell this spring that it has this additional triangle
            springInfos2[springIndex2].Triangles.push_back(t);
            assert(springInfos2[springIndex2].Triangles.size() <= 2);
            ++(springInfos2[springIndex2].CoveringTrianglesCount);
            assert(springInfos2[springIndex2].CoveringTrianglesCount <= 2);

            // Tell the triangle about this sub spring
            assert(!triangleInfos2[t].Springs2.contains(springIndex2));
            triangleInfos2[t].Springs2.push_back(springIndex2);
        }
    }

    //
    // 3. Now find "traverse" springs - i.e. springs that are not edges of any triangles
    // (because of our tessellation algorithm) - and see whether they're fully covered
    // by two triangles; if they are, consider these springs as being covered by those
    // two triangles.
    //
    // A "traverse" spring would be the B-C spring in the following pair of triangles:
    //
    //   A     B
    //    *---*
    //    |\  |
    //    | \ |
    //    |  \|
    //    *---*
    //   C     D
    //

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        if (2 == springInfos2[s].Triangles.size())
        {
            // This spring is the common edge between two triangles
            // (A-D above)

            //
            // Find the B and C endpoints (old)
            //

            ElementIndex endpoint1Index = NoneElementIndex;
            ShipFactoryTriangle & triangle1 = triangleInfos2[springInfos2[s].Triangles[0]];
            for (ElementIndex triangleVertex1 : triangle1.PointIndices1)
            {
                if (triangleVertex1 != pointIndexRemap.NewToOld(springInfos2[s].PointAIndex)
                    && triangleVertex1 != pointIndexRemap.NewToOld(springInfos2[s].PointBIndex))
                {
                    endpoint1Index = triangleVertex1;
                    break;
                }
            }

            assert(NoneElementIndex != endpoint1Index);

            ElementIndex endpoint2Index = NoneElementIndex;
            ShipFactoryTriangle & triangle2 = triangleInfos2[springInfos2[s].Triangles[1]];
            for (ElementIndex triangleVertex1 : triangle2.PointIndices1)
            {
                if (triangleVertex1 != pointIndexRemap.NewToOld(springInfos2[s].PointAIndex)
                    && triangleVertex1 != pointIndexRemap.NewToOld(springInfos2[s].PointBIndex))
                {
                    endpoint2Index = triangleVertex1;
                    break;
                }
            }

            assert(NoneElementIndex != endpoint2Index);


            //
            // See if there's a B-C spring
            //

            auto const traverseSpringIt = pointPair1ToSpring2Map.find({ endpoint1Index, endpoint2Index });
            if (traverseSpringIt != pointPair1ToSpring2Map.end())
            {
                // We have a traverse spring

                assert(0 == springInfos2[traverseSpringIt->second].Triangles.size());

                // Tell the traverse spring that it has these 2 covering triangles
                springInfos2[traverseSpringIt->second].CoveringTrianglesCount += 2;
                assert(springInfos2[traverseSpringIt->second].CoveringTrianglesCount == 2);

                // Tell the triangles that they're covering this spring
                assert(!triangle1.CoveredTraverseSpringIndex2.has_value());
                triangle1.CoveredTraverseSpringIndex2 = traverseSpringIt->second;
                assert(!triangle2.CoveredTraverseSpringIndex2.has_value());
                triangle2.CoveredTraverseSpringIndex2 = traverseSpringIt->second;
            }
        }
    }
}

std::vector<ShipFactoryFrontier> ShipFactory::CreateShipFrontiers(
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    IndexRemap const & pointIndexRemap,
    std::vector<ShipFactoryPoint> const & pointInfos2,
    std::vector<ShipFactorySpring> const & springInfos2,
    ShipFactoryPointPairToIndexMap const & pointPairToSpringIndex1Map,
    IndexRemap const & springIndexRemap)
{
    auto const startTime = GameChronometer::Now();

    //
    // Detect and create frontiers
    //

    std::vector<ShipFactoryFrontier> shipFactoryFrontiers;

    // Set that flags edges (2) that have become frontiers
    std::set<ElementIndex> frontierEdges2;

    // From left to right, skipping padding columns
    for (int x = 1; x < pointIndexMatrix.width - 1; ++x)
    {
        // Frontierable points are points on border edges of triangles
        bool isInFrontierablePointsRegion = false;

        // From bottom to top, skipping padding columns
        for (int y = 1; y < pointIndexMatrix.height - 1; ++y)
        {
            if (isInFrontierablePointsRegion)
            {
                // Check whether we are leaving the region of frontierable points
                //
                // We are leaving the region of frontierable points iff:
                //  - There's no point here, or
                //  - There's a point, but no spring along <previous_point>-<point>, or
                //  - There's a spring along <previous_point>-<point>, but no triangles along it

                assert(pointIndexMatrix[vec2i(x, y - 1)].has_value()); // We come from a frontierable region
                ElementIndex const previousPointIndex1 = *pointIndexMatrix[{x, y - 1}];

                if (!pointIndexMatrix[{x, y}].has_value())
                {
                    // No point here
                    isInFrontierablePointsRegion = false;
                }
                else
                {
                    ElementIndex const pointIndex1 = *pointIndexMatrix[{x, y}];

                    auto const springIndex1It = pointPairToSpringIndex1Map.find({ previousPointIndex1, pointIndex1 });
                    if (springIndex1It == pointPairToSpringIndex1Map.cend())
                    {
                        // No spring along <previous_point>-<point>
                        isInFrontierablePointsRegion = false;
                    }
                    else
                    {
                        ElementIndex const springIndex2 = springIndexRemap.OldToNew(springIndex1It->second);
                        if (springInfos2[springIndex2].Triangles.empty())
                        {
                            // No triangles along this spring
                            isInFrontierablePointsRegion = false;
                        }
                    }
                }

                if (!isInFrontierablePointsRegion)
                {
                    //
                    // Left the region of frontierable points
                    //

                    // See if may create a new external frontier
                    auto edgeIndices = PropagateFrontier(
                        previousPointIndex1,
                        vec2i(x, y - 1),
                        6, // N: the external point is at N of starting point
                        pointIndexMatrix,
                        frontierEdges2,
                        springInfos2,
                        pointPairToSpringIndex1Map,
                        springIndexRemap);

                    if (!edgeIndices.empty())
                    {
                        assert(edgeIndices.size() >= 3);

                        // Create new internal frontier
                        shipFactoryFrontiers.emplace_back(
                            FrontierType::Internal,
                            std::move(edgeIndices));
                    }
                }
            }

            if (!isInFrontierablePointsRegion)
            {
                // Check whether we are entering the region of frontierable points
                //
                // We are entering the region of frontierable points iff:
                //  - There's a point here, and
                //  - There's at least one triangle edge attached to this point

                if (pointIndexMatrix[{x, y}].has_value())
                {
                    ElementIndex const pointIndex1 = *pointIndexMatrix[{x, y}];
                    ElementIndex const pointIndex2 = pointIndexRemap.OldToNew(pointIndex1);

                    if (!pointInfos2[pointIndex2].ConnectedTriangles1.empty())
                    {
                        //
                        // Entered the region of frontierable points
                        //

                        isInFrontierablePointsRegion = true;

                        // See if may create a new external frontier
                        auto edgeIndices = PropagateFrontier(
                            pointIndex1,
                            vec2i(x, y),
                            2, // S: the external point is at S of starting point
                            pointIndexMatrix,
                            frontierEdges2,
                            springInfos2,
                            pointPairToSpringIndex1Map,
                            springIndexRemap);

                        if (!edgeIndices.empty())
                        {
                            assert(edgeIndices.size() >= 3);

                            // Create new external frontier
                            shipFactoryFrontiers.emplace_back(
                                FrontierType::External,
                                std::move(edgeIndices));
                        }
                    }
                }
            }
        }
    }

    LogMessage("ShipFactory: completed frontiers:",
        " time=", std::chrono::duration_cast<std::chrono::microseconds>(GameChronometer::Now() - startTime).count(), "us");

    return shipFactoryFrontiers;
}

std::vector<ElementIndex> ShipFactory::PropagateFrontier(
    ElementIndex startPointIndex1,
    vec2i startPointCoordinates,
    Octant startOctant,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::set<ElementIndex> & frontierEdges2,
    std::vector<ShipFactorySpring> const & springInfos2,
    ShipFactoryPointPairToIndexMap const & pointPairToSpringIndex1Map,
    IndexRemap const & springIndexRemap)
{
    std::vector<ElementIndex> edgeIndices;

#ifdef _DEBUG
    std::vector<ElementIndex> frontierPoints1;
    frontierPoints1.push_back(startPointIndex1);
#endif

    //
    // March until we get back to the starting point; if we realize
    // that we're following an already-existing frontier (and we're
    // gonna realize that immediately after finding the first edge),
    // bail out and return an empty list of edges.
    //

    ElementIndex pointIndex1 = startPointIndex1;
    vec2i pointCoords = startPointCoordinates;

    Octant octant = startOctant;

    while (true)
    {
        //
        // From the octant next to the starting octant, walk CW until we find
        // a frontierable point
        //

        ElementIndex nextPointIndex1 = NoneElementIndex;
        vec2i nextPointCoords;
        ElementIndex springIndex2 = NoneElementIndex;
        Octant nextOctant = octant;
        while (true)
        {
            // Advance to next octant
            nextOctant = (nextOctant + 1) % 8;

            // We are guaranteed to find another point, as the starting point is on a frontier
            assert(nextOctant != octant);
            if (nextOctant == octant) // Just for sanity
            {
                throw GameException("Cannot find a frontierable point at any octant");
            }

            // Get coords of next point
            nextPointCoords = pointCoords + vec2i(TessellationCircularOrderDirections[nextOctant][0], TessellationCircularOrderDirections[nextOctant][1]);

            // Check whether it's a frontierable point
            //
            // The next point is a frontierable point iff:
            //  - There's a point here, and
            //  - There's a spring along <previous_point>-<point>, and
            //  - There's one and only one triangle along it

            if (!pointIndexMatrix[nextPointCoords].has_value())
            {
                // No point here
                continue;
            }

            nextPointIndex1 = *pointIndexMatrix[nextPointCoords];

            auto const springIndex1It = pointPairToSpringIndex1Map.find({ pointIndex1, nextPointIndex1 });
            if (springIndex1It == pointPairToSpringIndex1Map.cend())
            {
                // No spring here
                continue;
            }

            springIndex2 = springIndexRemap.OldToNew(springIndex1It->second);
            if (springInfos2[springIndex2].Triangles.size() != 1)
            {
                // No triangles along this spring, or two triangles along it
                continue;
            }

            //
            // Found it!
            //

            break;
        }

        assert(nextPointIndex1 != NoneElementIndex);
        assert(springIndex2 != NoneElementIndex);
        assert(nextOctant != octant);

        //
        // See whether this edge already belongs to a frontier,
        // and if not, flag it
        //

        auto [_, isInserted] = frontierEdges2.insert(springIndex2);
        if (!isInserted)
        {
            // This may only happen at the beginning
            assert(edgeIndices.empty());

            // No need to propagate along this frontier, it has already been created
            break;
        }

        //
        // Store edge
        //

        edgeIndices.push_back(springIndex2);

        //
        // See whether we have closed the loop
        //

        if (nextPointIndex1 == startPointIndex1)
            break;

#ifdef _DEBUG
        frontierPoints1.push_back(nextPointIndex1);
#endif

        //
        // Advance
        //

        pointIndex1 = nextPointIndex1;
        pointCoords = nextPointCoords;
        octant = (nextOctant + 4) % 8; // Flip 180
    }

    return edgeIndices;
}

std::tuple<Physics::Points, std::set<ElectricalElementInstanceIndex>> ShipFactory::CreatePoints(
    std::vector<ShipFactoryPoint> const & pointInfos2,
    World & parentWorld,
    MaterialDatabase const & materialDatabase,
    SimulationEventDispatcher & simulationEventDispatcher,
    SimulationParameters const & simulationParameters,
    ShipPhysicsData const & physicsData)
{
    Physics::Points points(
        static_cast<ElementIndex>(pointInfos2.size()),
        parentWorld,
        materialDatabase,
        simulationEventDispatcher,
        simulationParameters);

    std::set<ElectricalElementInstanceIndex> allElectricalElementInstanceIndices;

    float const internalPressure =
        physicsData.InternalPressure // Default internal pressure is 1atm
        * SimulationParameters::AirPressureAtSeaLevel; // The ship's (initial) internal pressure is just relative to a constant 1 atm

    ElementIndex electricalElementCounter = 0;
    for (size_t p = 0; p < pointInfos2.size(); ++p)
    {
        ShipFactoryPoint const & pointInfo = pointInfos2[p];

        ElementIndex electricalElementIndex = NoneElementIndex;
        if (nullptr != pointInfo.ElectricalMtl)
        {
            // This point has an associated electrical element
            electricalElementIndex = electricalElementCounter;
            ++electricalElementCounter;
        }

        //
        // Create point
        //

        points.Add(
            pointInfo.Position,
            pointInfo.Water,
            internalPressure,
            pointInfo.StructuralMtl,
            pointInfo.ElectricalMtl,
            pointInfo.IsRope,
            pointInfo.Strength,
            electricalElementIndex,
            pointInfo.IsLeaking,
            pointInfo.RenderColor,
            pointInfo.TextureCoordinates,
            GameRandomEngine::GetInstance().GenerateNormalizedUniformReal());

        //
        // Remember electrical element instance index
        //

        if (pointInfo.ElectricalElementInstanceIdx != NoneElectricalElementInstanceIndex)
        {
            auto [_, isInserted] = allElectricalElementInstanceIndices.insert(pointInfo.ElectricalElementInstanceIdx);
            assert(isInserted);
            (void)isInserted;
        }
    }

    return std::make_tuple(
        std::move(points),
        std::move(allElectricalElementInstanceIndices));
}

Physics::Springs ShipFactory::CreateSprings(
    std::vector<ShipFactorySpring> const & springInfos2,
    ElementCount perfectSquareCount,
    Physics::Points & points,
    Physics::World & parentWorld,
    SimulationEventDispatcher & simulationEventDispatcher,
    SimulationParameters const & simulationParameters)
{
    Physics::Springs springs(
        static_cast<ElementIndex>(springInfos2.size()),
        perfectSquareCount,
        parentWorld,
        simulationEventDispatcher,
        simulationParameters);

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        // Create spring
        springs.Add(
            springInfos2[s].PointAIndex,
            springInfos2[s].PointBIndex,
            springInfos2[s].PointAAngle,
            springInfos2[s].PointBAngle,
            springInfos2[s].Triangles,
            springInfos2[s].CoveringTrianglesCount,
            points);

        // Add spring to its endpoints
        points.AddFactoryConnectedSpring(
            springInfos2[s].PointAIndex,
            s,
            springInfos2[s].PointBIndex);
        points.AddFactoryConnectedSpring(
            springInfos2[s].PointBIndex,
            s,
            springInfos2[s].PointAIndex);
    }

    return springs;
}

Physics::Triangles ShipFactory::CreateTriangles(
    std::vector<ShipFactoryTriangle> const & triangleInfos2,
    Physics::Points & points,
    IndexRemap const & pointIndexRemap,
    std::vector<ShipFactorySpring> const & springInfos2,
    ShipFactoryFloorPlan const & floorPlan2)
{
    Physics::Triangles triangles(static_cast<ElementIndex>(triangleInfos2.size()));

    for (ElementIndex t = 0; t < triangleInfos2.size(); ++t)
    {
        assert(triangleInfos2[t].Springs2.size() == 3);

        // Derive whether this is a sealed triangle
        bool isSealedTriangle = true;
        for (int iEdge = 0; iEdge < 3; ++iEdge)
        {
            ElementIndex const springIndex2 = triangleInfos2[t].Springs2[iEdge];

            ElementIndex const edgePointAIndex2 = springInfos2[springIndex2].PointAIndex;
            ElementIndex const edgePointBIndex2 = springInfos2[springIndex2].PointBIndex;

            isSealedTriangle = isSealedTriangle && (floorPlan2.find({ edgePointAIndex2, edgePointBIndex2 }) != floorPlan2.end());
        }

        // Calculate opposite triangles and floor types
        std::array<std::pair<ElementIndex, int>, 3> subSpringsOppositeTriangle;
        std::array<NpcFloorKindType, 3> subSpringsFloorKind;
        std::array<NpcFloorGeometryType, 3> subSpringsFloorGeometry;
        for (int iEdge = 0; iEdge < 3; ++iEdge)
        {
            ElementIndex const springIndex2 = triangleInfos2[t].Springs2[iEdge];
            assert(springInfos2[springIndex2].Triangles.size() >= 1 && springInfos2[springIndex2].Triangles.size() <= 2);

            if (springInfos2[springIndex2].Triangles[0] == t)
            {
                if (springInfos2[springIndex2].Triangles.size() >= 2)
                {
                    assert(springInfos2[springIndex2].Triangles.size() == 2);
                    subSpringsOppositeTriangle[iEdge].first = springInfos2[springIndex2].Triangles[1];
                }
                else
                {
                    subSpringsOppositeTriangle[iEdge].first = NoneElementIndex;
                }
            }
            else if (springInfos2[springIndex2].Triangles.size() >= 2)
            {
                assert(springInfos2[springIndex2].Triangles.size() == 2);
                assert(springInfos2[springIndex2].Triangles[1] == t);
                subSpringsOppositeTriangle[iEdge].first = springInfos2[springIndex2].Triangles[0];
            }
            else
            {
                subSpringsOppositeTriangle[iEdge].first = NoneElementIndex;
            }

            if (subSpringsOppositeTriangle[iEdge].first != NoneElementIndex)
            {
                if (triangleInfos2[subSpringsOppositeTriangle[iEdge].first].Springs2[0] == triangleInfos2[t].Springs2[iEdge])
                {
                    subSpringsOppositeTriangle[iEdge].second = 0;
                }
                else if (triangleInfos2[subSpringsOppositeTriangle[iEdge].first].Springs2[1] == triangleInfos2[t].Springs2[iEdge])
                {
                    subSpringsOppositeTriangle[iEdge].second = 1;
                }
                else
                {
                    assert(triangleInfos2[subSpringsOppositeTriangle[iEdge].first].Springs2[2] == triangleInfos2[t].Springs2[iEdge]);
                    subSpringsOppositeTriangle[iEdge].second = 2;
                }
            }

            //
            // Triangle's subedge is floor if:
            //  - Spring is floor, AND
            //  - NOT is sealed, OR (is sealed and) there's no triangle on the other side of this subedge
            //

            ElementIndex const edgePointAIndex2 = springInfos2[springIndex2].PointAIndex;
            ElementIndex const edgePointBIndex2 = springInfos2[springIndex2].PointBIndex;

            if (const auto floorIt = floorPlan2.find({ edgePointAIndex2, edgePointBIndex2 });
                floorIt != floorPlan2.cend()
                && (!isSealedTriangle || subSpringsOppositeTriangle[iEdge].first == NoneElementIndex))
            {
                subSpringsFloorKind[iEdge] = floorIt->second.FloorKind;
                subSpringsFloorGeometry[iEdge] = floorIt->second.FloorGeometry;
            }
            else
            {
                subSpringsFloorKind[iEdge] = NpcFloorKindType::NotAFloor;
                subSpringsFloorGeometry[iEdge] = NpcFloorGeometryType::NotAFloor;
            }
        }

        // Create triangle
        triangles.Add(
            pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[0]),
            pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[1]),
            pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[2]),
            triangleInfos2[t].Springs2[0],
            triangleInfos2[t].Springs2[1],
            triangleInfos2[t].Springs2[2],
            { subSpringsOppositeTriangle[0].first, subSpringsOppositeTriangle[0].second },
            { subSpringsOppositeTriangle[1].first, subSpringsOppositeTriangle[1].second },
            { subSpringsOppositeTriangle[2].first, subSpringsOppositeTriangle[2].second },
            { subSpringsFloorKind[0], subSpringsFloorGeometry[0] },
            { subSpringsFloorKind[1], subSpringsFloorGeometry[1] },
            { subSpringsFloorKind[2], subSpringsFloorGeometry[2] },
            triangleInfos2[t].CoveredTraverseSpringIndex2);

        // Add triangle to its endpoints
        points.AddFactoryConnectedTriangle(pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[0]), t, true); // Owner
        points.AddFactoryConnectedTriangle(pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[1]), t, false); // Not owner
        points.AddFactoryConnectedTriangle(pointIndexRemap.OldToNew(triangleInfos2[t].PointIndices1[2]), t, false); // Not owner
    }

    return triangles;
}

ElectricalElements ShipFactory::CreateElectricalElements(
    Physics::Points const & points,
    std::vector<ShipFactoryPoint> const & pointInfos2,
    std::set<ElectricalElementInstanceIndex> const & allElectricalElementInstanceIndices,
    ElectricalPanel const & electricalPanel,
    bool flipH,
    bool flipV,
    bool rotate90CW,
    ShipId shipId,
    Physics::World & parentWorld,
    SimulationEventDispatcher & simulationEventDispatcher,
    SimulationParameters const & simulationParameters)
{
    assert(points.GetRawShipPointCount() == pointInfos2.size());

    //
    // Verify all panel metadata indices are valid instance IDs
    //

    for (auto const & entry : electricalPanel)
    {
        if (allElectricalElementInstanceIndices.count(entry.first) == 0)
        {
            throw GameException("Index '" + std::to_string(entry.first) + "' of electrical panel metadata cannot be found among electrical element indices");
        }
    }

    //
    // - Get indices of points with electrical elements, together with their panel metadata
    // - Count number of lamps
    //

    struct ElectricalElementInfo
    {
        ElementIndex elementIndex;
        ElectricalElementInstanceIndex instanceIndex;
        std::optional<ElectricalPanel::ElementMetadata> panelElementMetadata;

        ElectricalElementInfo(
            ElementIndex _elementIndex,
            ElectricalElementInstanceIndex _instanceIndex,
            std::optional<ElectricalPanel::ElementMetadata> _panelElementMetadata)
            : elementIndex(_elementIndex)
            , instanceIndex(_instanceIndex)
            , panelElementMetadata(_panelElementMetadata)
        {}
    };

    std::vector<ElectricalElementInfo> electricalElementInfos;
    ElementIndex lampElementCount = 0;
    for (auto pointIndex : points.RawShipPoints())
    {
        ElectricalMaterial const * const electricalMaterial = points.GetElectricalMaterial(pointIndex);
        if (nullptr != electricalMaterial)
        {
            auto const instanceIndex = pointInfos2[pointIndex].ElectricalElementInstanceIdx;

            // Get panel metadata
            std::optional<ElectricalPanel::ElementMetadata> panelElementMetadata;
            if (electricalMaterial->IsInstanced)
            {
                assert(instanceIndex != NoneElectricalElementInstanceIndex);

                auto const panelEntryIt = electricalPanel.Find(instanceIndex);
                if (panelEntryIt != electricalPanel.end()) // Note: we allow instanced elements to not have a panel entry
                {
                    // Take metadata
                    panelElementMetadata = panelEntryIt->second;
                }
            }

            electricalElementInfos.emplace_back(pointIndex, instanceIndex, panelElementMetadata);

            if (ElectricalMaterial::ElectricalElementType::Lamp == electricalMaterial->ElectricalType)
                ++lampElementCount;
        }
    }

    //
    // Create electrical elements
    //

    ElectricalElements electricalElements(
        static_cast<ElementCount>(electricalElementInfos.size()),
        lampElementCount,
        shipId,
        parentWorld,
        simulationEventDispatcher,
        simulationParameters);

    for (auto const & elementInfo : electricalElementInfos)
    {
        ElectricalMaterial const * const electricalMaterial = points.GetElectricalMaterial(elementInfo.elementIndex);
        assert(nullptr != electricalMaterial);

        // Add element
        electricalElements.Add(
            elementInfo.elementIndex,
            elementInfo.instanceIndex,
            elementInfo.panelElementMetadata,
            *electricalMaterial,
            flipH,
            flipV,
            rotate90CW,
            points);
    }


    //
    // Connect electrical elements that are connected by springs to each other
    //

    for (auto electricalElementIndex : electricalElements)
    {
        auto const pointIndex = electricalElements.GetPointIndex(electricalElementIndex);

        for (auto const & cs : points.GetConnectedSprings(pointIndex).ConnectedSprings)
        {
            auto const otherEndpointElectricalElementIndex = points.GetElectricalElement(cs.OtherEndpointIndex);
            if (NoneElementIndex != otherEndpointElectricalElementIndex)
            {
                // Add element
                electricalElements.AddFactoryConnectedElectricalElement(
                    electricalElementIndex,
                    otherEndpointElectricalElementIndex);
            }
        }
    }

    return electricalElements;
}

Physics::Frontiers ShipFactory::CreateFrontiers(
    std::vector<ShipFactoryFrontier> const & shipFactoryFrontiers,
    Physics::Points const & points,
    Physics::Springs const & springs)
{
    //
    // Create Frontiers container
    //

    Frontiers frontiers = Frontiers(
        points.GetElementCount(),
        springs.GetElementCount());

    //
    // Add all frontiers
    //

    for (auto const & sbf : shipFactoryFrontiers)
    {
        frontiers.AddFrontier(
            sbf.Type,
            sbf.EdgeIndices2,
            springs);
    }

    return frontiers;
}

#ifdef _DEBUG
void ShipFactory::VerifyShipInvariants(
    Physics::Points const & points,
    Physics::Springs const & /*springs*/,
    Physics::Triangles const & triangles)
{
    //
    // Triangles' points are in CW order
    //

    for (auto t : triangles)
    {
        Verify(triangles.AreVerticesInCwOrder(t, points));
    }
}
#endif
