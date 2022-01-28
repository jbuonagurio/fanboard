/*
 * FreeRTOS V202112.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://aws.amazon.com/freertos
 *
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Application specific definitions. */
/* Note: system locks required by newlib are not implemented. */
#define configUSE_PREEMPTION                       1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION    1
#define configUSE_TICKLESS_IDLE                    1
#define configEXPECTED_IDLE_TIME_BEFORE_SLEEP      5
#define configCPU_CLOCK_HZ                         ((unsigned long)80000000)
#define configTICK_RATE_HZ                         ((TickType_t)1000)
#define configMAX_PRIORITIES                       (10UL)
#define configMINIMAL_STACK_SIZE                   ((unsigned short)256)
#define configMAX_TASK_NAME_LEN                    (12)
#define configUSE_16_BIT_TICKS                     0
#define configIDLE_SHOULD_YIELD                    0
#define configUSE_TASK_NOTIFICATIONS               1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES      1
#define configUSE_MUTEXES                          1
#define configUSE_RECURSIVE_MUTEXES                1
#define configUSE_COUNTING_SEMAPHORES              1
#define configQUEUE_REGISTRY_SIZE                  10
#define configUSE_QUEUE_SETS                       0
#define configUSE_TIME_SLICING                     0
#define configUSE_NEWLIB_REENTRANT                 1
#define configENABLE_BACKWARD_COMPATIBILITY        0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS    0
#define configSTACK_DEPTH_TYPE                     uint16_t
#define configMESSAGE_BUFFER_LENGTH_TYPE           size_t

/* Memory allocation related definitions. */
#define configSUPPORT_STATIC_ALLOCATION            1
#define configSUPPORT_DYNAMIC_ALLOCATION           1
#define configTOTAL_HEAP_SIZE                      ((size_t)(0x10000))
#define configAPPLICATION_ALLOCATED_HEAP           0
#define configSTACK_ALLOCATION_FROM_SEPARATE_HEAP  0

/* Hook function related definitions. */
#define configUSE_IDLE_HOOK                        0
#define configUSE_TICK_HOOK                        0
#define configCHECK_FOR_STACK_OVERFLOW             2
#define configUSE_MALLOC_FAILED_HOOK               1
#define configUSE_DAEMON_TASK_STARTUP_HOOK         0
#define configUSE_APPLICATION_TASK_TAG             1

/* Run time and task stats gathering related definitions. */
#define configGENERATE_RUN_TIME_STATS              0
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS()
#define portGET_RUN_TIME_COUNTER_VALUE()           0
#define configUSE_TRACE_FACILITY                   1
#define configUSE_STATS_FORMATTING_FUNCTIONS       0

/* Co-routine related definitions. */
#define configUSE_CO_ROUTINES                      0
#define configMAX_CO_ROUTINE_PRIORITIES            1

/* Software timer related definitions. */
#define configUSE_TIMERS                           1
#define configTIMER_TASK_PRIORITY                  (5)
#define configTIMER_QUEUE_LENGTH                   (20)
#define configTIMER_TASK_STACK_DEPTH               (configMINIMAL_STACK_SIZE * 2)

/* Idle task stack size in words. */
#define configIDLE_TASK_STACK_DEPTH                (configMINIMAL_STACK_SIZE * 2)

/* Default stack size for TI-POSIX threads in words. */
#define configPOSIX_STACK_SIZE                     ((unsigned short)512)

/* Initialize ISR stack to known value for TI Runtime Object View. */
#define configENABLE_ISR_STACK_INIT                0

/* Cortex-M4 interrupt priority configuration. */
/* See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configPRIO_BITS 3
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 0x07
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 1
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Define to trap errors during development. */
#define configASSERT(x) if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;); }

/* Optional functions. */
#define INCLUDE_vTaskPrioritySet                   1
#define INCLUDE_uxTaskPriorityGet                  1
#define INCLUDE_vTaskDelete                        1
#define INCLUDE_vTaskCleanUpResources              0
#define INCLUDE_vTaskSuspend                       1
#define INCLUDE_xTaskResumeFromISR                 1
#define INCLUDE_xTaskDelayUntil                    1
#define INCLUDE_vTaskDelay                         1
#define INCLUDE_xTaskAbortDelay                    1
#define INCLUDE_xTaskGetSchedulerState             1
#define INCLUDE_xTimerPendFunctionCall             0
#define INCLUDE_xQueueGetMutexHolder			   1
#define INCLUDE_uxTaskGetStackHighWaterMark        1
#define INCLUDE_eTaskGetState                      1
#define INCLUDE_xTaskGetHandle                     1
#define INCLUDE_xTaskGetIdleTaskHandle             1
#define INCLUDE_xTaskGetCurrentTaskHandle          1
#define INCLUDE_pxTaskGetStackStart                1

/* SEGGER SystemView support. */
#include "SEGGER_SYSVIEW_FreeRTOS.h"

#endif /* FREERTOS_CONFIG_H */
