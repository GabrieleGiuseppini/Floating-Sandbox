/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <chrono>
#include <ctime>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace /* anonymous */ {

	template<typename T>
	std::stringstream & _LogToStream(std::stringstream & ss, T&& t)
	{
		ss << std::forward<T>(t) << std::endl;
		return ss;
	}

	template<typename T, typename... TArgs>
	std::stringstream & _LogToStream(std::stringstream & ss, T&& t, TArgs&&... args)
	{
		ss << std::forward<T>(t);
		return _LogToStream(ss, std::forward<TArgs>(args)...);
	}

    template<typename T>
    void _LogToNothing(T&& /*t*/)
    {
    }

    template<typename T, typename... TArgs>
    void _LogToNothing(T&& /*t*/, TArgs&&... args)
    {
        _LogToNothing(std::forward<TArgs>(args)...);
    }

}

class Logger
{
public:

	Logger()
		: mCurrentListener()
		, mStoredMessages()
        , mMutex()
	{
	}

	Logger(Logger const &) = delete;
	Logger(Logger &&) = delete;
	Logger & operator=(Logger const &) = delete;
	Logger & operator=(Logger &&) = delete;

	void RegisterListener(std::function<void(std::string const & message)> listener)
	{
        // At this moment we don't expect to (and thus don't support) having more than one listener at
        // any given moment in time, so we're catching ill-conceived attempts here
		assert(!mCurrentListener);

        std::scoped_lock lock(mMutex);

        // Register listener
        mCurrentListener = std::move(listener);

        // Publish all the messages so far
        for (std::string const & message : mStoredMessages)
        {
            mCurrentListener(message);
        }
	}

	void UnregisterListener()
	{
        if (!!mCurrentListener)
        {
            std::scoped_lock lock(mMutex);

            mCurrentListener = {};
        }
	}

	template<typename...TArgs>
	void Log(TArgs&&... args)
	{
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        auto usecs = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch() % std::chrono::seconds(1)).count();

		std::stringstream ss;

        ss
            << std::put_time(std::localtime(&now_c), "%T") << "."
            << std::setfill('0') << std::setw(6) << usecs << ":";

		_LogToStream(ss, std::forward<TArgs>(args)...);

		std::string const & message = ss.str();

		// Store and publish
        {
            std::scoped_lock lock(mMutex);

            mStoredMessages.push_back(message);
            if (mStoredMessages.size() > MaxStoredMessages)
            {
                mStoredMessages.pop_front();
            }

            // Publish
            if (!!mCurrentListener)
            {
                mCurrentListener(message);
            }
        }

        // Output to stdout
        std::cout << message << std::endl;

#ifdef _DEBUG
        LogToDebugStream(message);
#endif
	}

    template<typename...TArgs>
    void LogToNothing(TArgs&&... args)
    {
        _LogToNothing(std::forward<TArgs>(args)...);
    }

    std::string GetAll()
    {
        std::stringstream ss;

        {
            std::scoped_lock lock(mMutex);

            for (auto const & s : mStoredMessages)
            {
                ss << s;
            }
        }

        return ss.str();
    }

private:

    void LogToDebugStream(std::string const & message);

public:

	static Logger Instance;

private:

	// The current listener
	std::function<void(std::string const & message)> mCurrentListener;

	// The messages stored so far
	std::deque<std::string> mStoredMessages;
	static constexpr size_t MaxStoredMessages = 1000;

    // The mutex for multi-threaded access
    std::mutex mMutex;
};

//
// Global aliases
//

template<typename... TArgs>
void LogMessage(TArgs&&... args)
{
	Logger::Instance.Log(std::forward<TArgs>(args)...);
}

#ifdef _DEBUG
#define LogDebug(...) Logger::Instance.Log(__VA_ARGS__);
#else
#define LogDebug(...)
#endif
