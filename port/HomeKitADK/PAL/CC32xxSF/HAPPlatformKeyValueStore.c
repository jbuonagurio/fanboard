// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include "HAPPlatformKeyValueStore+Init.h"

#include <ti/drivers/net/wifi/simplelink.h>

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "KeyValueStore" };

void HAPPlatformKeyValueStoreCreate(
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPPlatformKeyValueStoreOptions* options) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(options);
    HAPPrecondition(options->rootDirectory);

    HAPLogDebug(&logObject, "Storage configuration: keyValueStore = %lu", (unsigned long) sizeof *keyValueStore);

    keyValueStore->rootDirectory = options->rootDirectory;
}

/**
 * Gets the file path under which data for a specified key is stored.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain.
 * @param      key                  Key.
 * @param[out] filePath             File path for the domain / key. NULL-terminated.
 * @param      maxFilePathLength    Maximum length that the filePath buffer may hold.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_OutOfResources If @p cap not large enough.
 */
HAP_RESULT_USE_CHECK
static HAPError GetFilePath(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        char *filePath,
        size_t maxFilePathLength) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);
    HAPPrecondition(filePath);

    HAPAssert(sizeof domain == sizeof(uint8_t));
    HAPAssert(sizeof key == sizeof(uint8_t));
    HAPError err = HAPStringWithFormat(filePath, maxFilePathLength, "%s/%02X.%02X", keyValueStore->rootDirectory, domain, key);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        HAPLogError(
                &logObject,
                "Not enough resources to get path: %s/%02X.%02X",
                keyValueStore->rootDirectory,
                domain,
                key);
        return kHAPError_OutOfResources;
    }

    return kHAPError_None;
}

/**
 * Reads a file.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain.
 * @param      key                  Key.
 * @param[out] bytes                Buffer for the content of the file, if exists.
 * @param      maxBytes             Capacity of the bytes buffer.
 * @param[out] numBytes             Effective length of the bytes buffer, if exists.
 * @param[out] found                Whether the file path exists and could be read.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreGet(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        void* _Nullable bytes,
        size_t maxBytes,
        size_t* _Nullable numBytes,
        bool* found) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);
    HAPPrecondition(!maxBytes || bytes);
    HAPPrecondition((bytes == NULL) == (numBytes == NULL));
    HAPPrecondition(found);

    *found = false;

    // Get file name.
    char filePath[SL_FS_MAX_FILE_NAME_LENGTH];
    HAPError err = GetFilePath(keyValueStore, domain, key, filePath, sizeof filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return kHAPError_Unknown;
    }

    int32_t handle = sl_FsOpen((unsigned char *)filePath, SL_FS_READ, NULL);
    if (handle < 0) {
        if (handle == SL_ERROR_FS_FILE_NOT_EXISTS) {
            // File does not exist.
            return kHAPError_None;
        }
        HAPLogError(&logObject, "sl_FsOpen %s failed: %d.", filePath, (int)handle);
        return kHAPError_Unknown;
    }
    
    *found = true;

    int32_t retval = sl_FsRead(handle, 0, (unsigned char *)bytes, maxBytes);
    if (retval < 0) {
        HAPLogError(&logObject, "sl_FsRead %s failed: %d.", filePath, (int)retval);
        return kHAPError_Unknown;
    }
    
    HAPLogBufferDebug(&logObject, bytes, maxBytes, "Read %02X.%02X", domain, key);

    // Return the size of the output buffer.
    *numBytes = maxBytes;
    sl_FsClose(handle, 0, 0, 0);
    return kHAPError_None;
}

/**
 * Writes a file atomically.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain.
 * @param      key                  Key.
 * @param      bytes                Buffer with the content of the file, if exists. numBytes != 0 implies bytes.
 * @param      numBytes             Effective length of the bytes buffer.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file access failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreSet(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        const void* bytes,
        size_t numBytes) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);
    HAPPrecondition(bytes || numBytes);

    HAPLogBufferDebug(&logObject, bytes, numBytes, "Write %02X.%02X", domain, key);
    
    // Get file name.
    char filePath[SL_FS_MAX_FILE_NAME_LENGTH];
    HAPError err = GetFilePath(keyValueStore, domain, key, filePath, sizeof filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return kHAPError_Unknown;
    }

    // Write the KVS file.
    int32_t handle = sl_FsOpen((unsigned char *)filePath,
        SL_FS_CREATE | SL_FS_CREATE_FAILSAFE | SL_FS_OVERWRITE | SL_FS_CREATE_MAX_SIZE(numBytes), NULL);
    if (handle < 0) {
        HAPLogError(&logObject, "sl_FsOpen %s failed: %d.", filePath, (int)handle);
        return kHAPError_Unknown;
    }

    int32_t retval = sl_FsWrite(handle, 0, (unsigned char *)bytes, numBytes);
    if (retval < 0) {
        HAPLogError(&logObject, "sl_FsWrite %s failed: %d.", filePath, (int)retval);
        return kHAPError_Unknown;
    }

    sl_FsClose(handle, 0, 0, 0);
    return kHAPError_None;
}

/**
 * Removes a file.
 *
 * @param      keyValueStore        Key-value store.
 * @param      domain               Domain.
 * @param      key                  Key.
 *
 * @return kHAPError_None           If successful.
 * @return kHAPError_Unknown        If the file removal failed.
 */
HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreRemove(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);

    // Get file name.
    char filePath[SL_FS_MAX_FILE_NAME_LENGTH];
    HAPError err = GetFilePath(keyValueStore, domain, key, filePath, sizeof filePath);
    if (err) {
        HAPAssert(err == kHAPError_OutOfResources);
        return kHAPError_Unknown;
    }

    HAPLogDebug(&logObject, "Delete %s", filePath);

    // Remove file.
    int16_t rc = sl_FsDel((unsigned char *)filePath, 0);
    if (rc < 0) {
        if (rc == SL_ERROR_FS_FILE_NOT_EXISTS) {
            // File does not exist.
            return kHAPError_None;
        }
        HAPLogError(&logObject, "sl_FsDel %s failed: %d.", filePath, rc);
        return kHAPError_Unknown;
    }

    return kHAPError_None;
}

static uint8_t ParseHexByte(const char *str) {
    HAPPrecondition(str);

    char hex[3] = { '\0' };
    HAPRawBufferCopyBytes(hex, str, 2);

    char *end;
    unsigned long value = strtoul(hex, &end, 16);
    HAPAssert(end == &hex[2]);
    HAPAssert(value < UINT8_MAX);
    return (uint8_t)value;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStoreEnumerate(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreEnumerateCallback callback,
        void* _Nullable context) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);
    HAPPrecondition(callback);

    typedef struct {
        SlFileAttributes_t attribute;
        char filePath[SL_FS_MAX_FILE_NAME_LENGTH];
    } FsEntry;
    
    FsEntry entries[5];
    long chunkIndex = -1;
    int32_t fileCount = 1;
    bool shouldContinue = true;
    size_t separatorPos = HAPStringGetNumBytes(keyValueStore->rootDirectory);
    HAPAssert(separatorPos + 5 < SL_FS_MAX_FILE_NAME_LENGTH);

    do {
        fileCount = sl_FsGetFileList(&chunkIndex, HAPArrayCount(entries),
            sizeof(FsEntry), (uint8_t *)entries, SL_FS_GET_FILE_ATTRIBUTES);
        
        if (fileCount < 0) {
            HAPLogError(&logObject, "sl_FsGetFileList failed: %d.", (int)fileCount);
            return kHAPError_Unknown;
        }
        
        for (int32_t i = 0; i < fileCount; ++i) {
            char *fileName = &entries[i].filePath[separatorPos + 1];
            
            // Check root directory.
            if (!HAPRawBufferAreEqual(entries[i].filePath, keyValueStore->rootDirectory, separatorPos))
                continue;

            // Check path format ("%s/%02X.%02X").
            if ((entries[i].filePath[separatorPos] != '/') ||
                (fileName[2] != '.') || (fileName[5] != '\0'))
                continue;

            // Check domain.
            if (domain != ParseHexByte(&fileName[0]))
                continue;

            // Extract key and invoke callback.
            HAPPlatformKeyValueStoreKey key = ParseHexByte(&fileName[3]);
            HAPError err = callback(context, keyValueStore, domain, key, &shouldContinue);
            if (err) {
                HAPAssert(err == kHAPError_Unknown);
                return err;
            }

            if (!shouldContinue)
                return kHAPError_None;
        }
    } while (fileCount > 0);
    
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
static HAPError PurgeDomainEnumerateCallback(
        void* _Nullable context HAP_UNUSED,
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain,
        HAPPlatformKeyValueStoreKey key,
        bool* shouldContinue) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);
    HAPPrecondition(shouldContinue);

    HAPError err;

    err = HAPPlatformKeyValueStoreRemove(keyValueStore, domain, key);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HAPPlatformKeyValueStorePurgeDomain(
        HAPPlatformKeyValueStoreRef keyValueStore,
        HAPPlatformKeyValueStoreDomain domain) {
    HAPPrecondition(keyValueStore);
    HAPPrecondition(keyValueStore->rootDirectory);

    HAPError err;

    err = HAPPlatformKeyValueStoreEnumerate(keyValueStore, domain, PurgeDomainEnumerateCallback, NULL);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        return err;
    }

    return kHAPError_None;
}
