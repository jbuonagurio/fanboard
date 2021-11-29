// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <FreeRTOS.h>
#include <timers.h>

#include "HAPPlatform.h"
#include "HAPPlatformTimer+Init.h"
#include "HAPPlatformRunLoop+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Timer" };

#define kTimerStorage_MaxTimers ((size_t) 32)

static HAPPlatformTimer timers[kTimerStorage_MaxTimers] = { 0 };
static TimerHandle_t timerHandles[kTimerStorage_MaxTimers] = { 0 };
static StaticTimer_t timerBuffers[kTimerStorage_MaxTimers] = { 0 };

static void TimerCallback(TimerHandle_t handle)
{
    size_t i = (size_t)pvTimerGetTimerID(handle);
    HAPAssert(i < HAPArrayCount(timers));

    HAPLogDebug(&logObject, "Expired timer: %u", i);

    // Invoke user callback.
    // TODO: Timer callback functions execute in the context of the timer service task.
    // Use HAPPlatformRunLoopScheduleCallback to schedule.
    if (timers[i].callback) {
        timers[i].callback((HAPPlatformTimerRef)i, timers[i].context);
        timers[i].callback = NULL;
    }
}

void HAPPlatformTimerCreate(void)
{
    // Create software timer instances for later use.
    for (uint32_t i = 0; i < kTimerStorage_MaxTimers; ++i) {
        char timerName[4] =  { '\0' };
        HAPError err = HAPStringWithFormat(timerName, sizeof timerName, "T%02lu", i + 1);
        HAPAssert(!err);
        timerHandles[i] = xTimerCreateStatic(timerName,           // pcTimerName
                                             1,                   // xTimerPeriod
                                             pdFALSE,             // uxAutoReload
                                             (void *)i,           // pvTimerID
                                             TimerCallback,       // pxCallbackFunction
                                             &(timerBuffers[i])); // pxTimerBuffer
    }
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformTimerRegister(HAPPlatformTimerRef *id,
                                  HAPTime deadline,
                                  HAPPlatformTimerCallback callback,
                                  void *_Nullable context)
{
    HAPPrecondition(id);
    HAPPrecondition(callback);
    
    // Find timer slot.
    size_t i;
    for (i = 0; i <= kTimerStorage_MaxTimers; ++i) {
        if (i == kTimerStorage_MaxTimers) {
            HAPLog(&logObject, "Cannot allocate more timers.");
            return kHAPError_OutOfResources;
        }
        else if (xTimerIsTimerActive(timerHandles[i]) == pdFALSE) {
            break;
        }
    }

    // Store client data.
    timers[i].deadline = deadline;
    timers[i].callback = callback;
    timers[i].context = context;

    // Store timer ID.
    *id = (HAPPlatformTimerRef)pvTimerGetTimerID(timerHandles[i]);
    
    HAPLogDebug(&logObject, "Added timer: %lu (deadline %8llu.%03llu).",
                (unsigned long) *id,
                (unsigned long long) (timers[i].deadline / HAPSecond),
                (unsigned long long) (timers[i].deadline % HAPSecond));

    // Calculate the timer period.
    TickType_t period;
    HAPTime currentTime = HAPPlatformClockGetCurrent();
    if (deadline > currentTime) {
        period = pdMS_TO_TICKS(deadline - currentTime);
    } else {
        period = 1;
    }

    // Start the timer.
    BaseType_t e = xTimerChangePeriod(timerHandles[i], period, 0);
    if (e == pdFAIL) {
        HAPLogError(&logObject, "Failed to send change period command to timer command queue.");
        return kHAPError_Busy;
    }

    return kHAPError_None;
}

void HAPPlatformTimerDeregister(HAPPlatformTimerRef id)
{
    HAPPrecondition((size_t)id < HAPArrayCount(timers));

    size_t i = (size_t)id;
    HAPAssert(timers[i].callback);
    xTimerStop(timerHandles[i], 0);
    timers[i].callback = NULL;
    timers[i].deadline = 0;

    HAPLogDebug(&logObject, "Removed timer: %lu", (unsigned long) id);
    return;
}

// TODO: Remove
HAP_RESULT_USE_CHECK
HAPTime HAPPlatformTimerGetNextDeadline(void)
{
    HAPTime deadline = 0;
    HAPTime now = HAPPlatformClockGetCurrent();

    // Find timer.
    for (size_t i = 0; i < kTimerStorage_MaxTimers; i++) {
        if (timers[i].deadline > now && timers[i].deadline > deadline) {
            deadline = timers[i].deadline;
        }
    }

    return deadline;
}
