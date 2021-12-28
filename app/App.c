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
#include "FanControl.h"
#include "UART.h"

/**
 * Global accessory configuration.
 */
typedef struct {
    struct {
        HAPCharacteristicValue_Active active;
        float fanRotationSpeed;
        HAPCharacteristicValue_RotationDirection fanRotationDirection;
        bool lightBulbOn;
        int32_t lightBulbBrightness;
    } state;
    HAPAccessoryServerRef *server;
    HAPPlatformKeyValueStoreRef keyValueStore;
} AccessoryConfiguration;

static AccessoryConfiguration accessoryConfiguration;

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
                                                                            &lightBulbService,
                                                                            NULL },
                                  .callbacks = { .identify = IdentifyAccessory }
};

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
        accessoryConfiguration.state.fanRotationSpeed = 1;
        accessoryConfiguration.state.fanRotationDirection = kHAPCharacteristicValue_RotationDirection_Clockwise;
        accessoryConfiguration.state.lightBulbOn = false;
        accessoryConfiguration.state.lightBulbBrightness = 0;
    }
}

/**
 * Save the accessory state to persistent memory.
 */
static void SaveAccessoryState(void)
{
    HAPPrecondition(accessoryConfiguration.keyValueStore);
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    
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

static void ToggleFanActive(void)
{
    switch (accessoryConfiguration.state.active) {
    case kHAPCharacteristicValue_Active_Inactive:
        accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Active;
        SendFanControlCommand(0x0000);
        break;
    case kHAPCharacteristicValue_Active_Active:
        accessoryConfiguration.state.active = kHAPCharacteristicValue_Active_Inactive;
        SendFanControlCommand(0xFFFF);
        break;
    default:
        break;
    }

    SaveAccessoryState();
    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, &fanActiveCharacteristic, &fanService, &accessory);
}

static void IncreaseFanRotationSpeed(void)
{

}

static void DecreaseFanRotationSpeed(void)
{

}

static void ToggleLightBulbState(void)
{
    accessoryConfiguration.state.lightBulbOn = !accessoryConfiguration.state.lightBulbOn;
    if (accessoryConfiguration.state.lightBulbOn) {
        SendLightControlCommand(0xFFFF);
    }
    else {
        SendLightControlCommand(0x0000);
    }

    SaveAccessoryState();
    HAPAccessoryServerRaiseEvent(accessoryConfiguration.server, &fanActiveCharacteristic, &fanService, &accessory);
}

static void IncreaseLightBulbBrightness(void)
{

}

static void DecreaseLightBulbBrightness(void)
{

}

/**
 * Signal handler. Invoked from the run loop.
 */
static void HandleRemoteControlEventCallback(void *_Nullable context, size_t contextSize)
{
    HAPPrecondition(context);
    HAPAssert(contextSize == sizeof(uint16_t));
    
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    
    uint16_t event = *((uint16_t *) context);
    switch (event) {
    case kRemoteControlEvent_FanOnOff:
        ToggleFanActive();
        break;
    case kRemoteControlEvent_LightOnOff:
        ToggleLightBulbState();
        break;
    case kRemoteControlEvent_FanPlus:
        IncreaseFanRotationSpeed();
        break;
    case kRemoteControlEvent_FanMinus:
        DecreaseFanRotationSpeed();
        break;
    case kRemoteControlEvent_LightPlus:
        IncreaseLightBulbBrightness();
        break;
    case kRemoteControlEvent_LightMinus:
        DecreaseLightBulbBrightness();
        break;
    default:
        break;
    }
}

void HandleRemoteControlEvent(uint16_t event)
{
    HAPError err = HAPPlatformRunLoopScheduleCallback(HandleRemoteControlEventCallback, &event, sizeof event);
    if (err) {
        HAPLogError(&kHAPLog_Default, "HAPPlatformRunLoopScheduleCallback failed.");
        HAPFatalError();
    }
}

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
        ToggleFanActive();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
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
        // Set fan rotation speed
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleFanRotationDirectionRead(HAPAccessoryServerRef* server HAP_UNUSED,
                                        const HAPIntCharacteristicReadRequest* request HAP_UNUSED,
                                        int32_t* value,
                                        void* _Nullable context HAP_UNUSED)
{
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
        // Set fan rotation direction
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnRead(HAPAccessoryServerRef *server HAP_UNUSED,
                               const HAPBoolCharacteristicReadRequest *request HAP_UNUSED,
                               bool *value,
                               void *_Nullable context HAP_UNUSED)
{
    *value = accessoryConfiguration.state.lightBulbOn;
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, *value ? "true" : "false");
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnWrite(HAPAccessoryServerRef *server,
                                const HAPBoolCharacteristicWriteRequest *request,
                                bool value,
                                void *_Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s: %s", __func__, value ? "true" : "false");
    if (accessoryConfiguration.state.lightBulbOn != value) {
        accessoryConfiguration.state.lightBulbOn = value;
        ToggleLightBulbState();
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
    }
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbBrightnessRead(HAPAccessoryServerRef *server,
                                        const HAPIntCharacteristicReadRequest *request,
                                        int32_t* value,
                                        void* _Nullable context HAP_UNUSED)
{
    *value = accessoryConfiguration.state.lightBulbBrightness;
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int)(*value));
    return kHAPError_None;
}

HAP_RESULT_USE_CHECK
HAPError HandleLightBulbBrightnessWrite(HAPAccessoryServerRef *server,
                                         const HAPIntCharacteristicWriteRequest *request,
                                         int32_t value,
                                         void* _Nullable context HAP_UNUSED)
{
    HAPLogInfo(&kHAPLog_Default, "%s: %d", __func__, (int)(value));
    if (accessoryConfiguration.state.lightBulbBrightness != value) {
        accessoryConfiguration.state.lightBulbBrightness = value;
        // Set brightness.
        SaveAccessoryState();
        HAPAccessoryServerRaiseEvent(server, request->characteristic, request->service, request->accessory);
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
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Not Implemented
}

void AppAccessoryServerStart(void)
{
    HAPAccessoryServerStart(accessoryConfiguration.server, &accessory);
}

void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef *server, void *_Nullable context)
{
    HAPPrecondition(server);
    HAPPrecondition(!context);

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
    // Not Implemented
}

void AppDeinitialize()
{
    HAPLogInfo(&kHAPLog_Default, "%s", __func__);
    // Not Implemented
}
