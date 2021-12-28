// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <stdint.h>

#include <ti/net/slneterr.h> 
#include <ti/net/slnetsock.h>
#include <ti/net/slnetutils.h>

#include "HAPPlatform.h"
#include "HAPPlatformSyslog+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Syslog" };

static struct {
    SlNetSock_AddrIn_t addrIn;
    int16_t socketDescriptor;
} syslog = { .socketDescriptor = -1 };

void HAPPlatformSyslogInitialize(const HAPPlatformSyslogOptions* options)
{
    HAPPrecondition(options);

    HAPRawBufferZero(&syslog.addrIn, sizeof syslog.addrIn);
    syslog.addrIn.sin_family = SLNETSOCK_AF_INET;
    syslog.addrIn.sin_port = SlNetUtil_htons(options->port);
    if (SlNetUtil_inetPton(SLNETSOCK_AF_INET, options->ip, &(syslog.addrIn.sin_addr.s_addr)) == 0) {
        HAPLogError(&logObject, "Invalid IP address.");
        return;
    }

    syslog.socketDescriptor = SlNetSock_create(SLNETSOCK_AF_INET, SLNETSOCK_SOCK_DGRAM, SLNETSOCK_PROTO_UDP, 0, 0);
    if (syslog.socketDescriptor < 0) {
        HAPLogError(&logObject, "Failed to open UDP socket.");
        return;
    }
}

void HAPPlatformSyslogWrite(const void* bytes, size_t maxBytes, size_t* _Nullable numBytes)
{
    HAPPrecondition(bytes);

    int32_t n = 0;

    if (syslog.socketDescriptor >= 0) {
        n = SlNetSock_sendTo(syslog.socketDescriptor,
                            bytes,
                            maxBytes,
                            0,
                            (SlNetSock_Addr_t *)&syslog.addrIn,
                            sizeof(syslog.addrIn));
    }

    if (numBytes) {
        *numBytes = n > 0 ? (size_t)n : 0;
    }
}
