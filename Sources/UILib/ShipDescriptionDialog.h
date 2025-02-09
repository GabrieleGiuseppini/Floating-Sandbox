/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/GameAssetManager.h>

#include <Simulation/ShipMetadata.h>

#include <wx/custombgwin.h>
#include <wx/dialog.h>

#include <optional>

class ShipDescriptionDialog : public wxCustomBackgroundWindow<wxDialog>
{
public:

    ShipDescriptionDialog(
        wxWindow* parent,
        ShipMetadata const & shipMetadata,
        bool isAutomatic,
        GameAssetManager const & gameAssetManager);

    virtual ~ShipDescriptionDialog();

    std::optional<bool> GetShowDescriptionsUserPreference() const
    {
        return mShowDescriptionsUserPreference;
    }

private:

    static std::string MakeHtml(ShipMetadata const & shipMetadata);

private:

    std::optional<bool> mShowDescriptionsUserPreference;
};
