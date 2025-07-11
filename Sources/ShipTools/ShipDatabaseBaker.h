/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-07-09
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/

#include <Simulation/ShipLocator.h>

#include <Core/GameTypes.h>

#include <filesystem>
#include <vector>

class ShipDatabaseBaker
{
public:

	struct ShipDirectory
	{
		std::vector<ShipLocator> Locators;

		explicit ShipDirectory(std::vector<ShipLocator> && locators)
			: Locators(std::move(locators))
		{
		}

		static ShipDirectory Deserialize(picojson::value const & specification);
	};

    static void Bake(
        std::filesystem::path const & inputShipDirectoryFilePath,
        std::filesystem::path const & inputShipRootDirectoryPath,
        std::filesystem::path const & outputDirectoryPath,
        ImageSize const & maxPreviewImageSize);
};
