// Copyright (c) 2012-2018 Matt Campbell
// MIT license (see License.txt)

#if defined(__cplusplus)
extern "C" {
#endif

double Time_GetCurrentTime(void);
void Time_StartNewFrame(void);
float Time_GetDT(void);
u64 Time_GetFrameNumber(void);

#if defined(__cplusplus)
}
#endif
