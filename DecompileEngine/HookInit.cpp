// HookInit.cpp : Defines the exported functions for the DLL.
//

#include "framework.h"
#include "HookInit.h"
#include "MinHook.h"
#include <iostream>
#include <Windows.h>

DWORD gFreqOffset = 0;

bool TRACE(const wchar_t* format, ...)
{
	wchar_t buffer[1000];

	va_list argptr;
	va_start(argptr, format);
	wvsprintf(buffer, format, argptr);
	va_end(argptr);

	OutputDebugString(buffer);

	return true;
}

typedef int (WINAPI* LSTRLENA)(LPCSTR lpString);

// reference to original function
LSTRLENA original_lstrlenA = NULL;
int WINAPI hooked_lstrlenA(LPCSTR lpString)
{
	DWORD dwNumberOfCharactersWritten;
	if (strstr(lpString, ";Encoded   WinBatch file") != NULL)
	{
		while (*lpString!= '\0') {
			if (*lpString == 13)
			{
				WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),"\r\n", 2, &dwNumberOfCharactersWritten, NULL); 
			}
			else
			{
				if (*lpString != 10)
				{
					WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), lpString, 1, &dwNumberOfCharactersWritten, NULL); 
				}
			}
			lpString++;
		}
	
		TerminateProcess(GetCurrentProcess(), 0);
	}
	return original_lstrlenA(lpString);
}

// This is an example of an exported function.
HookInit_API void HookInit(void)
{
	AttachConsole(ATTACH_PARENT_PROCESS);

	TRACE(L"HOOK: HOOKINIT");
	int i = 0;

	TRACE(L"HOOK: INITIALIZE");
	// Initialize MinHook.
	if (MH_Initialize() != MH_OK)
	{
		TRACE(L"HOOK INITIALIZATION FAILED!");
		return;
	}
	
	TRACE(L"HOOK: CREATEHOOKAPIEX");
	// Create a hook for lstrlenA, in disabled state.
	if (MH_CreateHook(&lstrlenA, &hooked_lstrlenA,
		reinterpret_cast<LPVOID*>(&original_lstrlenA)) != MH_OK)
	{
		TRACE(L"CREATE HOOK FAILED!");
		return;
	}

	TRACE(L"HOOK: ENABLEHOOK");

	// Enable the hook for lstrlenA.
	if (MH_EnableHook(&lstrlenA) != MH_OK)
	{
		TRACE(L"ENABLE HOOK FAILED!");
		return;
	}

	TRACE(L"HOOK: RETURNING");
	std::cout << "Hooking complete!!" << std::endl;
}
