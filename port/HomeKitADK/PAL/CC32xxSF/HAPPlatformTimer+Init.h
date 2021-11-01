// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#ifndef HAP_PLATFORM_TIMER_INIT_H
#define HAP_PLATFORM_TIMER_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

typedef struct {
    /**
     * Deadline after which the timer expires.
     */
    HAPTime deadline;

    /**
     * Callback. NULL if timer inactive.
     */
    HAPPlatformTimerCallback _Nullable callback;

    /**
     * The context parameter given to the HAPPlatformTimerRegister function.
     */
    void* _Nullable context;
} HAPPlatformTimer;

/**
 * Create timers.
 */
void HAPPlatformTimerCreate(void);

/**
 * Register timer.
 */
HAPError HAPPlatformTimerRegister(HAPPlatformTimerRef *id,
                                  HAPTime deadline,
                                  HAPPlatformTimerCallback callback,
                                  void *_Nullable context);

/**
 * Deregister timer.
 */
void HAPPlatformTimerDeregister(HAPPlatformTimerRef id);

/**
 * Returns next deadline for all timers.
 */
HAPTime HAPPlatformTimerGetNextDeadline(void);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
