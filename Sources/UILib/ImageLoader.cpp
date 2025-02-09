/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-01-25
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageLoader.h"

#include <Game/GameAssetManager.h>

#include <wx/bitmap.h>
#include <wx/image.h>
//#include <wx/wx.h>

#include <cassert>
#include <stdexcept>

namespace ImageLoader {

RgbaImageData LoadImageRgba(std::filesystem::path const & filepath)
{
	if (filepath.extension() == ".jpg")
	{
		wxImage image = wxImage(filepath.string(), wxBITMAP_TYPE_JPEG);
		if (!image.IsOk())
		{
			throw std::runtime_error("Cannot load JPG image \"" + filepath.string() + "\"");
		}

		if (!image.HasAlpha())
		{
			image.InitAlpha();
		}

		image = image.Mirror(false);

		RgbaImageData result(ImageSize(image.GetWidth(), image.GetHeight()));

		unsigned char const * rgbSrc = image.GetData();
		unsigned char const * alphaSrc = image.GetAlpha();
		assert(alphaSrc != nullptr);

		rgbaColor * rgbaTrg = result.Data.get();

		size_t const linearSize = image.GetWidth() * image.GetHeight();
		for (size_t i = 0; i < linearSize; ++i, rgbSrc += 3, alphaSrc += 1, rgbaTrg += 1)
		{
			rgbaColor const c(rgbSrc[0], rgbSrc[1], rgbSrc[2], alphaSrc[0]);
			*rgbaTrg = c;
		}

		return result;
	}
	else
	{
		assert(filepath.extension() == ".png");
		return GameAssetManager::LoadPngImageRgba(filepath);
	}
}

}