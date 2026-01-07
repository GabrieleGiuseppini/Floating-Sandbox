/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-10-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <Core/GameTypes.h>
#include <Core/ImageData.h>

#include <wx/dialog.h>
#include <wx/sizer.h>

#include <array>

namespace ShipBuilder {

class BaseResizeDialog : public wxDialog
{
public:

    bool ShowModal(
        RgbaImageData const & image,
        ShipSpaceSize const & shipSize);

protected:

    void CreateLayout(
        wxWindow * parent,
        wxString const & caption,
        GameAssetManager const & gameAssetManager);

    virtual void InternalCreateLayout(
        wxBoxSizer * dialogVSizer,
        GameAssetManager const & gameAssetManager) = 0;

    virtual void InternalReconciliateUI(
        RgbaImageData const & image,
        ShipSpaceSize const & shipSize) = 0;

    virtual void InternalOnClose() = 0;

private:

    // Make ShowModal() inaccessible from outside
    using wxDialog::ShowModal;

    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
};

}