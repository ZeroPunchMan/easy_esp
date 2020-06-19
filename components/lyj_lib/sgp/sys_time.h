#pragma once
#include "cl_common.h"

#define SYS_TIME_SECOND(s)  ((s) * 1000UL)
extern uint64_t g_sysTotalTime; //系统全局时间

inline static uint64_t GetSysTime()
{
    return g_sysTotalTime;
}

//更新系统时间
inline static void UpdateSysTime(uint16_t interval)
{
    g_sysTotalTime += interval;
}


//计算preTime到现在经过的时间 ms
inline static uint64_t TimeElapsed(uint64_t preTime)
{
    return g_sysTotalTime - preTime;
}

//*pTime设置为当前时间
inline static void SetToCurTime(uint64_t* pTime)
{
    *pTime = g_sysTotalTime;
}


extern void DelayMs(uint16_t ms);
extern void DelayUs(uint16_t us);

