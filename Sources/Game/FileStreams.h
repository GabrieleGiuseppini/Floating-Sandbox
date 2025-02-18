/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Core/GameException.h>
#include <Core/Streams.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

/*
 * Implementation of BinaryReadStream for file streams.
 */
class FileBinaryReadStream final : public BinaryReadStream
{
public:

	FileBinaryReadStream(std::filesystem::path const & filePath)
	{
		mStream = std::ifstream(filePath, std::ios::in | std::ios::binary);
		if (!mStream.is_open())
		{
			throw GameException("Cannot open file \"" + filePath.string() + "\" for reading");
		}
	}

	~FileBinaryReadStream()
	{
		mStream.close();
	}

	size_t GetSize() override
	{
		auto const currentPos = mStream.tellg();
		mStream.seekg(0, std::ios::end);
		auto const size = mStream.tellg();
		mStream.seekg(currentPos);
		return size;
	}

	size_t GetCurrentPosition() override
	{
		return static_cast<size_t>(mStream.tellg());
	}

	void SetPosition(size_t offset) override
	{
		mStream.seekg(offset, std::ios_base::beg);
	}

	size_t Read(std::uint8_t * buffer, size_t size) override
	{
		mStream.read(reinterpret_cast<char *>(buffer), size);
		return mStream.gcount();
	}

	size_t Skip(size_t size) override
	{
		auto const prePosition = mStream.tellg();
		mStream.seekg(size, std::ios_base::cur);
		auto const postPosition = mStream.tellg();
		return static_cast<size_t>(postPosition - prePosition);
	}

private:

	std::ifstream mStream;
};

/*
 * Implementation of TextReadStream for file streams.
 */
class FileTextReadStream final : public TextReadStream
{
public:

	FileTextReadStream(std::filesystem::path const & filePath)
	{
		mStream = std::ifstream(filePath, std::ios::in);
		if (!mStream.is_open())
		{
			throw GameException("Cannot open file \"" + filePath.string() + "\" for reading");
		}
	}

	~FileTextReadStream()
	{
		mStream.close();
	}

	std::string ReadAll() override
	{
		std::stringstream ss;
		ss << mStream.rdbuf();
		std::string content = ss.str();

		// For some reason, the preferences file sometimes is made of all null characters
		content.erase(
			std::find(content.begin(), content.end(), '\0'),
			content.end());

		return content;
	}

	std::vector<std::string> ReadAllLines() override
	{
		std::vector<std::string> lines;

		std::string line;
		while (std::getline(mStream, line))
		{
			lines.emplace_back(line);
		}

		return lines;
	}

private:

	std::ifstream mStream;
};

/*
 * Implementation of BinaryWriteStream for file streams.
 */
class FileBinaryWriteStream final : public BinaryWriteStream
{
public:

	FileBinaryWriteStream(std::filesystem::path const & filePath)
	{
		mStream = std::ofstream(filePath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
		if (!mStream)
		{
			throw GameException("Cannot open file \"" + filePath.filename().string() + "\" for writing");
		}
	}

	~FileBinaryWriteStream()
	{
		mStream.flush();
		mStream.close();
	}

	void Write(std::uint8_t const * buffer, size_t size) override
	{
		mStream.write(reinterpret_cast<char const *>(buffer), size);
	}

private:

	std::ofstream mStream;
};

/*
 * Implementation of TextWriteStream for file streams.
 */
class FileTextWriteStream final : public TextWriteStream
{
public:

	FileTextWriteStream(std::filesystem::path const & filePath)
	{
		mStream = std::ofstream(filePath, std::ios_base::out | std::ios_base::trunc);
		if (!mStream)
		{
			throw GameException("Cannot open file \"" + filePath.filename().string() + "\" for writing");
		}
	}

	~FileTextWriteStream()
	{
		mStream.flush();
		mStream.close();
	}

	void Write(std::string const & content) override
	{
		mStream << content;
	}

private:

	std::ofstream mStream;
};
