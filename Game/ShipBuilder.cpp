/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
*
* Contains code from Tom Forsyth - https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
***************************************************************************************/
#include "ShipBuilder.h"

#include <GameCore/ImageTools.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <map>
#include <sstream>
#include <unordered_map>
#include <utility>

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Ship> ShipBuilder::Create(
    ShipId shipId,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    std::shared_ptr<TaskThreadPool> taskThreadPool,
    ShipDefinition const & shipDefinition,
    MaterialDatabase const & materialDatabase,
    GameParameters const & gameParameters)
{
    int const structureWidth = shipDefinition.StructuralLayerImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth) / 2.0f;
    int const structureHeight = shipDefinition.StructuralLayerImage.Size.Height;

    // PointInfo's
    std::vector<PointInfo> pointInfos;

    // SpringInfo's
    std::vector<SpringInfo> springInfos;

    // RopeSegment's, indexed by the rope color key
    std::map<MaterialDatabase::ColorKey, RopeSegment> ropeSegments;

    // TriangleInfo's
    std::vector<TriangleInfo> triangleInfos;


    //
    // Process structural layer points and:
    // - Identify all points, calculate texture coordinates, and create PointInfo's for them
    // - Build a 2D matrix containing indices to the points above
    // - Identify rope endpoints on structural layer, and create RopeSegment's for them
    //

    // Matrix of points - we allocate 2 extra dummy rows and cols - around - to avoid checking for boundaries
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> pointIndexMatrix(new std::unique_ptr<std::optional<ElementIndex>[]>[structureWidth + 2]);
    for (int c = 0; c < structureWidth + 2; ++c)
    {
        pointIndexMatrix[c] = std::unique_ptr<std::optional<ElementIndex>[]>(new std::optional<ElementIndex>[structureHeight + 2]);
    }

    // Visit all columns
    for (int x = 0; x < structureWidth; ++x)
    {
        // From bottom to top
        for (int y = 0; y < structureHeight; ++y)
        {
            MaterialDatabase::ColorKey const colorKey = shipDefinition.StructuralLayerImage.Data[x + y * structureWidth];
            StructuralMaterial const * structuralMaterial = materialDatabase.FindStructuralMaterial(colorKey);
            if (nullptr != structuralMaterial)
            {
                float water = 0.0f;

                //
                // Transform water point to air point+water
                //

                if (structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Water))
                {
                    structuralMaterial = &materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Air);
                    water = 1.0f;
                }

                //
                // Make a point
                //

                ElementIndex const pointIndex = static_cast<ElementIndex>(pointInfos.size());

                pointIndexMatrix[x + 1][y + 1] = static_cast<ElementIndex>(pointIndex);

                pointInfos.emplace_back(
                    IntegralPoint(x, y),
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y)) + shipDefinition.Metadata.Offset,
                    MakeTextureCoordinates(x, y, shipDefinition.StructuralLayerImage.Size),
                    structuralMaterial->RenderColor,
                    *structuralMaterial,
                    structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope),
                    water);

                //
                // Check if it's a (custom) rope endpoint
                //

                if (structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope)
                    && !materialDatabase.IsUniqueStructuralMaterialColorKey(StructuralMaterial::MaterialUniqueType::Rope, colorKey))
                {
                    // Store in RopeSegments, using the color key as the color of the rope
                    RopeSegment & ropeSegment = ropeSegments[colorKey];
                    if (!ropeSegment.SetEndpoint(pointIndex, colorKey))
                    {
                        throw GameException(
                            "More than two \"" + Utils::RgbColor2Hex(colorKey) + "\" rope endpoints found at "
                            + IntegralPoint(x, y).ToString());
                    }
                }
            }
            else
            {
                // Just ignore this pixel
            }
        }
    }


    //
    // Process the rope layer - if any - and append rope endpoints
    //

    if (!!(shipDefinition.RopesLayerImage))
    {
        // Make sure dimensions match
        if (shipDefinition.RopesLayerImage->Size != shipDefinition.StructuralLayerImage.Size)
        {
            throw GameException("The size of the image used for the ropes layer must match the size of the image used for the structural layer");
        }

        // Append rope endpoints
        AppendRopeEndpoints(
            *(shipDefinition.RopesLayerImage),
            ropeSegments,
            pointInfos,
            pointIndexMatrix,
            materialDatabase,
            shipDefinition.Metadata.Offset);
    }

    //
    // Process the electrical layer - if any - and decorate existing points with electrical materials
    //

    if (!!(shipDefinition.ElectricalLayerImage))
    {
        // Make sure dimensions match
        if (shipDefinition.ElectricalLayerImage->Size != shipDefinition.StructuralLayerImage.Size)
        {
            throw GameException("The size of the image used for the electrical layer must match the size of the image used for the structural layer");
        }

        // Decorate points with electrical materials from the electrical layer
        DecoratePointsWithElectricalMaterials(
            *(shipDefinition.ElectricalLayerImage),
            pointInfos,
            true, // isDedicatedElectricalLayer
            pointIndexMatrix,
            materialDatabase);
    }
    else
    {
        // Decorate points with electrical materials from the structural layer
        DecoratePointsWithElectricalMaterials(
            shipDefinition.StructuralLayerImage,
            pointInfos,
            false, // isDedicatedElectricalLayer
            pointIndexMatrix,
            materialDatabase);
    }

    //
    // Process all identified rope endpoints and:
    // - Fill-in points between the endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
    //

    AppendRopes(
        ropeSegments,
        shipDefinition.StructuralLayerImage.Size,
        materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Rope),
        pointInfos,
        springInfos);


    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded PointInfo's as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    size_t leakingPointsCount;

    CreateShipElementInfos(
        pointIndexMatrix,
        shipDefinition.StructuralLayerImage.Size,
        pointInfos,
        springInfos,
        triangleInfos,
        leakingPointsCount);


    //
    // Optimize order of PointInfo's and SpringInfo's to minimize cache misses
    //

    float originalSpringACMR = CalculateACMR(springInfos);

    // Tiling algorithm
    //auto [pointInfos2, pointIndexRemap2, springInfos2] = ReorderPointsAndSpringsOptimally_Tiling<2>(
    auto [pointInfos2, pointIndexRemap2, springInfos2] = ReorderPointsAndSpringsOptimally_Stripes<4>(
        pointInfos,
        springInfos,
        pointIndexMatrix,
        shipDefinition.StructuralLayerImage.Size);

    float optimizedSpringACMR = CalculateACMR(springInfos2);

    LogMessage("Spring ACMR: original=", originalSpringACMR, ", optimized=", optimizedSpringACMR);


    //
    // Optimize order of Triangles
    //

    // Note: we don't optimize triangles, as tests indicate that performance gets (marginally) worse,
    // and at the same time, it makes sense to use the natural order of the triangles as it ensures
    // that higher elements in the ship cover lower elements when they are semi-detached.

    ////float originalACMR = CalculateACMR(triangleInfos);
    ////float originalVMR = CalculateVertexMissRatio(triangleInfos);

    ////triangleInfos = ReorderTrianglesOptimally_TomForsyth(triangleInfos, pointInfos.size());

    ////float optimizedACMR = CalculateACMR(triangleInfos);
    ////float optimizedVMR = CalculateVertexMissRatio(triangleInfos);

    ////LogMessage("Triangles ACMR: original=", originalACMR, ", optimized=", optimizedACMR);
    ////LogMessage("Triangles VMR: original=", originalVMR, ", optimized=", optimizedVMR);


    //
    // Visit all PointInfo's and create Points, i.e. the entire set of points
    //

    std::vector<ElectricalElementInstanceIndex> electricalElementInstanceIndices;
    Physics::Points points = CreatePoints(
        pointInfos2,
        parentWorld,
        materialDatabase,
        gameEventDispatcher,
        gameParameters,
        electricalElementInstanceIndices);


    //
    // Filter out redundant triangles
    //

    auto triangleInfos2 = FilterOutRedundantTriangles(
        triangleInfos,
        points,
        pointIndexRemap2,
        springInfos2);


    //
    // Associate all springs with the triangles that cover them
    //

    ConnectSpringsAndTriangles(
        springInfos2,
        triangleInfos2);


    //
    // Create Springs for all SpringInfo's
    //

    Springs springs = CreateSprings(
        springInfos2,
        points,
        pointIndexRemap2,
        parentWorld,
        gameEventDispatcher,
        gameParameters);


    //
    // Create Triangles for all (filtered out) TriangleInfo's
    //

    Triangles triangles = CreateTriangles(
        triangleInfos2,
        points,
        pointIndexRemap2);


    //
    // Create Electrical Elements
    //

    ElectricalElements electricalElements = CreateElectricalElements(
        points,
        springs,
        electricalElementInstanceIndices,
        shipDefinition.Metadata.ElectricalPanelMetadata,
        shipId,
        parentWorld,
        gameEventDispatcher,
        gameParameters);


    //
    // We're done!
    //

    LogMessage("Created ship: W=", shipDefinition.StructuralLayerImage.Size.Width, ", H=", shipDefinition.StructuralLayerImage.Size.Height, ", ",
        points.GetRawShipPointCount(), "/", points.GetBufferElementCount(), "buf points, ",
        springs.GetElementCount(), " springs, ", triangles.GetElementCount(), " triangles, ",
        electricalElements.GetElementCount(), " electrical elements.");

    return std::make_unique<Ship>(
        shipId,
        parentWorld,
        materialDatabase,
        std::move(gameEventDispatcher),
        std::move(taskThreadPool),
        std::move(points),
        std::move(springs),
        std::move(triangles),
        std::move(electricalElements));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Building helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

void ShipBuilder::AppendRopeEndpoints(
    RgbImageData const & ropeLayerImage,
    std::map<MaterialDatabase::ColorKey, RopeSegment> & ropeSegments,
    std::vector<PointInfo> & pointInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> & pointIndexMatrix,
    MaterialDatabase const & materialDatabase,
    vec2f const & shipOffset)
{
    int const width = ropeLayerImage.Size.Width;
    float const halfWidth = static_cast<float>(width) / 2.0f;
    int const height = ropeLayerImage.Size.Height;

    constexpr MaterialDatabase::ColorKey BackgroundColorKey = { 0xff, 0xff, 0xff };

    for (int x = 0; x < width; ++x)
    {
        // From bottom to top
        for (int y = 0; y < height; ++y)
        {
            // Get color
            MaterialDatabase::ColorKey colorKey = ropeLayerImage.Data[x + y * width];

            // Check if background
            if (colorKey != BackgroundColorKey)
            {
                // Check whether we have a structural point here
                ElementIndex pointIndex;
                if (!pointIndexMatrix[x + 1][y + 1])
                {
                    // Make a point
                    pointIndex = static_cast<ElementIndex>(pointInfos1.size());
                    pointInfos1.emplace_back(
                        IntegralPoint(x, y),
                        vec2f(
                            static_cast<float>(x) - halfWidth,
                            static_cast<float>(y))
                        + shipOffset,
                        MakeTextureCoordinates(x, y, ropeLayerImage.Size),
                        colorKey.toVec4f(1.0f),
                        materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Rope),
                        true, // IsRope
                        0.0f); // Water

                    pointIndexMatrix[x + 1][y + 1] = pointIndex;
                }
                else
                {
                    pointIndex = *(pointIndexMatrix[x + 1][y + 1]);
                }

                // Make sure we don't have a rope already with an endpoint here
                for (auto const & [srchColorKey, srchRopeSegment] : ropeSegments)
                {
                    if (pointIndex == srchRopeSegment.PointAIndex1
                        || pointIndex == srchRopeSegment.PointBIndex1)
                    {
                        throw GameException("There is already a rope at point " + IntegralPoint(x, y).ToString());
                    }
                }

                // Store in RopeSegments
                RopeSegment & ropeSegment = ropeSegments[colorKey];
                if (!ropeSegment.SetEndpoint(pointIndex, colorKey))
                {
                    throw GameException(
                        "More than two \"" + Utils::RgbColor2Hex(colorKey) + "\" rope endpoints found at "
                        + IntegralPoint(x, y).ToString() + " in the rope layer image");
                }

                // Change endpoint's color to match the rope's - or else the spring will look bad,
                // and make it a rope point so that the first spring segment is a rope spring
                pointInfos1[pointIndex].RenderColor = colorKey.toVec4f(1.0f);
                pointInfos1[pointIndex].IsRope = true;
            }
        }
    }
}

void ShipBuilder::DecoratePointsWithElectricalMaterials(
    RgbImageData const & layerImage,
    std::vector<PointInfo> & pointInfos1,
    bool isDedicatedElectricalLayer,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    MaterialDatabase const & materialDatabase)
{
    int const width = layerImage.Size.Width;
    int const height = layerImage.Size.Height;

    constexpr MaterialDatabase::ColorKey BackgroundColorKey = { 0xff, 0xff, 0xff };

    for (int x = 0; x < width; ++x)
    {
        // From bottom to top
        for (int y = 0; y < height; ++y)
        {
            // Get color
            MaterialDatabase::ColorKey const & colorKey = layerImage.Data[x + y * width];

            // Check if it's an electrical material
            ElectricalMaterial const * const electricalMaterial = materialDatabase.FindElectricalMaterial(colorKey);
            if (nullptr == electricalMaterial)
            {
                //
                // Not an electrical material
                //

                if (isDedicatedElectricalLayer
                    && colorKey != BackgroundColorKey)
                {
                    throw GameException(
                        "Cannot find electrical material for color key \"" + Utils::RgbColor2Hex(colorKey)
                        + "\" of pixel found at " + IntegralPoint(x, y).ToString()
                        + " in the " + (isDedicatedElectricalLayer ? "electrical" : "structural")
                        + " layer image");
                }
                else
                {
                    // Just ignore
                }
            }
            else
            {
                //
                // Electrical material found on this particle
                //

                // Make sure we have a structural point here
                if (!pointIndexMatrix[x + 1][y + 1])
                {
                    throw GameException(
                        "The electrical layer image specifies an electrical material at "
                        + IntegralPoint(x, y).ToString()
                        + ", but no pixel may be found at those coordinates in the structural layer image");
                }

                // Store electrical material
                auto const pointIndex = *(pointIndexMatrix[x + 1][y + 1]);
                assert(nullptr == pointInfos1[pointIndex].ElectricalMtl);
                pointInfos1[pointIndex].ElectricalMtl = electricalMaterial;

                // Store instance index, if material requires one
                if (electricalMaterial->IsInstanced)
                {
                    pointInfos1[pointIndex].ElectricalElementInstanceIndex = MaterialDatabase::GetElectricalElementInstanceIndex(colorKey);
                }
                else
                {
                    assert(pointInfos1[pointIndex].ElectricalElementInstanceIndex == NoneElectricalElementInstanceIndex);
                }
            }
        }
    }

    //
    // Check for duplicate electrical element instance indices
    //

    std::map<ElectricalElementInstanceIndex, IntegralPoint> seenInstanceIndicesToOriginalCoords;
    for (auto const & pi : pointInfos1)
    {
        if (nullptr != pi.ElectricalMtl
            && pi.ElectricalMtl->IsInstanced)
        {
            auto searchIt = seenInstanceIndicesToOriginalCoords.find(pi.ElectricalElementInstanceIndex);
            if (searchIt != seenInstanceIndicesToOriginalCoords.end())
            {
                // Dupe

                assert(pi.OriginalDefinitionCoordinates.has_value()); // Instanced electricals come from layers

                throw GameException(
                    "Found two electrical elements with instance ID \""
                    + std::to_string(pi.ElectricalElementInstanceIndex)
                    + "\" in the electrical layer image, at " + pi.OriginalDefinitionCoordinates->ToString()
                    + " and at " + searchIt->second.ToString() + "; "
                    + " make sure that all instanced elements"
                    + " have unique values for the blue component of their color codes!");
            }
            else
            {
                // First time we see it
                seenInstanceIndicesToOriginalCoords.emplace(
                    pi.ElectricalElementInstanceIndex,
                    *(pi.OriginalDefinitionCoordinates));
            }
        }
    }
}

void ShipBuilder::AppendRopes(
    std::map<MaterialDatabase::ColorKey, RopeSegment> const & ropeSegments,
    ImageSize const & structureImageSize,
    StructuralMaterial const & ropeMaterial,
    std::vector<PointInfo> & pointInfos1,
    std::vector<SpringInfo> & springInfos1)
{
    //
    // - Fill-in points between each pair of endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
    //

    // Visit all RopeSegment's
    for (auto const & ropeSegmentEntry : ropeSegments)
    {
        auto const & ropeSegment = ropeSegmentEntry.second;

        // Make sure we've got both endpoints
        assert(NoneElementIndex != ropeSegment.PointAIndex1);
        if (NoneElementIndex == ropeSegment.PointBIndex1)
        {
            throw GameException(
                std::string("Only one rope endpoint found with color key <")
                + Utils::RgbColor2Hex(ropeSegmentEntry.first)
                + ">");
        }

        // Get endpoint positions
        vec2f startPos = pointInfos1[ropeSegment.PointAIndex1].Position;
        vec2f endPos = pointInfos1[ropeSegment.PointBIndex1].Position;

        // Get endpoint electrical materials

        ElectricalMaterial const * startElectricalMaterial = nullptr;
        if (auto const pointAElectricalMaterial = pointInfos1[ropeSegment.PointAIndex1].ElectricalMtl;
            nullptr != pointAElectricalMaterial
            && (pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Cable
                || pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Generator
                || pointAElectricalMaterial->ElectricalType == ElectricalMaterial::ElectricalElementType::Lamp)
            && !pointAElectricalMaterial->IsInstanced)
            startElectricalMaterial = pointAElectricalMaterial;

        ElectricalMaterial const * endElectricalMaterial = nullptr;
        if (auto const pointBElectricalMaterial = pointInfos1[ropeSegment.PointBIndex1].ElectricalMtl;
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

        float dx = endPos.x - startPos.x;
        float dy = endPos.y - startPos.y;
        bool widestIsX;
        float slope;
        float startW, startN;
        float endW;
        float stepW;
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

        auto curStartPointIndex = ropeSegment.PointAIndex1;
        while (true)
        {
            curW += stepW;
            curN += slope * stepW;

            if (fabs(endW - curW) <= 0.5f)
            {
                // Reached destination
                break;
            }

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

            auto newPointIndex = static_cast<ElementIndex>(pointInfos1.size());

            // Add SpringInfo
            ElementIndex const springIndex = static_cast<ElementIndex>(springInfos1.size());
            springInfos1.emplace_back(
                curStartPointIndex,
                factoryDirectionStart,
                newPointIndex,
                factoryDirectionEnd);

            // Add PointInfo
            pointInfos1.emplace_back(
                std::nullopt,
                newPosition,
                MakeTextureCoordinates(newPosition.x, newPosition.y, structureImageSize),
                ropeSegment.RopeColorKey.toVec4f(1.0f),
                ropeMaterial,
                true, // IsRope
                0.0f); // Water

            // Set electrical material
            pointInfos1.back().ElectricalMtl = (fabs(curW - startW) <= halfW)
                ? startElectricalMaterial // First half
                : endElectricalMaterial; // Second half

            // Connect points to spring
            pointInfos1[curStartPointIndex].AddConnectedSpring(springIndex);
            pointInfos1[newPointIndex].AddConnectedSpring(springIndex);

            // Advance
            curStartPointIndex = newPointIndex;
        }

        // Add last SpringInfo (no PointInfo as the endpoint has already a PointInfo)
        ElementIndex const lastSpringIndex = static_cast<ElementIndex>(springInfos1.size());
        springInfos1.emplace_back(
            curStartPointIndex,
            0,  // Arbitrary factory direction (E)
            ropeSegment.PointBIndex1,
            4); // Arbitrary factory direction (W)

        // Connect points to spring
        pointInfos1[curStartPointIndex].AddConnectedSpring(lastSpringIndex);
        pointInfos1[ropeSegment.PointBIndex1].AddConnectedSpring(lastSpringIndex);
    }
}

void ShipBuilder::CreateShipElementInfos(
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    std::vector<PointInfo> & pointInfos1,
    std::vector<SpringInfo> & springInfos1,
    std::vector<TriangleInfo> & triangleInfos1,
    size_t & leakingPointsCount)
{
    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded PointInfos as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    // Initialize count of leaking points
    leakingPointsCount = 0;

    // This is our local circular order
    static const int Directions[8][2] = {
        {  1,  0 },  // 0: E
        {  1, -1 },  // 1: SE
        {  0, -1 },  // 2: S
        { -1, -1 },  // 3: SW
        { -1,  0 },  // 4: W
        { -1,  1 },  // 5: NW
        {  0,  1 },  // 6: N
        {  1,  1 }   // 7: NE
    };

    // From bottom to top - excluding extras at boundaries
    for (int y = 1; y <= structureImageSize.Height; ++y)
    {
        // We're starting a new row, so we're not in a ship now
        bool isInShip = false;

        // From left to right - excluding extras at boundaries
        for (int x = 1; x <= structureImageSize.Width; ++x)
        {
            if (!!pointIndexMatrix[x][y])
            {
                //
                // A point exists at these coordinates
                //

                ElementIndex pointIndex = *pointIndexMatrix[x][y];

                // If a non-hull node has empty space on one of its four sides, it is leaking.
                // Check if a is leaking; a is leaking if:
                // - a is not hull, AND
                // - there is at least a hole at E, S, W, N
                if (!pointInfos1[pointIndex].StructuralMtl.IsHull)
                {
                    if (!pointIndexMatrix[x + 1][y]
                        || !pointIndexMatrix[x][y + 1]
                        || !pointIndexMatrix[x - 1][y]
                        || !pointIndexMatrix[x][y - 1])
                    {
                        pointInfos1[pointIndex].IsLeaking = true;
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
                    int adjx1 = x + Directions[i][0];
                    int adjy1 = y + Directions[i][1];

                    if (!!pointIndexMatrix[adjx1][adjy1])
                    {
                        // This point is adjacent to the first point at one of E, SE, S, SW

                        //
                        // Create SpringInfo
                        //

                        ElementIndex const otherEndpointIndex = *pointIndexMatrix[adjx1][adjy1];

                        ElementIndex const springIndex = static_cast<ElementIndex>(springInfos1.size());

                        springInfos1.emplace_back(
                            pointIndex,
                            i,
                            otherEndpointIndex,
                            (i + 4) % 8);

                        // Add the spring to its endpoints
                        pointInfos1[pointIndex].AddConnectedSpring(springIndex);
                        pointInfos1[otherEndpointIndex].AddConnectedSpring(springIndex);


                        //
                        // Check if a triangle exists
                        // - If this is the first point that is in a ship, we check all the way up to W;
                        // - Else, we check up to S, so to avoid covering areas already covered by the triangulation
                        //   at the previous point
                        //

                        // Check adjacent point in next CW direction
                        int adjx2 = x + Directions[i + 1][0];
                        int adjy2 = y + Directions[i + 1][1];
                        if ((!isInShip || i < 2)
                            && !!pointIndexMatrix[adjx2][adjy2])
                        {
                            // This point is adjacent to the first point at one of SE, S, SW, W

                            //
                            // Create TriangleInfo
                            //

                            triangleInfos1.emplace_back(
                                std::array<ElementIndex, 3>(
                                    {
                                        pointIndex,
                                        otherEndpointIndex,
                                        *pointIndexMatrix[adjx2][adjy2]
                                    }));
                        }

                        // Now, we also want to check whether the single "irregular" triangle from this point exists,
                        // i.e. the triangle between this point, the point at its E, and the point at its
                        // S, in case there is no point at SE.
                        // We do this so that we can forget the entire W side for inner points and yet ensure
                        // full coverage of the area
                        if (i == 0
                            && !pointIndexMatrix[x + Directions[1][0]][y + Directions[1][1]]
                            && !!pointIndexMatrix[x + Directions[2][0]][y + Directions[2][1]])
                        {
                            // If we're here, the point at E exists
                            assert(!!pointIndexMatrix[x + Directions[0][0]][y + Directions[0][1]]);

                            //
                            // Create TriangleInfo
                            //

                            triangleInfos1.emplace_back(
                                std::array<ElementIndex, 3>(
                                    {
                                        pointIndex,
                                        *pointIndexMatrix[x + Directions[0][0]][y + Directions[0][1]],
                                        *pointIndexMatrix[x + Directions[2][0]][y + Directions[2][1]]
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

Physics::Points ShipBuilder::CreatePoints(
    std::vector<PointInfo> const & pointInfos2,
    World & parentWorld,
    MaterialDatabase const & materialDatabase,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters,
    std::vector<ElectricalElementInstanceIndex> & electricalElementInstanceIndices)
{
    Physics::Points points(
        static_cast<ElementIndex>(pointInfos2.size()),
        parentWorld,
        materialDatabase,
        std::move(gameEventDispatcher),
        gameParameters);

    electricalElementInstanceIndices.reserve(pointInfos2.size());

    ElementIndex electricalElementCounter = 0;
    for (size_t p = 0; p < pointInfos2.size(); ++p)
    {
        PointInfo const & pointInfo = pointInfos2[p];

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
            pointInfo.StructuralMtl,
            pointInfo.ElectricalMtl,
            pointInfo.IsRope,
            electricalElementIndex,
            pointInfo.IsLeaking,
            pointInfo.RenderColor,
            pointInfo.TextureCoordinates,
            GameRandomEngine::GetInstance().GenerateNormalizedUniformReal());

        //
        // Store electrical element instance index
        //

        electricalElementInstanceIndices.push_back(pointInfo.ElectricalElementInstanceIndex);
    }

    return points;
}

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::FilterOutRedundantTriangles(
    std::vector<TriangleInfo> const & triangleInfos,
    Physics::Points const & points,
    std::vector<ElementIndex> const & pointIndexRemap,
    std::vector<SpringInfo> const & springInfos)
{
    //
    // First pass: collect indices of those that need to stay
    //
    // Remove:
    //  - Those whose vertices are all rope points, of which at least one is connected exclusively
    //    to rope points (these would be knots "sticking out" of the structure)
    //      - This happens when two or more rope endpoints - from the structural layer - are next to each other
    //

    std::vector<ElementIndex> triangleIndices;
    triangleIndices.reserve(triangleInfos.size());

    for (ElementIndex t = 0; t < triangleInfos.size(); ++t)
    {
        if (points.IsRope(pointIndexRemap[triangleInfos[t].PointIndices1[0]])
            && points.IsRope(pointIndexRemap[triangleInfos[t].PointIndices1[1]])
            && points.IsRope(pointIndexRemap[triangleInfos[t].PointIndices1[2]]))
        {
            // Do not add triangle if at least one vertex is connected to rope points only
            if (!IsConnectedToNonRopePoints(pointIndexRemap[triangleInfos[t].PointIndices1[0]], points, pointIndexRemap, springInfos)
                || !IsConnectedToNonRopePoints(pointIndexRemap[triangleInfos[t].PointIndices1[1]], points, pointIndexRemap, springInfos)
                || !IsConnectedToNonRopePoints(pointIndexRemap[triangleInfos[t].PointIndices1[2]], points, pointIndexRemap, springInfos))
            {
                continue;
            }
        }

        // Remember to create this triangle
        triangleIndices.push_back(t);
    }

    //
    // Second pass: create new list of triangle info's
    //

    std::vector<TriangleInfo> newTriangleInfos;
    newTriangleInfos.reserve(triangleIndices.size());

    for (ElementIndex t = 0; t < triangleIndices.size(); ++t)
    {
        newTriangleInfos.push_back(
            triangleInfos[triangleIndices[t]]);
    }

    return newTriangleInfos;
}

void ShipBuilder::ConnectSpringsAndTriangles(
    std::vector<SpringInfo> & springInfos2,
    std::vector<TriangleInfo> & triangleInfos2)
{
    //
    // 1. Build Edge -> Spring table
    //

    std::unordered_map<Edge, ElementIndex, Edge::Hasher> pointToSpringMap;

    for (ElementIndex s = 0; s < springInfos2.size(); ++s)
    {
        pointToSpringMap.emplace(
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

            // Lookup spring for this edge
            auto const springIt = pointToSpringMap.find({ endpointIndex, nextEndpointIndex });
            assert(springIt != pointToSpringMap.end());

            ElementIndex const springIndex = springIt->second;

            // Tell this spring that it has an extra super triangle
            springInfos2[springIndex].SuperTriangles2.push_back(t);
            assert(springInfos2[springIndex].SuperTriangles2.size() <= 2);

            // Tell the triangle about this sub spring
            assert(!triangleInfos2[t].SubSprings2.contains(springIndex));
            triangleInfos2[t].SubSprings2.push_back(springIndex);
        }
    }


    //
    // 3. Now find "traverse" springs - i.e. springs that are not edges of any triangles
    // (because of our tessellation algorithm) - and see whether they're fully covered by two triangles;
    // if they are, consider these springs as sub-springs of those two triangles.
    //
    // A "traverse" spring would be a B-C spring in the following tessellation neighborhood:
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
            TriangleInfo & triangle1 = triangleInfos2[springInfos2[s].SuperTriangles2[0]];
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
            TriangleInfo & triangle2 = triangleInfos2[springInfos2[s].SuperTriangles2[1]];
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

            auto const traverseSpringIt = pointToSpringMap.find({ endpoint1Index, endpoint2Index });
            if (traverseSpringIt != pointToSpringMap.end())
            {
                // We have a traverse spring

                assert(0 == springInfos2[traverseSpringIt->second].SuperTriangles2.size());

                // Tell the traverse spring that it has these super triangles
                springInfos2[traverseSpringIt->second].SuperTriangles2.push_back(springInfos2[s].SuperTriangles2[0]);
                springInfos2[traverseSpringIt->second].SuperTriangles2.push_back(springInfos2[s].SuperTriangles2[1]);
                assert(springInfos2[traverseSpringIt->second].SuperTriangles2.size() == 2);

                // Tell the triangles about this new sub spring of theirs
                triangle1.SubSprings2.push_back(traverseSpringIt->second);
                triangle2.SubSprings2.push_back(traverseSpringIt->second);
            }
        }
    }
}

Physics::Springs ShipBuilder::CreateSprings(
    std::vector<SpringInfo> const & springInfos2,
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

Physics::Triangles ShipBuilder::CreateTriangles(
    std::vector<TriangleInfo> const & triangleInfos2,
    Physics::Points & points,
    std::vector<ElementIndex> const & pointIndexRemap)
{
    Physics::Triangles triangles(static_cast<ElementIndex>(triangleInfos2.size()));

    for (ElementIndex t = 0; t < triangleInfos2.size(); ++t)
    {
        // Create triangle
        triangles.Add(
            pointIndexRemap[triangleInfos2[t].PointIndices1[0]],
            pointIndexRemap[triangleInfos2[t].PointIndices1[1]],
            pointIndexRemap[triangleInfos2[t].PointIndices1[2]],
            triangleInfos2[t].SubSprings2);

        // Add triangle to its endpoints
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[0]], t, true); // Owner
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[1]], t, false); // Not owner
        points.AddFactoryConnectedTriangle(pointIndexRemap[triangleInfos2[t].PointIndices1[2]], t, false); // Not owner
    }

    return triangles;
}

ElectricalElements ShipBuilder::CreateElectricalElements(
    Physics::Points const & points,
    Physics::Springs const & springs,
    std::vector<ElectricalElementInstanceIndex> const & electricalElementInstanceIndices,
    std::map<ElectricalElementInstanceIndex, ElectricalPanelElementMetadata> const & panelMetadata,
    ShipId shipId,
    Physics::World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters)
{
    //
    // Verify all panel metadata indices are valid instance IDs
    //

    for (auto const it : panelMetadata)
    {
        if (std::find(
            electricalElementInstanceIndices.cbegin(),
            electricalElementInstanceIndices.cend(),
            it.first) == electricalElementInstanceIndices.cend())
        {
            throw GameException("Index '" + std::to_string(it.first) + "' of electrical panel metadata cannot be found among electrical element indices");
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
        std::optional<ElectricalPanelElementMetadata> panelElementMetadata;

        ElectricalElementInfo(
            ElementIndex _elementIndex,
            ElectricalElementInstanceIndex _instanceIndex,
            std::optional<ElectricalPanelElementMetadata> _panelElementMetadata)
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
            std::optional<ElectricalPanelElementMetadata> panelElementMetadata;
            if (electricalMaterial->IsInstanced)
            {
                assert(NoneElectricalElementInstanceIndex != instanceIndex);

                auto const findIt = panelMetadata.find(instanceIndex);

                if (findIt != panelMetadata.end())
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
        auto pointIndex = electricalElements.GetPointIndex(electricalElementIndex);

        for (auto const & cs : points.GetConnectedSprings(pointIndex).ConnectedSprings)
        {
            auto otherEndpointElectricalElementIndex = points.GetElectricalElement(cs.OtherEndpointIndex);
            if (NoneElementIndex != otherEndpointElectricalElementIndex)
            {
                // Get octant between this element and the other element
                Octant octant = springs.GetFactoryEndpointOctant(
                    cs.SpringIndex,
                    pointIndex);

                // Add element
                electricalElements.AddFactoryConnectedElectricalElement(
                    electricalElementIndex,
                    otherEndpointElectricalElementIndex,
                    octant);
            }
        }
    }

    return electricalElements;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Reordering
//////////////////////////////////////////////////////////////////////////////////////////////////

template <int StripeLength>
ShipBuilder::ReorderingResults ShipBuilder::ReorderPointsAndSpringsOptimally_Stripes(
    std::vector<PointInfo> const & pointInfos1,
    std::vector<SpringInfo> const & springInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize)
{
    //
    // 1. Build Edge -> Spring table
    //

    std::unordered_map<Edge, ElementIndex, Edge::Hasher> edgeToSpringIndex1Map;

    for (ElementIndex s = 0; s < springInfos1.size(); ++s)
    {
        edgeToSpringIndex1Map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springInfos1[s].PointAIndex1, springInfos1[s].PointBIndex1),
            std::forward_as_tuple(s));
    }


    //
    // 2. Visit the point matrix by all rows, from top to bottom
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);
    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);

    std::vector<PointInfo> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());

    std::vector<ElementIndex> pointIndexRemap(pointInfos1.size(), NoneElementIndex);

    std::vector<SpringInfo> springInfos2;
    springInfos2.reserve(springInfos1.size());

    // From top to bottom, starting at second row from top (i.e. first real row)
    for (int y = structureImageSize.Height; y >= 1; y -= (StripeLength - 1))
    {
        ReorderPointsAndSpringsOptimally_Stripes_Stripe<StripeLength>(
            y,
            pointInfos1,
            reorderedPointInfos1,
            springInfos1,
            reorderedSpringInfos1,
            pointIndexMatrix,
            structureImageSize,
            edgeToSpringIndex1Map,
            pointInfos2,
            pointIndexRemap,
            springInfos2);
    }


    //
    // 3. Add/Sort leftovers
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
            springInfos2.push_back(springInfos1[springIndex1]);
        }
    }

    //
    // 4. Return results
    //

    assert(pointInfos2.size() == pointInfos1.size());
    assert(pointIndexRemap.size() == pointInfos1.size());
    assert(springInfos2.size() == springInfos1.size());

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2);
}

template <int StripeLength>
void ShipBuilder::ReorderPointsAndSpringsOptimally_Stripes_Stripe(
    int y,
    std::vector<PointInfo> const & pointInfos1,
    std::vector<bool> & reorderedPointInfos1,
    std::vector<SpringInfo> const & springInfos1,
    std::vector<bool> & reorderedSpringInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    std::unordered_map<Edge, ElementIndex, Edge::Hasher> const & edgeToSpringIndex1Map,
    std::vector<PointInfo> & pointInfos2,
    std::vector<ElementIndex> & pointIndexRemap,
    std::vector<SpringInfo> & springInfos2)
{
    //
    // Collect points in a vertical stripe - 2 cols wide, StripeLength high
    //

    std::vector<ElementIndex> stripePointIndices1;

    // From left to right, start at first real col
    for (int x1 = 1; x1 <= structureImageSize.Width; ++x1)
    {
        //
        // 1. Build sets of indices of points left and right of the stripe
        //

        stripePointIndices1.clear();

        // From top to bottom
        for (int y1 = y; y1 > y - StripeLength && y1 >= 1; --y1)
        {
            // Check if left exists
            if (!!pointIndexMatrix[x1][y1])
            {
                stripePointIndices1.push_back(*(pointIndexMatrix[x1][y1]));
            }

            // Check if right exists
            if (!!pointIndexMatrix[x1 + 1][y1])
            {
                stripePointIndices1.push_back(*(pointIndexMatrix[x1 + 1][y1]));
            }
        }


        //
        // 2. Add/sort all not yet reordered springs connecting all points among themselves
        //

        for (int i1 = 0; i1 < static_cast<int>(stripePointIndices1.size()) - 1; ++i1)
        {
            for (int i2 = i1 + 1; i2 < static_cast<int>(stripePointIndices1.size()); ++i2)
            {
                auto const springIndexIt = edgeToSpringIndex1Map.find({ stripePointIndices1[i1], stripePointIndices1[i2] });
                if (springIndexIt != edgeToSpringIndex1Map.end())
                {
                    ElementIndex const springIndex1 = springIndexIt->second;
                    if (!reorderedSpringInfos1[springIndex1])
                    {
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

ShipBuilder::ReorderingResults ShipBuilder::ReorderPointsAndSpringsOptimally_Blocks(
    std::vector<PointInfo> const & pointInfos1,
    std::vector<SpringInfo> const & springInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize)
{
    //
    // 1. Build Edge -> Spring table
    //

    std::unordered_map<Edge, ElementIndex, Edge::Hasher> edgeToSpringIndex1Map;

    for (ElementIndex s = 0; s < springInfos1.size(); ++s)
    {
        edgeToSpringIndex1Map.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springInfos1[s].PointAIndex1, springInfos1[s].PointBIndex1),
            std::forward_as_tuple(s));
    }


    //
    // 2. Visit the point matrix by all rows, from top to bottom
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);
    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);

    std::vector<PointInfo> pointInfos2;
    pointInfos2.reserve(pointInfos1.size());

    std::vector<ElementIndex> pointIndexRemap(pointInfos1.size(), NoneElementIndex);

    std::vector<SpringInfo> springInfos2;
    springInfos2.reserve(springInfos1.size());

    // From top to bottom, starting at second row from top (i.e. first real row),
    // skipping one row of points to ensure full squares
    for (int y = structureImageSize.Height; y >= 1; y -= 2)
    {
        ReorderPointsAndSpringsOptimally_Blocks_Row(
            y,
            pointInfos1,
            reorderedPointInfos1,
            springInfos1,
            reorderedSpringInfos1,
            pointIndexMatrix,
            structureImageSize,
            edgeToSpringIndex1Map,
            pointInfos2,
            pointIndexRemap,
            springInfos2);
    }


    //
    // 3. Add/Sort leftovers
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
            springInfos2.push_back(springInfos1[springIndex1]);
        }
    }

    //
    // 4. Return results
    //

    assert(pointInfos2.size() == pointInfos1.size());
    assert(pointIndexRemap.size() == pointInfos1.size());
    assert(springInfos2.size() == springInfos1.size());

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2);
}

void ShipBuilder::ReorderPointsAndSpringsOptimally_Blocks_Row(
    int y,
    std::vector<PointInfo> const & pointInfos1,
    std::vector<bool> & reorderedPointInfos1,
    std::vector<SpringInfo> const & springInfos1,
    std::vector<bool> & reorderedSpringInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    std::unordered_map<Edge, ElementIndex, Edge::Hasher> const & edgeToSpringIndex1Map,
    std::vector<PointInfo> & pointInfos2,
    std::vector<ElementIndex> & pointIndexRemap,
    std::vector<SpringInfo> & springInfos2)
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
    for (int x = 1; x <= structureImageSize.Width; ++x)
    {
        squarePointIndices1.clear();

        // Check if b exists
        if (!!pointIndexMatrix[x][y])
        {
            //
            // 1. Collect all the points that we have around this square
            //

            // Add a if it exists
            if (!!pointIndexMatrix[x][y - 1])
                squarePointIndices1.push_back(*(pointIndexMatrix[x][y - 1]));

            // Add b
            squarePointIndices1.push_back(*(pointIndexMatrix[x][y]));

            // Add c if it exists
            if (!!pointIndexMatrix[x + 1][y])
                squarePointIndices1.push_back(*(pointIndexMatrix[x + 1][y]));

            // Add d if it exists
            if (!!pointIndexMatrix[x + 1][y - 1])
                squarePointIndices1.push_back(*(pointIndexMatrix[x + 1][y - 1]));

            //
            // 2. Add/sort all existing, not-yet-reordered springs among all these points
            //

            for (size_t i1 = 0; i1 < squarePointIndices1.size() - 1; ++i1)
            {
                for (size_t i2 = i1 + 1; i2 < squarePointIndices1.size(); ++i2)
                {
                    auto const springIndexIt = edgeToSpringIndex1Map.find({ squarePointIndices1[i1], squarePointIndices1[i2] });
                    if (springIndexIt != edgeToSpringIndex1Map.end())
                    {
                        ElementIndex const springIndex1 = springIndexIt->second;
                        if (!reorderedSpringInfos1[springIndex1])
                        {
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
ShipBuilder::ReorderingResults ShipBuilder::ReorderPointsAndSpringsOptimally_Tiling(
    std::vector<PointInfo> const & pointInfos1,
    std::vector<SpringInfo> const & springInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize)
{
    //
    // 1. Visit the point matrix in 2x2 blocks, and add all springs connected to any
    // of the included points (0..4 points), except for already-added ones
    //

    std::vector<bool> reorderedSpringInfos1(springInfos1.size(), false);

    std::vector<SpringInfo> springInfos2;
    springInfos2.reserve(springInfos1.size());

    // From bottom to top
    for (int y = 1; y <= structureImageSize.Height; y += BlockSize)
    {
        for (int x = 1; x <= structureImageSize.Width; x += BlockSize)
        {
            for (int y2 = 0; y2 < BlockSize && y + y2 <= structureImageSize.Height; ++y2)
            {
                for (int x2 = 0; x2 < BlockSize && x + x2 <= structureImageSize.Width; ++x2)
                {
                    if (!!pointIndexMatrix[x + x2][y + y2])
                    {
                        ElementIndex pointIndex = *(pointIndexMatrix[x + x2][y + y2]);

                        // Add all springs connected to this point
                        for (auto connectedSpringIndex : pointInfos1[pointIndex].ConnectedSprings1)
                        {
                            if (!reorderedSpringInfos1[connectedSpringIndex])
                            {
                                springInfos2.push_back(springInfos1[connectedSpringIndex]);
                                reorderedSpringInfos1[connectedSpringIndex] = true;
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
            springInfos2.push_back(springInfos1[s]);
    }

    assert(springInfos2.size() == springInfos1.size());


    //
    // 3. Order points in the order they first appear when visiting springs linearly
    //
    // a.k.a. Bas van den Berg's optimization!
    //

    std::vector<bool> reorderedPointInfos1(pointInfos1.size(), false);

    std::vector<PointInfo> pointInfos2;
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

    return std::make_tuple(pointInfos2, pointIndexRemap, springInfos2);
}

std::vector<ShipBuilder::SpringInfo> ShipBuilder::ReorderSpringsOptimally_TomForsyth(
    std::vector<SpringInfo> const & springInfos1,
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
    std::vector<SpringInfo> springInfos2;
    springInfos2.reserve(springInfos1.size());
    for (size_t ti : optimalIndices)
    {
        springInfos2.push_back(springInfos1[ti]);
    }

    return springInfos2;
}

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::ReorderTrianglesOptimally_ReuseOptimization(
    std::vector<TriangleInfo> const & triangleInfos1,
    size_t /*pointCount*/)
{
    std::vector<TriangleInfo> triangleInfos2;
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

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::ReorderTrianglesOptimally_TomForsyth(
    std::vector<TriangleInfo> const & triangleInfos1,
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
    std::vector<TriangleInfo> triangleInfos2;
    triangleInfos2.reserve(triangleInfos1.size());
    for (size_t ti : optimalIndices)
    {
        triangleInfos2.push_back(triangleInfos1[ti]);
    }

    return triangleInfos2;
}

float ShipBuilder::CalculateACMR(std::vector<SpringInfo> const & springInfos)
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

float ShipBuilder::CalculateACMR(std::vector<TriangleInfo> const & triangleInfos)
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

float ShipBuilder::CalculateVertexMissRatio(std::vector<TriangleInfo> const & triangleInfos)
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
std::vector<size_t> ShipBuilder::ReorderOptimally(
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

void ShipBuilder::AddVertexToCache(
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
float ShipBuilder::CalculateVertexScore(VertexData const & vertexData)
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
            assert(vertexData.CachePosition < VertexCacheSize);

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
bool ShipBuilder::TestLRUVertexCache<Size>::UseVertex(size_t vertexIndex)
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
std::optional<size_t> ShipBuilder::TestLRUVertexCache<Size>::GetCachePosition(size_t vertexIndex)
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