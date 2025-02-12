/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "TestRun.h"

#include <Core/FloatingPoint.h>
#include <Core/Log.h>

#include <stdexcept>
#include <string>

/*
 * Yet another class for test cases. If only gtest were reusable from within a Windows application.
 */

#define TEST_VERIFY(expr)                                               \
    if (!(expr))                                                        \
        this->OnVerifyFail(std::string("Failure at " __FILE__ ", line ") + std::to_string(__LINE__) + ": " + std::string(#expr));

#define TEST_VERIFY_EQ(a, b)                                                    \
    if (!((a) == (b)))                                                          \
        this->OnVerifyFail(std::string("Failure at " __FILE__ ", line ") + std::to_string(__LINE__) + ": value \"" + std::to_string((a)) + "\" does not match \"" + std::to_string((b)) + "\"");

#define TEST_VERIFY_FLOAT_EQ(a, b)                                              \
    {                                                                           \
        const FloatingPoint _a(a), _b(b);                                       \
        if (!_a.AlmostEquals(_b))                                               \
            this->OnVerifyFail(std::string("Failure at " __FILE__ ", line ") + std::to_string(__LINE__) + ": value \"" + std::to_string((a)) + "\" does not match \"" + std::to_string((b)) + "\"");\
    }

struct TestCaseFailException : std::exception
{};

class TestCase
{
public:

    virtual void Run() final
    {
        mIsPass = true;

        LogMessage(TestLogSeparator);

        LogMessage("TEST_START: ", mTestName);

        LogMessage(TestLogSeparator);

        try
        {
            this->InternalRun();
        }
        catch (TestCaseFailException const &)
        {

        }
        catch (std::exception const & ex)
        {
            OnFail(std::string("Exception thrown: ") + ex.what());
        }

        LogMessage(TestLogSeparator);

        if (mIsPass)
            LogMessage("TEST_END: PASS: ", mTestName);
        else
            LogMessage("TEST_END: FAIL: ", mTestName);

        LogMessage(TestLogSeparator);
    }

protected:

    TestCase(std::string const & testName)
        : mTestName(testName)
    {}

    virtual void InternalRun() = 0;

protected:

    void OnVerifyFail(std::string const & message)
    {
        OnFail(message);

        throw TestCaseFailException();
    }

    void OnFail(std::string const & message)
    {
        LogMessage("TEST_FAIL: " + message);

        mIsPass = false;

        TestRun::GetInstance().OnFail();
    }

    template <typename T>
    void LogBuffer(std::string const & name, T const * buffer, size_t size)
    {
        if (size <= 5)
        {
            for (size_t i = 0; i < size; ++i)
                LogMessage(name, "[", i, "]:", buffer[i]);
        }
        else
        {
            for (size_t i = 0; i < 3; ++i)
                LogMessage(name, "[", i, "]:", buffer[i]);
            LogMessage(name, "[...]");
            for (size_t i = size-3; i < size; ++i)
                LogMessage(name, "[", i, "]:", buffer[i]);
        }
    }

private:

    std::string const mTestName;

    bool mIsPass;
};
