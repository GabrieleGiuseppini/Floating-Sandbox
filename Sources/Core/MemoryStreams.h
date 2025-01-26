/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2019-10-05
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <streambuf>
#include <vector>

class memory_streambuf final : public std::streambuf
{
public:

    memory_streambuf()
        : mStreamBuffer()
    {
    }

    memory_streambuf(std::string const & initString)
        : memory_streambuf()
    {
        mStreamBuffer.resize(initString.size());
        initString.copy(mStreamBuffer.data(), initString.size());

        rewind();
    }

    memory_streambuf(char * initData, size_t initDataSize)
        : memory_streambuf()
    {
        mStreamBuffer.resize(initDataSize);
        std::memcpy(mStreamBuffer.data(), initData, initDataSize);

        rewind();
    }

    char const * data() const
    {
        return mStreamBuffer.data();
    }

    std::size_t size() const
    {
        return mStreamBuffer.size();
    }

    void rewind()
    {
        this->setg(mStreamBuffer.data(), mStreamBuffer.data(), mStreamBuffer.data() + mStreamBuffer.size());
        this->setp(nullptr, nullptr);
    }

private:

    std::streambuf::int_type overflow(std::streambuf::int_type ch) override
    {
        assert(ch != EOF);

        // Save current offset, as we need to provide
        // pointers again due to std:vector possibly re-allocating
        auto const consumedSoFar = this->gptr() - this->eback();

        // Store new char
        mStreamBuffer.push_back(static_cast<char>(ch));

        // Set new get pointers
        this->setg(mStreamBuffer.data(), mStreamBuffer.data() + consumedSoFar, mStreamBuffer.data() + mStreamBuffer.size());

        return ch;
    }

    std::streambuf::int_type underflow() override
    {
        return EOF;
    }

    std::streampos seekpos(
        std::streampos pos,
        std::ios_base::openmode /*which*/) override
    {
        if (pos < 0 || pos > static_cast<std::streampos>(mStreamBuffer.size()))
            return std::streampos(-1);

        this->setg(mStreamBuffer.data(), mStreamBuffer.data() + static_cast<size_t>(pos), mStreamBuffer.data() + mStreamBuffer.size());
        this->setp(nullptr, nullptr);

        return pos;
    }

    std::streampos seekoff(
        std::streamoff off,
        std::ios_base::seekdir way,
        std::ios_base::openmode /*which*/) override
    {
        if (way == std::ios_base::beg)
        {
            if (this->eback() + off > this->egptr())
                return std::streampos(-1);

            this->setg(mStreamBuffer.data(), this->eback() + off, mStreamBuffer.data() + mStreamBuffer.size());
            this->setp(nullptr, nullptr);
        }
        else if (way == std::ios_base::cur)
        {
            if (this->gptr() + off > this->egptr())
                return std::streampos(-1);

            this->setg(mStreamBuffer.data(), this->gptr() + off, mStreamBuffer.data() + mStreamBuffer.size());
            this->setp(nullptr, nullptr);
        }
        else if (way == std::ios_base::end)
        {
            if (this->egptr() + off < this->eback())
                return std::streampos(-1);

            this->setg(mStreamBuffer.data(), this->egptr() + off, mStreamBuffer.data() + mStreamBuffer.size());
            this->setp(nullptr, nullptr);
        }
        else
        {
            return std::streampos(-1);
        }

        return std::streampos(this->gptr() - this->eback());
    }

private:

    std::vector<char> mStreamBuffer;
};
