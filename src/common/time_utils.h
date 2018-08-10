// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#if defined(__cplusplus)
extern "C" {
#endif

double Time_GetCurrentTime(void);
void Time_StartNewFrame(void);
float Time_GetDT(void);
float Time_GetCurrentFrameElapsed(void);
u64 Time_GetFrameNumber(void);
const char *Time_StringFromEpochTime(u32 epochTime);

#if defined(__cplusplus)
}
#endif
