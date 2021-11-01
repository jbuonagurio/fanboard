// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <FreeRTOS.h>
#include <task.h>

#include "HAPPlatform.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Clock" };

HAPTime HAPPlatformClockGetCurrent(void)
{
    static HAPTime previousNow;
    TimeOut_t xCurrentTime = { 0 };
    uint64_t ullTickCount = 0ull;

    // Get the current tick count and overflow count.
    vTaskInternalSetTimeOutState(&xCurrentTime);

    // Adjust the tick count for the number of times a TickType_t has overflowed.
    // portMAX_DELAY should be the maximum value of a TickType_t.
    ullTickCount = (uint64_t) (xCurrentTime.xOverflowCount) << (sizeof(TickType_t) * 8);

    // Add the current tick count.
    ullTickCount += xCurrentTime.xTimeOnEntering;

    // Convert to milliseconds.
    HAPTime now = ullTickCount / portTICK_PERIOD_MS;

    if (now < previousNow) {
        HAPLog(&logObject, "Time jumped backwards by %lu ms.", (unsigned long) (previousNow - now));
        HAPFatalError();
    }

    if (now & (1ull << 63)) {
        HAPLog(&logObject, "Time overflowed (capped at 2^63 - 1).");
        HAPFatalError();
    }

    previousNow = now;
    return now;
}
