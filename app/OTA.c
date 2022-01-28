//  Copyright 2022 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "OTA.h"

#include <stdint.h>
#include <string.h>

#include <multipartparser.h>

#include <ti/drivers/net/wifi/netapp.h>

#include <HAPPlatform.h>
#include <HAPPlatformOTA+Init.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "OTA" };

// OTA context.
static HAPPlatformOTAContext otaContext;

HAP_ENUM_BEGIN(uint8_t, PartDataType) {
    kPartDataType_Unknown = 0,
    kPartDataType_File = 1,
    kPartDataType_Signature = 2,
} HAP_ENUM_END(uint8_t, PartDataType);

static PartDataType partDataType = kPartDataType_Unknown;
static size_t otaFileSize = 0;

static struct {
    char bytes[20];
    size_t numBytes;
} headerField;

static struct {
    char bytes[40 + SL_FS_MAX_FILE_NAME_LENGTH];
    size_t numBytes;
} contentDisposition;

multipart_data_cb prevDataCallback = NULL;

static int OnDataCallback(multipartparser *p, const char *data, size_t size)
{
    if (partDataType == kPartDataType_File) {
        int16_t retval = HAPPlatformOTAWriteBlock(&otaContext, otaFileSize, (uint8_t *)data, (uint32_t)size);
        if (retval > 0)
            otaFileSize += (size_t)retval;
    }
    else if (partDataType == kPartDataType_Signature) {
        HAPAssert(otaContext.signatureSize + size <= sizeof(otaContext.signature));
        HAPRawBufferCopyBytes(&otaContext.signature[otaContext.signatureSize], data, size);
        otaContext.signatureSize += size;
    }
    
    prevDataCallback = OnDataCallback;
    return 0;
}

static int OnHeaderFieldCallback(multipartparser *p, const char *data, size_t size)
{
    if (prevDataCallback != OnHeaderFieldCallback) {
        headerField.numBytes = 0;
    }

    // Read header field.
    HAPAssert(headerField.numBytes + size <= sizeof(headerField.bytes));
    HAPRawBufferCopyBytes(&headerField.bytes[headerField.numBytes], data, size);
    headerField.numBytes += size;
    
    prevDataCallback = OnHeaderFieldCallback;
    return 0;
}

static int OnHeaderValueCallback(multipartparser *p, const char *data, size_t size)
{
    static bool contentDispositionMatch = false;

    if (prevDataCallback == OnHeaderFieldCallback) {
        contentDispositionMatch = HAPRawBufferAreEqual(headerField.bytes, "Content-Disposition", 19);
        if (contentDispositionMatch) {
            contentDisposition.numBytes = 0;
        }
    }

    // Read header value. Ignore all header fields except Content-Disposition.
    if (contentDispositionMatch) {
        HAPAssert(contentDisposition.numBytes + size <= sizeof(contentDisposition.bytes));
        HAPRawBufferCopyBytes(&contentDisposition.bytes[contentDisposition.numBytes], data, size);
        contentDisposition.numBytes += size;
    }

    prevDataCallback = OnHeaderValueCallback;
    return 0;
}

static int OnHeadersCompleteCallback(multipartparser *p)
{
    // Reset payload state.
    partDataType = kPartDataType_Unknown;
    
    // Parse Content-Disposition: form-data; name="file"; filename="file.bin"
    const char prefix[] = "form-data; name=\"";
    if (HAPRawBufferAreEqual(contentDisposition.bytes, prefix, sizeof(prefix) - 1)) {
        char *start = contentDisposition.bytes + sizeof(prefix) - 1;
        if (HAPRawBufferAreEqual(start, "file\"", 5)) {
            partDataType = kPartDataType_File;
            otaFileSize = 0;
        }
        else if (HAPRawBufferAreEqual(start, "signature\"", 10)) {
            partDataType = kPartDataType_Signature;
            otaContext.signatureSize = 0;
        }
    }
    
    return 0;
}

void OTAPutCallback(HTTPRequest *pRequest)
{
    HAPLogDebug(&logObject, "%s", __func__);

    int16_t responseCode = SL_NETAPP_HTTP_RESPONSE_201_CREATED;

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

    // Initialize the OTA context.
    static const char *filePath = "/sys/mcuflashimg.bin";
    static const char *certFilePath = "dummy-root-ca-cert";
    otaContext.filePath = (uint8_t *)filePath;
    otaContext.maxFileSize = kHAPPlatformOTA_MaxImageSize;
    otaContext.certFilePath = (uint8_t *)certFilePath;
    otaContext.signatureSize = 0;

    // Open file for write. This must be followed by a call to HAPPlatformOTAClose.
    if (HAPPlatformOTACreate(&otaContext) != kHAPError_None) {
        HAPPlatformOTAAbort(&otaContext);
        responseCode = SL_NETAPP_HTTP_RESPONSE_500_INTERNAL_SERVER_ERROR;
    }
    else {
        // Configure the parser for multipart/form-data.
        multipartparser_callbacks callbacks;
        multipartparser parser;
        multipartparser_callbacks_init(&callbacks);
        callbacks.on_header_field = OnHeaderFieldCallback;
        callbacks.on_header_value = OnHeaderValueCallback;
        callbacks.on_headers_complete = OnHeadersCompleteCallback;
        callbacks.on_data = OnDataCallback;
        multipartparser_init(&parser, boundary);

        // Read the initial payload.
        if (pRequest->payloadLen > 0) {
            multipartparser_execute(&parser, &callbacks, (char *)pRequest->payload, pRequest->payloadLen);
        }

        // Read any remaining chunks.
        uint16_t chunkSize = SL_NETAPP_REQUEST_MAX_DATA_LEN;
        static char chunkBuffer[SL_NETAPP_REQUEST_MAX_DATA_LEN];
        while (pRequest->requestFlags & SL_NETAPP_REQUEST_RESPONSE_FLAGS_CONTINUATION) {
            int16_t rc = sl_NetAppRecv(pRequest->requestHandle, (uint16_t *)&chunkSize,
                                       (uint8_t *)chunkBuffer, &pRequest->requestFlags);
            if (rc < 0) {
                HAPLogError(&logObject, "sl_NetAppRecv failed: %d.", rc);
                HAPPlatformOTAAbort(&otaContext);
                responseCode = SL_NETAPP_HTTP_RESPONSE_500_INTERNAL_SERVER_ERROR;
                break;
            }
        
            multipartparser_execute(&parser, &callbacks, chunkBuffer, chunkSize);
        }
    }

    HAPPlatformOTAClose(&otaContext);

    // Transfer complete.
    HAPLogDebug(&logObject, "OTA File Size = %u", otaFileSize);
    HAPLogBufferDebug(&logObject, otaContext.signature, otaContext.signatureSize, "Signature");
    sl_NetAppSend(pRequest->requestHandle,
                  sizeof(HTTPStatusResponse),
                  (uint8_t *)(&(const HTTPStatusResponse) {
                      .headerType = SL_NETAPP_REQUEST_METADATA_TYPE_STATUS,
                      .headerLen = 2,
                      .responseCode = responseCode
                  }),
                  SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);
    
    // 1. Reset the MCU to test the image.
    // 2. Query the image state.
    // 3. On success, commit the image.
    // 4. On failure, reset or rollback the image.
    HAPPlatformOTAActivateNewImage(&otaContext);
}

void OTAGetCallback(HTTPRequest *pRequest)
{
    HAPLogDebug(&logObject, "%s", __func__);

    const HTTPStatusResponse metadata = { .headerType = SL_NETAPP_REQUEST_METADATA_TYPE_STATUS,
                                          .headerLen = 2,
                                          .responseCode = SL_NETAPP_HTTP_RESPONSE_204_OK_NO_CONTENT };
    
    sl_NetAppSend(pRequest->requestHandle,
                  sizeof(HTTPStatusResponse),
                  (uint8_t *)&metadata,
                  SL_NETAPP_REQUEST_RESPONSE_FLAGS_METADATA);
}
