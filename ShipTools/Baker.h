/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-10
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <filesystem>
#include <string>

class Baker
{
public:

    static void BakeRegularAtlas(
        std::filesystem::path const & jsonSpecificationFilePath,
        std::filesystem::path const & outputDirectoryPath,
        std::string const & atlasFilenamesStem);
};
