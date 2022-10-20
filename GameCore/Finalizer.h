/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-04
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <functional>

struct [[nodiscard]] Finalizer final
{
public:

    Finalizer(std::function<void()> finalizerAction)
        : mFinalizerAction(std::move(finalizerAction))
    {}

    ~Finalizer()
    {
        if (mFinalizerAction)
        {
            mFinalizerAction();
        }
    }

    Finalizer(Finalizer const &) = delete;
    Finalizer(Finalizer &&) = default;

    Finalizer & operator=(Finalizer const &) = delete;
    Finalizer & operator=(Finalizer &&) = default;

private:

    std::function<void()> mFinalizerAction;
};
