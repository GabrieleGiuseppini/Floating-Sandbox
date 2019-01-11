/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "TestRun.h"

#include <GameCore/Log.h>

#include <stdexcept>
#include <string>

/*
 * Yet another class for test cases. If only gtest were reusable from within a Windows application.
 */
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
        catch (std::exception const & ex)
        {
            LogMessage("Exception thrown: ", ex.what());

            OnFail();
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

private:

    void OnFail()
    {
        mIsPass = false;

        TestRun::GetInstance().OnFail();
    }

    std::string const mTestName;

    bool mIsPass;
};
