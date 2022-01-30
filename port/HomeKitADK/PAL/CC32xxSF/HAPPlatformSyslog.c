// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <FreeRTOS.h>
#include <semphr.h>

#include <ti/drivers/dpl/HwiP.h>
#include <ti/net/slneterr.h> 
#include <ti/net/slnetsock.h>
#include <ti/net/slnetutils.h>

#include <SEGGER_RTT_Conf.h>
#include <SEGGER_RTT.h>

#include "HAPPlatform.h"
#include "HAPPlatformSyslog+Init.h"

#define kRTT_BufferSizeUp (BUFFER_SIZE_UP)

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Syslog" };

static struct {
    SlNetSock_AddrIn_t addrIn;
    int16_t socketDescriptor;
    uint8_t buffer[kRTT_BufferSizeUp];
    SemaphoreHandle_t _Nullable mutex;
} syslog = {
    .socketDescriptor = -1,
    .mutex = NULL
};

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

    // Use a mutex to guard access to RTT buffer and socket.
    syslog.mutex = xSemaphoreCreateMutex();
}

void HAPPlatformSyslogSuspend(void)
{
    if (syslog.mutex) {
        xSemaphoreTake(syslog.mutex, portMAX_DELAY);
    }
}

void HAPPlatformSyslogResume(void)
{
    if (syslog.mutex) {
        xSemaphoreGive(syslog.mutex);
    }
}

void HAPPlatformSyslogCapture(unsigned bufferIndex)
{
    // Make sure we are not logging from an ISR.
    HAPAssert(!HwiP_inISR());

    if (syslog.socketDescriptor < 0)
        return;
    
    if (xSemaphoreTake(syslog.mutex, pdMS_TO_TICKS(20))) {
        // SEGGER_RTT_ReadUpBuffer locks against all other RTT operations
        // and must not be called when J-Link might also do RTT.
        unsigned numBytes = SEGGER_RTT_ReadUpBuffer(bufferIndex, syslog.buffer, kRTT_BufferSizeUp);
        SlNetSock_sendTo(syslog.socketDescriptor, syslog.buffer, (uint32_t)numBytes, 0,
                         (SlNetSock_Addr_t *)&syslog.addrIn, sizeof(syslog.addrIn));
        xSemaphoreGive(syslog.mutex);
    }
}
