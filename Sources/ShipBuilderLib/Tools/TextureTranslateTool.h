/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-14
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include "../GenericEphemeralVisualizationRestorePayload.h"

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>

#include <optional>

namespace ShipBuilder {

template<LayerType TLayer>
class TextureTranslateTool : public Tool
{
public:

    ~TextureTranslateTool() = default;

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;
    void OnMouseLeft() override {}

protected:

protected:

    TextureTranslateTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    void DoTranslate(
        ImageCoordinates const & startPosition,
        ImageCoordinates const & endPosition,
        bool & isFirst);

private:

    struct EngagementData
    {
        ImageCoordinates StartPosition;
        std::unique_ptr<TextureLayerData> OriginalTextureLayerData;
        bool IsDirty;

        EngagementData(
            ImageCoordinates const & startPosition,
            std::unique_ptr<TextureLayerData> && originalTextureLayerData)
            : StartPosition(startPosition)
            , OriginalTextureLayerData(std::move(originalTextureLayerData))
            , IsDirty(false)
        {}
    };

    // When set, we're engaged (dragging)
    std::optional<EngagementData> mEngagementData;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;
};

class ExteriorTextureTranslateTool final : public TextureTranslateTool<LayerType::ExteriorTexture>
{
public:

    ExteriorTextureTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class InteriorTextureTranslateTool final : public TextureTranslateTool<LayerType::InteriorTexture>
{
public:

    InteriorTextureTranslateTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

}
