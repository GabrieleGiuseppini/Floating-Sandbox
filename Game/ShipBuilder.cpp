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
#include <unordered_map>
#include <unordered_set>
#include <utility>

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Ship> ShipBuilder::Create(
    ShipId shipId,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
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

    // Matrix of points - we allocate 2 extra dummy rows and cols to avoid checking for boundaries
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
            MaterialDatabase::ColorKey colorKey = shipDefinition.StructuralLayerImage.Data[x + (structureHeight - y - 1) * structureWidth];
            StructuralMaterial const * structuralMaterial = materialDatabase.FindStructuralMaterial(colorKey);
            if (nullptr != structuralMaterial)
            {
                //
                // Make a point
                //

                ElementIndex const pointIndex = static_cast<ElementIndex>(pointInfos.size());

                pointIndexMatrix[x + 1][y + 1] = static_cast<ElementIndex>(pointIndex);

                pointInfos.emplace_back(
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y))
                    + shipDefinition.Metadata.Offset,
                    MakeTextureCoordinates(x, y, shipDefinition.StructuralLayerImage.Size),
                    structuralMaterial->RenderColor,
                    *structuralMaterial,
                    structuralMaterial->IsUniqueType(StructuralMaterial::MaterialUniqueType::Rope));

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
                            std::string("More than two \"" + Utils::RgbColor2Hex(colorKey) + "\" rope endpoints found at (")
                            + std::to_string(x) + "," + std::to_string(structureHeight - y - 1) + ")");
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
            true,
            pointIndexMatrix,
            materialDatabase);
    }
    else
    {
        // Decorate points with electrical materials from the structural layer
        DecoratePointsWithElectricalMaterials(
            shipDefinition.StructuralLayerImage,
            pointInfos,
            false,
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
    // Optimize order of SpringInfo's to minimize cache misses
    //

    float originalSpringACMR = CalculateACMR(springInfos);

    // Tiling algorithm
    springInfos = ReorderSpringsOptimally_Tiling<2>(
        springInfos,
        pointIndexMatrix,
        shipDefinition.StructuralLayerImage.Size,
        pointInfos);

    float optimizedSpringACMR = CalculateACMR(springInfos);

    LogMessage("Spring ACMR: original=", originalSpringACMR, ", optimized=", optimizedSpringACMR);


    // Note: we don't optimize triangles, as tests indicate that performance gets (marginally) worse,
    // and at the same time, it makes sense to use the natural order of the triangles as it ensures
    // that higher elements in the ship cover lower elements when they are semi-detached


    //
    // Now reorder points to improve data locality when visiting springs
    //

    std::vector<ElementIndex> pointIndexRemap;

    pointInfos = ReorderPointsOptimally_FollowingSprings(
        pointInfos,
        springInfos,
        pointIndexRemap);


    //
    // Visit all PointInfo's and create Points, i.e. the entire set of points
    //

    Points points = CreatePoints(
        pointInfos,
        parentWorld,
        gameEventDispatcher,
        gameParameters);


    //
    // Filter out redundant triangles
    //

    triangleInfos = FilterOutRedundantTriangles(
        triangleInfos,
        points,
        pointIndexRemap,
        springInfos);


    //
    // Associate all springs with the triangles that cover them
    //

    ConnectSpringsAndTriangles(
        springInfos,
        triangleInfos);


    //
    // Create Springs for all SpringInfo's
    //

    Springs springs = CreateSprings(
        springInfos,
        points,
        pointIndexRemap,
        parentWorld,
        gameEventDispatcher,
        gameParameters);


    //
    // Create Triangles for all (filtered out) TriangleInfo's
    //

    Triangles triangles = CreateTriangles(
        triangleInfos,
        points,
        pointIndexRemap);


    //
    // Create Electrical Elements
    //

    ElectricalElements electricalElements = CreateElectricalElements(
        points,
        parentWorld,
        gameEventDispatcher);


    //
    // We're done!
    //

    LogMessage("Created ship: W=", shipDefinition.StructuralLayerImage.Size.Width, ", H=", shipDefinition.StructuralLayerImage.Size.Height, ", ",
        points.GetShipPointCount(), " points, ", springs.GetElementCount(), " springs, ", triangles.GetElementCount(), " triangles, ",
        electricalElements.GetElementCount(), " electrical elements.");

    return std::make_unique<Ship>(
        shipId,
        parentWorld,
        gameEventDispatcher,
        materialDatabase,
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
            MaterialDatabase::ColorKey colorKey = ropeLayerImage.Data[x + (height - y - 1) * width];

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
                        vec2f(
                            static_cast<float>(x) - halfWidth,
                            static_cast<float>(y))
                        + shipOffset,
                        MakeTextureCoordinates(x, y, ropeLayerImage.Size),
                        colorKey.toVec4f(1.0f),
                        materialDatabase.GetUniqueStructuralMaterial(StructuralMaterial::MaterialUniqueType::Rope),
                        true);

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
                        throw GameException(
                            std::string("There is already a rope at point (")
                            + std::to_string(x) + "," + std::to_string(height - y - 1) + ")");
                    }
                }

                // Store in RopeSegments
                RopeSegment & ropeSegment = ropeSegments[colorKey];
                if (!ropeSegment.SetEndpoint(pointIndex, colorKey))
                {
                    throw GameException(
                        std::string("More than two \"" + Utils::RgbColor2Hex(colorKey) + "\" rope endpoints found at (")
                        + std::to_string(x) + "," + std::to_string(height - y - 1) + ") in the rope layer image");
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
            MaterialDatabase::ColorKey colorKey = layerImage.Data[x + (height - y - 1) * width];

            // Check if it's an electrical material
            ElectricalMaterial const * electricalMaterial = materialDatabase.FindElectricalMaterial(colorKey);
            if (nullptr == electricalMaterial)
            {
                if (isDedicatedElectricalLayer
                    && colorKey != BackgroundColorKey)
                {
                    throw GameException(
                        std::string("Cannot find electrical material for color key \"" + Utils::RgbColor2Hex(colorKey)
                        + "\" of pixel found at (")
                        + std::to_string(x) + "," + std::to_string(height - y - 1) + ") in the "
                        + (isDedicatedElectricalLayer ? "electrical" : "structural")
                        + " layer image");
                }

                // Just ignore
            }
            else
            {
                // Make sure we have a structural point here
                if (!pointIndexMatrix[x + 1][y + 1])
                {
                    throw GameException(
                        std::string("The electrical layer image specifies an electrical material at (")
                        + std::to_string(x) + "," + std::to_string(height - y - 1)
                        + "), but no pixel may be found at those coordinates in the structural layer image");
                }

                // Store electrical material
                auto const pointIndex = *(pointIndexMatrix[x + 1][y + 1]);
                assert(nullptr == pointInfos1[pointIndex].ElectricalMtl);
                pointInfos1[pointIndex].ElectricalMtl = electricalMaterial;
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
        ElectricalMaterial const * startElectricalMaterial = pointInfos1[ropeSegment.PointAIndex1].ElectricalMtl;
        ElectricalMaterial const * endElectricalMaterial = pointInfos1[ropeSegment.PointBIndex1].ElectricalMtl;

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
                newPosition,
                MakeTextureCoordinates(newPosition.x, newPosition.y, structureImageSize),
                ropeSegment.RopeColorKey.toVec4f(1.0f),
                ropeMaterial,
                true); // IsRope

            // Set electrical material
            pointInfos1.back().ElectricalMtl =
                (fabs(curW - startW) <= halfW)
                ? startElectricalMaterial
                : endElectricalMaterial;

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

    // From bottom to top
    for (int y = 1; y <= structureImageSize.Height; ++y)
    {
        // We're starting a new row, so we're not in a ship now
        bool isInShip = false;

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

template <int BlockSize>
std::vector<ShipBuilder::SpringInfo> ShipBuilder::ReorderSpringsOptimally_Tiling(
    std::vector<SpringInfo> const & springInfos1,
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    std::vector<PointInfo> const & pointInfos1)
{
    //
    // 1. Visit the point matrix in 2x2 blocks, and add all springs connected to any
    // of the included points (0..4 points), except for already-added ones
    //

    std::vector<SpringInfo> springInfos2;
    springInfos2.reserve(springInfos1.size());

    std::vector<bool> addedSprings;
    addedSprings.resize(springInfos1.size(), false);

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
                            if (!addedSprings[connectedSpringIndex])
                            {
                                springInfos2.push_back(springInfos1[connectedSpringIndex]);
                                addedSprings[connectedSpringIndex] = true;
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
        if (!addedSprings[s])
            springInfos2.push_back(springInfos1[s]);
    }

    assert(springInfos2.size() == springInfos1.size());

    return springInfos2;
}

std::vector<ShipBuilder::PointInfo> ShipBuilder::ReorderPointsOptimally_FollowingSprings(
    std::vector<ShipBuilder::PointInfo> const & pointInfos1,
    std::vector<ShipBuilder::SpringInfo> const & springInfos2,
    std::vector<ElementIndex> & pointIndexRemap)
{
    //
    // Order points in the order they first appear when visiting springs linearly
    //
    // a.k.a. Bas van den Berg's optimization!
    //

    std::vector<PointInfo> pointInfos2;
    pointIndexRemap.resize(pointInfos1.size());

    std::unordered_set<ElementIndex> visitedPoints;

    for (auto const & springInfo : springInfos2)
    {
        if (visitedPoints.insert(springInfo.PointAIndex1).second)
        {
            pointIndexRemap[springInfo.PointAIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[springInfo.PointAIndex1]);
        }

        if (visitedPoints.insert(springInfo.PointBIndex1).second)
        {
            pointIndexRemap[springInfo.PointBIndex1] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[springInfo.PointBIndex1]);
        }
    }


    //
    // Add missing points
    //

    for (ElementIndex p = 0; p < pointInfos1.size(); ++p)
    {
        if (0 == visitedPoints.count(p))
        {
            pointIndexRemap[p] = static_cast<ElementIndex>(pointInfos2.size());
            pointInfos2.push_back(pointInfos1[p]);
        }
    }

    assert(pointInfos2.size() == pointInfos1.size());

    return pointInfos2;
}

std::vector<ShipBuilder::PointInfo> ShipBuilder::ReorderPointsOptimally_Idempotent(
    std::vector<ShipBuilder::PointInfo> const & pointInfos1,
    std::vector<ElementIndex> & pointIndexRemap)
{
    pointIndexRemap.reserve(pointInfos1.size());

    for (ElementIndex p = 0; p < pointInfos1.size(); ++p)
    {
        pointIndexRemap.push_back(p);
    }

    return pointInfos1;
}

Points ShipBuilder::CreatePoints(
    std::vector<PointInfo> const & pointInfos2,
    World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher,
    GameParameters const & gameParameters)
{
    Physics::Points points(
        static_cast<ElementIndex>(pointInfos2.size()),
        parentWorld,
        std::move(gameEventDispatcher),
        gameParameters);

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
            pointInfo.StructuralMtl,
            pointInfo.ElectricalMtl,
            pointInfo.IsRope,
            electricalElementIndex,
            pointInfo.IsLeaking,
            pointInfo.RenderColor,
            pointInfo.TextureCoordinates);
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

    struct Edge
    {
        ElementIndex Endpoint1Index;
        ElementIndex Endpoint2Index;

        Edge(
            ElementIndex endpoint1Index,
            ElementIndex endpoint2Index)
            : Endpoint1Index(std::min(endpoint1Index, endpoint2Index))
            , Endpoint2Index(std::max(endpoint1Index, endpoint2Index))
        {}

        bool operator==(Edge const & other) const
        {
            return this->Endpoint1Index == other.Endpoint1Index
                && this->Endpoint2Index == other.Endpoint2Index;
        }

        struct Hasher
        {
            size_t operator()(Edge const & edge) const
            {
                return edge.Endpoint1Index * 23
                    + edge.Endpoint2Index;
            }
        };
    };

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
        int characteristics = 0;

        // The spring is hull if at least one node is hull
        // (we don't propagate water along a hull spring)
        if (points.IsHull(pointIndexRemap[springInfos2[s].PointAIndex1])
            || points.IsHull(pointIndexRemap[springInfos2[s].PointBIndex1]))
            characteristics |= static_cast<int>(Springs::Characteristics::Hull);

        // If both nodes are rope, then the spring is rope
        // (non-rope <-> rope springs are "connections" and not to be treated as ropes)
        if (points.IsRope(pointIndexRemap[springInfos2[s].PointAIndex1])
            && points.IsRope(pointIndexRemap[springInfos2[s].PointBIndex1]))
            characteristics |= static_cast<int>(Springs::Characteristics::Rope);

        // Create spring
        springs.Add(
            pointIndexRemap[springInfos2[s].PointAIndex1],
            pointIndexRemap[springInfos2[s].PointBIndex1],
            springInfos2[s].PointAAngle,
            springInfos2[s].PointBAngle,
            springInfos2[s].SuperTriangles2,
            static_cast<Springs::Characteristics>(characteristics),
            points);

        // Add spring to its endpoints
        points.AddFactoryConnectedSpring(
            pointIndexRemap[springInfos2[s].PointAIndex1],
            s,
            pointIndexRemap[springInfos2[s].PointBIndex1],
            true); // Owner
        points.AddFactoryConnectedSpring(
            pointIndexRemap[springInfos2[s].PointBIndex1],
            s,
            pointIndexRemap[springInfos2[s].PointAIndex1],
            false); // Not owner
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
    Physics::World & parentWorld,
    std::shared_ptr<GameEventDispatcher> gameEventDispatcher)
{
    //
    // Get indices of points with electrical elements
    //

    std::vector<ElementIndex> electricalElementPointIndices;
    for (auto pointIndex : points)
    {
        if (NoneElementIndex != points.GetElectricalElement(pointIndex))
        {
            electricalElementPointIndices.push_back(pointIndex);
        }
    }

    //
    // Create electrical elements
    //

    ElectricalElements electricalElements(
        static_cast<ElementCount>(electricalElementPointIndices.size()),
        parentWorld,
        gameEventDispatcher);

    for (auto pointIndex : electricalElementPointIndices)
    {
        electricalElements.Add(
            pointIndex,
            *points.GetElectricalMaterial(pointIndex));
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
                electricalElements.AddConnectedElectricalElement(
                    electricalElementIndex,
                    otherEndpointElectricalElementIndex);
            }
        }
    }

    return electricalElements;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex cache optimization
//////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ShipBuilder::SpringInfo> ShipBuilder::ReorderSpringsOptimally_TomForsyth(
    std::vector<SpringInfo> & springInfos1,
    size_t vertexCount)
{
    std::vector<VertexData> vertexData(vertexCount);
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

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::ReorderTrianglesSpringsOptimally_TomForsyth(
    std::vector<TriangleInfo> & triangleInfos1,
    size_t vertexCount)
{
    std::vector<VertexData> vertexData(vertexCount);
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
        int32_t currentCachePosition = 0;
        for (auto it = modelLruVertexCache.begin(); it != modelLruVertexCache.end(); ++it, ++currentCachePosition)
        {
            vertexData[*it].CachePosition = (currentCachePosition < VertexCacheSize)
                ? currentCachePosition
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

float ShipBuilder::CalculateACMR(std::vector<SpringInfo> const & springInfos)
{
    //
    // Calculate the average cache miss ratio
    //

    if (springInfos.empty())
    {
        return 0.0f;
    }

    TestLRUVertexCache<VertexCacheSize> cache;

    float cacheMisses = 0.0f;

    for (auto const & springInfo : springInfos)
    {
        if (!cache.UseVertex(springInfo.PointAIndex1))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(springInfo.PointBIndex1))
        {
            cacheMisses += 1.0f;
        }
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

        if (vertexData.CachePosition < VerticesInElement)
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