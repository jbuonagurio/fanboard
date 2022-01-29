//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ti/drivers/net/wifi/simplelink.h>

#ifdef __cplusplus
extern "C" {
#endif

void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *pSlFatalErrorEvent);

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pSlDeviceEvent);

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pSlNetAppEvent);

void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pSlHttpServerEvent,
                                      SlNetAppHttpServerResponse_t *pSlHttpServerResponse);

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse);

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer);

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent);

void SimpleLinkSockEventHandler(SlSockEvent_t *pSlSockEvent);

#ifdef __cplusplus
}
#endif
