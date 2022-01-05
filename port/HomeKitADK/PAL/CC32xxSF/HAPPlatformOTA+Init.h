/*
 * FreeRTOS OTA PAL for CC3220SF-LAUNCHXL V2.0.0
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2021 John Buonagurio
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

#ifndef HAP_PLATFORM_OTA_INIT_H
#define HAP_PLATFORM_OTA_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * OTA image state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformOTAImageState) {
    kHAPPlatformOTAImageState_Unknown,  /*!< @brief The initial state of the OTA MCU Image. */
    kHAPPlatformOTAImageState_Testing,  /*!< @brief The state of the OTA MCU Image post successful download and reboot. */
    kHAPPlatformOTAImageState_Accepted, /*!< @brief The state of the OTA MCU Image post successful download and successful self test. */
    kHAPPlatformOTAImageState_Rejected, /*!< @brief The state of the OTA MCU Image when the job has been rejected. */
    kHAPPlatformOTAImageState_Aborted   /*!< @brief The state of the OTA MCU Image after a timeout publish to the stream request fails. */
} HAP_ENUM_END(uint8_t, HAPPlatformOTAImageState);

/**
 * OTA PAL image state.
 */
HAP_ENUM_BEGIN(uint8_t, HAPPlatformOTAPALImageState) {
    kHAPPlatformOTAPALImageState_Unknown,       /*!< @brief The initial state of the OTA PAL Image. */
    kHAPPlatformOTAPALImageState_PendingCommit, /*!< @brief OTA PAL Image awaiting update. */
    kHAPPlatformOTAPALImageState_Valid,         /*!< @brief OTA PAL Image is valid. */
    kHAPPlatformOTAPALImageState_Invalid        /*!< @brief OTA PAL Image is invalid. */
} HAP_ENUM_END(uint8_t, HAPPlatformOTAPALImageState);

/**
 * OTA file context.
 */
typedef struct {
    uint8_t *filePath;           /*!< @brief Update file pathname. */
    int32_t fileDescriptor;      /*!< @brief File descriptor. */
    uint32_t fileSize;           /*!< @brief The size of the file in bytes. */
    uint8_t *certFilePath;       /*!< @brief Pathname of the certificate file used to validate the receive file. */
    uint8_t *signature;          /*!< @brief Pointer to the file's signature. */
    uint32_t signatureSize;      /*!< @brief The size of the file's signature in bytes. */
} HAPPlatformOTAContext;

/**
 * Create a new receive file for the data chunks as they come in.
 *
 * @param[in]  otaContext           OTA file context information.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If the file to be created exceeds the device's non-volatile memory size constraints.
 * @return kHAPError_Unknown        Other errors creating the file in the device's non-volatile memory.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformOTACreate(HAPPlatformOTAContext *otaContext);

/**
 * Authenticate and close the underlying receive file in the specified OTA context.
 * 
 * @param[in] otaContext            OTA file context information.
 * 
 * @return kHAPError_None           If successful.
 * @return kHAPError_NotAuthorized  If cryptographic signature verification fails.
 * @return kHAPError_Unknown        Other errors.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformOTAClose(const HAPPlatformOTAContext *otaContext);

/**
 * Write a block of data to the specified file at the given offset.
 *
 * @param[in] otaContext            OTA file context information.
 * @param[in] offset                Byte offset to write to from the beginning of the file.
 * @param[in] bytes                 Pointer to the byte array of data to write.
 * @param[in] numBytes              The number of bytes to write.
 *
 * @return The number of bytes written on success, or a negative error code.
 */
int16_t HAPPlatformOTAWriteBlock(const HAPPlatformOTAContext *otaContext,
                                 uint32_t offset,
                                 uint8_t *bytes,
                                 uint32_t numBytes);

/**
 * Activate the newest MCU image received via OTA. This function should not return.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If activation failed.
 */
HAPError HAPPlatformOTAActivateNewImage(const HAPPlatformOTAContext *otaContext);

/**
 * Reset the device. This function should not return.
 * 
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If reset failed.
 */
HAPError HAPPlatformOTAResetDevice(void);

/**
 * Attempt to set the state of the OTA update image.
 *
 * @param[in] otaContext            File context of type OtaFileContext_t.
 * @param[in] state                 The desired state of the OTA update image.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If state could not be set.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformOTASetImageState(const HAPPlatformOTAContext *otaContext,
                                     HAPPlatformOTAImageState state);

/**
 * @brief Get the state of the OTA update image.
 *
 * @param[in] otaContext            File context of type OtaFileContext_t.
 *
 * @return A value of type HAPPlatformOTAPALImageState.
 */
HAP_RESULT_USE_CHECK
HAPPlatformOTAPALImageState HAPPlatformOTAGetImageState(const HAPPlatformOTAContext *otaContext);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
