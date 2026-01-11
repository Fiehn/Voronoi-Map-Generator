#include <windows.h>
#include <dbghelp.h>
#include <iostream>
#include "Logger.h"

#pragma comment(lib, "dbghelp.lib")

LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
{
	// 1. Log the fatal error immidiately to the ring buffer
	LOG_ERROR(General, "CRASH DETECTED! Code: %x", pExceptionInfo->ExceptionRecord->ExceptionCode);

	// 2. Flush the logger to ensure all logs are written
	Logger::Get().Flush();

	// 3. Create a minidump file
	HANDLE hFile = CreateFileA("crash_dump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ExceptionPointers = pExceptionInfo;
		dumpInfo.ClientPointers = TRUE;

		// MiniDumpWithIndirectlyReferencedMemory flag to capture more memory
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpWithIndirectlyReferencedMemory, &dumpInfo, NULL, NULL);
		CloseHandle(hFile);
	}

	// 4. Show a message box to the user
	MessageBoxA(NULL, "The application died. Use the opportunity to take a break.", "Fatal Error", MB_OK | MB_ICONERROR);
	
	return EXCEPTION_EXECUTE_HANDLER;
}

void InitCrashHandler()
{
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
}