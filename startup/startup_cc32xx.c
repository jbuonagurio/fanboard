/*
 * Copyright (c) 2016-2020, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include <reent.h>

#include <ti/devices/cc32xx/inc/hw_nvic.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/inc/hw_ints.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/inc/hw_common_reg.h>

#include <ti/devices/cc32xx/driverlib/interrupt.h>
#include <ti/devices/cc32xx/inc/hw_apps_rcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>

#include <FreeRTOS.h>
#include <task.h>

/*
 * Exception handlers.
 */
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);

#define SVC_Handler vPortSVCHandler
#define PendSV_Handler xPortPendSVHandler
#define SysTick_Handler xPortSysTickHandler

__attribute__((noreturn))                            void Default_Handler(void);
__attribute__((noreturn))                            void Reset_Handler(void);
__attribute__((noreturn))                            void HardFault_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void NMI_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void MemManage_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void BusFault_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void UsageFault_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void SecureFault_Handler(void);
__attribute__((noreturn, alias("Default_Handler")))  void DebugMon_Handler(void);

/**
 * This structure prevents the CC32XXSF bootloader from overwriting the
 * internal FLASH; this allows us to flash a program that will not be
 * overwritten by the bootloader with the encrypted program saved in
 * "secure/serial flash".
 * 
 * This structure must be placed at the beginning of internal FLASH (so
 * the bootloader is able to recognize that it should not overwrite
 * internal FLASH).
 * 
 * To enable retention of the application for debug purposes, define
 * __SF_DEBUG__. If retention of the application is no longer desired,
 * define __SF_NODEBUG__.
 */
#if defined(__SF_DEBUG__)
__attribute__((section(".dbghdr"))) const uint32_t dbghdr[] = {
    0x5AA5A55A,
    0x000FF800,
    0xEFA3247D
};
#elif defined (__SF_NODEBUG__)
__attribute__((section(".dbghdr"))) const uint32_t dbghdr[] = {
    0xFFFFFFFF,
    0xFFFFFFFF,
    0xFFFFFFFF
};
#endif

/**
 * Linker variables that mark the top and bottom of stack.
 */
extern unsigned long __stack_end;
extern void *__stack;

/**
 * The vector table.  Note that the proper constructs must be placed on this to
 * ensure that it ends up at physical address 0x00000000.
 */
__attribute__((section(".reset_vectors"), used))
static void (* const __reset_vectors[16])(void) =
{
    (void (*)(void))((uint32_t)&__stack_end),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    SecureFault_Handler,
    0, /* Reserved */
    0, /* Reserved */
    0, /* Reserved */
    SVC_Handler,
    DebugMon_Handler,
    0, /* Reserved */
    PendSV_Handler,
    SysTick_Handler
};

__attribute__((section(".ram_vectors")))
static unsigned long __ram_vectors[195];

/**
 * Arrays of pointers to constructor functions that need to be called during
 * startup to initialize global objects.
 */
extern void (*__init_array_start []) (void);
extern void (*__init_array_end []) (void);

/**
 * Required for C++ support.
 */
void *__dso_handle = (void *) &__dso_handle;

/**
 * Initialize the .data and .bss sections and copy the first 16 vectors from
 * the read-only/reset table to the runtime RAM table. Fill the remaining
 * vectors with a stub. This vector table will be updated at runtime.
 */
void LocalProgramStart(void)
{
    extern uint32_t __bss_start__, __bss_end__;
    extern uint32_t __data_load__, __data_start__, __data_end__;
	
    /* Disable interrupts. */
    uint32_t newBasePri;
    __asm volatile (
        "mov %0, %1 \n\t"
        "msr basepri, %0 \n\t"
        "isb \n\t"
        "dsb \n\t"
        :"=r" (newBasePri) : "i" (configMAX_SYSCALL_INTERRUPT_PRIORITY) : "memory"
    );

    /* Initialize .bss to zero. */
    uint32_t *bs = &__bss_start__;
    uint32_t *be = &__bss_end__;
    while (bs < be) {
        *bs = 0;
        bs++;
    }

    /* Relocate the .data section. */
    uint32_t *dl = &__data_load__;
    uint32_t *ds = &__data_start__;
    uint32_t *de = &__data_end__;
    if (dl != ds) {
        while (ds < de) {
            *ds = *dl;
            dl++;
            ds++;
        }
    }

    /* Run any constructors. */
    uint32_t count = (uint32_t)(__init_array_end - __init_array_start);
    for (uint32_t i = 0; i < count; ++i)
        __init_array_start[i]();

    /* Copy flash vector table into RAM vector table. */
    memcpy(__ram_vectors, __reset_vectors, 16*4);

    /* Set remaining vectors to default handler. */
    for (uint32_t i = 16; i < 195; ++i)
        __ram_vectors[i] = (unsigned long)Default_Handler;

    /* Set the NVIC VTable base. */
    MAP_IntVTableBaseSet((unsigned long)&__ram_vectors[0]);

    /* Call the entry point for the application. */
    extern int main(void);
    main();
}

/**
 * This is the code that gets called when the processor first starts execution
 * following a reset event.  Only the absolutely necessary set is performed,
 * after which the application supplied entry() routine is called.  Any fancy
 * actions (such as making decisions based on the reset cause register, and
 * resetting the bits in that register) are left solely in the hands of the
 * application.
 */
void __attribute__((naked)) Reset_Handler(void)
{
    __asm__ __volatile__ (
        " movw r0, #:lower16:__reset_vectors\n"
        " movt r0, #:upper16:__reset_vectors\n"
        " ldr r0, [r0]\n"
        " mov sp, r0\n"
        " bl LocalProgramStart"
    );
}

__attribute__((noreturn)) void HardFault_Handler(void)
{
    while(1) {}
}

__attribute__((noreturn)) void Default_Handler(void)
{
    while(1) {}
}

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
