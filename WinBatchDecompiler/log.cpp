#include "log.h"
#include <stdarg.h>
#include <iostream>
#include <time.h>

FILE* pLog = NULL;
void log_to_file(const wchar_t* format, ...)
{
	time_t now = time(0);
	if (pLog == NULL)
	{
		fopen_s(&pLog, DEBUG_FILENAME, "w+");
	}

	if (pLog == NULL)
		return;
	va_list args;
	va_start(args, format);
	time_t rawTime = time(NULL);
	struct tm sTm;
	localtime_s(&sTm, &rawTime);

	fwprintf(pLog, L"%04i-%02i-%02i %02i:%02i:%02i", sTm.tm_year+1900, sTm.tm_mon, sTm.tm_mday, sTm.tm_hour, sTm.tm_min, sTm.tm_sec);
	vfwprintf(pLog, format, args);
	fwprintf(pLog, L"\r\n"); 
	va_end(args);
	fflush(pLog);
}