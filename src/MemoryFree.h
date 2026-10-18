/**
 * MemoryFree - FreeRTOS/heap_3-aware memory introspection for the minibook.
 *
 * This build of Arduino_FreeRTOS uses heap_3 (avr-libc malloc/free), so there
 * is no xPortGetFreeHeapSize(). Every allocation - task stacks and all UI
 * pvPortMalloc's - lives in the single avr-libc heap, which we measure directly.
 * The classic SP-minus-heap-top formula is not used: inside a task the stack is
 * a malloc'd block below the heap top, which made the old number go negative.
 */

#ifndef	MEMORY_FREE_H
#define MEMORY_FREE_H

#include "Arduino_FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Approximate free bytes in the avr-libc heap that malloc/pvPortMalloc could
 * still hand out (heap ceiling minus heap top, plus the free-list). This is the
 * real "free RAM" figure and is comparable over time for leak detection.
 */
int getFreeMemory(void);

/**
 * Lowest getFreeMemory() value seen so far (tracked in this module). If it keeps
 * dropping as the app returns to the same idle state, you have a leak. Call
 * getFreeMemory() or memoryCheck() periodically to keep it updated.
 */
int getMinMemory(void);

/**
 * Samples getFreeMemory() to refresh the min-ever low-water mark. Call it from
 * a periodic task if nothing else calls getFreeMemory() regularly.
 */
void memoryCheck(void);

/**
 * Minimum free stack space (in BYTES) ever seen for the given task; pass NULL
 * for the calling task. Use this to right-size the depths passed to xTaskCreate.
 * Returns 0 if INCLUDE_uxTaskGetStackHighWaterMark is not enabled.
 */
unsigned int getStackFreeBytes(TaskHandle_t task);

#ifdef  __cplusplus
}
#endif

#endif
