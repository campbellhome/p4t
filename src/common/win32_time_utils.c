// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#include "common.h"
#include "time_utils.h"

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
