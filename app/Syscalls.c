//  Copyright 2022 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <stdint.h>
#include <string.h>
#include <reent.h>

#include <FreeRTOS.h>
#include <task.h>

/**
 * This function is called by __libc_fini_array which gets called when exit()
 * is called. In order to support exit(), an empty _fini() stub function is
 * required.
 */
void _fini(void) {}

/**
 * Implementation of _sbrk to support FreeRTOS with configUSE_NEWLIB_REENTRANT.
 */
void * __attribute__((used)) _sbrk_r(struct _reent *r, int incr)
{
    extern char __heap_start__;
    extern char __heap_end__;

    static char *heap_start = &__heap_start__;

    vTaskSuspendAll();

    if (heap_start + incr > &__heap_end__)
    {
#if (configUSE_MALLOC_FAILED_HOOK == 1)
        extern void vApplicationMallocFailedHook(void);
        vApplicationMallocFailedHook();
#endif
        return (char *) - 1;
    }

    char *prev_heap_start = heap_start;
    heap_start += incr;

    xTaskResumeAll();

    return prev_heap_start;
}

char * __attribute__((used))  sbrk(int incr) { return _sbrk_r(_impure_ptr, incr); }
char * __attribute__((used)) _sbrk(int incr) { return _sbrk_r(_impure_ptr, incr); }

/*
void * __attribute__((used)) _malloc_r(struct _reent *r, size_t size)
{
    return pvPortMalloc(size);
}

void * __attribute__((used)) _calloc_r(struct _reent *r, size_t n, size_t size)
{
    void *p = pvPortMalloc(n * size);
    if (p)
        memset(p, 0, n * size);
    return p;
}

void __attribute__((used)) _free_r(struct _reent *r, void *p)
{
    vPortFree(p);
}

void * __attribute__((used)) malloc(size_t size) { return _malloc_r(_impure_ptr, size); }
void * __attribute__((used)) calloc(size_t n, size_t size) { return _calloc_r(_impure_ptr, n, size); }
void __attribute__((used)) free(void *p) { _free_r(_impure_ptr, p); }
*/