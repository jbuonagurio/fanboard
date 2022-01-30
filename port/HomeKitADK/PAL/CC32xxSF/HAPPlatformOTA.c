/*
 * FreeRTOS OTA PAL for CC3220SF-LAUNCHXL V2.0.0
 * Copyright (c) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Copyright (c) 2022 John Buonagurio
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

#include <stdint.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>

#include "FreeRTOS.h"
#include "task.h"

#include "HAPPlatformOTA+Init.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "OTA" };

/* This is our specific file token for OTA updates. */
#define kOTA_VendorToken 1952007250UL

/* Maximum allowed block write retries (in addition to the first try). */
#define kOTA_MaxBlockWriteRetries 3U

/* Maximum allowed retries to create the OTA receive file. */
#define kOTA_MaxCreateRetries 1

/* Use a 16 second watchdog timer. /2 for 2x factor from system clock. */
#define kOTA_WDTTimeout (16UL / 2UL)

/* TI SimpleLink watchdog timer start key (APPS_WDT_START_KEY). */
#define kOTA_WDTStartKey 0xAE42DB15UL

/* The TI SimpleLink watchdog clock source runs at 80MHz. */
#define kOTA_WDTClockHz 80000000UL

typedef struct {
    uint8_t ucActiveImg;
    uint32_t ulImgStatus;
    uint32_t ulStartWdtKey;
    uint32_t ulStartWdtTime;
} sBootInfo_t;

static void RollbackBundle(void)
{
    SlFsControl_t fsControl;
    fsControl.IncludeFilters = 0U;
    int32_t retval = sl_FsCtl(SL_FS_CTL_BUNDLE_ROLLBACK,
                              0UL,
                              (uint8_t *)NULL,
                              (uint8_t *)&fsControl,
                              (uint16_t)sizeof(SlFsControl_t),
                              (uint8_t *)NULL,
                              0U,
                              (unsigned long *)NULL);
    if (retval != 0) {
        HAPLogError(&logObject, "Bundle rollback failed (%d).", (int)retval);
    }
    else {
        HAPLogInfo(&logObject, "Bundle rollback succeeded.");
    }
}

static void RollbackRxFile(HAPPlatformOTAContext *otaContext)
{
    uint32_t newToken = 0UL; /* New token is not retained. */
    SlFsControl_t fsControl;
    fsControl.IncludeFilters = 0U;
    int32_t retval = sl_FsCtl(SL_FS_CTL_ROLLBACK,
                              kOTA_VendorToken,
                              (uint8_t *)otaContext->filePath,
                              (uint8_t *)&fsControl,
                              (uint16_t)sizeof(SlFsControl_t),
                              (uint8_t *)NULL,
                              0U,
                              (unsigned long *)&newToken);
    if (retval != 0) {
        HAPLogError(&logObject, "File %s rollback failed (%d).", otaContext->filePath, (int)retval);
    }
    else {
        HAPLogInfo(&logObject, "File %s rolled back.", otaContext->filePath);
    }
}

static void DeleteRxFile(HAPPlatformOTAContext *otaContext)
{
    int32_t retval = sl_FsDel((uint8_t *)otaContext->filePath, kOTA_VendorToken);
    if (retval != 0) {
        HAPLogError(&logObject, "File %s delete failed (%d).", otaContext->filePath, (int)retval);
    }
    else {
        HAPLogInfo(&logObject, "File %s deleted.", otaContext->filePath);
    }
}

static int32_t CreateBootInfoFile(void)
{
    uint32_t token = kOTA_VendorToken;
    int32_t retval;

    int32_t fd = sl_FsOpen((const uint8_t *)"/sys/mcubootinfo.bin",
                            SL_FS_CREATE | SL_FS_OVERWRITE | SL_FS_CREATE_MAX_SIZE(sizeof(sBootInfo_t)) |
                            SL_FS_CREATE_SECURE | SL_FS_CREATE_VENDOR_TOKEN |
                            SL_FS_CREATE_PUBLIC_WRITE | SL_FS_CREATE_NOSIGNATURE,
                            (_u32 *)&token);

    if (fd < 0) {
        HAPLogError(&logObject, "Error opening bootinfo file: %d.", (int)fd);
        /* Propagate error to caller. */
        retval = fd;
    }
    else {
        sBootInfo_t bootInfo;
        HAPRawBufferZero(&bootInfo, sizeof(sBootInfo_t));
        bootInfo.ulStartWdtTime = kOTA_WDTClockHz * kOTA_WDTTimeout;
        bootInfo.ulStartWdtKey = kOTA_WDTStartKey;
        retval = sl_FsWrite(fd, 0UL, (uint8_t *)&bootInfo, (uint32_t)sizeof(sBootInfo_t));
        if (retval != (int32_t)sizeof(sBootInfo_t)) {
            HAPLogError(&logObject, "Error writing bootinfo file: %d.", (int)retval);
            if (retval > 0) {
                /* Force a fail result to the caller. Map to zero bytes written. */
                retval = 0;
            }
            /* Else retval is a negative error code. */
        }

        /* Close the file even after a write failure. */
        int32_t closeResult = sl_FsClose(fd, (uint8_t *) NULL, (uint8_t *) NULL, 0UL);
        if (closeResult < 0) {
            HAPLogError(&logObject, "Error closing bootinfo file: %d.", (int)closeResult);
            /* Return the most recent error code to the caller. */
            retval = closeResult;
        }
    }

    return retval;
}

HAPError HAPPlatformOTAAbort(HAPPlatformOTAContext *otaContext)
{
    /* Do nothing if file descriptor is invalid. */
    if (otaContext->fileDescriptor < 0)
        return kHAPError_None;

    /* Call sl_FsClose with signature = 'A' for abort. */
    uint8_t signature[] = "A";
    int32_t retval = sl_FsClose(otaContext->fileDescriptor, (uint8_t *)NULL, (uint8_t *)signature, 1);
    if (retval < 0) {
        HAPLogError(&logObject, "File abort failed (%d).", (int)retval);
    }

    otaContext->fileDescriptor = -1;
    RollbackBundle();

    if (retval != 0)
        return kHAPError_Unknown;
    
    return kHAPError_None;
}

HAPError HAPPlatformOTACreate(HAPPlatformOTAContext *otaContext)
{
    HAPError err = kHAPError_Unknown;
    uint32_t token = kOTA_VendorToken;
    uint32_t flags;
    int32_t retval;

    otaContext->fileDescriptor = -1;

    if (otaContext->maxFileSize <= kHAPPlatformOTA_MaxImageSize) {
        bool isFlashImage = HAPStringAreEqual((const char *)otaContext->filePath, "/sys/mcuflashimg.bin");
        if (isFlashImage) {
            flags = SL_FS_CREATE | SL_FS_OVERWRITE | SL_FS_CREATE_FAILSAFE |
                    SL_FS_CREATE_PUBLIC_WRITE | SL_FS_WRITE_BUNDLE_FILE |
                    SL_FS_CREATE_SECURE | SL_FS_CREATE_VENDOR_TOKEN;
            /* Create a boot info file for configuring watchdog timer. */
            retval = CreateBootInfoFile();
        }
        else {
            flags = SL_FS_CREATE | SL_FS_OVERWRITE | SL_FS_CREATE_NOSIGNATURE;
            /* Set the retval explicitly as we do not create a boot info file for non firmware files. */
            retval = 1;
        }

        /* CreateBootInfoFile returns the number of bytes written or negative error. 0 is not allowed. */
        if (retval > 0) {
            int32_t retry = 0;
            do {
                /* The file remains open until the OTA agent calls HAPPlatformOTAClose() after transfer or failure. */
                retval = sl_FsOpen((uint8_t *)otaContext->filePath,
                                   (uint32_t)(flags | SL_FS_CREATE_MAX_SIZE(otaContext->maxFileSize)),
                                   (unsigned long *)&token);
                
                if (retval > 0) {
                    HAPLogInfo(&logObject, "Receive file created. Token: %u.", (unsigned)token);
                    otaContext->fileDescriptor = (int32_t)retval;
                    err = kHAPError_None;
                }
                else {
                    HAPLogError(&logObject, "Error (%d) trying to create receive file.", (int)retval);
                    if (retval == SL_ERROR_FS_FILE_IS_ALREADY_OPENED) {
                        /* System is in an inconsistent state and must be rebooted. */
                        if (HAPPlatformOTAResetDevice() != kHAPError_None) {
                            HAPLogError(&logObject, "Failed to reset the device via software.");
                        }
                    }
                    else if (retval == SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY) {
                        /* Attempt to delete the invalid receive file and try again. */
                        DeleteRxFile(otaContext);
                    }
                    else if (retval == SL_ERROR_FS_FILE_IS_PENDING_COMMIT) {
                        /* Attempt to rollback the receive file and try again. */
                        RollbackRxFile(otaContext);
                    }
                    else {
                        if (!isFlashImage) {
                            /* Attempt to rollback the bundle and try again. */
                            RollbackBundle();
                        }
                    }
                    retry++;
                    err = kHAPError_Unknown;
                }
            } while ((err != kHAPError_None) && (retry <= (int32_t)kOTA_MaxCreateRetries));
        }
        else {
            /* Failed to create bootinfo file. */
            err = kHAPError_Unknown;
        }
    }
    else {
        /* File is too big for the platform. */
        HAPLogError(&logObject, "Error (%ld) trying to create receive file.", SL_ERROR_FS_FILE_MAX_SIZE_EXCEEDED);
        err = kHAPError_OutOfResources;
    }

    return err;
}

HAPError HAPPlatformOTAClose(const HAPPlatformOTAContext *otaContext)
{
    /* Do nothing if file descriptor is invalid. */
    if (otaContext->fileDescriptor < 0)
        return kHAPError_None;
    
    HAPLogInfo(&logObject, "Authenticating and closing file.");
    int16_t retval = sl_FsClose(otaContext->fileDescriptor,
                                otaContext->certFilePath,
                                &otaContext->signature[0],
                                (uint32_t)(otaContext->signatureSize));
    switch (retval) {
    case 0L:
        return kHAPError_None;
    case SL_ERROR_FS_WRONG_SIGNATURE_SECURITY_ALERT:
    case SL_ERROR_FS_WRONG_SIGNATURE_OR_CERTIFIC_NAME_LENGTH:
    case SL_ERROR_FS_CERT_IN_THE_CHAIN_REVOKED_SECURITY_ALERT:
    case SL_ERROR_FS_INIT_CERTIFICATE_STORE:
    case SL_ERROR_FS_ROOT_CA_IS_UNKOWN:
    case SL_ERROR_FS_CERT_CHAIN_ERROR_SECURITY_ALERT:
    case SL_ERROR_FS_FILE_NOT_EXISTS:
    case SL_ERROR_FS_ILLEGAL_SIGNATURE:
    case SL_ERROR_FS_WRONG_CERTIFICATE_FILE_NAME:
    case SL_ERROR_FS_NO_CERTIFICATE_STORE:
        HAPLogError(&logObject, "Failed to close file (%d).", retval);
        return kHAPError_NotAuthorized;
    default:
        HAPLogError(&logObject, "Failed to close file (%d).", retval);
        return kHAPError_Unknown;
    }
}

int16_t HAPPlatformOTAWriteBlock(const HAPPlatformOTAContext *otaContext,
                                 uint32_t offset,
                                 uint8_t *bytes,
                                 uint32_t numBytes)
{
    int32_t retval;
    uint32_t numBytesWritten = 0;
    uint32_t retry;
    for (retry = 0UL; retry <= kOTA_MaxBlockWriteRetries; retry++) {
        retval = sl_FsWrite(otaContext->fileDescriptor, offset + numBytesWritten, &bytes[numBytesWritten], numBytes);
        if (retval >= 0) {
            if (numBytes == (uint32_t)retval) {
                /* If we wrote all of the bytes requested, we're done. */
                break;
            }
            else {
                numBytesWritten += (uint32_t)retval; /* Add to total bytes written counter. */
                numBytes -= (uint32_t)retval; /* Reduce block size by amount just written. */
            }
        }
    }

    if (retry > kOTA_MaxBlockWriteRetries) {
        HAPLogError(&logObject, "Aborted after %u retries.", kOTA_MaxBlockWriteRetries);
        return (int16_t)SL_ERROR_FS_FAILED_TO_WRITE;
    }
    
    return (int16_t)retval;
}

HAPError HAPPlatformOTAActivateNewImage(const HAPPlatformOTAContext *otaContext HAP_UNUSED)
{
    HAPLogInfo(&logObject, "Activating the new MCU image.");
    return HAPPlatformOTAResetDevice();
}

HAPError HAPPlatformOTAResetDevice(void)
{
    HAPLogInfo(&logObject, "Stopping NWP and resetting the device.");

    /* Stop the NWP. This will activate the bundle. Otherwise, we'll get a commit error later. */
    sl_Stop(200U);

    /* Short delay for debug log output before reset. */
    vTaskDelay(pdMS_TO_TICKS(500UL));
    MAP_PRCMHibernateCycleTrigger();

    /* We shouldn't actually get here if the board supports the auto reset.
     * But, it doesn't hurt anything if we do although someone will need to
     * reset the device for the new image to boot. */
    return kHAPError_Unknown;
}

HAPError HAPPlatformOTASetImageState(const HAPPlatformOTAContext *_Nullable otaContext HAP_UNUSED,
                                     HAPPlatformOTAImageState state)
{
    if (state == kHAPPlatformOTAImageState_Accepted) {
        PRCMPeripheralReset((uint32_t)PRCM_WDT);
        SlFsControl_t fsControl;
        fsControl.IncludeFilters = 0U;
        int32_t rc = sl_FsCtl(SL_FS_CTL_BUNDLE_COMMIT, (uint32_t)0, NULL, (uint8_t *)&fsControl, (uint16_t)sizeof(SlFsControl_t), NULL, (uint16_t)0, NULL);
        if (rc != 0) {
            HAPLogError(&logObject, "Accepted final image but commit failed (%d).", (int)rc);
            return kHAPError_Unknown;
        }
        else {
            HAPLogInfo(&logObject, "Accepted and committed final image.");
            return kHAPError_None;
        }
    }
    else if (state == kHAPPlatformOTAImageState_Rejected) {
        SlFsControl_t fsControl;
        fsControl.IncludeFilters = 0U;
        int32_t rc = sl_FsCtl(SL_FS_CTL_BUNDLE_ROLLBACK, (uint32_t)0, NULL, (uint8_t *)&fsControl, (uint16_t)sizeof(SlFsControl_t), NULL, (uint16_t)0, NULL);
        if (rc != 0) {
            HAPLogError(&logObject, "Bundle rollback failed after reject (%d).", (int)rc);
            return kHAPError_Unknown;
        }
        else {
            HAPLogInfo(&logObject, "Image was rejected and bundle files rolled back.");
            return kHAPError_None;
        }
    }
    else if (state == kHAPPlatformOTAImageState_Aborted) {
        SlFsControl_t fsControl;
        fsControl.IncludeFilters = 0U;
        int32_t rc = sl_FsCtl(SL_FS_CTL_BUNDLE_ROLLBACK, (uint32_t)0, NULL, (uint8_t *)&fsControl, (uint16_t)sizeof(SlFsControl_t), NULL, (uint16_t)0, NULL);
        if (rc != 0) {
            HAPLogError(&logObject, "Bundle rollback failed after abort (%d).", (int)rc);
            return kHAPError_Unknown;
        }
        else {
            HAPLogInfo(&logObject, "Agent aborted and bundle files rolled back.");
            return kHAPError_None;
        }
    }
    else if (state == kHAPPlatformOTAImageState_Testing) {
        return kHAPError_None;
    }
    else {
        HAPLogError(&logObject, "Unknown state received (%d).", (int)state);
        return kHAPError_Unknown;
    }
}

HAPPlatformOTAPALImageState HAPPlatformOTAGetImageState(const HAPPlatformOTAContext *_Nullable otaContext HAP_UNUSED)
{
    const uint32_t checkFlags = SL_FS_INFO_SYS_FILE | SL_FS_INFO_SECURE |
                                SL_FS_INFO_NOSIGNATURE | SL_FS_INFO_PUBLIC_WRITE |
                                SL_FS_INFO_PUBLIC_READ | SL_FS_INFO_NOT_VALID;

    const uint32_t checkFlagsGood = SL_FS_INFO_SYS_FILE | SL_FS_INFO_SECURE | SL_FS_INFO_PUBLIC_WRITE;

    SlFsFileInfo_t fileInfo;
    int32_t rc = sl_FsGetInfo((const uint8_t *)"/sys/mcuflashimg.bin", kOTA_VendorToken, &fileInfo);
    if (rc == 0) {
        HAPLogDebug(&logObject, "Current platform image flags: %04x.", fileInfo.Flags);
        if ((fileInfo.Flags & (uint16_t)SL_FS_INFO_PENDING_BUNDLE_COMMIT) != 0U) {
            HAPLogDebug(&logObject, "Current platform image state: kHAPPlatformOTAPALImageState_PendingCommit.");
            return kHAPPlatformOTAPALImageState_PendingCommit;
        }
        else if ((fileInfo.Flags & checkFlags) == checkFlagsGood) {
            HAPLogDebug(&logObject, "Current platform image state: kHAPPlatformOTAPALImageState_Valid.");
            return kHAPPlatformOTAPALImageState_Valid;
        }
        else {
            HAPLogDebug(&logObject, "Current platform image state: kHAPPlatformOTAPALImageState_Invalid.");
            return kHAPPlatformOTAPALImageState_Invalid;
        }
    }
    else if (rc == SL_ERROR_FS_FILE_NOT_EXISTS) {
        HAPLogDebug(&logObject, "Current platform image state: kHAPPlatformOTAPALImageState_Unknown.");
        return kHAPPlatformOTAPALImageState_Unknown;
    }
    else if (rc == SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY) {
        HAPLogDebug(&logObject, "Current platform image state: kHAPPlatformOTAPALImageState_Invalid.");
        return kHAPPlatformOTAPALImageState_Invalid;
    }
    else {
        HAPLogError(&logObject, "sl_FsGetInfo failed (%d) on /sys/mcuflashimg.bin.", (int)rc);
        return kHAPPlatformOTAPALImageState_Invalid;
    }
}