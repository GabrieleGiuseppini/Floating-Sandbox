#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

#include <vector>

size_t MakeSize(size_t count);

std::vector<float> MakeFloats(size_t count);
std::vector<float> MakeFloats(size_t count, float value);
std::vector<PlaneId> MakePlaneIds(size_t count);
std::vector<vec2f> MakeVectors(size_t count);

struct SpringEndpoints
{
    ElementIndex PointAIndex;
    ElementIndex PointBIndex;
};

void MakeGraph(
    size_t count,
    std::vector<vec2f> & points,
    std::vector<SpringEndpoints> & springs);

void MakeGraph2(
    size_t count,
    std::vector<vec2f> & pointsPosition,
    std::vector<vec2f> & pointsVelocity,
    std::vector<vec2f> & pointsForce,
    std::vector<SpringEndpoints> & springsEndpoint,
    std::vector<float> & springsStiffnessCoefficient,
    std::vector<float> & springsDamperCoefficient,
    std::vector<float> & springsRestLength);
