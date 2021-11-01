// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#ifndef HAP_PLATFORM_KEY_VALUE_STORE_INIT_H
#define HAP_PLATFORM_KEY_VALUE_STORE_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "HAPPlatform.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Key-value store initialization options.
 */
typedef struct {
    /**
     * Root directory into which the values will be stored as files.
     */
    const char* rootDirectory;
} HAPPlatformKeyValueStoreOptions;

/**
 * Key-value store.
 */
struct HAPPlatformKeyValueStore {
    // Opaque type. Do not access the instance fields directly.
    /**@cond */
    const char* rootDirectory;
    /**@endcond */
};

/**
 * Initializes the key-value store.
 *
 * @param[out] keyValueStore        Pointer to an allocated but uninitialized HAPPlatformKeyValueStore structure.
 * @param      options              Initialization options.
 */
void HAPPlatformKeyValueStoreCreate(
        HAPPlatformKeyValueStoreRef keyValueStore,
        const HAPPlatformKeyValueStoreOptions* options);

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif

#endif
