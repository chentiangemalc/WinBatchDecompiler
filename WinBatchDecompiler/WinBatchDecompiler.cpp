// LaunchAndInject.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <Psapi.h>
#include <pathcch.h>
#include "log.h"
#include <wchar.h>
#pragma comment(lib,"Pathcch.lib")

typedef void (WINAPI* PHookInit)();

HMODULE WINAPI GetRemoteModuleHandle(HANDLE hProcess, LPCWSTR lpModuleName)
{
	HMODULE* ModuleArray = NULL;
	DWORD ModuleArraySize = 100;
	DWORD NumModules = 0;
	WCHAR lpModuleNameCopy[MAX_PATH] = { 0 };
	WCHAR ModuleNameBuffer[MAX_PATH] = { 0 };

	if (lpModuleName == NULL) return NULL;
	ModuleArray = new HMODULE[ModuleArraySize];
	if (ModuleArray == NULL) return NULL;

	if (!EnumProcessModulesEx(hProcess, ModuleArray,
		ModuleArraySize * sizeof(HMODULE), &NumModules, LIST_MODULES_ALL))
	{
		DWORD dwResult = GetLastError();
		LOG_E("Unable to get modules in process Error %i", dwResult);
	}
	else
	{
		NumModules /= sizeof(HMODULE);
		if (NumModules > ModuleArraySize)
		{
			delete[] ModuleArray;
			ModuleArray = NULL;
			ModuleArray = new HMODULE[NumModules]; 
			if (ModuleArray != NULL)
			{
				ModuleArraySize = NumModules; 
				if (EnumProcessModulesEx(
						hProcess, 
					ModuleArray,
					ModuleArraySize * sizeof(HMODULE), 
					&NumModules, 
					LIST_MODULES_ALL))
				{
					NumModules /= sizeof(HMODULE);
				}
			}
		}
	}
	
	for (DWORD i = 0; i <= NumModules; ++i)
	{
		GetModuleBaseNameW(hProcess, ModuleArray[i],
			ModuleNameBuffer, MAX_PATH);
		LOG_I("Module = '%s'", ModuleNameBuffer);
		if (_wcsicmp(ModuleNameBuffer, lpModuleName) == 0)
		{
			LOG_I("Target module found!");
			HMODULE TempReturn = ModuleArray[i];
			delete[] ModuleArray;
			return TempReturn;
		}
	}

	if (ModuleArray != NULL)
		delete[] ModuleArray;

	return NULL;
}

int wmain(int argc, wchar_t* argv[], wchar_t* envp[])
{
	LOG_I(L"WinBatch Decompiler Started");

	wchar_t CurrentProcessDirectory[MAX_PATH];
	wchar_t TargetDllFilename[MAX_PATH];

#ifdef _WIN64
	wchar_t TargetDllName[] = L"DecompleEngine64.dll";
#else
	wchar_t TargetDllName[] = L"DecompileEngine32.dll";
#endif

	char TargetFunctionName[] = "HookInit";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	DWORD dwTimeOut = 60000;

	if (argc < 2)
	{
		LOG_E(L"No command line parameters specified.");
		return 1;
	}

	wchar_t* cmd_pos = wcsstr(GetCommandLine(), argv[1]) - 1;

	if (cmd_pos)
	{
		if (cmd_pos[0] != L'"')
		{
			cmd_pos += 1;
		}
	}

	LOG_I(L"Command Line='%s'", cmd_pos);
	DWORD dwResult = GetModuleFileNameW(NULL, CurrentProcessDirectory, MAX_PATH);
	PathCchRemoveFileSpec(CurrentProcessDirectory, MAX_PATH);
	PathCchCombine(TargetDllFilename, MAX_PATH, CurrentProcessDirectory, TargetDllName);

	LOG_I(L"Current Directory='%s' Result='%i'", CurrentProcessDirectory, dwResult);
	LOG_I(L"Target DLL='%s'", TargetDllFilename);

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL,	// No module name (use command line)
		cmd_pos,				// Command line
		NULL,					// Process handle not inheritable
		NULL,					// Thread handle not inheritable
		FALSE,					// Set handle inheritance to FALSE
		CREATE_SUSPENDED | CREATE_NO_WINDOW,		// No creation flags
		NULL,					// Use parent's environment block
		NULL,					// Use parent's starting directory 
		&si,
		&pi)
		)
	{
		dwResult = GetLastError();
		LOG_E(L"CreateProcess Failed with Error #%i", dwResult);
		return 1;
	}

	LOG_I(L"Suspended Process created with PID '%i'", pi.dwProcessId);
	LOG_I("Loading Target DLL");

	// load DLL in this process first so we can calculate function offset
	HMODULE hModuleTargetDll = LoadLibraryW(TargetDllFilename);
	__int64 iTargetProcAddress = 0;
	__int64 iTargetOffset = 0;
	if (hModuleTargetDll != NULL)
	{
		iTargetProcAddress = (__int64)GetProcAddress(hModuleTargetDll, TargetFunctionName);
		iTargetOffset = iTargetProcAddress - (__int64)hModuleTargetDll;
		LOG_I("Function Target Offset = %i", iTargetOffset);
	}
	HMODULE hModuleKernel32 = GetModuleHandle(L"kernel32.dll");
	LPVOID pLoadLibraryAddress = NULL;
	
	if (hModuleKernel32 != NULL)
	{
		pLoadLibraryAddress = (LPVOID)GetProcAddress(hModuleKernel32, "LoadLibraryW");
	}
	else
	{
		LOG_E("Unable to get module handle for kernel32.dll");
	}

	if (pLoadLibraryAddress == NULL) {
		dwResult = GetLastError();
		LOG_E(L"ERROR: Unable to find LoadLibraryW in Kernel32.dll Error: %i", dwResult);
	}

	// allocate space for LoadLibrary arguments in target process
	size_t iTargetDllSize = (wcslen(TargetDllFilename) + 1) * sizeof(wchar_t);
	LPVOID pLoadLibraryArguments = (LPVOID)VirtualAllocEx(
		pi.hProcess,
		NULL,
		iTargetDllSize,
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


	if (pLoadLibraryArguments == NULL) {
		dwResult = GetLastError();
		LOG_E(L"ERROR: Unable to allocate %i bytes in target process Error: %i",
			iTargetDllSize,
			dwResult);
	}
	else
	{
		if (!WriteProcessMemory(
			pi.hProcess,
			pLoadLibraryArguments,
			TargetDllFilename,
			iTargetDllSize,
			NULL))
		{
			dwResult = GetLastError();
			LOG_E("Unable to write bytes into target process address space. Error %i", dwResult);
		}
		else
		{
			LOG_I("LoadLibrary Arguments Successfully written to target process address space.");
			HANDLE hThread = NULL;
			
			if (pLoadLibraryAddress != NULL)
			{
				hThread = CreateRemoteThread(
					pi.hProcess,
					NULL,
					0,
					(LPTHREAD_START_ROUTINE)pLoadLibraryAddress,
					pLoadLibraryArguments,
					NULL,
					NULL);
			}

			if (hThread == NULL) {
				dwResult = GetLastError();
				LOG_E("The remote thread calling LoadLibrary could not be created. Error %i", dwResult);
			}
			else {
				LOG_I("Remote Thread for LoadLibrary successfully created.");
				dwResult = WaitForSingleObject(hThread, dwTimeOut);
				if (dwResult == WAIT_FAILED)
				{
					dwResult = GetLastError();
					LOG_I("Remote Thread for LoadLibrary Failed Error %i", dwResult);
				}

				if (dwResult == WAIT_TIMEOUT)
				{
					LOG_E("Remote Thread for LoadLibrary in hung state");
				}
				
				HMODULE hInjected = GetRemoteModuleHandle(pi.hProcess, TargetDllName);
				PHookInit pHookInit = NULL;
				if (hInjected == NULL)
				{
					LOG_E("Unable to get module handle in target process");
				}
				else
				{
					pHookInit = (PHookInit)((__int64)hInjected + iTargetOffset);
				}

				if (pHookInit != NULL)
				{
					LOG_I("Running HookInit function!");
					hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pHookInit, NULL, NULL, NULL);
					if (hThread == NULL)
					{
						dwResult = GetLastError();
						LOG_E("The remote thread calling HookInit could not be created. Error %i", dwResult);
					}
					else
					{
						LOG_I("HookInit function started!");
						dwResult = WaitForSingleObject(hThread, dwTimeOut);
						if (dwResult == WAIT_FAILED)
						{
							dwResult = GetLastError();
							LOG_I("Remote Thread for HookInit Failed Error %i", dwResult);
						}

						if (dwResult == WAIT_TIMEOUT)
						{
							LOG_E("Remote Thread for HookInit in hung state");
						}
					}
				}
			}
		}
	}
	
	LOG_I("Resuming threads in target process");
	ResumeThread(pi.hThread);
	LOG_I("Process Resumed. Waiting for process to exit");

	dwResult = WaitForSingleObject(pi.hProcess, INFINITE);

	DWORD exitCode = 0;
	if (GetExitCodeProcess(pi.hProcess, &exitCode))
	{
		LOG_I("Process Terminated with exit code %i", exitCode);
	}
	else
	{
		LOG_W("Process terminated, unable to determine Exit Code");
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
