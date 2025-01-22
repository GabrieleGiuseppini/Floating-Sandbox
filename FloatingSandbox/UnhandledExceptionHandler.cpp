/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-03-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UnhandledExceptionHandler.h"

#include <UILib/StandardSystemPaths.h>

#include <Game/Version.h>

#include <GameCore/Log.h>
#include <GameCore/Utils.h>

#include <filesystem>

#ifdef WIN32

#include <windows.h>
#include <Dbghelp.h>

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

void create_minidump(
    struct _EXCEPTION_POINTERS * apExceptionInfo,
    std::filesystem::path const & diagnosticsFolderPath,
    std::string const & dateTimeString)
{
    HMODULE hDbgHelp = ::LoadLibraryA("dbghelp.dll");
    if (NULL != hDbgHelp)
    {
        MINIDUMPWRITEDUMP pWriteDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
        if (NULL != pWriteDump)
        {
            //
            // Make crash filename
            //

            std::filesystem::path const dumpPath = diagnosticsFolderPath / std::string(APPLICATION_NAME_WITH_LONG_VERSION "_" + dateTimeString + "_core.dmp");
            std::string const dumpPathStr = dumpPath.string();

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

LONG WINAPI unhandled_exception_handler(struct _EXCEPTION_POINTERS* apExceptionInfo)
{
    try
    {
        std::filesystem::path const diagnosticsFolderPath = StandardSystemPaths::GetInstance().GetDiagnosticsFolderPath(true);
        std::string const dateTimeString = Utils::MakeNowDateAndTimeString();

        // Create minidump
        create_minidump(apExceptionInfo, diagnosticsFolderPath, dateTimeString);

        // Flush log
        Logger::Instance.FlushToFile(diagnosticsFolderPath, dateTimeString);
    }
    catch (...)
    { /* ignore */ }

    return EXCEPTION_CONTINUE_SEARCH;
}

void InstallUnhandledExceptionHandler()
{
    SetUnhandledExceptionFilter(unhandled_exception_handler);
}

#else

void InstallUnhandledExceptionHandler()
{
    // Nop at the moment
}

#endif