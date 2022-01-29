// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2021 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <mbedtls/config.h>
#include <mbedtls/platform.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include "HAPPlatform.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "RandomNumber" };

static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;

void HAPPlatformRandomNumberFill(void* bytes, size_t numBytes) {
    HAPPrecondition(bytes);

    static bool isInitialized = false;
    if (!isInitialized) {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        int e = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
        if (e != 0) {
            HAPLogError(&logObject, "mbedtls_ctr_drbg_seed failed: %d.", e);
            HAPFatalError();
        }
        isInitialized = true;
    }

    int e = mbedtls_ctr_drbg_random(&ctr_drbg, (unsigned char *)bytes, numBytes);
    if (e != 0) {
        HAPLogError(&logObject, "mbedtls_ctr_drbg_random failed: %d.", e);
        HAPFatalError();
    }

    // Verify random data.
    if (HAPRawBufferIsZero(bytes, numBytes)) {
        HAPLogError(&logObject, "mbedtls_ctr_drbg_random produced only zeros.");
        HAPFatalError();
    }
}
