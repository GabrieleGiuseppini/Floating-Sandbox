/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>
#include <Game/ShipMetadata.h>

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
        ResourceLocator const & resourceLocator);

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
