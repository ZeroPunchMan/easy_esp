#pragma once

#include "cl_common.h"

#ifdef __cplusplus
extern "C" {
#endif

//-------------event type------------------
typedef enum
{
    CL_Event_SgpRecvMsg = 0,
    CL_EventMax,
} CL_Event_t;

//---------------log-------------------------
#include "stdio.h"
#define USE_LDB_LOG
#define CL_PRINTF   CL_Printf
#define PRINTF_BUFF_SIZE    1024

typedef void (*PrintfFunction)(const char* str, int len);

void CL_SetSendFunc(PrintfFunction);
void CL_Printf(const char *format, ...);

#ifdef __cplusplus
}
#endif
