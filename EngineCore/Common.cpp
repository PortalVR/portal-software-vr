// Common.cpp - log function definitions
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "Common.h"

FILE	*g_pLogFile = NULL;

int BeginDataLog ()
{
	errno_t err;
	time_t long_time;
	struct tm newtime;
	char timebuf[26];

	if (NULL == g_pLogFile)
	{
		// open log file in append mode (to keep previous run logs)
		fopen_s (&g_pLogFile, LOG_FILE, "ab");

		if (NULL != g_pLogFile)
		{
			// timestamp the beginning of the current run
			time (&long_time); /* get current cal time */
			err = localtime_s (&newtime, &long_time); 
			asctime_s (timebuf, &newtime);
			fprintf (g_pLogFile, "%s\r\n", timebuf);
		}
	}

	return (NULL == g_pLogFile);
}

void EndDataLog ()
{
	if (NULL != g_pLogFile)
	{
		fclose (g_pLogFile);
		g_pLogFile = NULL;
	}
}

void LogData (const char *szFormat, ...)
{
	if (NULL == g_pLogFile)
	{
		int fResult = BeginDataLog ();
		if (0 != fResult)
			return;
	}
	va_list args;
	va_start(args, szFormat);
	vfprintf(g_pLogFile, szFormat, args);
	va_end(args);
	fflush (g_pLogFile);
}
