#ifndef FREERTOS_CONFIG_TRACE_H
#define FREERTOS_CONFIG_TRACE_H

#include <SEGGER_RTT.h>

// Called during the tick interrupt.
#define traceTASK_INCREMENT_TICK(xTickCount)

// Called before a new task is selected to run. At this point
// pxCurrentTCB contains the handle of the task about to leave the
// Running state.
#define traceTASK_SWITCHED_OUT()

// Called after a task has been selected to run. At this point
// pxCurrentTCB contains the handle of the task about to enter the
// Running state.
#define traceTASK_SWITCHED_IN() SEGGER_RTT_printf(0,                   \
 "\x1B[1;30m            \tTrace\tTASK_SWITCHED_IN (%s)\n\x1B[0m",      \
 pxCurrentTCB->pcTaskName);

// Called when a task is transitioned into the Ready state.
#define traceMOVED_TASK_TO_READY_STATE(xTask)

// Indicates that the currently executing task is about to block
// following an attempt to read from an empty queue, or an attempt to
// ‘take’ an empty semaphore or mutex.
#define traceBLOCKING_ON_QUEUE_RECEIVE(xQueue)

// Indicates that the currently executing task is about to block
// following an attempt to write to a full queue.
#define traceBLOCKING_ON_QUEUE_SEND(xQueue)

// Called from within xQueueCreate() or xQueueCreateStatic() if the
// queue was successfully created.
#define traceQUEUE_CREATE(pxNewQueue)

// Called from within xQueueCreate() or xQueueCreateStatic() if the
// queue was not successfully created due to there being insufficient
// heap memory available.
#define traceQUEUE_CREATE_FAILED(ucQueueType) SEGGER_RTT_printf(0,     \
 "\x1B[1;30m            \tTrace\t"                                     \
 "QUEUE_CREATE_FAILED (%u)\n\x1B[0m",                                  \
 ucQueueType);

// Called from within xSemaphoreCreateMutex() if the mutex was
// successfully created.
#define traceCREATE_MUTEX(pxNewMutex)

// Called from within xSemaphoreCreateMutex() if the mutex was not
// successfully created due to there being insufficient heap memory
// available.
#define traceCREATE_MUTEX_FAILED() SEGGER_RTT_printf(0,                \
 "\x1B[1;30m            \tTrace\t"                                     \
 "CREATE_MUTEX_FAILED\n\x1B[0m");                                      \

// Called from within xSemaphoreGiveRecursive() if the mutex was
// successfully ‘given’.
#define traceGIVE_MUTEX_RECURSIVE(xMutex)

// Called from within xSemaphoreGiveRecursive() if the mutex was not
// successfully given as the calling task was not the mutex owner.
#define traceGIVE_MUTEX_RECURSIVE_FAILED(xMutex)
/*
#define traceGIVE_MUTEX_RECURSIVE_FAILED(xMutex) SEGGER_RTT_printf(0,  \
 "\x1B[1;30m            \tTrace\t"                                     \
 "GIVE_MUTEX_RECURSIVE_FAILED (%d)\n\x1B[0m",                          \
 xMutex->uxQueueNumber);
*/

// Called from within xQueueTakeMutexRecursive().
#define traceTAKE_MUTEX_RECURSIVE(xMutex)

// Called from within xSemaphoreCreateCounting() if the semaphore was
// successfully created.
#define traceCREATE_COUNTING_SEMAPHORE()

// Called from within xSemaphoreCreateCounting() if the semaphore was
// not successfully created due to insufficient heap memory being
// available.
#define traceCREATE_COUNTING_SEMAPHORE_FAILED()

// Called from within xQueueSend(), xQueueSendToFront(),
// xQueueSendToBack(), or any of the semaphore ‘give’ functions, when
// the queue send was successful.
#define traceQUEUE_SEND(xQueue)

// Called from within xQueueSend(), xQueueSendToFront(),
// xQueueSendToBack(), or any of the semaphore ‘give’ functions when the
// queue send operation failed due to the queue being full (after any
// block time that was specified).
#define traceQUEUE_SEND_FAILED(xQueue)
/*
#define traceQUEUE_SEND_FAILED(xQueue) SEGGER_RTT_printf(0,            \
 "\x1B[1;30m            \tTrace\t"                                     \
 "QUEUE_SEND_FAILED (%d)\n\x1B[0m",                                    \
 xQueue->uxQueueNumber);
*/

// Called from within xQueueReceive() or any of the semaphore ‘take’
// functions when the queue receive was successful.
#define traceQUEUE_RECEIVE(xQueue)

// Called from within xQueueReceive() or any of the semaphore ‘take’
// functions when the queue receive operation failed because the queue
// was empty (after any block time that was specified).
#define traceQUEUE_RECEIVE_FAILED(xQueue)
/*
#define traceQUEUE_RECEIVE_FAILED(xQueue) SEGGER_RTT_printf(0,         \
 "\x1B[1;30m            \tTrace\t"                                     \
 "QUEUE_RECEIVE_FAILED (%d)\n\x1B[0m",                                 \
 xQueue->uxQueueNumber);
*/

// Called from within xQueuePeek()
#define traceQUEUE_PEEK(xQueue)

// Called from within xQueueSendFromISR() when the send operation was
// successful.
#define traceQUEUE_SEND_FROM_ISR(xQueue)

// Called from within xQueueSendFromISR() when the send operation failed
// due to the queue already being full.
#define traceQUEUE_SEND_FROM_ISR_FAILED(xQueue)
/*
#define traceQUEUE_SEND_FROM_ISR_FAILED(xQueue) SEGGER_RTT_printf(0,   \
 "\x1B[1;30m            \tTrace\t"                                     \
 "QUEUE_SEND_FROM_ISR_FAILED (%d)\n\x1B[0m",                           \
 xQueue->uxQueueNumber);
*/

// Called from within xQueueReceiveFromISR() when the receive operation
// was successful.
#define traceQUEUE_RECEIVE_FROM_ISR(xQueue)

// Called from within xQueueReceiveFromISR() when the receive operation
// failed due to the queue already being empty.
#define traceQUEUE_RECEIVE_FROM_ISR_FAILED(xQueue)
/*
#define traceQUEUE_RECEIVE_FROM_ISR_FAILED(xQueue) SEGGER_RTT_printf(0,\
 "\x1B[1;30m            \tTrace\t"                                     \
 "QUEUE_RECEIVE_FROM_ISR_FAILED (%d)\n\x1B[0m",                        \
 xQueue->uxQueueNumber);
*/
// Called from within vQueueDelete().
#define traceQUEUE_DELETE(xQueue)

// Called from within xTaskCreate() (or xTaskCreateStatic()) when the
// task is successfully created.
#define traceTASK_CREATE(xTask) SEGGER_RTT_printf(0,                   \
 "\x1B[1;30m            \tTrace\t"                                     \
 "TASK_CREATE (%s)\n\x1B[0m",                                          \
 xTask->pcTaskName);

// Called from within xTaskCreate() (or xTaskCreateStatic()) when the
// task was not successfully created due to there being insufficient
// heap space available.
#define traceTASK_CREATE_FAILED(pxNewTCB) SEGGER_RTT_printf(0,         \
 "\x1B[1;30m            \tTrace\t"                                     \
 "TASK_CREATE_FAILED (%s)\n\x1B[0m",                                   \
 pxNewTCB->pcTaskName);

// Called from within vTaskDelete().
#define traceTASK_DELETE(xTask) SEGGER_RTT_printf(0,                   \
 "\x1B[1;30m            \tTrace\t"                                     \
 "TASK_DELETE (%s)\n\x1B[0m",                                          \
 xTask->pcTaskName);

// Called from within vTaskDelayUntil().
#define traceTASK_DELAY_UNTIL(xTimeToWake)

// Called from within vTaskDelay().
#define traceTASK_DELAY()

// Called from within vTaskPrioritySet().
#define traceTASK_PRIORITY_SET(xTask,uxNewPriority)

// Called from within vTaskSuspend().
#define traceTASK_SUSPEND(xTask)

// Called from within vTaskResume().
#define traceTASK_RESUME(xTask)

// Called from within xTaskResumeFromISR().
#define traceTASK_RESUME_FROM_ISR(xTask)

// Called within the timer service task each time it receives a command,
// before the command is actually processed.
#define traceTIMER_COMMAND_RECEIVED(pxTimer, xCommandID, xCommandValue)

// Called from within any API function that sends a command to the timer
// service task, for example, xTimerReset(), xTimerStop(), etc. xStatus
// will be pdFAIL if the command was not successfully sent to the timer
// command queue.
#define traceTIMER_COMMAND_SEND(pxTimer, xCommandID, xOptionalValue, xStatus)

// Called from within xTimerCreate() if the timer was successfully
// created.
#define traceTIMER_CREATE(pxNewTimer)

// Called from within xTimerCreate() if the timer was not successfully
// created due to there being insufficient heap memory available.
#define traceTIMER_CREATE_FAILED() SEGGER_RTT_printf(0,                \
 "\x1B[1;30m            \tTrace\t"                                     \
 "TIMER_CREATE_FAILED\n\x1B[0m");

// Called when a software timer expires, before the timer callback is
// executed.
#define traceTIMER_EXPIRED(pxTimer)

#endif /* FREERTOS_CONFIG_TRACE_H */
