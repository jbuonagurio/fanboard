//  Copyright 2022 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "HTTPServer.h"
#include "OTA.h"

#include <stdint.h>

#include <ti/drivers/net/wifi/netapp.h>

#include <HAPPlatform.h>

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

// Called from NWP context.
void HTTPRequestHandler(SlNetAppRequest_t *pNetAppRequest,
                        SlNetAppResponse_t *pNetAppResponse)
{
    HAPLogDebug(&logObject, "%s", __func__);

    HTTPRequest sRequest = { .requestHandle = pNetAppRequest->Handle,
                             .requestFlags = pNetAppRequest->requestData.Flags };
    
    ParseHeaders(pNetAppRequest->requestData.pMetadata, pNetAppRequest->requestData.MetadataLen, &sRequest);
    
    sRequest.payloadLen = pNetAppRequest->requestData.PayloadLen;
    if (sRequest.payloadLen > 0) {
        sRequest.payload = pvPortMalloc(sRequest.payloadLen);
        if (sRequest.payload) {
            HAPRawBufferCopyBytes(sRequest.payload, pNetAppRequest->requestData.pPayload, sRequest.payloadLen);
        }
    }

    for (size_t i = 0; i < HAPArrayCount(httpEndpoints); ++i) {
        if (httpEndpoints[i].method == pNetAppRequest->Type && strcmp(httpEndpoints[i].uri, sRequest.requestURI) == 0) {
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
