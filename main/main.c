#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

void app_main(void)
{
    int i = 0;
    uart_set_baudrate(UART_NUM_0, 115200);
    while (1)
    {
        printf("[%d] Hello world!", i);
        i++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
