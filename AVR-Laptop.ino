#include "Arduino_FreeRTOS.h"
#include "src/Debug.h"
#include "src/Shell.h"
#include "src/MemoryFree.h"


void MemoryMonitorTask(void *pvParameters)
{
    const uint16_t stackSize = 140;
    while (true)
    {
        // Heap free = сколько свободно в куче FreeRTOS сейчас.
        // min ever = минимум за всё время
        DEBUG_PRINT_F("Heap free = ");
        DEBUG_PRINTLN(getFreeMemory());
        DEBUG_PRINT_F(" | min ever = ");
        DEBUG_PRINTLN(getMinMemory());

        nsShell::printStackUsage(shellTask);

        unsigned int freeBytes = getStackFreeBytes(NULL);
        DEBUG_PRINT_F("MemMon: used max=");
        DEBUG_PRINT((freeBytes <= stackSize) ? stackSize - freeBytes : 0);
        DEBUG_PRINT_F(" B, free min=");
        DEBUG_PRINT(freeBytes);
        DEBUG_PRINT_F(" B, allocated=");
        DEBUG_PRINTLN(stackSize);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup()
{
    // Оболочка
    xTaskCreate(nsShell::start, "Shell", SHELL_STACK_SIZE, NULL, 4, &activeTask);
    // Монитор занятой памяти
    //xTaskCreate(MemoryMonitorTask, "MemMon", 140, NULL, 4, NULL);
}

void loop()
{
}
