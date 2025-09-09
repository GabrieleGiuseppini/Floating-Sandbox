/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-09-07
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ImageFileTools.h"

#include <Core/MemoryStreams.h>
#include <Core/PngTools.h>

#include <fstream>

void ImageFileTools::SaveImage(
    RgbaImageData const & imageData,
    std::string const & path)
{
    MemoryBinaryWriteStream writeStream;
    PngTools::EncodeImage(imageData, writeStream);

	auto outFile = std::ofstream(path, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
	outFile.write(reinterpret_cast<char const *>(writeStream.GetData()), writeStream.GetSize());
	outFile.flush();
	outFile.close();
}
