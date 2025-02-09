/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-03-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageSaveDialog.h"

ImageSaveDialog::ImageSaveDialog(wxWindow * parent)
	: wxFileDialog(
		parent,
		wxEmptyString,
		wxEmptyString,
		wxEmptyString,
		wxEmptyString,
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_PREVIEW,
		wxDefaultPosition,
		wxDefaultSize)
{
	SetMessage(_("Save an Image"));

	SetWildcard(_("Image files") + wxS(" (*.png)|*.png"));
}
