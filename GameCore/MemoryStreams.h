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

class memory_streambuf : public std::streambuf
{
public:

    memory_streambuf()
        : mStreamBuffer()
        , mReadPos(0u)
    {
    }

    memory_streambuf(std::string const & initString)
        : memory_streambuf()
    {
        mStreamBuffer.resize(initString.size());
        initString.copy(mStreamBuffer.data(), initString.size());

        this->setg(mStreamBuffer.data(), mStreamBuffer.data(), mStreamBuffer.data() + mStreamBuffer.size());
        this->setp(nullptr, nullptr);
    }

    memory_streambuf(char * initData, size_t initDataSize)
        : memory_streambuf()
    {
        mStreamBuffer.resize(initDataSize);
        std::memcpy(mStreamBuffer.data(), initData, initDataSize);

        this->setg(mStreamBuffer.data(), mStreamBuffer.data(), mStreamBuffer.data() + mStreamBuffer.size());
        this->setp(nullptr, nullptr);
    }

    char const * data() const
    {
        return mStreamBuffer.data();
    }

    std::size_t size() const
    {
        return mStreamBuffer.size();
    }

private:

    virtual std::streambuf::int_type overflow(std::streambuf::int_type ch) override
    {
        assert(ch != EOF);

        auto consumed = this->gptr() - this->eback();

        mStreamBuffer.push_back(static_cast<char>(ch));

        this->setg(mStreamBuffer.data(), mStreamBuffer.data() + consumed, mStreamBuffer.data() + mStreamBuffer.size());

        return ch;    
    }

    virtual std::streambuf::int_type underflow() override
    {
        return EOF;
    }

private:

    std::vector<char> mStreamBuffer;
    size_t mReadPos;
};
