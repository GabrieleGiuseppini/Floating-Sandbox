/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UnhandledExceptionHandler.h"

#include "StandardSystemPaths.h"

#include <GameCore/Version.h>

#include <filesystem>

#ifdef WIN32

#include <windows.h>
#include <Dbghelp.h>

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

void create_minidump(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    HMODULE hDbgHelp = ::LoadLibraryA("dbghelp.dll");
    if (NULL != hDbgHelp)
    {
        MINIDUMPWRITEDUMP pWriteDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
        if (NULL != pWriteDump)
        {
            //
            // Make filename
            //

            std::filesystem::path folderPath = StandardSystemPaths::GetInstance().GetUserSettingsGameFolderPath()
                / "CrashDumps";

            if (!std::filesystem::exists(folderPath))
            {
                try
                {
                    std::filesystem::create_directories(folderPath);
                }
                catch (...)
                { /* ignore*/ }
            }

            std::filesystem::path dumpPath = folderPath / APPLICATION_NAME_WITH_VERSION "_core.dmp";
            std::string dumpPathStr = dumpPath.string();

            //
            // Write dump
            //

            HANDLE hDumpFile = ::CreateFileA(
                dumpPathStr.c_str(),
                GENERIC_WRITE,
                FILE_SHARE_WRITE,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (INVALID_HANDLE_VALUE != hDumpFile)
            {
                _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
                ExInfo.ThreadId = ::GetCurrentThreadId();
                ExInfo.ExceptionPointers = apExceptionInfo;
                ExInfo.ClientPointers = FALSE;

                pWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &ExInfo, NULL, NULL);

                ::CloseHandle(hDumpFile);

                std::string message = "An unhandled exception occurred, we apologize for the inconvenience. " \
                    "A crash dump file has been created at \"" + dumpPathStr + "\"; Floating Sandbox will " \
                    "now exit.";

                MessageBoxA(NULL, message.c_str(), "Maritime Super-Disaster", MB_OK | MB_ICONERROR);
            }
        }

        ::FreeLibrary(hDbgHelp);
    }
}

LONG WINAPI unhandled_handler(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    create_minidump(apExceptionInfo);
    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallUnhandledExceptionHandler()
{
    SetUnhandledExceptionFilter(unhandled_handler);
}

#else

void InstallUnhandledExceptionHandler()
{
    // Nop at the moment
}

#endif