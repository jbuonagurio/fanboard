//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "HTTP.h"

#include <stdint.h>
#include <string.h>

#include <multipartparser.h>

#include <ti/drivers/net/wifi/netapp.h>

#include <HAPPlatform.h>
#include <HAPPlatformOTA+Init.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "HTTP" };

// Maximum number of requests in HTTP queue.
#define kHTTP_QueueDepth ((size_t) 10)

// FreeRTOS task handle.
static TaskHandle_t httpTaskHandle = NULL;

// HTTP request queue.
QueueHandle_t httpQueue = NULL;

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

static void OTAPutCallback(HTTPRequest *pRequest);
static void OTAGetCallback(HTTPRequest *pRequest);

// HTTP endpoints.
static struct {
    uint8_t index;
    uint8_t method;
    const char *uri;
    void (*callback)(HTTPRequest *);
} httpEndpoints[] = {
    { 0, SL_NETAPP_REQUEST_HTTP_PUT, "/ota", OTAPutCallback },
    { 1, SL_NETAPP_REQUEST_HTTP_GET, "/ota", OTAGetCallback }
};

// curl -X PUT "http://simplelink.local/ota" -F "file=@fanboard.bin" -v | tee /dev/null

static size_t numPayloadBytes = 0;

int OnPartData(multipartparser *p, const char *data, size_t size)
{
    numPayloadBytes += size;
    return 0;
}

static void OTAPutCallback(HTTPRequest *pRequest)
{
    HAPLogInfo(&logObject, "%s", __func__);
    
    size_t numParsedBytes = 0;
    numPayloadBytes = 0;

    // Extract boundary string for multipart/form-data.
    char boundary[70 + 1] = {0};
    if (!pRequest->contentType) {
        HAPLogError(&logObject, "Missing Content-Type.");
        return;
    }

    char *boundaryPos = strstr(pRequest->contentType, "boundary=") + 9;
    if (!boundaryPos) {
        HAPLogError(&logObject, "Invalid Content-Type.");
        return;
    }

    strncpy(boundary, boundaryPos, 70);
    
    // Configure the parser for multipart/form-data.
    multipartparser_callbacks callbacks;
    multipartparser parser;
    multipartparser_callbacks_init(&callbacks);
    callbacks.on_data = OnPartData;
    multipartparser_init(&parser, boundary);

    // Read the initial payload.
    numParsedBytes += multipartparser_execute(&parser, &callbacks, (char *)pRequest->payload, pRequest->payloadLen);
    
    // Read any remaining chunks.
    uint16_t chunkSize = SL_NETAPP_REQUEST_MAX_DATA_LEN;
    static char chunkBuffer[SL_NETAPP_REQUEST_MAX_DATA_LEN];
    while (pRequest->requestFlags & SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION) {
        int16_t rc = sl_NetAppRecv(pRequest->requestHandle, (uint16_t *)&chunkSize,
                                   (uint8_t *)chunkBuffer, &pRequest->requestFlags);
        if (rc < 0) {
            HAPLogError(&logObject, "sl_NetAppRecv failed: %d.", rc);
            break;
        }
        numParsedBytes += multipartparser_execute(&parser, &callbacks, chunkBuffer, chunkSize);
    }

    // Transfer complete.
    HAPLogInfo(&logObject, "Parsed Bytes = %u", numParsedBytes);
    HAPLogInfo(&logObject, "Payload Bytes = %u", numPayloadBytes);
    sl_NetAppSend(pRequest->requestHandle,
                  sizeof(HTTPStatusResponse),
                  (uint8_t *)(&(const HTTPStatusResponse) {
                      .headerType = SL_NETAPP_REQUEST_METADATA_TYPE_STATUS,
                      .headerLen = 2,
                      .responseCode = SL_NETAPP_HTTP_RESPONSE_201_CREATED
                  }),
                  SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);
}

static void OTAGetCallback(HTTPRequest *pRequest)
{
    HAPLogInfo(&logObject, "%s", __func__);

    const HTTPStatusResponse metadata = { .headerType = SL_NETAPP_REQUEST_METADATA_TYPE_STATUS,
                                          .headerLen = 2,
                                          .responseCode = SL_NETAPP_HTTP_RESPONSE_204_OK_NO_CONTENT };
    
    sl_NetAppSend(pRequest->requestHandle,
                  sizeof(HTTPStatusResponse),
                  (uint8_t *)&metadata,
                  SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);
}

static void ParseHeaders(uint8_t *pMetadata, uint16_t metadataLen, HTTPRequest *pRequest)
{
    pRequest->requestURI = NULL;
    pRequest->queryString = NULL;
    pRequest->contentLen = 0;
    pRequest->contentType = NULL;

    if (metadataLen < 3)
        return;
    
    uint8_t *pTLV = pMetadata;
    uint8_t *pEnd = pMetadata + metadataLen;

    while (pTLV < pEnd) {
        uint8_t headerType = *pTLV;
        pTLV++;
        uint16_t headerLen = *(uint16_t *)pTLV;
        pTLV += 2;

        switch (headerType) {
        case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_REQUEST_URI:
            pRequest->requestURI = pvPortMalloc(headerLen + 1);
            if (pRequest->requestURI) {
                HAPRawBufferZero(pRequest->requestURI, headerLen + 1);
                HAPRawBufferCopyBytes(pRequest->requestURI, pTLV, headerLen);
            }
            break;
        case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_QUERY_STRING:
            pRequest->queryString = pvPortMalloc(headerLen + 1);
            if (pRequest->queryString) {
                HAPRawBufferZero(pRequest->queryString, headerLen + 1);
                HAPRawBufferCopyBytes(pRequest->queryString, pTLV, headerLen);
            }
            break;
        case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_LEN:
            if (headerLen < sizeof(pRequest->contentLen)) {
                HAPRawBufferCopyBytes(&pRequest->contentLen, pTLV, headerLen);
            }
            break;
        case SL_NETAPP_REQUEST_METADATA_TYPE_HTTP_CONTENT_TYPE:
            pRequest->contentType = pvPortMalloc(headerLen + 1);
            if (pRequest->contentType) {
                HAPRawBufferZero(pRequest->contentType, headerLen + 1);
                HAPRawBufferCopyBytes(pRequest->contentType, pTLV, headerLen);
            }
            break;
        default:
            break;
        }

        pTLV += headerLen;
    }
}

static void HTTPRequestFree(HTTPRequest *pRequest)
{
    if (pRequest) {
        vPortFree(pRequest->requestURI);
        vPortFree(pRequest->queryString);
        vPortFree(pRequest->contentType);
        vPortFree(pRequest->payload);
    }
}

void HTTPRequestHandler(SlNetAppRequest_t *pNetAppRequest,
                        SlNetAppResponse_t *pNetAppResponse)
{
    HTTPRequest sRequest = { .requestHandle = pNetAppRequest->Handle,
                             .requestFlags = pNetAppRequest->requestData.Flags };
    
    ParseHeaders(pNetAppRequest->requestData.pMetadata, pNetAppRequest->requestData.MetadataLen, &sRequest);

    sRequest.payloadLen = pNetAppRequest->requestData.PayloadLen;
    sRequest.payload = pvPortMalloc(sRequest.payloadLen);
    if (sRequest.payload) {
        HAPRawBufferCopyBytes(sRequest.payload, pNetAppRequest->requestData.pPayload, sRequest.payloadLen);
    }

    for (size_t i = 0; i < HAPArrayCount(httpEndpoints); ++i) {
        if (httpEndpoints[i].method == pNetAppRequest->Type &&
            HAPStringAreEqual(httpEndpoints[i].uri, sRequest.requestURI)) {
            pNetAppResponse->Status = SL_NETAPP_RESPONSE_PENDING;
            pNetAppResponse->ResponseData.pMetadata = NULL;
            pNetAppResponse->ResponseData.MetadataLen = 0;
            pNetAppResponse->ResponseData.pPayload = NULL;
            pNetAppResponse->ResponseData.PayloadLen = 0;
            pNetAppResponse->ResponseData.Flags = 0;
            
            // Assign callback and post to queue for processing in application context.
            // Do not block the NWP task.
            sRequest.callback = httpEndpoints[i].callback;
            if (xQueueSendToBack(httpQueue, (void *)&sRequest, 0) != pdTRUE) {
                pNetAppResponse->Status = SL_NETAPP_HTTP_RESPONSE_503_SERVICE_UNAVAILABLE;
                HAPLogError(&logObject, "Failed to post message to HTTP queue.");
            }
            break;
        }
    }
}

void HTTPTask(void *pvParameters)
{
    httpTaskHandle = xTaskGetCurrentTaskHandle();

    httpQueue = xQueueCreate(kHTTP_QueueDepth, sizeof(HTTPRequest));
    if (httpQueue == NULL) {
        HAPLogFault(&logObject, "Failed to create HTTP queue.");
        HAPFatalError();
    }

    vQueueAddToRegistry(httpQueue, "HTTP Queue");

    for (;;) {
        HTTPRequest sRequest;
        if (xQueueReceive(httpQueue, (void *)&sRequest, portMAX_DELAY) == pdPASS) {
            // Handle the request.
            if (sRequest.callback)
                sRequest.callback(&sRequest);
            HTTPRequestFree(&sRequest);
        }
    }
}
