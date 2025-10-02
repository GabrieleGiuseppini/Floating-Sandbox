/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-10-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <filesystem>

class SoundAtlasBaker
{
public:

	static std::tuple<size_t, size_t> Bake(
		std::filesystem::path const & soundsRootDirectoryPath,
		std::filesystem::path const & outputDirectoryPath);
};
