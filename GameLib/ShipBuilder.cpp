/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-04-22
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
*
* Contains code from Tom Forsyth - https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
***************************************************************************************/
#include "ShipBuilder.h"

#include "Log.h"

#include <algorithm>
#include <cassert>
#include <limits>

using namespace Physics;

//////////////////////////////////////////////////////////////////////////////

namespace /* anonymous */ {

    bool IsConnectedToNonRopePoints(
        ElementIndex pointIndex,
        Points const & points,
        Springs const & springs)
    {
        assert(points.GetMaterial(pointIndex)->IsRope);

        for (auto springIndex : points.GetConnectedSprings(pointIndex))
        {
            if (!points.IsRope(springs.GetPointAIndex(springIndex))
                || !points.IsRope(springs.GetPointBIndex(springIndex)))
            {
                return true;
            }
        }

        return false;
    }

}

//////////////////////////////////////////////////////////////////////////////

std::unique_ptr<Ship> ShipBuilder::Create(
    int shipId,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler,
    ShipDefinition const & shipDefinition,
    std::shared_ptr<MaterialDatabase> materials,
    GameParameters const & /*gameParameters*/,
    VisitSequenceNumber currentVisitSequenceNumber)
{
    int const structureWidth = shipDefinition.StructuralImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth) / 2.0f;
    int const structureHeight = shipDefinition.StructuralImage.Size.Height;

    // PointInfo's
    std::vector<PointInfo> pointInfos;

    // SpringInfo's
    std::vector<SpringInfo> springInfos;

    // RopeSegment's, indexed by the rope color
    std::map<std::array<uint8_t, 3u>, RopeSegment> ropeSegments;

    // TriangleInfo's
    std::vector<TriangleInfo> triangleInfos;


    //
    // Process image points and:
    // - Identify all points, and create PointInfo's for them
    // - Build a 2D matrix containing indices to the points above
    // - Identify rope endpoints, and create RopeSegment's for them
    //    

    // Matrix of points - we allocate 2 extra dummy rows and cols to avoid checking for boundaries
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> pointIndexMatrix(new std::unique_ptr<std::optional<ElementIndex>[]>[structureWidth + 2]);
    for (int c = 0; c < structureWidth + 2; ++c)
    {
        pointIndexMatrix[c] = std::unique_ptr<std::optional<ElementIndex>[]>(new std::optional<ElementIndex>[structureHeight + 2]);
    }

    // Visit all real columns
    for (int x = 0; x < structureWidth; ++x)
    {
        // From bottom to top
        for (int y = 0; y < structureHeight; ++y)
        {
            // R G B
            std::array<uint8_t, 3u> rgbColour = {
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 0],
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 1],
                shipDefinition.StructuralImage.Data[(x + (structureHeight - y - 1) * structureWidth) * 3 + 2] };

            bool isRopeEndpoint = false;

            Material const * material = materials->Find(rgbColour);
            if (nullptr == material)
            {
                // Check whether it's a rope endpoint (#000xxx)
                if (0x00 == rgbColour[0]
                    && 0 == (rgbColour[1] & 0xF0))
                {
                    // It's a rope endpoint
                    isRopeEndpoint = true;
                    
                    // Store in RopeSegments
                    RopeSegment & ropeSegment = ropeSegments[rgbColour];
                    if (NoneElementIndex == ropeSegment.PointAIndex)
                    {
                        ropeSegment.PointAIndex = static_cast<ElementIndex>(pointInfos.size());
                    }
                    else if (NoneElementIndex == ropeSegment.PointBIndex)
                    {
                        ropeSegment.PointBIndex = static_cast<ElementIndex>(pointInfos.size());
                    }
                    else
                    {
                        throw GameException(
                            std::string("More than two \"" + Utils::RgbColour2Hex(rgbColour) + "\" rope endpoints found at (")
                            + std::to_string(x) + "," + std::to_string(structureHeight - y - 1) + ")");
                    }

                    // Point to rope (#000000)
                    material = &(materials->GetRopeMaterial());
                }
            }

            if (nullptr != material)
            {
                //
                // Make a point
                //

                pointIndexMatrix[x + 1][y + 1] = static_cast<ElementIndex>(pointInfos.size());

                pointInfos.emplace_back(
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y))
                        + shipDefinition.Metadata.Offset,
                    vec2f(
                        static_cast<float>(x) / static_cast<float>(structureWidth),
                        static_cast<float>(y) / static_cast<float>(structureHeight)),
                    material,
                    isRopeEndpoint);
            }
        }
    }



    //
    // Process all identified rope endpoints and:
    // - Fill-in points between the endpoints, creating additional PointInfo's for them
    // - Fill-in springs between each pair of points in the rope, creating SpringInfo's for them
    //    

    CreateRopeSegments(
        ropeSegments,
        shipDefinition.StructuralImage.Size,
        materials->GetRopeMaterial(),
        pointInfos,
        springInfos);


    //
    // Visit all PointInfo's and create Points, i.e. the entire set of points
    //

    Points points = CreatePoints(
        pointInfos,
        parentWorld,
        gameEventHandler);


    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded Points as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    size_t leakingPointsCount;

    CreateShipElementInfos(
        pointIndexMatrix,
        shipDefinition.StructuralImage.Size,
        points,
        springInfos,
        triangleInfos,
        leakingPointsCount);


    //
    // Optimize order of SpringInfo's to minimize cache misses
    //

    float originalSpringACMR = CalculateACMR(springInfos);

    springInfos = ReorderOptimally(springInfos, pointInfos.size());

    float optimizedSpringACMR = CalculateACMR(springInfos);

    LogMessage("Spring ACMR: original=", originalSpringACMR, ", optimized=", optimizedSpringACMR);


    // Note: we don't optimize triangles, as tests indicate that performance gets (marginally) worse,
    // and at the same time, it makes sense to use the natural order of the triangles as it ensures
    // that higher elements in the ship cover lower elements when they are semi-detached


    //
    // Create Springs for all SpringInfo's
    //

    Springs springs = CreateSprings(
        springInfos,
        points,
        parentWorld,
        gameEventHandler);


    //
    // Create Triangles for all TriangleInfo's except those whose vertices
    // are all rope points, of which at least one is connected exclusively 
    // to rope points (these would be knots "sticking out" of the structure)
    //

    Triangles triangles = CreateTriangles(
        triangleInfos,
        points,
        springs);


    //
    // Create Electrical Elements
    //

    ElectricalElements electricalElements = CreateElectricalElements(
        points,
        springs,
        parentWorld,
        gameEventHandler);


    //
    // We're done!
    //

    LogMessage("Created ship: W=", shipDefinition.StructuralImage.Size.Width, ", H=", shipDefinition.StructuralImage.Size.Height, ", ",
        points.GetElementCount(), " points, ", springs.GetElementCount(), " springs, ", triangles.GetElementCount(), " triangles, ",
        electricalElements.GetElementCount(), " electrical elements.");

    return std::make_unique<Ship>(
        shipId, 
        parentWorld,
        gameEventHandler,
        std::move(points),
        std::move(springs),
        std::move(triangles),
        std::move(electricalElements),
        materials,
        currentVisitSequenceNumber);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Building helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

void ShipBuilder::CreateRopeSegments(
    std::map<std::array<uint8_t, 3u>, RopeSegment> const & ropeSegments,
    ImageSize const & structureImageSize,
    Material const & ropeMaterial,
    std::vector<PointInfo> & pointInfos,
    std::vector<SpringInfo> & springInfos)
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
        assert(NoneElementIndex != ropeSegment.PointAIndex);
        if (NoneElementIndex == ropeSegment.PointBIndex)
        {
            throw GameException(
                std::string("Only one rope endpoint found with index <")
                + std::to_string(static_cast<int>(ropeSegmentEntry.first[1]))
                + "," + std::to_string(static_cast<int>(ropeSegmentEntry.first[2])) + ">");
        }

        // Get endpoint positions
        vec2f startPos = pointInfos[ropeSegment.PointAIndex].Position;
        vec2f endPos = pointInfos[ropeSegment.PointBIndex].Position;

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
        float curW, curN;
        float endW;
        float stepW;
        if (fabs(dx) > fabs(dy))
        {
            widestIsX = true;
            slope = dy / dx;
            curW = startPos.x;
            curN = startPos.y;
            endW = endPos.x;
            stepW = dx / fabs(dx);
        }
        else
        {
            widestIsX = false;
            slope = dx / dy;
            curW = startPos.y;
            curN = startPos.x;
            endW = endPos.y;
            stepW = dy / fabs(dy);
        }

        auto curStartPointIndex = ropeSegment.PointAIndex;
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

            auto nexPointIndex = static_cast<ElementIndex>(pointInfos.size());

            // Add SpringInfo
            springInfos.emplace_back(curStartPointIndex, nexPointIndex);

            // Advance
            curStartPointIndex = nexPointIndex;

            // Add PointInfo
            pointInfos.emplace_back(
                newPosition,
                vec2f(
                    newPosition.x / static_cast<float>(structureImageSize.Width),
                    newPosition.y / static_cast<float>(structureImageSize.Height)),
                &ropeMaterial,
                false);
        }

        // Add last SpringInfo (no PointInfo as the endpoint has already a PointInfo)
        springInfos.emplace_back(curStartPointIndex, ropeSegment.PointBIndex);
    }
}

Points ShipBuilder::CreatePoints(
    std::vector<PointInfo> const & pointInfos,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler)
{
    Physics::Points points(
        static_cast<ElementIndex>(pointInfos.size()),
        parentWorld,
        std::move(gameEventHandler));

    ElementIndex electricalElementCounter = 0;
    for (size_t p = 0; p < pointInfos.size(); ++p)
    {
        PointInfo const & pointInfo = pointInfos[p];

        Material const * mtl = pointInfo.Mtl;

        // Make point non-hull if it's endpoint of a rope, otherwise springs connected
        // to this point would be hull and thus this point would never catch water
        bool isHull;
        if (pointInfo.IsRopeEndpoint)
        {
            isHull = false;
        }
        else
        {
            isHull = mtl->IsHull;
        }

        ElementIndex electricalElementIndex = NoneElementIndex;
        if (!!mtl->Electrical)
        {
            // This point has an associated electrical element
            electricalElementIndex = electricalElementCounter;
            ++electricalElementCounter;
        }

        float buoyancy = mtl->IsHull ? 0.0f : 1.0f; // No buoyancy if it's a hull material, as it can't get water

        //
        // Create point
        //

        points.Add(
            pointInfo.Position,
            mtl,
            isHull,
            mtl->IsRope,
            electricalElementIndex,
            buoyancy,
            mtl->RenderColour,
            pointInfo.TextureCoordinates);
    }

    return points;
}

void ShipBuilder::CreateShipElementInfos(
    std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]> const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    Physics::Points & points,
    std::vector<SpringInfo> & springInfos,
    std::vector<TriangleInfo> & triangleInfos,
    size_t & leakingPointsCount)
{
    //
    // Visit point matrix and:
    //  - Set non-fully-surrounded Points as "leaking"
    //  - Detect springs and create SpringInfo's for them (additional to ropes)
    //  - Do tessellation and create TriangleInfo's
    //

    // Initialize count of leaking points
    leakingPointsCount = 0;

    // This is our local circular order
    static const int Directions[8][2] = {
        {  1,  0 },  // E
        {  1, -1 },  // SE
        {  0, -1 },  // S
        { -1, -1 },  // SW
        { -1,  0 },  // W
        { -1,  1 },  // NW
        {  0,  1 },  // N
        {  1,  1 }   // NE
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
                if (!points.GetMaterial(pointIndex)->IsHull)
                {
                    if (!pointIndexMatrix[x + 1][y]
                        || !pointIndexMatrix[x][y + 1]
                        || !pointIndexMatrix[x - 1][y]
                        || !pointIndexMatrix[x][y - 1])
                    {
                        points.SetLeaking(pointIndex);

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

                        springInfos.emplace_back(
                            pointIndex,
                            *pointIndexMatrix[adjx1][adjy1]);


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

                            triangleInfos.emplace_back(
                                pointIndex,
                                *pointIndexMatrix[adjx1][adjy1],
                                *pointIndexMatrix[adjx2][adjy2]);
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

                            triangleInfos.emplace_back(
                                pointIndex,
                                *pointIndexMatrix[x + Directions[0][0]][y + Directions[0][1]],
                                *pointIndexMatrix[x + Directions[2][0]][y + Directions[2][1]]);
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

Physics::Springs ShipBuilder::CreateSprings(
    std::vector<SpringInfo> const & springInfos,
    Physics::Points & points,
    World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler)
{
    Physics::Springs springs(
        static_cast<ElementIndex>(springInfos.size()),
        parentWorld,
        std::move(gameEventHandler));

    for (ElementIndex s = 0; s < springInfos.size(); ++s)
    {
        int characteristics = 0;

        // The spring is hull if at least one node is hull 
        // (we don't propagate water along a hull spring)
        if (points.IsHull(springInfos[s].PointAIndex) || points.IsHull(springInfos[s].PointBIndex))
            characteristics |= static_cast<int>(Springs::Characteristics::Hull);

        // If both nodes are rope, then the spring is rope 
        // (non-rope <-> rope springs are "connections" and not to be treated as ropes)
        if (points.IsRope(springInfos[s].PointAIndex) && points.IsRope(springInfos[s].PointBIndex))
            characteristics |= static_cast<int>(Springs::Characteristics::Rope);

        // Create spring
        springs.Add(
            springInfos[s].PointAIndex,
            springInfos[s].PointBIndex,
            static_cast<Springs::Characteristics>(characteristics),
            points);

        // Add spring to its endpoints
        points.AddConnectedSpring(springInfos[s].PointAIndex, s);
        points.AddConnectedSpring(springInfos[s].PointBIndex, s);
    }

    return springs;
}

Physics::Triangles ShipBuilder::CreateTriangles(
    std::vector<TriangleInfo> const & triangleInfos,
    Physics::Points & points,
    Physics::Springs & springs)
{
    //
    // First pass: filter out triangles and keep indices of those that need to be created
    //

    std::vector<ElementIndex> triangleIndices;
    triangleIndices.reserve(triangleInfos.size());

    for (ElementIndex t = 0; t < triangleInfos.size(); ++t)
    {
        if (points.IsRope(triangleInfos[t].PointAIndex)
            && points.IsRope(triangleInfos[t].PointBIndex)
            && points.IsRope(triangleInfos[t].PointCIndex))
        {
            // Do not add triangle if at least one vertex is connected to rope points only
            if (!IsConnectedToNonRopePoints(triangleInfos[t].PointAIndex, points, springs)
                || !IsConnectedToNonRopePoints(triangleInfos[t].PointBIndex, points, springs)
                || !IsConnectedToNonRopePoints(triangleInfos[t].PointCIndex, points, springs))
            {
                continue;
            }
        }

        // Remember to create this triangle
        triangleIndices.push_back(t);
    }

    //
    // Second pass: create actual triangles
    //

    Physics::Triangles triangles(static_cast<ElementIndex>(triangleIndices.size()));

    for (ElementIndex t = 0; t < triangleIndices.size(); ++t)
    {
        auto triangleIndex = triangleIndices[t];

        // Create triangle
        triangles.Add(
            triangleInfos[triangleIndex].PointAIndex,
            triangleInfos[triangleIndex].PointBIndex,
            triangleInfos[triangleIndex].PointCIndex);

        // Add triangle to its endpoints
        points.AddConnectedTriangle(triangleInfos[triangleIndex].PointAIndex, t);
        points.AddConnectedTriangle(triangleInfos[triangleIndex].PointBIndex, t);
        points.AddConnectedTriangle(triangleInfos[triangleIndex].PointCIndex, t);
    }

    return triangles;
}

ElectricalElements ShipBuilder::CreateElectricalElements(
    Physics::Points const & points,
    Physics::Springs const & springs,
    Physics::World & parentWorld,
    std::shared_ptr<IGameEventHandler> gameEventHandler)
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
        gameEventHandler);

    for (auto pointIndex : electricalElementPointIndices)
    {
        electricalElements.Add(
            pointIndex,
            points.GetMaterial(pointIndex)->Electrical->ElementType,
            points.GetMaterial(pointIndex)->Electrical->IsSelfPowered);
    }


    //
    // Connect electrical elements that are connected by springs to each other
    //

    for (auto electricalElementIndex : electricalElements)
    {
        auto pointIndex = electricalElements.GetPointIndex(electricalElementIndex);

        for (auto springIndex : points.GetConnectedSprings(pointIndex))
        {
            ElementIndex otherEndpointElectricalElement;

            auto pointAIndex = springs.GetPointAIndex(springIndex);
            if (pointAIndex != pointIndex)
            {
                assert(springs.GetPointBIndex(springIndex) == pointIndex);
                otherEndpointElectricalElement = points.GetElectricalElement(pointAIndex);
            }
            else
            {
                assert(springs.GetPointBIndex(springIndex) != pointIndex);

                otherEndpointElectricalElement = points.GetElectricalElement(springs.GetPointBIndex(springIndex));
            }

            if (NoneElementIndex != otherEndpointElectricalElement)
            {
                electricalElements.AddConnectedElectricalElement(
                    electricalElementIndex,
                    otherEndpointElectricalElement);
            }
        }
    }

    return electricalElements;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex cache optimization
//////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<ShipBuilder::SpringInfo> ShipBuilder::ReorderOptimally(
    std::vector<SpringInfo> & springInfos,
    size_t vertexCount)
{
    std::vector<VertexData> vertexData(vertexCount);
    std::vector<ElementData> elementData(springInfos.size());

    // Fill-in cross-references between vertices and springs
    for (size_t s = 0; s < springInfos.size(); ++s)
    {
        vertexData[springInfos[s].PointAIndex].RemainingElementIndices.push_back(s);
        vertexData[springInfos[s].PointBIndex].RemainingElementIndices.push_back(s);

        elementData[s].VertexIndices.push_back(static_cast<size_t>(springInfos[s].PointAIndex));
        elementData[s].VertexIndices.push_back(static_cast<size_t>(springInfos[s].PointBIndex));
    }

    // Get optimal indices
    auto optimalIndices = ReorderOptimally<2>(
        vertexData,
        elementData);

    // Build optimally-ordered set of springs
    std::vector<SpringInfo> newSpringInfos;
    newSpringInfos.reserve(springInfos.size());
    for (size_t ti : optimalIndices)
    {
        newSpringInfos.push_back(springInfos[ti]);
    }

    return newSpringInfos;
}

std::vector<ShipBuilder::TriangleInfo> ShipBuilder::ReorderOptimally(
    std::vector<TriangleInfo> & triangleInfos,
    size_t vertexCount)
{
    std::vector<VertexData> vertexData(vertexCount);
    std::vector<ElementData> elementData(triangleInfos.size());

    // Fill-in cross-references between vertices and triangles
    for (size_t t = 0; t < triangleInfos.size(); ++t)
    {
        vertexData[triangleInfos[t].PointAIndex].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos[t].PointBIndex].RemainingElementIndices.push_back(t);
        vertexData[triangleInfos[t].PointCIndex].RemainingElementIndices.push_back(t);

        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos[t].PointAIndex));
        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos[t].PointBIndex));
        elementData[t].VertexIndices.push_back(static_cast<size_t>(triangleInfos[t].PointCIndex));
    }

    // Get optimal indices
    auto optimalIndices = ReorderOptimally<3>(
        vertexData,
        elementData);

    // Build optimally-ordered set of triangles
    std::vector<TriangleInfo> newTriangleInfos;
    newTriangleInfos.reserve(triangleInfos.size());
    for (size_t ti : optimalIndices)
    {
        newTriangleInfos.push_back(triangleInfos[ti]);
    }

    return newTriangleInfos;
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
        if (!cache.UseVertex(springInfo.PointAIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(springInfo.PointBIndex))
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
        if (!cache.UseVertex(triangleInfo.PointAIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointBIndex))
        {
            cacheMisses += 1.0f;
        }

        if (!cache.UseVertex(triangleInfo.PointCIndex))
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
    for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
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
