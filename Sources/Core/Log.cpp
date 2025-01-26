/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-01-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Log.h"

Logger Logger::Instance;

#if FS_IS_OS_WINDOWS()
#include "windows.h"
#endif

void Logger::LogToDebugStream(std::string const & message)
{
#if FS_IS_OS_WINDOWS()
    OutputDebugStringA(message.c_str());
#endif
}