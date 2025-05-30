/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Streams.h"

#include <cstring>
#include <sstream>
#include <vector>

/*
 * Implementation of BinaryReadStream for in-memory streams.
 */
class MemoryBinaryReadStream final : public BinaryReadStream
{
public:

	MemoryBinaryReadStream(std::vector<std::uint8_t> && data)
		: mData(std::move(data))
		, mReadOffset(0u)
	{}

	size_t GetSize() override
	{
		return mData.size();
	}

	size_t GetCurrentPosition() override
	{
		return mReadOffset;
	}

	void SetPosition(size_t offset) override
	{
		mReadOffset = std::min(offset, mData.size());
	}

	size_t Read(std::uint8_t * buffer, size_t size) override
	{
		size_t const szToRead = std::min(size, mData.size() - mReadOffset);
		std::memcpy(buffer, &(mData[mReadOffset]), szToRead);
		mReadOffset += szToRead;
		return szToRead;
	}

	size_t Skip(size_t size) override
	{
		size_t const szToRead = std::min(size, mData.size() - mReadOffset);
		mReadOffset += szToRead;
		return szToRead;
	}

private:

	std::vector<std::uint8_t> const mData;
	size_t mReadOffset;
};

/*
 * Implementation of TextReadStream for in-memory streams.
 */
class MemoryTextReadStream final : public TextReadStream
{
public:

	MemoryTextReadStream(std::string && data)
		: mData(std::move(data))
	{}

	std::string ReadAll() override
	{
		return mData;
	}

	std::vector<std::string> ReadAllLines() override
	{
		std::vector<std::string> lines;

		std::stringstream ss(mData);
		std::string line;
		while (std::getline(ss, line))
		{
			lines.emplace_back(line);
		}

		return lines;
	}

private:

	std::string const mData;
};

/*
 * Implementation of BinaryWriteStream for in-memory streams.
 */
class MemoryBinaryWriteStream final : public BinaryWriteStream
{
public:

	MemoryBinaryWriteStream()
		: mData()
	{}

	MemoryBinaryWriteStream(size_t capacity)
		: mData()
	{
		mData.reserve(capacity);
	}

	std::uint8_t const * GetData() const
	{
		return mData.data();
	}

	size_t GetSize() const
	{
		return mData.size();
	}

	void Write(std::uint8_t const * buffer, size_t size) override
	{
		size_t const oldSize = mData.size();
		mData.resize(mData.size() + size);
		std::memcpy(&(mData[oldSize]), buffer, size);
	}

	MemoryBinaryReadStream MakeReadStreamCopy() const
	{
		std::vector<std::uint8_t> copy = mData;
		return MemoryBinaryReadStream(std::move(copy));
	}

private:

	std::vector<std::uint8_t> mData;
};

/*
 * Implementation of TetWriteStream for in-memory streams.
 */
class MemoryTextWriteStream final : public TextWriteStream
{
public:

	MemoryTextWriteStream()
		: mData()
	{}

	std::string const & GetData() const
	{
		return mData;
	}

	void Write(std::string const & content) override
	{
		mData += content;
	}

	MemoryTextReadStream MakeReadStreamCopy() const
	{
		std::string copy = mData;
		return MemoryTextReadStream(std::move(copy));
	}

private:

	std::string mData;
};