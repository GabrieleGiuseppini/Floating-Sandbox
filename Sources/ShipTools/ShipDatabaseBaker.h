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
		struct Entry
		{
			ShipLocator Locator;
			bool HasExternalPreviewImage;

			Entry(
				ShipLocator locator,
				bool hasExternalPreviewImage)
				: Locator(locator)
				, HasExternalPreviewImage(hasExternalPreviewImage)
			{
			}
		};

		std::vector<Entry> Entries;

		explicit ShipDirectory(std::vector<Entry> && entries)
			: Entries(std::move(entries))
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
