/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>

/*
 * Abstraction for a read-only binary stream.
 */
class BinaryReadStream
{
public:

	virtual ~BinaryReadStream() = default;

	virtual size_t GetCurrentPosition() const = 0;

	virtual size_t Read(std::uint8_t * buffer, size_t size) = 0;

	virtual size_t Skip(size_t size) = 0;
};

/*
 * Abstraction for a write-only binary stream.
 */
class BinaryWriteStream
{
public:

	virtual ~BinaryWriteStream() = default;

	virtual void Write(std::uint8_t const * buffer, size_t size) = 0;
};