/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2025-02-02
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <vector>

/*
 * Abstraction for a read-only binary stream.
 */
class BinaryReadStream
{
public:

	virtual ~BinaryReadStream() = default;

	virtual size_t GetCurrentPosition() = 0;

	virtual size_t Read(std::uint8_t * buffer, size_t size) = 0;

	virtual size_t Skip(size_t size) = 0;
};

/*
 * Abstraction for a read-only text stream.
 */
class TextReadStream
{
public:

	virtual ~TextReadStream() = default;

	virtual std::string ReadAll() = 0;

	virtual std::vector<std::string> ReadAllLines() = 0;
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

/*
 * Abstraction for a write-only text stream.
 */
class TextWriteStream
{
public:

	virtual ~TextWriteStream() = default;

	virtual void Write(std::string const & content) = 0;
};