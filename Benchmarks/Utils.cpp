#include "Utils.h"

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

std::vector<float> MakeFloats(size_t count, float value)
{
    std::vector<float> floats(count, value);

    return floats;
}