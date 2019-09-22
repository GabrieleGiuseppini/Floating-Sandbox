#include "Utils.h"

size_t MakeSize(size_t count)
{
    if (0 == (count % 16))
        return count;
    else
        return count + 16 - (count % 16);
}

std::vector<float> MakeFloats(size_t count)
{
    std::vector<float> floats;
    floats.reserve(count);

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats.push_back(static_cast<float>(i));
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats.push_back(static_cast<float>(i) / 1000000.0f);
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats.push_back(static_cast<float>(i) / 0.000001f);
    }

    for (size_t i = 0; i < count / 4; ++i)
    {
        floats.push_back(25.0f / (static_cast<float>(i) + 1));
    }

    return floats;
}

std::vector<PlaneId> MakePlaneIds(size_t count)
{
    std::vector<PlaneId> planeIds;
    planeIds.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        planeIds.push_back(static_cast<PlaneId>(i % 100));
    }

    return planeIds;
}

std::vector<float> MakeFloats(size_t count, float value)
{
    std::vector<float> floats(count, value);

    return floats;
}

std::vector<vec2f> MakeVectors(size_t count)
{
    std::vector<vec2f> vectors;
    vectors.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        vectors.emplace_back(static_cast<float>(i), static_cast<float>(i) / 5.0f);
    }

    return vectors;
}

void MakeGraph(
    size_t count,
    std::vector<vec2f> & points,
    std::vector<SpringEndpoints> & springs)
{
    points.clear();
    points.reserve(count);

    springs.clear();
    springs.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        points.emplace_back(static_cast<float>(i), static_cast<float>(i) / 5.0f);
        springs.push_back({
            static_cast<ElementIndex>(i < count / 2 ? i + count / 2 : i),
            static_cast<ElementIndex>(i >= count / 2 ? i - count / 2 : i)
        });
    }
}

void MakeGraph2(
    size_t count,
    std::vector<vec2f> & pointsPosition,
    std::vector<vec2f> & pointsVelocity,
    std::vector<vec2f> & pointsForce,
    std::vector<SpringEndpoints> & springsEndpoints,
    std::vector<float> & springsStiffnessCoefficient,
    std::vector<float> & springsDamperCoefficient,
    std::vector<float> & springsRestLength)
{
    pointsPosition.clear();
    pointsPosition.reserve(count);
    pointsVelocity.clear();
    pointsVelocity.reserve(count);
    pointsForce.clear();
    pointsForce.reserve(count);

    springsEndpoints.clear();
    springsEndpoints.reserve(count);
    springsStiffnessCoefficient.clear();
    springsStiffnessCoefficient.reserve(count);
    springsDamperCoefficient.clear();
    springsDamperCoefficient.reserve(count);
    springsRestLength.clear();
    springsRestLength.reserve(count);

    for (size_t i = 0; i < count; ++i)
    {
        pointsPosition.emplace_back(static_cast<float>(i), static_cast<float>(i) / 5.0f);
        pointsVelocity.emplace_back(static_cast<float>(i) * 0.3f, static_cast<float>(i) / 2.0f);
        pointsForce.emplace_back(0.f, 0.f);

        springsEndpoints.push_back({
            static_cast<ElementIndex>(i < count / 2 ? i + count / 2 : i),
            static_cast<ElementIndex>(i >= count / 2 ? i - count / 2 : i)
            });

        springsStiffnessCoefficient.emplace_back(static_cast<float>(i) * 0.4f);
        springsDamperCoefficient.emplace_back(static_cast<float>(i) * 0.5f);
        springsRestLength.emplace_back(1.0f + static_cast<float>(i % 2));
    }
}