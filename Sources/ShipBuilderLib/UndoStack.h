/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Simulation/Layers.h>

#include <wx/string.h>

#include <deque>
#include <memory>

namespace ShipBuilder {

// Forward declarations
class Controller;

class UndoStack
{
public:

    UndoStack()
        : mStack()
        , mTotalCost(0)
    {}

    bool IsEmpty() const
    {
        return mStack.empty();
    }

    size_t GetSize() const
    {
        return mStack.size();
    }

    wxString const & GetTitleAt(size_t index) const
    {
        return mStack.at(index)->Title;
    }

    template<typename F>
    void Push(
        wxString const & title,
        size_t cost,
        ModelDirtyState const & originalDirtyState,
        F && undoFunction)
    {
        auto undoAction = std::make_unique<UndoActionLambda<F>>(
            title,
            cost,
            originalDirtyState,
            std::move(undoFunction));

        // Update total cost
        mTotalCost += cost;

        // Push undo action
        mStack.push_back(std::move(undoAction));

        // Trim stack if too big
        while (mStack.size() > MaxEntries || mTotalCost > MaxCost)
        {
            assert(mTotalCost >= mStack.front()->Cost);
            mTotalCost -= mStack.front()->Cost;
            mStack.pop_front();
        }
    }

    void PopAndApply(Controller & controller);

    void RewindAndApply(
        size_t startIndex, // Last index remaining after rewind
        Controller & controller);

private:

    struct UndoAction
    {
        wxString Title;
        size_t Cost;
        ModelDirtyState OriginalDirtyState; // The model's dirty state that was in effect when the edit action being undode was applied

        virtual ~UndoAction() = default;

        virtual void ApplyAndConsume(Controller & controller) = 0;

    protected:

        UndoAction(
            wxString const & title,
            size_t cost,
            ModelDirtyState const & originalDirtyState)
            : Title(title)
            , Cost(cost)
            , OriginalDirtyState(originalDirtyState)
        {}
    };

    template<typename F>
    struct UndoActionLambda final : public UndoAction
    {
        F UndoFunction;

        UndoActionLambda(
            wxString const & title,
            size_t cost,
            ModelDirtyState const & originalDirtyState,
            F && undoFunction)
            : UndoAction(
                title,
                cost,
                originalDirtyState)
            , UndoFunction(std::move(undoFunction))
        {}

        void ApplyAndConsume(Controller & controller) override
        {
            UndoFunction(controller);
        }
    };

private:

    static size_t constexpr MaxEntries = 20;
    static size_t constexpr MaxCost = (1000 * 1000) * 80;

    std::deque<std::unique_ptr<UndoAction>> mStack;
    size_t mTotalCost;
};

}