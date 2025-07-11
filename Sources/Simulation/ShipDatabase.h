/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2025-07-09
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipDefinitionFormatDeSerializer.h"
#include "ShipLocator.h"
#include "ShipPreviewData.h"

#include <Core/GameTypes.h>
#include <Core/ImageData.h>
#include <Core/Streams.h>
#include <Core/TextureAtlas.h>

#include <picojson.h>

#include <string>
#include <vector>

/*
 * Note: this is used exclusively by the Android port; it's here for use by ShipTools, which is easier to operate on Windows.
 */

struct ShipDatabase final
{
	static int constexpr MaxPreviewAtlasSize = 4096;

	static std::string const SpecificationFilename;

	static std::string MakePreviewAtlasFilename(size_t atlasIndex);

	struct ShipEntry
	{
		ShipLocator Locator;

		ShipPreviewData PreviewData;

		size_t PreviewAtlasIndex;
		TextureFrameIndex PreviewFrameIndex;

		ShipEntry(
			ShipLocator && locator,
			ShipPreviewData && previewData,
			size_t previewAtlasIndex,
			TextureFrameIndex previewFrameIndex)
			: Locator(std::move(locator))
			, PreviewData(std::move(previewData))
			, PreviewAtlasIndex(previewAtlasIndex)
			, PreviewFrameIndex(previewFrameIndex)
		{ }

		picojson::value Serialize() const;

		static ShipEntry Deserialize(picojson::value const & entryRoot);
	};

	std::vector<ShipEntry> Ships;

	enum class ShipPreviewTextureGroups : uint16_t
	{
		Preview = 0,

		_Last = Preview
	};

	struct ShipPreviewTextureDatabase
	{
		static inline std::string DatabaseName = "ShipPreview";

		using TextureGroupsType = ShipPreviewTextureGroups;

		static ShipPreviewTextureGroups StrToTextureGroup(std::string const & str)
		{
			if (Utils::CaseInsensitiveEquals(str, "Preview"))
				return ShipPreviewTextureGroups::Preview;
			else
				throw GameException("Unrecognized ShipPreview texture group \"" + str + "\"");
		}
	};

	std::vector<TextureAtlasMetadata<ShipPreviewTextureDatabase>> PreviewAtlasMetadatas;

	ShipDatabase(
		std::vector<ShipEntry> && ships,
		std::vector<TextureAtlasMetadata<ShipPreviewTextureDatabase>> && previewAtlasMetadatas)
		: Ships(std::move(ships))
		, PreviewAtlasMetadatas(std::move(previewAtlasMetadatas))
	{ }

	picojson::value Serialize() const;

	static ShipDatabase Deserialize(picojson::value const & specification);
};

class ShipDatabaseBuilder
{
public:

	explicit ShipDatabaseBuilder(ImageSize maxPreviewImageSize)
		: mMaxPreviewImageSize(maxPreviewImageSize)
	{ }

	void AddShip(BinaryReadStream && inputStream, ShipLocator locator);

	struct Output
	{
		ShipDatabase Database;
		std::vector<RgbaImageData> PreviewAtlasImages;

		Output(
			ShipDatabase && database,
			std::vector<RgbaImageData> && previewAtlasImages)
			: Database(std::move(database))
			, PreviewAtlasImages(std::move(previewAtlasImages))
		{ }
	};

	Output Build();

private:

	ImageSize const mMaxPreviewImageSize;

	struct WorkingListEntry
	{
		ShipLocator Locator;
		ShipPreviewData PreviewData;
		TextureFrame<ShipDatabase::ShipPreviewTextureDatabase> PreviewImageFrame;

		WorkingListEntry(
			ShipLocator && locator,
			ShipPreviewData && previewData,
			TextureFrame<ShipDatabase::ShipPreviewTextureDatabase> && previewImageFrame)
			: Locator(std::move(locator))
			, PreviewData(std::move(previewData))
			, PreviewImageFrame(std::move(previewImageFrame))
		{ }
	};

	std::vector<WorkingListEntry> mWorkingList;
};