/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "BinaryStreams.h"

#include <cstring>
#include <vector>

/*
 * Implementation of BinaryReadStream for in-memory streams.
 */
class MemoryBinaryReadStream final : public BinaryReadStream
{
public:

	MemoryBinaryReadStream(std::vector<std::uint8_t> && data)
		: mData(data)
		, mReadOffset(0u)
	{}

	size_t Read(std::uint8_t * buffer, size_t size) override
	{
		size_t const szToRead = std::min(size, mData.size() - mReadOffset);
		std::memcpy(buffer, &(mData[mReadOffset]), szToRead);
		mReadOffset += szToRead;
		return szToRead;
	}

private:

	std::vector<std::uint8_t> const mData;
	size_t mReadOffset;
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

private:

	std::vector<std::uint8_t> mData;
};