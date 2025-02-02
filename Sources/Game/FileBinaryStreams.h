/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FileSystem.h"

#include <Core/BinaryStreams.h>
#include <Core/GameException.h>

#include <filesystem>
#include <fstream>
#include <memory>

/*
 * Implementation of BinaryReadStream for file streams.
 */
class FileBinaryReadStream final : public BinaryReadStream
{
public:

	FileBinaryReadStream(std::filesystem::path const & filePath)
		: mStream(FileSystem::OpenBinaryInputStream(filePath))
	{
		if (!mStream)
		{
			throw GameException("Cannot open file \"" + filePath.filename().string() + "\" for reading");
		}
	}

	~FileBinaryReadStream()
	{
		mStream.reset();
	}

	size_t GetCurrentPosition() const override
	{
		return static_cast<size_t>(mStream->tellg());
	}

	size_t Read(std::uint8_t * buffer, size_t size) override
	{
		mStream->read(reinterpret_cast<char *>(buffer), size);
		return mStream->gcount();
	}

	size_t Skip(size_t size) override
	{
		auto const prePosition = mStream->tellg();
		mStream->seekg(size, std::ios_base::cur);
		auto const postPosition = mStream->tellg();
		return static_cast<size_t>(postPosition - prePosition);
	}

private:

	std::shared_ptr<std::istream> mStream;
};

/*
 * Implementation of BinaryWriteStream for file streams.
 */
class FileBinaryWriteStream final : public BinaryWriteStream
{
public:

	FileBinaryWriteStream(std::filesystem::path const & filePath)
		: mStream(FileSystem::OpenBinaryOutputStream(filePath))
	{
		if (!mStream)
		{
			throw GameException("Cannot open file \"" + filePath.filename().string() + "\" for writing");
		}
	}

	~FileBinaryWriteStream()
	{
		mStream->flush();
		mStream.reset();
	}

	void Write(std::uint8_t const * buffer, size_t size) override
	{
		mStream->write(reinterpret_cast<char const *>(buffer), size);
	}

private:

	std::shared_ptr<std::ostream> mStream;
};
