/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-09-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameTypes.h>
#include <Core/Vectors.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class RecordedEvent
{
public:

    virtual ~RecordedEvent()
    {}

    enum class RecordedEventType
    {
        PointDetachForDestroy,
        TriangleDestroy
    };

    RecordedEventType GetType() const
    {
        return mType;
    }

    virtual std::string ToString() const = 0;

protected:

    RecordedEvent(RecordedEventType type)
        : mType(type)
    {
    }

private:

    RecordedEventType const mType;
};

class RecordedPointDetachForDestroyEvent final : public RecordedEvent
{
public:

    RecordedPointDetachForDestroyEvent(
        ElementIndex pointIndex,
        vec2f detachVelocity,
        float simulationTime)
        : RecordedEvent(RecordedEventType::PointDetachForDestroy)
        , mPointIndex(pointIndex)
        , mDetachVelocity(detachVelocity)
        , mSimulationTime(simulationTime)
    {}

    ElementIndex GetPointIndex() const
    {
        return mPointIndex;
    }

    vec2f const & GetDetachVelocity() const
    {
        return mDetachVelocity;
    }

    float GetSimulationTime() const
    {
        return mSimulationTime;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "PointDetachOnDestroy:" << mPointIndex;
        return ss.str();
    }

private:

    ElementIndex const mPointIndex;
    vec2f const mDetachVelocity;
    float const mSimulationTime;
};

class RecordedTriangleDestroyEvent final : public RecordedEvent
{
public:

    RecordedTriangleDestroyEvent(ElementIndex pointIndex)
        : RecordedEvent(RecordedEventType::TriangleDestroy)
        , mPointIndex(pointIndex)
    {}

    ElementIndex GetPointIndex() const
    {
        return mPointIndex;
    }

    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "TriangleDestroy:" << mPointIndex;
        return ss.str();
    }

private:

    ElementIndex const mPointIndex;
};

class RecordedEvents
{
public:

    RecordedEvents(std::vector<std::unique_ptr<RecordedEvent>> && recordedEvents)
        : mEvents(std::move(recordedEvents))
    {
    }

    size_t GetSize() const
    {
        return mEvents.size();
    }

    RecordedEvent const & GetEvent(size_t index) const
    {
        return *mEvents[index];
    }

private:

    std::vector<std::unique_ptr<RecordedEvent>> mEvents;
};

class EventRecorder
{
public:

    EventRecorder(std::function<void(uint32_t, RecordedEvent const &)> onEventCallback)
        : mEvents()
        , mOnEventCallback(std::move(onEventCallback))
    {}

    template<typename TRecordedEvent, typename... Args>
    void RecordEvent(Args&&... args)
    {
        mEvents.emplace_back(
            new TRecordedEvent(std::forward<Args>(args)...));

        if (mOnEventCallback)
        {
            mOnEventCallback(
                static_cast<uint32_t>(mEvents.size() - 1),
                *(mEvents.back()));
        }
    }

    RecordedEvents StopRecording()
    {
        return RecordedEvents(std::move(mEvents));
    }

private:

    std::vector<std::unique_ptr<RecordedEvent>> mEvents;

    std::function<void(uint32_t, RecordedEvent const &)> mOnEventCallback;
};