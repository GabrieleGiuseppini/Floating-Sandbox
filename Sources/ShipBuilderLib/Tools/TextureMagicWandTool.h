/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/GameAssetManager.h>

namespace ShipBuilder {

template<LayerType TLayerType>
class TextureMagicWandTool : public Tool
{
public:

    virtual ~TextureMagicWandTool() = default;

    void OnMouseMove(DisplayLogicalCoordinates const &) override {}
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override {};
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {};
    void OnShiftKeyDown() override {};
    void OnShiftKeyUp() override {};
    void OnMouseLeft() override {};

protected:

    TextureMagicWandTool(
        ToolType toolType,
        Controller & controller,
        GameAssetManager const & gameAssetManager);

private:

    ImageSize GetTextureSize() const;
};

class ExteriorTextureMagicWandTool final : public TextureMagicWandTool<LayerType::ExteriorTexture>
{
public:

    ExteriorTextureMagicWandTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};

class InteriorTextureMagicWandTool final : public TextureMagicWandTool<LayerType::InteriorTexture>
{
public:

    InteriorTextureMagicWandTool(
        Controller & controller,
        GameAssetManager const & gameAssetManager);
};


}