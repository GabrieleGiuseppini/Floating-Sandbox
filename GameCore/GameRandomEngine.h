/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameMath.h"
#include "Vectors.h"

#include <random>

/*
 * The random engine for the entire game.
 *
 * Not so random - always uses the same seed. On purpose! We want two instances
 * of the game to be identical to each other.
 *
 * Singleton.
 */
class GameRandomEngine
{
public:

    static GameRandomEngine & GetInstance()
    {
        static GameRandomEngine * instance = new GameRandomEngine();

        return *instance;
    }

    /*
     * Returns a value between 0 and count - 1, included.
     */
    template <typename T>
    inline T Choose(T count)
    {
        return GenerateUniformInteger<T>(0, count - 1);
    }

    /*
     * Returns a value between 0 and count - 1, included, with the exclusion of
     * previous.
     */
    template <typename T>
    inline T ChooseNew(
        T count,
        T previous)
    {
        // Choose randomly, but avoid choosing the last-chosen again
        T chosen = GenerateUniformInteger<T>(0, count - 2);
        if (chosen >= previous)
        {
            ++chosen;
        }

        return chosen;
    }

    /*
     * Returns a value between first and last, included, with the exclusion of
     * previous, if the latter is in the first-last range.
     */
    template <typename T>
    inline T ChooseNew(
        T first,
        T last,
        T previous)
    {
        // Choose randomly, but avoid choosing the last-chosen again
        if (previous >= first && previous <= last)
        {
            T chosen = GenerateUniformInteger<T>(first, last - 1);
            if (chosen >= previous)
            {
                ++chosen;
            }

            return chosen;
        }
        else
        {
            return GenerateUniformInteger<T>(first, last);
        }
    }

    template <typename T>
    inline T GenerateUniformInteger(
        T minValue,
        T maxValue)
    {
        std::uniform_int_distribution<T> dis(minValue, maxValue);
        return dis(mRandomEngine);
    }

    inline float GenerateNormalizedUniformReal()
    {
        return mRandomUniformDistribution(mRandomEngine);
    }

    inline float GenerateUniformReal(
        float minValue,
        float maxValue)
    {
        return minValue + GenerateNormalizedUniformReal() * (maxValue - minValue);
    }

    inline vec2f GenerateUniformRadialVector(
        float minMagnitude,
        float maxMagnitude)
    {
        //
        // Choose a vector: point on a circle with random radius and random angle
        //

        float const magnitude = GenerateUniformReal(
            minMagnitude, maxMagnitude);

        float const angle = GenerateUniformReal(0.0f, 2.0f * Pi<float>);

        return vec2f::fromPolar(magnitude, angle);
    }

	/*
	 * Returns true with the specified probability. A probability of zero implies
	 * that true is never returned.
	 */
    inline bool GenerateUniformBoolean(float trueProbability)
    {
        return GenerateNormalizedUniformReal() < trueProbability;
    }

    inline float GenerateExponentialReal(float lambda)
    {
        std::exponential_distribution<float> dis(lambda);
        return dis(mRandomEngine);
    }

    /*
     * Generates a random number between -INF and +INF, distributed
     * according to a Gaussian with mean zero and stdev 1.
     */
    inline float GenerateStandardNormalReal()
    {
        return mNormalDistribution(mRandomEngine);
    }

    /*
     * Generates a random number between -INF and +INF, distributed
     * according to a Gaussian with the specified mean and stdev.
     */
    inline float GenerateNormalReal(
        float mean,
        float stdev)
    {
        return mean + mNormalDistribution(mRandomEngine) * stdev;
    }

private:

    GameRandomEngine()
    {
        std::seed_seq seed_seq({ 1, 242, 19730528 });
        mRandomEngine = std::ranlux48_base(seed_seq);
        mRandomUniformDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
        mNormalDistribution = std::normal_distribution<float>(0.0f, 1.0f);
    }

    std::ranlux48_base mRandomEngine;
    std::uniform_real_distribution<float> mRandomUniformDistribution;
    std::normal_distribution<float> mNormalDistribution;
};
