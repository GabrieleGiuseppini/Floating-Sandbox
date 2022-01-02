/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-04
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <functional>

class Finalizer final
{
public:

    Finalizer(std::function<void()> finalizerAction)
        : mFinalizerAction(std::move(finalizerAction))
    {}

    ~Finalizer()
    {
        mFinalizerAction();
    }

private:

    std::function<void()> mFinalizerAction;
};
