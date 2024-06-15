/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/ResourceLocator.h>

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
        ResourceLocator const & resourceLocator);

private:

    ImageSize GetTextureSize() const;
};

class ExteriorTextureMagicWandTool final : public TextureMagicWandTool<LayerType::ExteriorTexture>
{
public:

    ExteriorTextureMagicWandTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};

class InteriorTextureMagicWandTool final : public TextureMagicWandTool<LayerType::InteriorTexture>
{
public:

    InteriorTextureMagicWandTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);
};


}