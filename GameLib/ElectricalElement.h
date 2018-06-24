/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Physics.h"

#include <cstdint>

namespace Physics
{	

class ElectricalElement
{
public:

    enum class Type
    {
        Cable,
        Generator,
        Lamp
    };

public:

    virtual ~ElectricalElement()
    {
    }
    
    inline ElementIndex GetPointIndex() const
    {
        return mPointIndex;
    }

    inline Type GetType() const
    {
        return mType;
    }

    inline uint64_t GetLastGraphVisitStepSequenceNumber() const
    {
        return mLastGraphVisitStepSequenceNumber;
    }

    inline void RecordGraphVisit(uint64_t currentStepSequenceNumber)
    {
        mLastGraphVisitStepSequenceNumber = currentStepSequenceNumber;
    }

protected:

    ElectricalElement(        
        ElementIndex pointIndex,
        Type type);

    ElementIndex const mPointIndex;
    Type const mType;

    // The sequence number of the step on which we last graph-visited this element
    uint64_t mLastGraphVisitStepSequenceNumber;
};

}
