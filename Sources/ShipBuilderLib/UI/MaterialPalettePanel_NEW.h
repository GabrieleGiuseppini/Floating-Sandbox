/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2026-08-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "../ShipBuilderTypes.h"

#include <Game/GameAssetManager.h>

#include <Simulation/Materials.h>
#include <Simulation/ShipTexturizer.h>

#include <wx/wx.h>

#include <vector>

namespace ShipBuilder {

struct IMaterialPalettePanel : public wxPanel
{
public:

    IMaterialPalettePanel(wxWindow * parent)
        : wxPanel(parent)
    { }

    virtual ~IMaterialPalettePanel() = default;
};

template<LayerType TLayer>
class MaterialPalettePanel final :
    public IMaterialPalettePanel
{
public:

    using TMaterial = typename LayerTypeTraits<TLayer>::material_type;

    MaterialPalettePanel(
        wxWindow * parent,
        ShipTexturizer const & shipTexturizer,
        GameAssetManager const & gameAssetManager);

    void StartBuild();
    void Add(TMaterial const * material, bool startNewRow);
    void AddSeparator();
    void EndBuild();

private:

    void OnPaint(wxPaintEvent & event);

    void RenderPanel();

private:

    ShipTexturizer const & mShipTexturizer;
    GameAssetManager const & mGameAssetManager;

    std::unique_ptr<wxBitmap> mRenderBuffer;
};

}