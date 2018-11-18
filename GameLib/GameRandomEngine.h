/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-05-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

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

public:

    static GameRandomEngine & GetInstance()
    {
        static GameRandomEngine * instance = new GameRandomEngine();
        
        return *instance;
    }

    template <typename T>
    inline T Choose(T count)
    {
        return GenerateRandomInteger<T>(0, count - 1);
    }

    template <typename T>
    inline T ChooseNew(
        T count,
        T previous)
    {
        // Choose randomly, but avoid choosing the last-chosen again
        T chosen = GenerateRandomInteger<T>(0, count - 2);
        if (chosen >= previous)
        {
            ++chosen;
        }

        return chosen;
    }

    template <typename T>
    inline T ChooseNew(
        T first,
        T last,
        T previous)
    {
        // Choose randomly, but avoid choosing the last-chosen again
        if (previous >= first && previous <= last)
        {
            T chosen = GenerateRandomInteger<T>(first, last - 1);
            if (chosen >= previous)
            {
                ++chosen;
            }

            return chosen;
        }
        else
        {
            return GenerateRandomInteger<T>(first, last);
        }
    }

    template <typename T>
    inline T GenerateRandomInteger(
        T minValue,
        T maxValue)
    {
        std::uniform_int_distribution<T> dis(minValue, maxValue);
        return dis(mRandomEngine);
    }

    inline float GenerateRandomNormalReal()
    {        
        return mRandomNormalDistribution(mRandomEngine);
    }

    inline float GenerateRandomReal(
        float minValue,
        float maxValue)
    {
        return minValue + mRandomNormalDistribution(mRandomEngine) * (maxValue - minValue);
    }

private:

    GameRandomEngine()
    {
        std::seed_seq seed_seq({ 1, 242, 19730528 });
        mRandomEngine = std::ranlux48_base(seed_seq);
        mRandomNormalDistribution = std::uniform_real_distribution<float>(0.0f, 1.0f);
    }

    std::ranlux48_base mRandomEngine;
    std::uniform_real_distribution<float> mRandomNormalDistribution;
};
