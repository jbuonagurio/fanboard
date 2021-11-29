// Copyright (c) 2015-2019 The HomeKit ADK Contributors
// 
// Copyright (c) 2021 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <limits.h>

#include <ti/drivers/net/wifi/simplelink.h>

#include "HAPPlatformServiceDiscovery+Init.h"
#include "HAPPlatformServiceDiscovery+Test.h"
#include "HAP+Internal.h"

// TI Network Processor User's Guide (SWRU455L), section 10.7.
#define kHAPPlatformServiceDiscovery_MaxTXTRecordBufferBytes (255)

#define kHAPPlatformServiceDiscovery_TTL (120) // 4500?

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "ServiceDiscovery" };

void HAPPlatformServiceDiscoveryCreate(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);

    HAPRawBufferZero(serviceDiscovery, sizeof *serviceDiscovery);

    int e = sl_NetAppStart(SL_NETAPP_MDNS_ID);
    if (e < 0 && e != SL_ERROR_NET_APP_MDNS_ALREADY_STARTED) {
        HAPLogError(&logObject, "Failed to start the mDNS service: %d.", e);
        HAPFatalError();
    }
}

void HAPPlatformServiceDiscoveryRegister(HAPPlatformServiceDiscoveryRef serviceDiscovery,
        const char* name,
        const char* protocol,
        HAPNetworkPort port,
        HAPPlatformServiceDiscoveryTXTRecord* txtRecords,
        size_t numTXTRecords) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(!HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
    HAPPrecondition(name);
    HAPPrecondition(protocol);
    HAPPrecondition(port);
    HAPPrecondition(txtRecords);

    HAPLog(&logObject, "%s - %s.%s @ %u.", __func__, name, protocol, port);

    // Copy name.
    HAPPrecondition(HAPStringGetNumBytes(name) < sizeof serviceDiscovery->name);
    HAPRawBufferCopyBytes(serviceDiscovery->name, name, HAPStringGetNumBytes(name) + 1);

    // Copy protocol.
    HAPPrecondition(HAPStringGetNumBytes(protocol) < sizeof serviceDiscovery->protocol);
    HAPRawBufferCopyBytes(serviceDiscovery->protocol, protocol, HAPStringGetNumBytes(protocol) + 1);

    // Copy port.
    serviceDiscovery->port = port;

    // Copy TXT records.
    HAPPrecondition(numTXTRecords < HAPArrayCount(serviceDiscovery->txtRecords));
    for (size_t i = 0; i < numTXTRecords; i++) {
        HAPLogBuffer(&logObject, txtRecords[i].value.bytes, txtRecords[i].value.numBytes, "%s", txtRecords[i].key);

        // Copy key.
        HAPPrecondition(HAPStringGetNumBytes(txtRecords[i].key) < sizeof serviceDiscovery->txtRecords[i].key);
        HAPRawBufferCopyBytes(
                serviceDiscovery->txtRecords[i].key, txtRecords[i].key, HAPStringGetNumBytes(txtRecords[i].key) + 1);

        // Copy value.
        HAPPrecondition(txtRecords[i].value.numBytes < sizeof serviceDiscovery->txtRecords[i].value.bytes);
        HAPRawBufferCopyBytes(
                serviceDiscovery->txtRecords[i].value.bytes,
                HAPNonnullVoid(txtRecords[i].value.bytes),
                txtRecords[i].value.numBytes);
        serviceDiscovery->txtRecords[i].value.bytes[txtRecords[i].value.numBytes] = '\0';
        HAPAssert(txtRecords[i].value.numBytes <= UINT8_MAX);
        serviceDiscovery->txtRecords[i].value.numBytes = (uint8_t) txtRecords[i].value.numBytes;
    }

    char serviceName[102]; // 65 + 31 + 5 + 1
    HAPError err = HAPStringWithFormat(serviceName, sizeof serviceName, "%s.%s.local", name, protocol);
    HAPAssert(!err);

    char txtRecordBytes[kHAPPlatformServiceDiscovery_MaxTXTRecordBufferBytes];
    HAPStringBuilderRef stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, txtRecordBytes, sizeof txtRecordBytes);
    for (size_t i = 0; i < numTXTRecords; i++) {
        if (i == 0) {
            HAPStringBuilderAppend(&stringBuilder, "%s=%s",
                                   serviceDiscovery->txtRecords[i].key,
                                   serviceDiscovery->txtRecords[i].value.bytes);
        } else {
            HAPStringBuilderAppend(&stringBuilder, ";%s=%s",
                                   serviceDiscovery->txtRecords[i].key,
                                   serviceDiscovery->txtRecords[i].value.bytes);
        }
    }

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogError(&logObject, "TXT record truncated.");
    }

    const char *txtRecordString = HAPStringBuilderGetString(&stringBuilder);

    uint32_t options = SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT |
                       SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT;
    
    // Register the mDNS service.
    int e = sl_NetAppMDNSRegisterService((signed char *)serviceName,
                                         (unsigned char)strlen(serviceName),
                                         (signed char *)txtRecordString,
                                         (unsigned char)strlen(txtRecordString),
                                         (unsigned short)port,
                                         kHAPPlatformServiceDiscovery_TTL,
                                         options);
    if (e < 0) {
        HAPLogError(&logObject, "Failed to register the mDNS service: %d.", e);
        HAPFatalError();
    }
    
    // Set the timing parameters.
    SlNetAppServiceAdvertiseTimingParameters_t timing = {
        .t = 200,                  // Number of ticks for the initial period
        .p = 2,                    // Number of repetitions
        .k = 2,                    // Increasing interval factor
        .RetransInterval = 0,      // Number of ticks to wait before retransmission
        .Maxinterval = ULONG_MAX,  // Number of ticks between two announcement periods
        .max_time = 5              // Maximum announcement period (seconds)
    };

    e = sl_NetAppSet(SL_NETAPP_MDNS_ID, SL_NETAPP_MDNS_TIMING_PARAMS_OPT, sizeof(timing), (unsigned char *)&timing);
    if (e < 0) {
        HAPLogError(&logObject, "Failed to set mDNS timing parameters: %d.", e);
    }

    HAPAssert(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
}

void HAPPlatformServiceDiscoveryUpdateTXTRecords(
        HAPPlatformServiceDiscoveryRef serviceDiscovery,
        HAPPlatformServiceDiscoveryTXTRecord* txtRecords,
        size_t numTXTRecords) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
    HAPPrecondition(txtRecords);

    HAPLog(&logObject, "%s.", __func__);

    // Reset TXT records.
    HAPRawBufferZero(serviceDiscovery->txtRecords, sizeof serviceDiscovery->txtRecords);

    // Copy TXT records.
    HAPPrecondition(numTXTRecords < HAPArrayCount(serviceDiscovery->txtRecords));
    for (size_t i = 0; i < numTXTRecords; i++) {
        HAPLogBuffer(&logObject, txtRecords[i].value.bytes, txtRecords[i].value.numBytes, "%s", txtRecords[i].key);

        // Copy key.
        HAPPrecondition(HAPStringGetNumBytes(txtRecords[i].key) < sizeof serviceDiscovery->txtRecords[i].key);
        HAPRawBufferCopyBytes(
                serviceDiscovery->txtRecords[i].key, txtRecords[i].key, HAPStringGetNumBytes(txtRecords[i].key) + 1);

        // Copy value.
        HAPPrecondition(txtRecords[i].value.numBytes <= sizeof serviceDiscovery->txtRecords[i].value.bytes);
        HAPRawBufferCopyBytes(
                serviceDiscovery->txtRecords[i].value.bytes,
                HAPNonnullVoid(txtRecords[i].value.bytes),
                txtRecords[i].value.numBytes);
        HAPAssert(txtRecords[i].value.numBytes <= UINT8_MAX);
        serviceDiscovery->txtRecords[i].value.numBytes = (uint8_t) txtRecords[i].value.numBytes;
    }

    char serviceName[102]; // 65 + 31 + 5 + 1
    HAPError err = HAPStringWithFormat(serviceName, sizeof serviceName, "%s.%s.local", serviceDiscovery->name, serviceDiscovery->protocol);
    HAPAssert(!err);

    char txtRecordBytes[kHAPPlatformServiceDiscovery_MaxTXTRecordBufferBytes];
    HAPStringBuilderRef stringBuilder;
    HAPStringBuilderCreate(&stringBuilder, txtRecordBytes, sizeof txtRecordBytes);
    for (size_t i = 0; i < numTXTRecords; i++) {
        if (i == 0) {
            HAPStringBuilderAppend(&stringBuilder, "%s=%s",
                                   serviceDiscovery->txtRecords[i].key,
                                   serviceDiscovery->txtRecords[i].value.bytes);
        } else {
            HAPStringBuilderAppend(&stringBuilder, ";%s=%s",
                                   serviceDiscovery->txtRecords[i].key,
                                   serviceDiscovery->txtRecords[i].value.bytes);
        }
    }

    if (HAPStringBuilderDidOverflow(&stringBuilder)) {
        HAPLogError(&logObject, "TXT record truncated.");
    }

    const char *txtRecordString = HAPStringBuilderGetString(&stringBuilder);
    
    // Update text fields without re-registering the service.
    uint32_t options = SL_NETAPP_MDNS_OPTIONS_IS_UNIQUE_BIT |
                       SL_NETAPP_MDNS_OPTIONS_IS_NOT_PERSISTENT |
                       SL_NETAPP_MDNS_OPTION_UPDATE_TEXT;

    int e = sl_NetAppMDNSRegisterService((signed char *)serviceDiscovery->name,
                                         (unsigned char)strlen(serviceDiscovery->name),
                                         (signed char *)txtRecordString,
                                         (unsigned char)strlen(txtRecordString),
                                         (unsigned short)serviceDiscovery->port,
                                         kHAPPlatformServiceDiscovery_TTL,
                                         options);
    if (e < 0) {
        HAPLogError(&logObject, "Failed to update mDNS service TXT records: %d.", e);
        HAPFatalError();
    }

    HAPAssert(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
}

void HAPPlatformServiceDiscoveryStop(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));

    HAPLog(&logObject, "%s.", __func__);

    // Reset service discovery.
    HAPRawBufferZero(serviceDiscovery, sizeof *serviceDiscovery);

    // Stop the mDNS service.
    int e = sl_NetAppStop(SL_NETAPP_MDNS_ID);
    if (e < 0) {
        HAPLogError(&logObject, "Failed to stop the mDNS service: %d.", e);
    }

    HAPAssert(!HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
}

HAP_RESULT_USE_CHECK
bool HAPPlatformServiceDiscoveryIsAdvertising(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);

    return serviceDiscovery->port != 0;
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformServiceDiscoveryGetName(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));

    return serviceDiscovery->name;
}

HAP_RESULT_USE_CHECK
const char* HAPPlatformServiceDiscoveryGetProtocol(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));

    return serviceDiscovery->protocol;
}

HAP_RESULT_USE_CHECK
HAPNetworkPort HAPPlatformServiceDiscoveryGetPort(HAPPlatformServiceDiscoveryRef serviceDiscovery) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));

    return serviceDiscovery->port;
}

void HAPPlatformServiceDiscoveryEnumerateTXTRecords(
        HAPPlatformServiceDiscoveryRef serviceDiscovery,
        HAPPlatformServiceDiscoveryEnumerateTXTRecordsCallback callback,
        void* _Nullable context) {
    HAPPrecondition(serviceDiscovery);
    HAPPrecondition(HAPPlatformServiceDiscoveryIsAdvertising(serviceDiscovery));
    HAPPrecondition(callback);

    bool shouldContinue = true;
    for (size_t i = 0; shouldContinue && i < HAPArrayCount(serviceDiscovery->txtRecords); i++) {
        if (!HAPStringGetNumBytes(serviceDiscovery->txtRecords[i].key)) {
            break;
        }

        callback(context,
                 serviceDiscovery,
                 serviceDiscovery->txtRecords[i].key,
                 serviceDiscovery->txtRecords[i].value.bytes,
                 serviceDiscovery->txtRecords[i].value.numBytes,
                 &shouldContinue);
    }
}
