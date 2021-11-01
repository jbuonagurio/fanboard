//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "Utilities.h"

#include <HAP.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

void PrintDeviceInfo()
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

void PrintStorageInfo()
{
    SlFsControlGetStorageInfoResponse_t storageInfo;
    int32_t rc = sl_FsCtl((SlFsCtl_e)SL_FS_CTL_GET_STORAGE_INFO,
        0, NULL, NULL, 0, (uint8_t *)&storageInfo,
        sizeof(SlFsControlGetStorageInfoResponse_t), NULL);

    if (rc < 0) {
        HAPLog(&kHAPLog_Default, "SL_FS_CTL_GET_STORAGE_INFO failed: %ld.", rc);
        return;
    }

    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.DeviceBlockSize = %u", storageInfo.DeviceUsage.DeviceBlockSize);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfAllocatedBlocks = %u", storageInfo.DeviceUsage.NumOfAllocatedBlocks);
    HAPLogInfo(&kHAPLog_Default, "DeviceUsage.NumOfAvailableBlocksForUserFiles = %u", storageInfo.DeviceUsage.NumOfAvailableBlocksForUserFiles);

    HAPLogInfo(&kHAPLog_Default, "FilesUsage.MaxFsFiles = %u", (unsigned short)storageInfo.FilesUsage.MaxFsFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.ActualNumOfUserFiles = %u", (unsigned short)storageInfo.FilesUsage.ActualNumOfUserFiles);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.NumOfAlerts = %lu", storageInfo.FilesUsage.NumOfAlerts);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.NumOfAlertsThreshold = %lu", storageInfo.FilesUsage.NumOfAlertsThreshold);
    HAPLogInfo(&kHAPLog_Default, "FilesUsage.FATWriteCounter = %u", storageInfo.FilesUsage.FATWriteCounter);
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
}

void PrintFileList()
{
    typedef struct {
        SlFileAttributes_t attribute;
        char filename[SL_FS_MAX_FILE_NAME_LENGTH];
    } FileListEntry;

    static FileListEntry buffer[4];

    int32_t chunkIndex = -1;
    int32_t fileCount = 1;

    do {
        fileCount = sl_FsGetFileList(&chunkIndex, HAPArrayCount(buffer),
            sizeof(FileListEntry), (uint8_t *)buffer, SL_FS_GET_FILE_ATTRIBUTES);
        for (int32_t i = 0; i < fileCount; ++i) {
            HAPLogInfo(&kHAPLog_Default, buffer[i].filename);
        }
    } while (fileCount > 0);
}