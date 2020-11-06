#pragma once
#include <stdio.h>
#include <string.h>
#include <tchar.h>

/*
* Author: sontx, www.sontx.in
* A simple log header for debug, support log output to stderr, file, console and IDE(only win32)
* Log will auto be removed in release mode before source compile for performance.
*/


#define DEBUG_D_TAG				L"DBG"				/* debug tag */
#define DEBUG_E_TAG				L"ERR"				/* error tag */
#define DEBUG_W_TAG				L"WRN"				/* warning tag */
#define DEBUG_I_TAG				L"INF"				/* info tag */

#define DEBUG_FORMAT			L"[%s]: "
#define __FILENAME__			L"WinBatchDecompiler.log"

// write a log to file
void log_to_file(const wchar_t* format, ...);

#define LOG_D(format, ...) {log_to_file(DEBUG_FORMAT format, DEBUG_D_TAG, __VA_ARGS__);}
#define LOG_E(format, ...) {log_to_file(DEBUG_FORMAT format, DEBUG_E_TAG, __VA_ARGS__);}
#define LOG_W(format, ...) {log_to_file(DEBUG_FORMAT format, DEBUG_W_TAG, __VA_ARGS__);}
#define LOG_I(format, ...) {log_to_file(DEBUG_FORMAT format, DEBUG_I_TAG, __VA_ARGS__);}
#define LOG(format, ...) LOG_D(format, __VA_ARGS__)
#define DEBUG_FILENAME			"WinBatchDecompiler.log"

