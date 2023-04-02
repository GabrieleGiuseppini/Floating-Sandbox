/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2022-06-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <set>
#include <cstddef>

class SystemThreadManager final
{
public:

    static SystemThreadManager & GetInstance()
    {
        static SystemThreadManager * instance = new SystemThreadManager();

        return *instance;
    }

    size_t GetNumberOfProcessors() const;

    void InitializeThisThread();

private:

    using CpuIdType = std::uint8_t;

    SystemThreadManager()
    {}

    void AffinitizeThisThread();

    std::set<CpuIdType> mAllocatedProcessors;
};
