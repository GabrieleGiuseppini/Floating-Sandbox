/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <Game/LayerBuffers.h>

#include <wx/string.h>

#include <string>

namespace ShipBuilder {

// Forward declarations
class Controller;

/*
 * Base class of the hierarchy of undo actions.
 *
 * Some examples of specializations include:
 * - Material region replace
 * - Resize
 */
class UndoAction
{
public:

    virtual ~UndoAction() = default;

    wxString const & GetTitle() const
    {
        return mTitle;
    }

    size_t GetCost() const
    {
        return mCost;
    }

    virtual void Apply(Controller & controller) const = 0;

protected:

    UndoAction(
        wxString const & title,
        size_t cost)
        : mTitle(title)
        , mCost(cost)
    {}

private:

    wxString const mTitle;
    size_t const mCost;
};

template<typename TLayerBuffer>
class LayerBufferRegionUndoAction final : public UndoAction
{
public:

    LayerBufferRegionUndoAction(
        wxString const & title,
        TLayerBuffer && layerBufferRegion,
        ShipSpaceCoordinates const & origin)
        : UndoAction(
            title,
            layerBufferRegion.GetByteSize())
        , mLayerBufferRegion(std::move(layerBufferRegion))
        , mOrigin(origin)
    {}

    void Apply(Controller & controller) const override;

private:

    TLayerBuffer mLayerBufferRegion;
    ShipSpaceCoordinates mOrigin;
};

class UndoStack
{
    // TODO
};

}