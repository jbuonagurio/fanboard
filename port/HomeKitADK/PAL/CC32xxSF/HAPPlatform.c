// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2020 John Buonagurio
//
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include "HAPPlatform.h"

HAP_RESULT_USE_CHECK
uint32_t HAPPlatformGetCompatibilityVersion(void) {
    return HAP_PLATFORM_COMPATIBILITY_VERSION;
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetIdentification(void) {
    return "CC32xxSF";
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetVersion(void) {
    return "Internal";
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformGetBuild(void) {
    HAP_DIAGNOSTIC_PUSH
    HAP_DIAGNOSTIC_IGNORED_CLANG("-Wdate-time")
    const char* build = __DATE__ " " __TIME__;
    HAP_DIAGNOSTIC_POP
    return build;
}
