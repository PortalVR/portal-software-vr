// Common.h - common definitions / log function declarations
#pragma once

// used globally for error logging
int BeginDataLog ();
void EndDataLog ();
void LogData (const char *szFormat, ...);
#define error_log LogData

// if set, all data files are output as text, else as binary data
#define _TEXT_DATA

// infinity
#define INF 1.0E10

// infinitesimal
#define EPSILON	1E-6

// defines
#define MAX_STRLEN			256

#define TEMP_FOLDER			"temp"
#define LOG_FILE			"error.log"
