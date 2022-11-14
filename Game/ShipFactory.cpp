/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
*
* Contains code from Tom Forsyth - https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
***************************************************************************************/
#include "ShipFactory.h"

#include "Formulae.h"

#include <GameCore/GameDebug.h>
#include <GameCore/GameMath.h>
#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

// This is our local circular order (clockwise, starting from E)
// Note: cardinal directions are labeled according to y growing upwards
static int const TessellationCircularOrderDirections[8][2] = {
    {  1,  0 },  // 0: E
    {  1, -1 },  // 1: SE
    {  0, -1 },  // 2: S
    { -1, -1 },  // 3: SW
    { -1,  0 },  // 4: W
    { -1,  1 },  // 5: NW
    {  0,  1 },  // 6: N
    {  1,  1 }   // 7: NE
};

//////////////////////////////////////////////////////////////////////////////

std::tuple<std::unique_ptr<Physics::Ship>, RgbaImageData> ShipFactory::Create(
    ShipId shipId,
    World & parentWorld,
    ShipDefinition && shipDefinition,
    ShipLoadOptions const & shipLoadOptions,
    MaterialDatabase const & materialDatabase,
    ShipTexturizer const & shipTexturizer,
    ShipStrengthRandomizer const & shipStrengthRandomizer,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::shared_ptr<TaskThreadPool> taskThreadPool,
    GameParameters const & gameParameters)
{
    auto const totalStartTime = std::chrono::steady_clock::now();

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

    PointPairToIndexMap pointPairToSpringIndex1Map;

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
    // Optimize order of ShipFactoryPoint's and ShipFactorySpring's to minimize cache misses
    //

    float originalSpringACMR = CalculateACMR(springInfos1);

    // Tiling algorithm
    //auto [pointInfos2, pointIndexRemap2, springInfos2, springIndexRemap2] = ReorderPointsAndSpringsOptimally_Tiling<2>(
    auto [pointInfos2, pointIndexRemap2, springInfos2, springIndexRemap2] = ReorderPointsAndSpringsOptimally_Stripes<4>(
        pointInfos1,
        springInfos1,
        pointPairToSpringIndex1Map,
        pointIndexMatrix);

    float optimizedSpringACMR = CalculateACMR(springInfos2);

    LogMessage("ShipFactory: Spring ACMR: original=", originalSpringACMR, ", optimized=", optimizedSpringACMR);

    //
    // Optimize order of Triangles
    //

    // Note: we don't optimize triangles, as tests indicate that performance gets (marginally) worse,
    // and at the same time, it makes sense to use the natural order of the triangles as it ensures
    // that higher elements in the ship cover lower elements when they are semi-detached.

    ////float originalACMR = CalculateACMR(triangleInfos);
    ////float originalVMR = CalculateVertexMissRatio(triangleInfos);

    ////triangleInfos = ReorderTrianglesOptimally_TomForsyth(triangleInfos, pointInfos1.size());

    ////float optimizedACMR = CalculateACMR(triangleInfos);
    ////float optimizedVMR = CalculateVertexMissRatio(triangleInfos);

    ////LogMessage("ShipFactory: Triangles ACMR: original=", originalACMR, ", optimized=", optimizedACMR);
    ////LogMessage("ShipFactory: Triangles VMR: original=", originalVMR, ", optimized=", optimizedVMR);

    //
    // Associate all springs with the triangles that run through them (supertriangles)
    //

    ConnectSpringsAndTriangles(
        springInfos2,
        triangleInfos);

    //
    // Create frontiers
    //

    auto const frontiersStartTime = std::chrono::steady_clock::now();

    std::vector<ShipFactoryFrontier> shipFactoryFrontiers = CreateShipFrontiers(
        pointIndexMatrix,
        pointIndexRemap2,
        pointInfos2,
        springInfos2,
        pointPairToSpringIndex1Map,
        springIndexRemap2);

    auto const frontiersEndTime = std::chrono::steady_clock::now();

    //
    // Randomize strength
    //

    shipStrengthRandomizer.RandomizeStrength(
        pointIndexMatrix,
        vec2i(minX, minY) + vec2i(1, 1), // Image -> PointIndexMatrix
        vec2i(maxX - minX + 1, maxY - minY + 1),
        pointInfos2,
        pointIndexRemap2,
        springInfos2,
        triangleInfos,
        shipFactoryFrontiers);

    //
    // Visit all ShipFactoryPoint's and create Points, i.e. the entire set of points
    //

    std::vector<ElectricalElementInstanceIndex> electricalElementInstanceIndices;
    Physics::Points points = CreatePoints(
        pointInfos2,
        parentWorld,
        materialDatabase,
        gameEventDispatcher,
        gameParameters,
        electricalElementInstanceIndices,
        shipDefinition.PhysicsData);

    //
    // Create Springs for all ShipFactorySpring's
    //

    Springs springs = CreateSprings(
        springInfos2,
        points,
        pointIndexRemap2,
        parentWorld,
        gameEventDispatcher,
        gameParameters);

    //
    // Create Triangles for all ShipFactoryTriangle's
    //

    Triangles triangles = CreateTriangles(
        triangleInfos,
        points,
        pointIndexRemap2);

    //
    // Create Electrical Elements
    //

    ElectricalElements electricalElements = CreateElectricalElements(
        points,
        electricalElementInstanceIndices,
        shipDefinition.Layers.ElectricalLayer
            ? shipDefinition.Layers.ElectricalLayer->Panel
            : ElectricalPanel(),
        shipId,
        parentWorld,
        gameEventDispatcher,
        gameParameters);

    //
    // Create frontiers
    //

    Frontiers frontiers = CreateFrontiers(
        shipFactoryFrontiers,
        points,
        springs);

    //
    // Create texture, if needed
    //

    RgbaImageData textureImage = shipDefinition.Layers.TextureLayer
        ? std::move(shipDefinition.Layers.TextureLayer->Buffer) // Use provided texture
        : shipTexturizer.MakeAutoTexture(
            *shipDefinition.Layers.StructuralLayer,
            shipDefinition.AutoTexturizationSettings); // Auto-texturize

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
        points.GetRawShipPointCount(), "/", points.GetBufferElementCount(), "buf points, ",
        springs.GetElementCount(), " springs, ", triangles.GetElementCount(), " triangles, ",
        electricalElements.GetElementCount(), " electrical elements, ",
        frontiers.GetElementCount(), " frontiers.");

    auto ship = std::make_unique<Ship>(
        shipId,
        parentWorld,
        materialDatabase,
        std::move(gameEventDispatcher),
        std::move(taskThreadPool),
        std::move(points),
        std::move(springs),
        std::move(triangles),
        std::move(electricalElements),
        std::move(frontiers));

    LogMessage("ShipFactory: Create() took ",
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - totalStartTime).count(),
        " us (frontiers: ", std::chrono::duration_cast<std::chrono::microseconds>(frontiersEndTime - frontiersStartTime).count(), " us)");

    return std::make_tuple(
        std::move(ship),
        std::move(textureImage));
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
    PointPairToIndexMap & pointPairToSpringIndex1Map)
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
    PointPairToIndexMap & pointPairToSpringIndex1Map,
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
        bool isInShip = false;

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
                // Check if a spring exists
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


                        //
                        // Check if a triangle exists
                        // - If this is the first point that is in a ship, we check all the way up to W;
                        // - Else, we check only up to S, so to avoid covering areas already covered by the triangulation
                        //   at the previous point
                        //

                        // Check adjacent point in next CW direction
                        int adjx2 = x + TessellationCircularOrderDirections[i + 1][0];
                        int adjy2 = y + TessellationCircularOrderDirections[i + 1][1];
                        if ((!isInShip || i < 2)
                            && !!pointIndexMatrix[{adjx2, adjy2}])
                        {
                            // This point is adjacent to the first point at one of SE, S, SW, W

                            //
                            // Create ShipFactoryTriangle
                            //

                            triangleInfos1.emplace_back(
                                std::array<ElementIndex, 3>( // Points are in CW order
                                    {
                                        pointIndex1,
                                        otherEndpointIndex1,
                                        *pointIndexMatrix[{adjx2, adjy2}]
                                    }));
                        }

                        // Now, we also want to check whether the single "irregular" triangle from this point exists,
                        // i.e. the triangle between this point, the point at its E, and the point at its
                        // S, in case there is no point at SE.
                        // We do this so that we can forget the entire W side for inner points and yet ensure
                        // full coverage of the area
                        if (i == 0
                            && !pointIndexMatrix[{x + TessellationCircularOrderDirections[1][0], y + TessellationCircularOrderDirections[1][1]}]
                            && !!pointIndexMatrix[{x + TessellationCircularOrderDirections[2][0], y + TessellationCircularOrderDirections[2][1]}])
                        {
                            // If we're here, the point at E exists
                            assert(!!pointIndexMatrix[vec2i(x + TessellationCircularOrderDirections[0][0], y + TessellationCircularOrderDirections[0][1])]);

                            //
                            // Create ShipFactoryTriangle
                            //

                            triangleInfos1.emplace_back(
                                std::array<ElementIndex, 3>( // Points are in CW order
                                    {
                                        pointIndex1,
                                        * pointIndexMatrix[{x + TessellationCircularOrderDirections[0][0], y + TessellationCircularOrderDirections[0][1]}],
                                        * pointIndexMatrix[{x + TessellationCircularOrderDirections[2][0], y + TessellationCircularOrderDirections[2][1]}]
                                    }));
                        }
                    }
                }

                // Remember now that we're in a ship
                isInShip = true;
            }
            else
            {
                //
                // No point exists at these coordinates
                //

                // From now on we're not in a ship anymore
                isInShip = false;
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

std::vector<ShipFactoryFrontier> ShipFactory::CreateShipFrontiers(
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::vector<ElementIndex> const & pointIndexRemap2,
    std::vector<ShipFactoryPoint> const & pointInfos2,
    std::vector<ShipFactorySpring> const & springInfos2,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    std::vector<ElementIndex> const & springIndexRemap2)
{
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
                        ElementIndex const springIndex2 = springIndexRemap2[springIndex1It->second];
                        if (springInfos2[springIndex2].SuperTriangles2.empty())
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
                        springIndexRemap2);

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
                //  - There's at least one a triangle edge attached to this point

                if (pointIndexMatrix[{x, y}].has_value())
                {
                    ElementIndex const pointIndex1 = *pointIndexMatrix[{x, y}];
                    ElementIndex const pointIndex2 = pointIndexRemap2[pointIndex1];

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
                            springIndexRemap2);

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

    return shipFactoryFrontiers;
}

std::vector<ElementIndex> ShipFactory::PropagateFrontier(
    ElementIndex startPointIndex1,
    vec2i startPointCoordinates,
    Octant startOctant, // Relative to starting point
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    std::set<ElementIndex> & frontierEdges2,
    std::vector<ShipFactorySpring> const & springInfos2,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    std::vector<ElementIndex> const & springIndexRemap2)
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

            springIndex2 = springIndexRemap2[springIndex1It->second];
            if (springInfos2[springIndex2].SuperTriangles2.size() != 1)
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

Physics::Points ShipFactory::CreatePoints(
    std::vector<ShipFactoryPoint> const & pointInfos2,
    World & parentWorld,
    MaterialDatabase const & materialDatabase,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters,
    std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices,
    ShipPhysicsData const & physicsData)
{
    Physics::Points points(
        static_cast<ElementIndex>(pointInfos2.size()),
        parentWorld,
        materialDatabase,
        std::move(gameEventDispatcher),
        gameParameters);

    electricalElementInstanceIndices.reserve(pointInfos2.size());

    float const internalPressure =
        physicsData.InternalPressure // Default internal pressure is 1atm
        * GameParameters::AirPressureAtSeaLevel; // The ship's (initial) internal pressure is just relative to a constant 1 atm

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
        // Store electrical element instance index
        //

        electricalElementInstanceIndices.push_back(pointInfo.ElectricalElementInstanceIdx);
    }

    return points;
}

void ShipFactory::ConnectSpringsAndTriangles(
    std::vector<ShipFactorySpring> & springInfos2,
    std::vector<ShipFactoryTriangle> & triangleInfos2)
{
    //
    // 1. Build Point Pair -> Spring table
    //

    std::unordered_map<PointPair, ElementIndex, PointPair::Hasher> pointPairToSpringMap;

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        pointPairToSpringMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springInfos2[s].PointAIndex1, springInfos2[s].PointBIndex1),
            std::forward_as_tuple(s));
    }


    //
    // 2. Visit all triangles and connect them to their springs
    //

    for (ElementIndex t = 0; t < triangleInfos2.size(); ++t)
    {
        for (size_t p = 0; p < triangleInfos2[t].PointIndices1.size(); ++p)
        {
            ElementIndex const endpointIndex = triangleInfos2[t].PointIndices1[p];

            ElementIndex const nextEndpointIndex =
                p < triangleInfos2[t].PointIndices1.size() - 1
                ? triangleInfos2[t].PointIndices1[p + 1]
                : triangleInfos2[t].PointIndices1[0];

            // Lookup spring for this pair
            auto const springIt = pointPairToSpringMap.find({ endpointIndex, nextEndpointIndex });
            assert(springIt != pointPairToSpringMap.end());

            ElementIndex const springIndex = springIt->second;

            // Tell this spring that it has this additional super triangle
            springInfos2[springIndex].SuperTriangles2.push_back(t);
            assert(springInfos2[springIndex].SuperTriangles2.size() <= 2);
            ++(springInfos2[springIndex].CoveringTrianglesCount);
            assert(springInfos2[springIndex].CoveringTrianglesCount <= 2);

            // Tell the triangle about this sub spring
            assert(!triangleInfos2[t].SubSprings2.contains(springIndex));
            triangleInfos2[t].SubSprings2.push_back(springIndex);
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
        if (2 == springInfos2[s].SuperTriangles2.size())
        {
            // This spring is the common edge between two triangles
            // (A-D above)

            //
            // Find the B and C endpoints
            //

            ElementIndex endpoint1Index = NoneElementIndex;
            ShipFactoryTriangle & triangle1 = triangleInfos2[springInfos2[s].SuperTriangles2[0]];
            for (ElementIndex triangleVertex : triangle1.PointIndices1)
            {
                if (triangleVertex != springInfos2[s].PointAIndex1
                    && triangleVertex != springInfos2[s].PointBIndex1)
                {
                    endpoint1Index = triangleVertex;
                    break;
                }
            }

            assert(NoneElementIndex != endpoint1Index);

            ElementIndex endpoint2Index = NoneElementIndex;
            ShipFactoryTriangle & triangle2 = triangleInfos2[springInfos2[s].SuperTriangles2[1]];
            for (ElementIndex triangleVertex : triangle2.PointIndices1)
            {
                if (triangleVertex != springInfos2[s].PointAIndex1
                    && triangleVertex != springInfos2[s].PointBIndex1)
                {
                    endpoint2Index = triangleVertex;
                    break;
                }
            }

            assert(NoneElementIndex != endpoint2Index);


            //
            // See if there's a B-C spring
            //

            auto const traverseSpringIt = pointPairToSpringMap.find({ endpoint1Index, endpoint2Index });
            if (traverseSpringIt != pointPairToSpringMap.end())
            {
                // We have a traverse spring

                assert(0 == springInfos2[traverseSpringIt->second].SuperTriangles2.size());

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

Physics::Springs ShipFactory::CreateSprings(
    std::vector<ShipFactorySpring> const & springInfos2,
    Physics::Points & points,
    std::vector<ElementIndex> const & pointIndexRemap,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters)
{
    Physics::Springs springs(
        static_cast<ElementIndex>(springInfos2.size()),
        parentWorld,
        std::move(gameEventDispatcher),
        gameParameters);

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        // Create spring
        springs.Add(
            pointIndexRemap[springInfos2[s].PointAIndex1],
            pointIndexRemap[springInfos2[s].PointBIndex1],
            springInfos2[s].PointAAngle,
            springInfos2[s].PointBAngle,
            springInfos2[s].SuperTriangles2,
            springInfos2[s].CoveringTrianglesCount,
            points);

        // Add spring to its endpoints
        points.AddFactoryConnectedSpring(
            pointIndexRemap[springInfos2[s].PointAIndex1],
            s,
            pointIndexRemap[springInfos2[s].PointBIndex1]);
        points.AddFactoryConnectedSpring(
            pointIndexRemap[springInfos2[s].PointBIndex1],
            s,
            pointIndexRemap[springInfos2[s].PointAIndex1]);
    }

    return springs;
}

Physics::Triangles ShipFactory::CreateTriangles(
    std::vector<ShipFactoryTriangle> const & triangleInfos2,
    Physics::Points & points,
    std::vector<ElementIndex> const & pointIndexRemap)
{
    Physics::Triangles triangles(static_cast<ElementIndex>(triangleInfos2.size()));

    for (ElementIndex t = 0; t < triangleInfos2.size(); ++t)
    {
        assert(triangleInfos2[t].SubSprings2.size() == 3);

        // Create triangle
        triangles.Add(
            pointIndexRemap[triangleInfos2[t].PointIndices1[0]],
            pointIndexRemap[triangleInfos2[t].PointIndices1[1]],
            pointIndexRemap[triangleInfos2[t].PointIndices1[2]],
            triangleInfos2[t].SubSprings2[0],
            triangleInfos2[t].SubSprings2[1],
            triangleInfos2[t].SubSprings2[2],
            triangleInfos2[t].CoveredTraverseSpringIndex2);

        // Add triangle to its endpoints
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[0]], t, true); // Owner
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[1]], t, false); // Not owner
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[2]], t, false); // Not owner
    }

    return triangles;
}

ElectricalElements ShipFactory::CreateElectricalElements(
    Physics::Points const & points,
    std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
    ElectricalPanel const & electricalPanel,
    ShipId shipId,
    Physics::World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters)
{
    //
    // Verify all panel metadata indices are valid instance IDs
    //

    for (auto const & entry : electricalPanel)
    {
        if (std::find(
            electricalElementInstanceIndices.cbegin(),
            electricalElementInstanceIndices.cend(),
            entry.first) == electricalElementInstanceIndices.cend())
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
    for (auto pointIndex : points)
    {
        ElectricalMaterial const * const electricalMaterial = points.GetElectricalMaterial(pointIndex);
        if (nullptr != electricalMaterial)
        {
            auto const instanceIndex = electricalElementInstanceIndices[pointIndex];

            // Get panel metadata
            std::optional<ElectricalPanel::ElementMetadata> panelElementMetadata;
            if (electricalMaterial->IsInstanced)
            {
                assert(NoneElectricalElementInstanceIndex != instanceIndex);

                auto const findIt = electricalPanel.Find(instanceIndex);
                if (findIt != electricalPanel.end())
                {
                    // Take metadata
                    panelElementMetadata = findIt->second;
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
        gameEventDispatcher,
        gameParameters);

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
        auto const pa = points.GetPosition(triangles.GetPointAIndex(t));
        auto const pb = points.GetPosition(triangles.GetPointBIndex(t));
        auto const pc = points.GetPosition(triangles.GetPointCIndex(t));

        Verify((pb.x - pa.x) * (pc.y - pa.y) - (pc.x - pa.x) * (pb.y - pa.y) < 0);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Reordering
//////////////////////////////////////////////////////////////////////////////////////////////////

template <int StripeLength>
ShipFactory::ReorderingResults ShipFactory::ReorderPointsAndSpringsOptimally_Stripes(
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix)
{
    //
    // 1. Visit the point matrix by all rows, from top to bottom
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);
    std::vector<ShipFactoryPoint> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());
    std::vector<ElementIndex> pointIndexRemap(pointInfos1.size(), NoneElementIndex);

    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);
    std::vector<ShipFactorySpring> springInfos2;
    springInfos2.reserve(springInfos1.size());
    std::vector<ElementIndex> springIndexRemap(springInfos1.size(), NoneElementIndex);

    // From top to bottom, starting at second row from top (i.e. first real row)
    for (int y = pointIndexMatrix.height - 1; y >= 1; y -= (StripeLength - 1))
    {
        ReorderPointsAndSpringsOptimally_Stripes_Stripe<StripeLength>(
            y,
            pointInfos1,
            reorderedPointInfos1,
            springInfos1,
            reorderedSpringInfos1,
            pointIndexMatrix,
            pointPairToSpringIndex1Map,
            pointInfos2,
            pointIndexRemap,
            springInfos2,
            springIndexRemap);
    }


    //
    // 2. Add/Sort leftovers
    //
    // At this moment leftovers are:
    //  - Points: rope endpoints (because unreachable via matrix)
    //  - Springs: spring connecting points on left edge of ship with points SW of those points, and rope springs
    //

    // LogMessage(" Leftover points: ", pointInfos1.size() - pointInfos2.size() , " springs: ", springInfos1.size() - springInfos2.size());

    // Here we use a greedy algorithm: for each not-yet-reordered point we add
    // all of its connected springs that are still not reordered
    for (size_t pointIndex1 = 0; pointIndex1 < pointInfos1.size(); ++pointIndex1)
    {
        if (!reorderedPointInfos1[pointIndex1])
        {
            // Add/sort point
            pointIndexRemap[pointIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[pointIndex1]);

            // Visit all connected not-yet-reordered springs
            for (ElementIndex springIndex1 : pointInfos1[pointIndex1].ConnectedSprings1)
            {
                if (!reorderedSpringInfos1[springIndex1])
                {
                    // Add/sort spring
                    springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos2.size());
                    springInfos2.push_back(springInfos1[springIndex1]);

                    // Don't reorder this spring again
                    reorderedSpringInfos1[springIndex1] = true;
                }
            }
        }
    }

    // Finally add all not-yet-reordered springs
    for (size_t springIndex1 = 0; springIndex1 < springInfos1.size(); ++springIndex1)
    {
        if (!reorderedSpringInfos1[springIndex1])
        {
            // Add/sort spring
            springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos2.size());
            springInfos2.push_back(springInfos1[springIndex1]);
        }
    }

    //
    // 3. Return results
    //

    assert(pointInfos2.size() == pointInfos1.size());
    assert(pointIndexRemap.size() == pointInfos1.size());
    assert(springInfos2.size() == springInfos1.size());
    assert(springIndexRemap.size() == springInfos1.size());

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2, springIndexRemap);
}

template <int StripeLength>
void ShipFactory::ReorderPointsAndSpringsOptimally_Stripes_Stripe(
    int y,
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<bool> & reorderedPointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1,
    std::vector<bool> & reorderedSpringInfos1,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    std::vector<ShipFactoryPoint> & pointInfos2,
    std::vector<ElementIndex> & pointIndexRemap,
    std::vector<ShipFactorySpring> & springInfos2,
    std::vector<ElementIndex> & springIndexRemap)
{
    //
    // Collect points in a vertical stripe - 2 cols wide, StripeLength high
    //

    std::vector<ElementIndex> stripePointIndices1;

    // From left to right, start at first real col
    for (int x1 = 1; x1 < pointIndexMatrix.width - 1; ++x1)
    {
        //
        // 1. Build sets of indices of points left and right of the stripe
        //

        stripePointIndices1.clear();

        // From top to bottom
        for (int y1 = y; y1 > y - StripeLength && y1 >= 1; --y1)
        {
            // Check if left exists
            if (!!pointIndexMatrix[{x1, y1}])
            {
                stripePointIndices1.push_back(*(pointIndexMatrix[{x1, y1}]));
            }

            // Check if right exists
            if (!!pointIndexMatrix[{x1 + 1, y1}])
            {
                stripePointIndices1.push_back(*(pointIndexMatrix[{x1 + 1, y1}]));
            }
        }


        //
        // 2. Add/sort all not yet reordered springs connecting all points among themselves
        //

        for (int i1 = 0; i1 < static_cast<int>(stripePointIndices1.size()) - 1; ++i1)
        {
            for (int i2 = i1 + 1; i2 < static_cast<int>(stripePointIndices1.size()); ++i2)
            {
                auto const springIndexIt = pointPairToSpringIndex1Map.find({ stripePointIndices1[i1], stripePointIndices1[i2] });
                if (springIndexIt != pointPairToSpringIndex1Map.end())
                {
                    ElementIndex const springIndex1 = springIndexIt->second;
                    if (!reorderedSpringInfos1[springIndex1])
                    {
                        springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos2.size());
                        springInfos2.push_back(springInfos1[springIndex1]);

                        // Don't reorder this spring again
                        reorderedSpringInfos1[springIndex1] = true;
                    }
                }
            }
        }


        //
        // 3. Add/sort all not yet reordered points among all these points
        //

        for (ElementIndex pointIndex1 : stripePointIndices1)
        {
            if (!reorderedPointInfos1[pointIndex1])
            {
                pointIndexRemap[pointIndex1] = static_cast<ElementIndex>(pointInfos2.size());
                pointInfos2.push_back(pointInfos1[pointIndex1]);

                // Don't reorder this point again
                reorderedPointInfos1[pointIndex1] = true;
            }
        }
    }
}

ShipFactory::ReorderingResults ShipFactory::ReorderPointsAndSpringsOptimally_Blocks(
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix)
{
    //
    // 1. Visit the point matrix by all rows, from top to bottom
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);
    std::vector<ShipFactoryPoint> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());
    std::vector<ElementIndex> pointIndexRemap(pointInfos1.size(), NoneElementIndex);

    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);
    std::vector<ShipFactorySpring> springInfos2;
    springInfos2.reserve(springInfos1.size());
    std::vector<ElementIndex> springIndexRemap(springInfos1.size(), NoneElementIndex);

    // From top to bottom, starting at second row from top (i.e. first real row),
    // skipping one row of points to ensure full squares
    for (int y = pointIndexMatrix.height - 1; y >= 1; y -= 2)
    {
        ReorderPointsAndSpringsOptimally_Blocks_Row(
            y,
            pointInfos1,
            reorderedPointInfos1,
            springInfos1,
            reorderedSpringInfos1,
            pointIndexMatrix,
            pointPairToSpringIndex1Map,
            pointInfos2,
            pointIndexRemap,
            springInfos2,
            springIndexRemap);
    }


    //
    // 2. Add/Sort leftovers
    //
    // At this moment leftovers are:
    //  - Points: rope endpoints (because unreachable via matrix)
    //  - Springs: spring connecting points on left edge of ship with points SW of those points, and rope springs
    //

    // Here we use a greedy algorithm: for each not-yet-reordered point we add
    // all of its connected springs that are still not reordered
    for (size_t pointIndex1 = 0; pointIndex1 < pointInfos1.size(); ++pointIndex1)
    {
        if (!reorderedPointInfos1[pointIndex1])
        {
            // Add/sort point
            pointIndexRemap[pointIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[pointIndex1]);

            // Visit all connected not-yet-reordered springs
            for (ElementIndex springIndex1 : pointInfos1[pointIndex1].ConnectedSprings1)
            {
                if (!reorderedSpringInfos1[springIndex1])
                {
                    // Add/sort spring
                    springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos2.size());
                    springInfos2.push_back(springInfos1[springIndex1]);

                    // Don't reorder this spring again
                    reorderedSpringInfos1[springIndex1] = true;
                }
            }
        }
    }

    // Finally add all not-yet-reordered springs
    for (size_t springIndex1 = 0; springIndex1 < springInfos1.size(); ++springIndex1)
    {
        if (!reorderedSpringInfos1[springIndex1])
        {
            // Add/sort spring
            springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos2.size());
            springInfos2.push_back(springInfos1[springIndex1]);
        }
    }

    //
    // 3. Return results
    //

    assert(pointInfos2.size() == pointInfos1.size());
    assert(pointIndexRemap.size() == pointInfos1.size());
    assert(springInfos2.size() == springInfos1.size());
    assert(springIndexRemap.size() == springInfos1.size());

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2, springIndexRemap);
}

void ShipFactory::ReorderPointsAndSpringsOptimally_Blocks_Row(
    int y,
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<bool> & reorderedPointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1,
    std::vector<bool> & reorderedSpringInfos1,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix,
    PointPairToIndexMap const & pointPairToSpringIndex1Map,
    std::vector<ShipFactoryPoint> & pointInfos2,
    std::vector<ElementIndex> & pointIndexRemap,
    std::vector<ShipFactorySpring> & springInfos2,
    std::vector<ElementIndex> & springIndexRemap)
{
    //
    // Visit each square as follows:
    //
    //  b----c
    //  |    |
    //  a----d
    //
    // ...where b is the current point

    std::vector<ElementIndex> squarePointIndices1;

    // From left to right, start at first real col
    for (int x = 1; x < pointIndexMatrix.width - 1; ++x)
    {
        squarePointIndices1.clear();

        // Check if b exists
        if (!!pointIndexMatrix[{x, y}])
        {
            //
            // 1. Collect all the points that we have around this square
            //

            // Add a if it exists
            if (!!pointIndexMatrix[{x, y - 1}])
                squarePointIndices1.push_back(*(pointIndexMatrix[{x, y - 1}]));

            // Add b
            squarePointIndices1.push_back(*(pointIndexMatrix[{x, y}]));

            // Add c if it exists
            if (!!pointIndexMatrix[{x + 1, y}])
                squarePointIndices1.push_back(*(pointIndexMatrix[{x + 1, y}]));

            // Add d if it exists
            if (!!pointIndexMatrix[{x + 1, y - 1}])
                squarePointIndices1.push_back(*(pointIndexMatrix[{x + 1, y - 1}]));

            //
            // 2. Add/sort all existing, not-yet-reordered springs among all these points
            //

            for (size_t i1 = 0; i1 < squarePointIndices1.size() - 1; ++i1)
            {
                for (size_t i2 = i1 + 1; i2 < squarePointIndices1.size(); ++i2)
                {
                    auto const springIndexIt = pointPairToSpringIndex1Map.find({ squarePointIndices1[i1], squarePointIndices1[i2] });
                    if (springIndexIt != pointPairToSpringIndex1Map.end())
                    {
                        ElementIndex const springIndex1 = springIndexIt->second;
                        if (!reorderedSpringInfos1[springIndex1])
                        {
                            springIndexRemap[springIndex1] = static_cast<ElementIndex>(springInfos1.size());
                            springInfos2.push_back(springInfos1[springIndex1]);

                            // Don't reorder this spring again
                            reorderedSpringInfos1[springIndex1] = true;
                        }
                    }
                }
            }


            //
            // 3. Add/sort all not yet reordered points among all these points
            //

            for (ElementIndex pointIndex1 : squarePointIndices1)
            {
                if (!reorderedPointInfos1[pointIndex1])
                {
                    pointIndexRemap[pointIndex1] = static_cast<ElementIndex>(pointInfos2.size());
                    pointInfos2.push_back(pointInfos1[pointIndex1]);

                    // Don't reorder this point again
                    reorderedPointInfos1[pointIndex1] = true;
                }
            }
        }
    }
}

template <int BlockSize>
ShipFactory::ReorderingResults ShipFactory::ReorderPointsAndSpringsOptimally_Tiling(
    std::vector<ShipFactoryPoint> const & pointInfos1,
    std::vector<ShipFactorySpring> const & springInfos1,
    ShipFactoryPointIndexMatrix const & pointIndexMatrix)
{
    //
    // 1. Visit the point matrix in 2x2 blocks, and add all springs connected to any
    // of the included points (0..4 points), except for already-added ones
    //

    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);
    std::vector<ShipFactorySpring> springInfos2;
    springInfos2.reserve(springInfos1.size());
    std::vector<ElementIndex> springIndexRemap(springInfos1.size(), NoneElementIndex);

    // From bottom to top
    for (int y = 1; y < pointIndexMatrix.height - 1; y += BlockSize)
    {
        for (int x = 1; x < pointIndexMatrix.width - 1; x += BlockSize)
        {
            for (int y2 = 0; y2 < BlockSize && y + y2 < pointIndexMatrix.height - 1; ++y2)
            {
                for (int x2 = 0; x2 < BlockSize && x + x2 < pointIndexMatrix.width - 1; ++x2)
                {
                    if (!!pointIndexMatrix[{x + x2, y + y2}])
                    {
                        ElementIndex pointIndex = *(pointIndexMatrix[{x + x2, y + y2}]);

                        // Add all springs connected to this point
                        for (auto connectedSpringIndex1 : pointInfos1[pointIndex].ConnectedSprings1)
                        {
                            if (!reorderedSpringInfos1[connectedSpringIndex1])
                            {
                                springIndexRemap[connectedSpringIndex1] = static_cast<ElementIndex>(springInfos2.size());
                                springInfos2.push_back(springInfos1[connectedSpringIndex1]);
                                reorderedSpringInfos1[connectedSpringIndex1] = true;
                            }
                        }
                    }
                }
            }
        }
    }

    //
    // 2. Add all remaining springs
    //

    for (size_t s = 0; s < springInfos1.size(); ++s)
    {
        if (!reorderedSpringInfos1[s])
        {
            springIndexRemap[s] = static_cast<ElementIndex>(springInfos2.size());
            springInfos2.push_back(springInfos1[s]);
        }
    }

    assert(springInfos2.size() == springInfos1.size());
    assert(springIndexRemap.size() == springInfos1.size());


    //
    // 3. Order points in the order they first appear when visiting springs linearly
    //
    // a.k.a. Bas van den Berg's optimization!
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);
    std::vector<ShipFactoryPoint> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());
    std::vector<ElementIndex> pointIndexRemap(pointInfos1.size(), NoneElementIndex);

    for (auto const & springInfo : springInfos2)
    {
        if (!reorderedPointInfos1[springInfo.PointAIndex1])
        {
            pointIndexRemap[springInfo.PointAIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[springInfo.PointAIndex1]);
            reorderedPointInfos1[springInfo.PointAIndex1] = true;
        }

        if (!reorderedPointInfos1[springInfo.PointBIndex1])
        {
            pointIndexRemap[springInfo.PointBIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[springInfo.PointBIndex1]);
            reorderedPointInfos1[springInfo.PointBIndex1] = true;
        }
    }


    //
    // Add missing points
    //

    for (ElementIndex p = 0; p < pointInfos1.size(); ++p)
    {
        if (!reorderedPointInfos1[p])
        {
            pointIndexRemap[p] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[p]);
        }
    }

    assert(pointInfos2.size() == pointInfos1.size());
    assert(pointIndexRemap.size() == pointInfos1.size());


    //
    // 4. Return results
    //

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2, springIndexRemap);
}

std::vector<ShipFactorySpring> ShipFactory::ReorderSpringsOptimally_TomForsyth(
    std::vector<ShipFactorySpring> const & springInfos1,
    size_t pointCount)
{
    std::vector<VertexData> vertexData(pointCount);
    std::vector<ElementData> elementData(springInfos1.size());

    // Fill-in cross-references between vertices and springs
    for (size_t s = 0; s < springInfos1.size(); ++s)
    {
        vertexData[springInfos1[s].PointAIndex1].RemainingElementIndices.push_back(s);
        vertexData[springInfos1[s].PointBIndex1].RemainingElementIndices.push_back(s);

        elementData[s].VertexIndices.push_back(static_cast<size_t>(springInfos1[s].PointAIndex1));
        elementData[s].VertexIndices.push_back(static_cast<size_t>(springInfos1[s].PointBIndex1));
    }

    // Get optimal indices
    auto optimalIndices = ReorderOptimally<2>(
        vertexData,
        elementData);

    // Build optimally-ordered set of springs
    std::vector<ShipFactorySpring> springInfos2;
    springInfos2.reserve(springInfos1.size());
    for (size_t ti : optimalIndices)
    {
        springInfos2.push_back(springInfos1[ti]);
    }

    return springInfos2;
}

std::vector<ShipFactoryTriangle> ShipFactory::ReorderTrianglesOptimally_ReuseOptimization(
    std::vector<ShipFactoryTriangle> const & triangleInfos1,
    size_t /*pointCount*/)
{
    std::vector<ShipFactoryTriangle> triangleInfos2;
    triangleInfos2.reserve(triangleInfos1.size());

    std::array<ElementIndex, 3> previousVertices;

    std::vector<bool> reorderedTriangles(triangleInfos1.size(), false);


    //
    // 1) Add triangles that have in common 2 vertices with the previous one
    //

    assert(triangleInfos1.size() > 0);

    triangleInfos2.push_back(triangleInfos1[0]);
    reorderedTriangles[0] = true;
    std::copy(
        triangleInfos1[0].PointIndices1.cbegin(),
        triangleInfos1[0].PointIndices1.cend(),
        previousVertices.begin());

    for (size_t t = 1; t < triangleInfos1.size(); ++t)
    {
        std::optional<ElementIndex> chosenTriangle;
        std::optional<ElementIndex> spareTriangle;
        for (size_t t2 = 1; t2 < triangleInfos1.size(); ++t2)
        {
            if (!reorderedTriangles[t2])
            {
                size_t commonVertices = std::count_if(
                    triangleInfos1[t2].PointIndices1.cbegin(),
                    triangleInfos1[t2].PointIndices1.cend(),
                    [&previousVertices](ElementIndex v)
                    {
                        return std::any_of(
                            previousVertices.cbegin(),
                            previousVertices.cend(),
                            [v](ElementIndex v2)
                            {
                                return v2 == v;
                            });
                    });

                if (commonVertices == 2)
                {
                    chosenTriangle = static_cast<ElementIndex>(t2);
                    break;
                }

                // Remember first spare
                if (!spareTriangle)
                    spareTriangle = static_cast<ElementIndex>(t2);
            }
        }

        if (!chosenTriangle)
        {
            // Choose first non-reordeded triangle
            assert(!!spareTriangle);
            chosenTriangle = spareTriangle;
        }

        //
        // Use this triangle
        //

        triangleInfos2.push_back(triangleInfos1[*chosenTriangle]);
        reorderedTriangles[*chosenTriangle] = true;

        std::copy(
            triangleInfos1[*chosenTriangle].PointIndices1.cbegin(),
            triangleInfos1[*chosenTriangle].PointIndices1.cend(),
            previousVertices.begin());
    }

    assert(triangleInfos2.size() == triangleInfos1.size());

    return triangleInfos2;
}

std::vector<ShipFactoryTriangle> ShipFactory::ReorderTrianglesOptimally_TomForsyth(
    std::vector<ShipFactoryTriangle> const & triangleInfos1,
    size_t pointCount)
{
    std::vector<VertexData> vertexData(pointCount);
    std::vector<ElementData> elementData(triangleInfos1.size());

    // Fill-in cross-references between vertices and triangles
    for (size_t t = 0; t < triangleInfos1.size(); ++t)
    {
        vertexData[triangleInfos1[t].PointIndices1[0]].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos1[t].PointIndices1[1]].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos1[t].PointIndices1[2]].RemainingElementIndices.push_back(t);

        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos1[t].PointIndices1[0]));
        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos1[t].PointIndices1[1]));
        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos1[t].PointIndices1[2]));
    }

    // Get optimal indices
    auto optimalIndices = ReorderOptimally<3>(
        vertexData,
        elementData);

    // Build optimally-ordered set of triangles
    std::vector<ShipFactoryTriangle> triangleInfos2;
    triangleInfos2.reserve(triangleInfos1.size());
    for (size_t ti : optimalIndices)
    {
        triangleInfos2.push_back(triangleInfos1[ti]);
    }

    return triangleInfos2;
}

float ShipFactory::CalculateACMR(std::vector<ShipFactorySpring> const & springInfos)
{
    //size_t constexpr IntervalReportingSamples = 1000;

    //
    // Calculate the average cache miss ratio
    //

    if (springInfos.empty())
    {
        return 0.0f;
    }

    TestLRUVertexCache<VertexCacheSize> cache;

    float cacheMisses = 0.0f;
    //float intervalCacheMisses = 0.0f;

    for (size_t s = 0; s < springInfos.size(); ++s)
    {
        if (!cache.UseVertex(springInfos[s].PointAIndex1))
        {
            cacheMisses += 1.0f;
            //intervalCacheMisses += 1.0f;
        }

        if (!cache.UseVertex(springInfos[s].PointBIndex1))
        {
            cacheMisses += 1.0f;
            //intervalCacheMisses += 1.0f;
        }

        ////if (s < 6 + 5 + 5)
        ////    LogMessage(s, ":", cacheMisses, " (", springInfos[s].PointAIndex1, " -> ", springInfos[s].PointBIndex1, ")");

        ////if (s > 0 && 0 == (s % IntervalReportingSamples))
        ////{
        ////    LogMessage("   ACMR @ ", s, ": T-avg=", static_cast<float>(intervalCacheMisses) / static_cast<float>(IntervalReportingSamples),
        ////        " So-far=", static_cast<float>(cacheMisses) / static_cast<float>(s));
        ////    intervalCacheMisses = 0.0f;
        ////}
    }

    return cacheMisses / static_cast<float>(springInfos.size());
}

float ShipFactory::CalculateACMR(std::vector<ShipFactoryTriangle> const & triangleInfos)
{
    //
    // Calculate the average cache miss ratio
    //

    if (triangleInfos.empty())
    {
        return 0.0f;
    }

    TestLRUVertexCache<VertexCacheSize> cache;

    float cacheMisses = 0.0f;

    for (auto const & triangleInfo : triangleInfos)
    {
        if (!cache.UseVertex(triangleInfo.PointIndices1[0]))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointIndices1[1]))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointIndices1[2]))
        {
            cacheMisses += 1.0f;
        }
    }

    return cacheMisses / static_cast<float>(triangleInfos.size());
}

float ShipFactory::CalculateVertexMissRatio(std::vector<ShipFactoryTriangle> const & triangleInfos)
{
    //
    // Ratio == 0 iff all triangles have two vertices in common with the previous triangle
    //

    std::array<ElementIndex, 3> previousVertices{ triangleInfos[0].PointIndices1 };

    float sumMisses = 0.0f;
    for (size_t t = 1; t < triangleInfos.size(); ++t)
    {
        auto commonVertices = std::count_if(
            triangleInfos[t].PointIndices1.cbegin(),
            triangleInfos[t].PointIndices1.cend(),
            [&previousVertices](ElementIndex v)
            {
                return std::any_of(
                    previousVertices.cbegin(),
                    previousVertices.cend(),
                    [v](ElementIndex v2)
                    {
                        return v2 == v;
                    });
            });

        assert(commonVertices <= 2.0f);

        sumMisses += 2.0f - static_cast<float>(commonVertices);

        std::copy(
            triangleInfos[t].PointIndices1.cbegin(),
            triangleInfos[t].PointIndices1.cend(),
            previousVertices.begin());
    }

    return sumMisses / (2.0f * static_cast<float>(triangleInfos.size()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex cache optimization
//////////////////////////////////////////////////////////////////////////////////////////////////

template <size_t VerticesInElement>
std::vector<size_t> ShipFactory::ReorderOptimally(
    std::vector<VertexData> & vertexData,
    std::vector<ElementData> & elementData)
{
    // Calculate vertex scores
    for (VertexData & v : vertexData)
    {
        v.CurrentScore = CalculateVertexScore<VerticesInElement>(v);
    }

    // Calculate element scores, remembering best so far
    float bestElementScore = std::numeric_limits<float>::lowest();
    std::optional<size_t> bestElementIndex(std::nullopt);
    for (size_t ei = 0; ei < elementData.size(); ++ei)
    {
        for (size_t vi : elementData[ei].VertexIndices)
        {
            elementData[ei].CurrentScore += vertexData[vi].CurrentScore;
        }

        if (elementData[ei].CurrentScore > bestElementScore)
        {
            bestElementScore = elementData[ei].CurrentScore;
            bestElementIndex = ei;
        }
    }


    //
    // Main loop - run until we've drawn all elements
    //

    std::list<size_t> modelLruVertexCache;

    std::vector<size_t> optimalElementIndices;
    optimalElementIndices.reserve(elementData.size());

    while (optimalElementIndices.size() < elementData.size())
    {
        //
        // Find best element
        //

        if (!bestElementIndex)
        {
            // Have to find best element
            bestElementScore = std::numeric_limits<float>::lowest();
            for (size_t ei = 0; ei < elementData.size(); ++ei)
            {
                if (!elementData[ei].HasBeenDrawn
                    && elementData[ei].CurrentScore > bestElementScore)
                {
                    bestElementScore = elementData[ei].CurrentScore > bestElementScore;
                    bestElementIndex = ei;
                }
            }
        }

        assert(!!bestElementIndex);
        assert(!elementData[*bestElementIndex].HasBeenDrawn);

        // Add the best element to the optimal list
        optimalElementIndices.push_back(*bestElementIndex);

        // Mark the best element as drawn
        elementData[*bestElementIndex].HasBeenDrawn = true;

        // Update all of the element's vertices
        for (auto vi : elementData[*bestElementIndex].VertexIndices)
        {
            // Remove the best element element from the lists of remaining elements for this vertex
            vertexData[vi].RemainingElementIndices.erase(
                std::remove(
                    vertexData[vi].RemainingElementIndices.begin(),
                    vertexData[vi].RemainingElementIndices.end(),
                    *bestElementIndex));

            // Update the LRU cache with this vertex
            AddVertexToCache(vi, modelLruVertexCache);
        }

        // Re-assign positions and scores of all vertices in the cache
        size_t currentCachePosition = 0;
        for (auto it = modelLruVertexCache.begin(); it != modelLruVertexCache.end(); ++it, ++currentCachePosition)
        {
            vertexData[*it].CachePosition = (currentCachePosition < VertexCacheSize)
                ? static_cast<int32_t>(currentCachePosition)
                : -1;

            vertexData[*it].CurrentScore = CalculateVertexScore<VerticesInElement>(vertexData[*it]);

            // Zero the score of this vertices' elements, as we'll be updating it next
            for (size_t ei : vertexData[*it].RemainingElementIndices)
            {
                elementData[ei].CurrentScore = 0.0f;
            }
        }

        // Update scores of all elements in the cache, maintaining best score at the same time
        bestElementScore = std::numeric_limits<float>::lowest();
        bestElementIndex = std::nullopt;
        for (size_t vi : modelLruVertexCache)
        {
            for (size_t ei : vertexData[vi].RemainingElementIndices)
            {
                assert(!elementData[ei].HasBeenDrawn);

                // Add this vertex's score to the element's score
                elementData[ei].CurrentScore += vertexData[vi].CurrentScore;

                // Check if best so far
                if (elementData[ei].CurrentScore > bestElementScore)
                {
                    bestElementScore = elementData[ei].CurrentScore;
                    bestElementIndex = ei;
                }
            }
        }

        // Shrink cache back to its size
        if (modelLruVertexCache.size() > VertexCacheSize)
        {
            modelLruVertexCache.resize(VertexCacheSize);
        }
    }

    return optimalElementIndices;
}

void ShipFactory::AddVertexToCache(
    size_t vertexIndex,
    ModelLRUVertexCache & cache)
{
    for (auto it = cache.begin(); it != cache.end(); ++it)
    {
        if (vertexIndex == *it)
        {
            // It's already in the cache...
            // ...move it to front
            cache.erase(it);
            cache.push_front(vertexIndex);

            return;
        }
    }

    // Not in the cache...
    // ...insert in front of cache
    cache.push_front(vertexIndex);
}

template <size_t VerticesInElement>
float ShipFactory::CalculateVertexScore(VertexData const & vertexData)
{
    static_assert(VerticesInElement < VertexCacheSize);

    //
    // Almost verbatim from Tom Forsyth
    //

    static constexpr float FindVertexScore_CacheDecayPower = 1.5f;
    static constexpr float FindVertexScore_LastElementScore = 0.75f;
    static constexpr float FindVertexScore_ValenceBoostScale = 2.0f;
    static constexpr float FindVertexScore_ValenceBoostPower = 0.5f;

    if (vertexData.RemainingElementIndices.size() == 0)
    {
        // No elements left using this vertex, give it a bad score
        return -1.0f;
    }

    float score = 0.0f;
    if (vertexData.CachePosition >= 0)
    {
        // This vertex is in the cache

        if (static_cast<size_t>(vertexData.CachePosition) < VerticesInElement)
        {
            // This vertex was used in the last element,
            // so it has a fixed score, whichever of the vertices
            // it is. Otherwise, you can get very different
            // answers depending on whether you add, for example,
            // a triangle's 1,2,3 or 3,1,2 - which is silly.
            score = FindVertexScore_LastElementScore;
        }
        else
        {
            assert(static_cast<size_t>(vertexData.CachePosition) < VertexCacheSize);

            // Score vertices high for being high in the cache
            float const scaler = 1.0f / (VertexCacheSize - VerticesInElement);
            score = 1.0f - (vertexData.CachePosition - VerticesInElement) * scaler;
            score = powf(score, FindVertexScore_CacheDecayPower);
        }
    }

    // Bonus points for having a low number of elements still
    // using this vertex, so we get rid of lone vertices quickly
    float valenceBoost = powf(
        static_cast<float>(vertexData.RemainingElementIndices.size()),
        -FindVertexScore_ValenceBoostPower);
    score += FindVertexScore_ValenceBoostScale * valenceBoost;

    return score;
}

template<size_t Size>
bool ShipFactory::TestLRUVertexCache<Size>::UseVertex(size_t vertexIndex)
{
    for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it)
    {
        if (vertexIndex == *it)
        {
            // It's already in the cache...
            // ...move it to front
            mEntries.erase(it);
            mEntries.push_front(vertexIndex);

            // It was a cache hit
            return true;
        }
    }

    // Not in the cache...
    // ...insert in front of cache
    mEntries.push_front(vertexIndex);

    // Trim
    while (mEntries.size() > Size)
    {
        mEntries.pop_back();
    }

    // It was a cache miss
    return false;
}

template<size_t Size>
std::optional<size_t> ShipFactory::TestLRUVertexCache<Size>::GetCachePosition(size_t vertexIndex)
{
    size_t position = 0;
    for (auto const & vi : mEntries)
    {
        if (vi == vertexIndex)
        {
            // Found!
            return position;
        }

        ++position;
    }

    // Not found
    return std::nullopt;
}