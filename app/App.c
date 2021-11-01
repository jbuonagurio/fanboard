//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include <HAP.h>

#include "App.h"
#include "AppDomains.h"
#include "Board.h"
#include "DB.h"

#include <ti/drivers/apps/LED.h>

static LED_Handle blueLEDHandle;

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        HAPCharacteristicValue_Active active;
        HAPCharacteristicValue_CurrentFanState currentState;
        HAPCharacteristicValue_TargetFanState targetState;
        float fanRotationSpeed;
        HAPCharacteristicValue_RotationDirection fanRotationDirection;
    } state;
    HAPAccessoryServerRef *server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

/**
 * Load the accessory state from persistent memory.
 */
static void LoadAccessoryState(void)
{
    HAPPrecondition(accessoryConfiguration.keyValueStore);
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPError err;

    // Load persistent state if available.
    bool found;
    size_t numBytes;

    err = HAPPlatformKeyValueStoreGet(
        accessoryConfiguration.keyValueStore,
        kAppKeyValueStoreDomain_Configuration,
        kAppKeyValueStoreKey_Configuration_State,
        &accessoryConfiguration.state,
        sizeof accessoryConfiguration.state,
        &numBytes,
        &found);

    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
    if (!found || numBytes != sizeof accessoryConfiguration.state) {
        if (found) {
            HAPLogError(&kHAPLog_Default, "Unexpected app state found in key-value store. Resetting to default.");
        }
        HAPRawBufferZero(&accessoryConfiguration.state, sizeof accessoryConfiguration.state);
        accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Inactive;
        accessoryConfiguration.state.currentState = kHAPCharacteristicValue_CurrentFanState_Idle;
        accessoryConfiguration.state.targetState = kHAPCharacteristicValue_TargetFanState_Manual;
        accessoryConfiguration.state.fanRotationSpeed = 1;
        accessoryConfiguration.state.fanRotationDirection = kHAPCharacteristicValue_RotationDirection_Clockwise;
    }

    if (accessoryConfiguration.state.active == kHAPCharacteristicValue_Active_Inactive)
        LED_setOff(blueLEDHandle);
    else 
        LED_setOn(blueLEDHandle, (uint8_t)accessoryConfiguration.state.fanRotationSpeed);
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(void)
{
    HAPPrecondition(accessoryConfiguration.keyValueStore);
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);

    if (accessoryConfiguration.state.active == kHAPCharacteristicValue_Active_Inactive)
        LED_setOff(blueLEDHandle);
    else 
        LED_setOn(blueLEDHandle, (uint8_t)accessoryConfiguration.state.fanRotationSpeed);
    
    HAPError err;
    err = HAPPlatformKeyValueStoreSet(
        accessoryConfiguration.keyValueStore,
        kAppKeyValueStoreDomain_Configuration,
        kAppKeyValueStoreKey_Configuration_State,
        &accessoryConfiguration.state,
        sizeof accessoryConfiguration.state);
    if (err) {
        HAPAssert(err == kHAPError_Unknown);
        HAPFatalError();
    }
}

/**
 * HomeKit accessory that provides the Fan service.
 *
 * Note: Not constant to enable BCT Manual Name Change.
 */

static HAPAccessory accessory = { .aid = 1,
                                  .category = kHAPAccessoryCategory_Fans,
                                  .name = "Haiku",
                                  .manufacturer = "Big Ass Fans",
                                  .model = "Haiku1,1",
                                  .serialNumber = "099DB48E9E28",
                                  .firmwareVersion = "1",
                                  .hardwareVersion = "1",
                                  .services = (const HAPService *const[]) { &accessoryInformationService,
                                                                            &hapProtocolInformationService,
                                                                            &pairingService,
                                                                            &fanService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessory }
};

HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(HAPAccessoryServerRef *server HAP_UNUSED,
                           const HAPAccessoryIdentifyRequest *request HAP_UNUSED,
                           void *_Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    return kHAPError_None;
}


HAP_RESULT_USE_CHECK
HAPError HandleFanActiveRead(HAPAccessoryServerRef* server HAP_UNUSED,
                             const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
                             uint8_t* value,
                             void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    *value = accessoryConfiguration.state.active;
    switch (*value) {
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active_Inactive");
        } break;
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active_Active");
        } break;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanActiveWrite(HAPAccessoryServerRef* server,
                              const HAPUInt8CharacteristicWriteRequest* request,
                              uint8_t value,
                              void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPCharacteristicValue_Active active = (HAPCharacteristicValue_Active) value;
    switch (active) {
        case kHAPCharacteristicValue_Active_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active_Inactive");
        } break;
        case kHAPCharacteristicValue_Active_Active: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "Active_Active");
        } break;
    }

    if (accessoryConfiguration.state.active != active) {
        accessoryConfiguration.state.active = active;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        SaveAccessoryState();
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleCurrentFanStateRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                   const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
                                   uint8_t* value,
                                   void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    *value = accessoryConfiguration.state.currentState;
    switch (*value) {
        case kHAPCharacteristicValue_CurrentFanState_Inactive: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "CurrentFanState_Inactive");
        } break;
        case kHAPCharacteristicValue_CurrentFanState_Idle: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "CurrentFanState_Idle");
        } break;
        case kHAPCharacteristicValue_CurrentFanState_BlowingAir: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "CurrentFanState_BlowingAir");
        } break;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleTargetFanStateRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                  const HAPUInt8CharacteristicReadRequest* request HAP_UNUSED,
                                  uint8_t* value,
                                  void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    *value = accessoryConfiguration.state.targetState;
    switch (*value) {
        case kHAPCharacteristicValue_TargetFanState_Auto: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "TargetFanState_Auto");
        } break;
        case kHAPCharacteristicValue_TargetFanState_Manual: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "TargetFanState_Manual");
        } break;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleTargetFanStateWrite(HAPAccessoryServerRef* server,
                                   const HAPUInt8CharacteristicWriteRequest* request,
                                   uint8_t value,
                                   void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPCharacteristicValue_TargetFanState targetState = (HAPCharacteristicValue_TargetFanState) value;
    switch (targetState) {
        case kHAPCharacteristicValue_TargetFanState_Auto: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "TargetFanState_Auto");
        } break;
        case kHAPCharacteristicValue_TargetFanState_Manual: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "TargetFanState_Manual");
        } break;
    }

    if (accessoryConfiguration.state.targetState != targetState) {
        accessoryConfiguration.state.targetState = targetState;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        SaveAccessoryState();
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanRotationSpeedRead(HAPAccessoryServerRef *server HAP_UNUSED,
                                    const HAPFloatCharacteristicReadRequest *request HAP_UNUSED,
                                    float *value,
                                    void *_Nullable context HAP_UNUSED)
{
    *value = accessoryConfiguration.state.fanRotationSpeed;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int)(*value));
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanRotationSpeedWrite(HAPAccessoryServerRef *server,
                                     const HAPFloatCharacteristicWriteRequest *request,
                                     float value,
                                     void *_Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int)(value));
    if (accessoryConfiguration.state.fanRotationSpeed != value) {
        accessoryConfiguration.state.fanRotationSpeed = value;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        SaveAccessoryState();
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanRotationDirectionRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                        int32_t* value,
                                        void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    *value = accessoryConfiguration.state.fanRotationDirection;
    switch (*value) {
        case kHAPCharacteristicValue_RotationDirection_Clockwise: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "RotationDirection_Clockwise");
        } break;
        case kHAPCharacteristicValue_RotationDirection_CounterClockwise: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "RotationDirection_CounterClockwise");
        } break;
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanRotationDirectionWrite(HAPAccessoryServerRef* server,
                                         const HAPIntCharacteristicWriteRequest* request,
                                         int32_t value,
                                         void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPCharacteristicValue_RotationDirection fanRotationDirection = (HAPCharacteristicValue_RotationDirection) value;
    switch (fanRotationDirection) {
        case kHAPCharacteristicValue_RotationDirection_Clockwise: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "RotationDirection_Clockwise");
        } break;
        case kHAPCharacteristicValue_RotationDirection_CounterClockwise: {
            HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, "RotationDirection_CounterClockwise");
        } break;
    }

    if (accessoryConfiguration.state.fanRotationDirection != fanRotationDirection) {
        accessoryConfiguration.state.fanRotationDirection = fanRotationDirection;
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
        SaveAccessoryState();
    }
    return kHAPError_None;
}

void AccessoryNotification(const HAPAccessory *accessory,
                           const HAPService *service,
                           const HAPCharacteristic *characteristic,
                           void *ctx HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, characteristic, service, accessory);
}

void AppCreate(HAPAccessoryServerRef *server, HAPPlatformKeyValueStoreRef keyValueStore)
{
    HAPPrecondition(server);
    HAPPrecondition(keyValueStore);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPRawBufferZero(&accessoryConfiguration, sizeof accessoryConfiguration);
    accessoryConfiguration.server = server;
    accessoryConfiguration.keyValueStore = keyValueStore;
    LoadAccessoryState();
}

void AppRelease(void)
{
}

void AppAccessoryServerStart(void)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef *server, void *_Nullable context)
{
    HAPPrecondition(server);
    HAPPrecondition(!context);

    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    switch (HAPAccessoryServerGetState(server)) {
    case kHAPAccessoryServerState_Idle: {
        HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Idle.");
        return;
    }
    case kHAPAccessoryServerState_Running: {
        HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Running.");
        return;
    }
    case kHAPAccessoryServerState_Stopping: {
        HAPLogInfo(&kHAPLog_Default, "Accessory Server State did update: Stopping.");
        return;
    }
    }
    HAPFatalError();
}

const HAPAccessory *AppGetAccessoryInfo()
{
    return &accessory;
}

void AppInitialize(HAPAccessoryServerOptions *serverOptions HAP_UNUSED,
                   HAPPlatform *hapPlatform HAP_UNUSED,
                   HAPAccessoryServerCallbacks *serverCallbacks HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    
    blueLEDHandle = LED_open(BOARD_LED1, NULL);
}

void AppDeinitialize()
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Not Implemented
}
