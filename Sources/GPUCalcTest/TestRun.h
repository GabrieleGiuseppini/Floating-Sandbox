/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2019-01-11
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <Core/Log.h>

#include <string>

static constexpr char TestLogSeparator[] = "--------------------------------------";

class TestRun
{
public:

    static TestRun & GetInstance()
    {
        static TestRun testRun;

        return testRun;
    }

    void Start()
    {
        mIsPass = true;

        LogMessage(TestLogSeparator);

        LogMessage("TEST_RUN_START");

        LogMessage(TestLogSeparator);
    }

    void End()
    {
        LogMessage(TestLogSeparator);

        if (mIsPass)
            LogMessage("TEST_RUN_END: PASS");
        else
            LogMessage("TEST_RUN_END: FAIL");

        LogMessage(TestLogSeparator);
    }

    void OnFail()
    {
        mIsPass = false;
    }

private:

    bool mIsPass;
};

class ScopedTestRun
{
public:

    ScopedTestRun()
    {
        TestRun::GetInstance().Start();
    }

    ~ScopedTestRun()
    {
        TestRun::GetInstance().End();
    }
};
