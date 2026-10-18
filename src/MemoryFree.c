/**
 * MemoryFree - FreeRTOS/heap_3-aware memory introspection for the minibook.
 *
 * This build of Arduino_FreeRTOS ships only heap_3.c (a thin wrapper around the
 * avr-libc malloc/free), so xPortGetFreeHeapSize() does not exist. All dynamic
 * memory - task stacks and every pvPortMalloc/UI allocation - therefore lives
 * in the single avr-libc heap, and we measure it directly.
 *
 * The classic getFreeMemory() (SP minus heap top) breaks under FreeRTOS: inside
 * a task the "stack pointer" is a malloc'd block located BELOW the heap top, so
 * the result went negative. Here we ignore the current SP entirely and instead
 * report what malloc could still hand out: (heap ceiling - heap top) plus the
 * bytes already sitting on malloc's free list.
 */

#include "MemoryFree.h"

#ifdef __AVR__

#include <avr/io.h>   /* RAMEND */
#include <stdlib.h>

/* avr-libc heap internals */
extern char   __heap_start;
extern char  *__brkval;         /* top of the heap, 0 before first malloc     */
extern char  *__malloc_heap_end;/* heap ceiling, 0 => SP - __malloc_margin    */
extern size_t __malloc_margin;

struct __freelist
{
    size_t sz;
    struct __freelist *nx;
};
extern struct __freelist *__flp; /* head of the free-block list */

/* Sum of the reclaimable bytes currently on the free list. */
static int freeListSize(void)
{
    struct __freelist *cur;
    int total = 0;
    for (cur = __flp; cur != 0; cur = cur->nx)
        total += (int)(sizeof(size_t) + cur->sz);
    return total;
}

static int computeFree(void)
{
    char *heapTop = (__brkval == 0) ? &__heap_start : __brkval;
    char *heapEnd = __malloc_heap_end;
    if (heapEnd == 0)
        heapEnd = (char *)(RAMEND - __malloc_margin);
    return (int)(heapEnd - heapTop) + freeListSize();
}

#else /* host build (syntax check only) */

static int computeFree(void) { return -1; }

#endif

static int minFree = 0x7FFF;

int getFreeMemory(void)
{
    int f = computeFree();
    if (f < minFree)
        minFree = f;
    return f;
}

int getMinMemory(void)
{
    return minFree;
}

void memoryCheck(void)
{
    (void)getFreeMemory(); /* refresh the min-ever low-water mark */
}

unsigned int getStackFreeBytes(TaskHandle_t task)
{
#if (INCLUDE_uxTaskGetStackHighWaterMark == 1)
    return (unsigned int)uxTaskGetStackHighWaterMark(task) * sizeof(StackType_t);
#else
    (void)task;
    return 0;
#endif
}
