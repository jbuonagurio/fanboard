// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#ifndef HAP_PLATFORM_SYSLOG_INIT_H
#define HAP_PLATFORM_SYSLOG_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Syslog initialization options.
 */
typedef struct {
    /**
     * IPv4 address for the remote syslog server.
     */
    const char *ip;

    /**
     * UDP port number for the remote syslog server.
     */
    HAPNetworkPort port;
} HAPPlatformSyslogOptions;

/**
 * Initializes logging to a remote syslog server.
 *
 * @param      options          Initialization options.
 */
void HAPPlatformSyslogInitialize(const HAPPlatformSyslogOptions* options);

/**
 * Suspend syslog logging.
 */
void HAPPlatformSyslogSuspend(void);

/**
 * Resume syslog logging.
 */
void HAPPlatformSyslogResume(void);

/**
 * Send RTT buffer to remote syslog server.
 *
 * @param      bufferIndex      Index of the RTT buffer to send.
 */
void HAPPlatformSyslogCapture(unsigned bufferIndex);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
