/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2024-11-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"
#include "SysSpecifics.h"

#include <cassert>
#include <memory>
#include <vector>

template<typename TValue>
class FixedSetSliderCore final : public ISliderCore<TValue>
{
public:

    FixedSetSliderCore(std::vector<TValue> values)
        : mValues(std::move(values))
    {
        assert(mValues.size() >= 2);
        for (int i = 0; i < mValues.size() - 1; ++i)
        {
            assert(mValues[i] < mValues[i + 1]);
        }
    }

    static std::unique_ptr<FixedSetSliderCore> FromPowersOfTwo(
        TValue minValue,
        TValue maxValue)
    {
        assert(minValue < maxValue);
        assert(minValue != 0);
        assert((maxValue / minValue) == ceil_power_of_two(maxValue / minValue));

        std::vector<TValue> values;
        for (TValue v = minValue; v <= maxValue; v *= 2)
        {
            values.push_back(v);
        }

        return std::make_unique<FixedSetSliderCore<TValue>>(std::move(values));
    }

    int GetNumberOfTicks() const override
    {
        return static_cast<int>(mValues.size());
    }

    TValue TickToValue(int tick) const override
    {
        assert(tick >= 0 && tick < mValues.size());
        return mValues[tick];
    }

    int ValueToTick(TValue value) const override
    {
        for (int i = 0; i < mValues.size() - 1; ++i)
        {
            if (value < mValues[i + 1])
            {
                // Find closest
                if ((value - mValues[i]) < (mValues[i + 1] - value))
                {
                    return i;
                }
                else
                {
                    return i+1;
                }
            }
        }

        // No luck
        return static_cast<int>(mValues.size() - 1);
    }

    TValue const & GetMinValue() const override
    {
        return mValues[0];
    }

    TValue const & GetMaxValue() const override
    {
        return mValues[mValues.size() - 1];
    }

private:

    std::vector<TValue> const mValues;
};
