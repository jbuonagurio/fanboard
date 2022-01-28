//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "HTTPServer.h"

#include <ti/drivers/net/wifi/netapp.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTTP request data.
typedef struct HTTPRequest {
    uint16_t requestHandle;
    uint32_t requestFlags;
    char *requestURI;
    char *queryString;
    uint32_t contentLen;
    char *contentType;
    uint16_t payloadLen;
    uint8_t *payload;
    void (*callback)(struct HTTPRequest *);
} HTTPRequest;

// HTTP status response data.
typedef struct __attribute__((packed)) {
    uint8_t headerType;
    uint16_t headerLen;
    uint16_t responseCode;
} HTTPStatusResponse;

void HTTPRequestHandler(SlNetAppRequest_t *pNetAppRequest,
                        SlNetAppResponse_t *pNetAppResponse);

void HTTPTask(void *pvParameters);

#ifdef __cplusplus
}
#endif

