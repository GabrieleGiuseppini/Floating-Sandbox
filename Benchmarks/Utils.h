#include <GameLib/GameTypes.h>
#include <GameLib/Vectors.h>

#include <vector>

size_t MakeSize(size_t count);

std::vector<float> MakeFloats(size_t count);
std::vector<float> MakeFloats(size_t count, float value);

std::vector<vec2f> MakeVectors(size_t count);

struct Spring
{
    ElementIndex PointAIndex;
    ElementIndex PointBIndex;
};

void MakeGraph(
    size_t count,
    std::vector<vec2f> & points,
    std::vector<Spring> & springs);