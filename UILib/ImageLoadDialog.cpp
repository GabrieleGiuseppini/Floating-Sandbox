/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-12-07
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageLoadDialog.h"

ImageLoadDialog::ImageLoadDialog(wxWindow * parent)
	: wxFileDialog(
		parent,
		wxEmptyString,
		wxEmptyString,
		wxEmptyString,
		wxEmptyString,
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW,
		wxDefaultPosition,
		wxDefaultSize)
{
	SetMessage(_("Load an Image"));

	SetWildcard(_("Image files") + wxS(" (*.jpg; *.png)|*.jpg; *.png"));
}
