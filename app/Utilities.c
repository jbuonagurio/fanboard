//  Copyright 2022 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "Utilities.h"

#include <HAP.h>
#include <HAP+Internal.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>

#include <FreeRTOS.h>
#include <task.h>

void PrintDeviceInfo(void)
{
    SlDeviceVersion_t ver = {};
    uint8_t veropt = SL_DEVICE_GENERAL_VERSION;
    uint16_t verlen = sizeof(ver);
    sl_DeviceGet(SL_DEVICE_GENERAL, &veropt, &verlen, (uint8_t*)(&ver));
    
    HAPLog(&kHAPLog_Default, "Chip ID: 0x%04lX", ver.ChipId);
    HAPLog(&kHAPLog_Default, "Host Driver: " SL_DRIVER_VERSION);
    HAPLog(&kHAPLog_Default, "MAC: %d.%d.%d.%d",
        (short)ver.FwVersion[0], (short)ver.FwVersion[1], (short)ver.FwVersion[2], (short)ver.FwVersion[3]);
    HAPLog(&kHAPLog_Default, "PHY: %d.%d.%d.%d",
        (short)ver.PhyVersion[0], (short)ver.PhyVersion[1], (short)ver.PhyVersion[2], (short)ver.PhyVersion[3]);
    HAPLog(&kHAPLog_Default, "NWP: %d.%d.%d.%d",
        (short)ver.NwpVersion[0], (short)ver.NwpVersion[1], (short)ver.NwpVersion[2], (short)ver.NwpVersion[3]);
    HAPLog(&kHAPLog_Default, "ROM: %d", (short)ver.RomVersion);
}

void PrintStorageInfo(void)
{
    SlFsControlGetStorageInfoResponse_t storageInfo;
    int32_t rc = sl_FsCtl((SlFsCtl_e)SL_FS_CTL_GET_STORAGE_INFO,
        0, NULL, NULL, 0, (uint8_t *)&storageInfo,
        sizeof(SlFsControlGetStorageInfoResponse_t), NULL);

    if (rc < 0) {
        HAPLogError(&kHAPLog_Default, "SL_FS_CTL_GET_STORAGE_INFO failed: %ld.", rc);
        return;
    }

    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.DeviceBlockSize = %u", storageInfo.DeviceUsage.DeviceBlockSize);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.DeviceBlocksCapacity = %u", storageInfo.DeviceUsage.DeviceBlocksCapacity);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfAllocatedBlocks = %u", storageInfo.DeviceUsage.NumOfAllocatedBlocks);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfReservedBlocks = %u", storageInfo.DeviceUsage.NumOfReservedBlocks);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfReservedBlocksForSystemFiles = %u", storageInfo.DeviceUsage.NumOfReservedBlocksForSystemfiles);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.LargestAllocatedGapInBlocks = %u", storageInfo.DeviceUsage.LargestAllocatedGapInBlocks);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfAvailableBlocksForUserFiles = %u", storageInfo.DeviceUsage.NumOfAvailableBlocksForUserFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.MaxFsFiles = %u", (unsigned short)storageInfo.FilesUsage.MaxFsFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.IsDevelopmentFormatType = %u", (unsigned short)storageInfo.FilesUsage.IsDevlopmentFormatType);
    switch (storageInfo.FilesUsage.Bundlestate) {
    case SL_FS_BUNDLE_STATE_STOPPED:
        HAPLogInfo(&kHAPLog_Default, "FilesUsage.BundleState = SL_FS_BUNDLE_STATE_STOPPED");
        break;
    case SL_FS_BUNDLE_STATE_STARTED:
        HAPLogInfo(&kHAPLog_Default, "FilesUsage.BundleState = SL_FS_BUNDLE_STATE_STARTED");
        break;
    case SL_FS_BUNDLE_STATE_PENDING_COMMIT:
        HAPLogInfo(&kHAPLog_Default, "FilesUsage.BundleState = SL_FS_BUNDLE_STATE_PENDING_COMMIT");
        break;
    default:
        break;
    }
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.ActualNumOfUserFiles = %u", (unsigned short)storageInfo.FilesUsage.ActualNumOfUserFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.ActualNumOfSysFiles = %u", (unsigned short)storageInfo.FilesUsage.ActualNumOfSysFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.NumOfAlerts = %lu", storageInfo.FilesUsage.NumOfAlerts);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.NumOfAlertsThreshold = %lu", storageInfo.FilesUsage.NumOfAlertsThreshold);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.FATWriteCounter = %u", storageInfo.FilesUsage.FATWriteCounter);
}

void PrintFileList(void)
{
    typedef struct {
        SlFileAttributes_t attribute;
        char filename[SL_FS_MAX_FILE_NAME_LENGTH];
    } FileListEntry;

    static FileListEntry buffer[4];

    int32_t chunkIndex = -1;
    int32_t fileCount = 1;
    char attributeBuffer[128];

    do {
        fileCount = sl_FsGetFileList(&chunkIndex,
                                     HAPArrayCount(buffer),
                                     sizeof(FileListEntry),
                                     (uint8_t *)buffer,
                                     SL_FS_GET_FILE_ATTRIBUTES);
        
        for (int32_t i = 0; i < fileCount; ++i) {
            HAPRawBufferZero(attributeBuffer, sizeof attributeBuffer);
            HAPStringBuilderRef stringBuilder;
            HAPStringBuilderCreate(&stringBuilder, attributeBuffer, sizeof attributeBuffer);
            if (buffer[i].attribute.Properties & SL_FS_INFO_MUST_COMMIT)
                HAPStringBuilderAppend(&stringBuilder, "MUST_COMMIT:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_BUNDLE_FILE)
                HAPStringBuilderAppend(&stringBuilder, "BUNDLE_FILE:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_PENDING_COMMIT)
                HAPStringBuilderAppend(&stringBuilder, "PENDING_COMMIT:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_PENDING_BUNDLE_COMMIT)
                HAPStringBuilderAppend(&stringBuilder, "PENDING_BUNDLE_COMMIT:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_SECURE)
                HAPStringBuilderAppend(&stringBuilder, "SECURE:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_NOT_FAILSAFE)
                HAPStringBuilderAppend(&stringBuilder, "NOT_FAILSAFE:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_SYS_FILE)
                HAPStringBuilderAppend(&stringBuilder, "SYS_FILE:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_NOT_VALID)
                HAPStringBuilderAppend(&stringBuilder, "NOT_VALID:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_PUBLIC_WRITE)
                HAPStringBuilderAppend(&stringBuilder, "PUBLIC_WRITE:");
            if (buffer[i].attribute.Properties & SL_FS_INFO_PUBLIC_READ)
                HAPStringBuilderAppend(&stringBuilder, "PUBLIC_READ:");
            const char *attributeString = HAPStringBuilderGetString(&stringBuilder);
            HAPLogInfo(&kHAPLog_Default, "%s %s", buffer[i].filename, attributeString);
        }
    } while (fileCount > 0);
}

void RemoveInvalidFiles(uint32_t token)
{
    typedef struct {
        SlFileAttributes_t attribute;
        char filename[SL_FS_MAX_FILE_NAME_LENGTH];
    } FileListEntry;

    static FileListEntry buffer[4];
    int32_t chunkIndex = -1;
    int32_t fileCount = 1;
    
    do {
        fileCount = sl_FsGetFileList(&chunkIndex, HAPArrayCount(buffer), sizeof(FileListEntry),
                                     (uint8_t *)buffer, SL_FS_GET_FILE_ATTRIBUTES);
        for (size_t i = 0; i < fileCount; ++i) {
            if (buffer[i].attribute.Properties & SL_FS_INFO_NOT_VALID) {
                int16_t rc = sl_FsDel((uint8_t *)buffer[i].filename, token);
                HAPLogDebug(&kHAPLog_Default, "sl_FsDel %s: %d", buffer[i].filename, rc);
            }
        }
    } while (fileCount > 0);
}

void RestoreFactoryImage(void)
{
    SlFsRetToFactoryCommand_t command = { .Operation = SL_FS_FACTORY_RET_TO_IMAGE };
    int32_t rc = sl_FsCtl((SlFsCtl_e)SL_FS_CTL_RESTORE, 0, NULL, (uint8_t *)&command, sizeof(SlFsRetToFactoryCommand_t), NULL, 0 , NULL);
    if (rc < 0) {
        uint16_t err = (uint16_t)rc & 0xFFFF;
        HAPLogError(&kHAPLog_Default, "SL_FS_FACTORY_RET_TO_IMAGE failed: %ld, %d", rc, err);
        return;
    }
    
    sl_Stop(200U);
    vTaskDelay(pdMS_TO_TICKS(500UL));
    MAP_PRCMHibernateCycleTrigger();
}