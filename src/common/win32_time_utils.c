// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "common.h"
#include "time_utils.h"
#include "va.h"

static u64 s_frameStartCounter;
static u64 s_counterFrequency;
static double s_counterToMilliseconds;

static double s_currentTime;
static float s_frameTime;
static u64 s_frameNumber;

double Time_GetCurrentTime(void)
{
	return s_currentTime;
}

void Time_StartNewFrame(void)
{
	u64 oldFrameStartCounter = s_frameStartCounter;
	u64 deltaCounter;

	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	s_counterFrequency = li.QuadPart;
	s_counterToMilliseconds = 1.0 / s_counterFrequency;

	QueryPerformanceCounter(&li);
	s_frameStartCounter = li.QuadPart;

	deltaCounter = s_frameStartCounter - oldFrameStartCounter;
	s_frameTime = (float)(deltaCounter * s_counterToMilliseconds);
	s_currentTime = s_frameStartCounter * s_counterToMilliseconds;
	++s_frameNumber;
}

float Time_GetDT(void)
{
	return s_frameTime;
}

u64 Time_GetFrameNumber(void)
{
	return s_frameNumber;
}

static SYSTEMTIME Ttime_SystemTimeFromEpochTime(u32 epochTime)
{
	// Windows uses 100 nanosecond intervals since Jan 1, 1601 UTC
	// https://support.microsoft.com/en-us/help/167296/how-to-convert-a-unix-time-t-to-a-win32-filetime-or-systemtime
	SYSTEMTIME st = { 0 };
	FILETIME ft;
	u64 ll = (((u64)epochTime) * ((u64)10000000)) + (u64)116444736000000000;
	ft.dwLowDateTime = (DWORD)ll;
	ft.dwHighDateTime = ll >> 32;
	FileTimeToLocalFileTime(&ft, &ft);
	FileTimeToSystemTime(&ft, &st);
	return st;
}

const char *Time_StringFromEpochTime(u32 epochTime)
{
	SYSTEMTIME st = Ttime_SystemTimeFromEpochTime(epochTime);
	char dateBuffer[64] = "";
	char timeBuffer[64] = "";
	GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateBuffer, sizeof(dateBuffer));
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, timeBuffer, sizeof(timeBuffer));
	return va("%s %s", dateBuffer, timeBuffer);
}
