#include "clib_config.h"
#include "stdarg.h"
#include "stdio.h"

static PrintfFunction printfFunc = CL_NULL;

void CL_SetSendFunc(PrintfFunction f)
{
    printfFunc = f;
}


void CL_Printf(const char *format, ...)
{
    static char buff[PRINTF_BUFF_SIZE];
    va_list ap;
    va_start(ap, format);
    int len = vsnprintf(buff, PRINTF_BUFF_SIZE, format, ap);
    va_end(ap);
    if(printfFunc != CL_NULL)
        printfFunc(buff, len);
    // uart_write_bytes(UART_NUM_0, buff, len);
}
