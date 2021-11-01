// Copyright (c) 2015-2019 The HomeKit ADK Contributors
// 
// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <SEGGER_RTT.h>

#include "HAP.h"
#include "HAPPlatformLog+Init.h"

// Link to XSI-compliant version of 'strerror_r' ('__xpg_strerror_r').
int __xpg_strerror_r(int, char *, size_t);

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Log" };

void HAPPlatformLogPOSIXError(
        HAPLogType type,
        const char* _Nonnull message,
        int errorNumber,
        const char* _Nonnull function,
        const char* _Nonnull file,
        int line) {
    HAPPrecondition(message);
    HAPPrecondition(function);
    HAPPrecondition(file);

    HAPError err;

    // Get error message.
    char errorString[256];
    int e = __xpg_strerror_r(errorNumber, errorString, sizeof errorString);
    if (e == EINVAL) {
        err = HAPStringWithFormat(errorString, sizeof errorString, "Unknown error %d", errorNumber);
        HAPAssert(!err);
    } else if (e) {
        HAPAssert(e == ERANGE);
        HAPLog(&logObject, "strerror_r error: ERANGE.");
        return;
    }

    // Perform logging.
    HAPLogWithType(&logObject, type, "%s:%d:%s - %s @ %s:%d", message, errorNumber, errorString, function, file, line);
}

HAP_RESULT_USE_CHECK
HAPPlatformLogEnabledTypes HAPPlatformLogGetEnabledTypes(const HAPLogObject* _Nonnull log HAP_UNUSED) {
    switch (HAP_LOG_LEVEL) {
        case 0: {
            return kHAPPlatformLogEnabledTypes_None;
        }
        case 1: {
            return kHAPPlatformLogEnabledTypes_Default;
        }
        case 2: {
            return kHAPPlatformLogEnabledTypes_Info;
        }
        case 3: {
            return kHAPPlatformLogEnabledTypes_Debug;
        }
        default: {
            HAPFatalError();
        }
    }
}

void HAPPlatformLogCapture(
        const HAPLogObject* log,
        HAPLogType type,
        const char* message,
        const void* _Nullable bufferBytes,
        size_t numBufferBytes) HAP_DIAGNOSE_ERROR(!bufferBytes && numBufferBytes, "empty buffer cannot have a length") {
    // Color.
    switch (type) {
        case kHAPLogType_Debug: {
            SEGGER_RTT_printf(0, RTT_CTRL_RESET);
        } break;
        case kHAPLogType_Info: {
            SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_GREEN);
        } break;
        case kHAPLogType_Default: {
            SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_MAGENTA);
        } break;
        case kHAPLogType_Error: {
            SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_RED);
        } break;
        case kHAPLogType_Fault: {
            SEGGER_RTT_printf(0, RTT_CTRL_TEXT_BRIGHT_RED);
        } break;
    }

    // Time.
    HAPTime now = HAPPlatformClockGetCurrent();
    (void) SEGGER_RTT_printf(0, "%8llu.%03llu", (unsigned long long) (now / HAPSecond), (unsigned long long) (now % HAPSecond));
    (void) SEGGER_RTT_printf(0, "\t");

    // Type.
    switch (type) {
        case kHAPLogType_Debug:
            (void) SEGGER_RTT_printf(0, "Debug");
            break;
        case kHAPLogType_Info:
            (void) SEGGER_RTT_printf(0, "Info");
            break;
        case kHAPLogType_Default:
            (void) SEGGER_RTT_printf(0, "Default");
            break;
        case kHAPLogType_Error:
            (void) SEGGER_RTT_printf(0, "Error");
            break;
        case kHAPLogType_Fault:
            (void) SEGGER_RTT_printf(0, "Fault");
            break;
    }
    (void) SEGGER_RTT_printf(0, "\t");

    // Subsystem / Category.
    if (log->subsystem) {
        (void) SEGGER_RTT_printf(0, "[%s", log->subsystem);
        if (log->category) {
            (void) SEGGER_RTT_printf(0, ":%s", log->category);
        }
        (void) SEGGER_RTT_printf(0, "] ");
    }

    // Message.
    (void) SEGGER_RTT_printf(0, "%s", message);
    (void) SEGGER_RTT_printf(0, "\n");

    // Buffer.
    if (bufferBytes) {
        size_t i, n;
        const uint8_t* b = bufferBytes;
        size_t length = numBufferBytes;
        if (length == 0) {
            (void) SEGGER_RTT_printf(0, "\n");
        } else {
            i = 0;
            do {
                (void) SEGGER_RTT_printf(0, "    %04zx ", i);
                for (n = 0; n != 8 * 4; n++) {
                    if (n % 4 == 0) {
                        (void) SEGGER_RTT_printf(0, " ");
                    }
                    if ((n <= length) && (i < length - n)) {
                        (void) SEGGER_RTT_printf(0, "%02x", b[i + n] & 0xff);
                    } else {
                        (void) SEGGER_RTT_printf(0, "  ");
                    }
                };
                (void) SEGGER_RTT_printf(0, "    ");
                for (n = 0; n != 8 * 4; n++) {
                    if (i != length) {
                        if ((32 <= b[i]) && (b[i] < 127)) {
                            (void) SEGGER_RTT_printf(0, "%c", b[i]);
                        } else {
                            (void) SEGGER_RTT_printf(0, ".");
                        }
                        i++;
                    }
                }
                (void) SEGGER_RTT_printf(0, "\n");
            } while (i != length);
        }
    }

    // Reset color.
    SEGGER_RTT_printf(0, RTT_CTRL_RESET);
}
