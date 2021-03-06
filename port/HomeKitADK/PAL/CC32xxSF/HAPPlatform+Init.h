// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2020 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#ifndef HAP_PLATFORM_INIT_H
#define HAP_PLATFORM_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Optional features set in Makefile.
 */
/**@{*/
#ifndef HAVE_DISPLAY
#define HAVE_DISPLAY 0
#endif

#ifndef HAVE_NFC
#define HAVE_NFC 0
#endif

#ifndef HAVE_MFI_HW_AUTH
#define HAVE_MFI_HW_AUTH 0
#endif
/**@}*/

#include <stddef.h>
#include <stdlib.h>

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Deallocate memory pointed to by ptr.
 *
 * @param      ptr                  Pointer to memory to be deallocated.
 */
#define HAPPlatformFreeSafe(ptr) \
    do { \
        HAPAssert(ptr); \
        free(ptr); \
        ptr = NULL; \
    } while (0)

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

/**
 * Platform events.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformEvent) {
    /**
     * WLAN disconnected.
     */
    kHAPPlatformEvent_Disconnected = 0x01,

    /**
     * WLAN connected.
     */
    kHAPPlatformEvent_Connected = 0x02,

    /**
     * NetApp IP acquired.
     */
    kHAPPlatformEvent_IPAcquired = 0x04
} HAP_ENUM_END(uint8_t, HAPPlatformEvent);

#ifdef __cplusplus
}
#endif

#endif
