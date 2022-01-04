//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <HAP.h>

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Handle a remote control event from the accessory.
 */
void HandleRemoteControlEvent(uint16_t event);

/**
 * Identify routine. Used to locate the accessory.
 */
HAP_RESULT_USE_CHECK
HAPError IdentifyAccessory(HAPAccessoryServerRef *server,
                           const HAPAccessoryIdentifyRequest *request,
                           void* _Nullable context);

/**
 * Handle read request to the 'TargetState' characteristic of the Fan service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleFanActiveRead(HAPAccessoryServerRef* server,
                             const HAPUInt8CharacteristicReadRequest* request,
                             uint8_t* value,
                             void* _Nullable context);

/**
 * Handle write request to the 'Active' characteristic of the Fan service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleFanActiveWrite(HAPAccessoryServerRef* server,
                              const HAPUInt8CharacteristicWriteRequest* request,
                              uint8_t value,
                              void* _Nullable context);

/**
 * Handle read request to the 'Rotation Speed' characteristic of the Fan service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleFanRotationSpeedRead(HAPAccessoryServerRef *server,
                                    const HAPFloatCharacteristicReadRequest *request,
                                    float* value,
                                    void* _Nullable context);

/**
 * Handle write request to the 'Rotation Speed' characteristic of the Fan service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleFanRotationSpeedWrite(HAPAccessoryServerRef *server,
                                     const HAPFloatCharacteristicWriteRequest *request,
                                     float value,
                                     void* _Nullable context);

/**
 * Handle read request to the 'On' characteristic of the LightBulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnRead(HAPAccessoryServerRef *server,
                               const HAPBoolCharacteristicReadRequest *request,
                               bool *value,
                               void *_Nullable context);

/**
 * Handle write request to the 'On' characteristic of the LightBulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightBulbOnWrite(HAPAccessoryServerRef *server,
                                const HAPBoolCharacteristicWriteRequest *request,
                                bool value,
                                void *_Nullable context);

/**
 * Handle read request to the 'Brightness' characteristic of the LightBulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightBulbBrightnessRead(HAPAccessoryServerRef *server,
                                       const HAPIntCharacteristicReadRequest *request,
                                       int32_t* value,
                                       void* _Nullable context);

/**
 * Handle write request to the 'Brightness' characteristic of the LightBulb service.
 */
HAP_RESULT_USE_CHECK
HAPError HandleLightBulbBrightnessWrite(HAPAccessoryServerRef *server,
                                        const HAPIntCharacteristicWriteRequest *request,
                                        int32_t value,
                                        void* _Nullable context);

/**
 * Initialize the application.
 */
void AppInitialize(HAPAccessoryServerOptions *serverOptions,
                   HAPPlatform *hapPlatform,
                   HAPAccessoryServerCallbacks *serverCallbacks);

/**
 * Create the application.
 */
void AppCreate(HAPAccessoryServerRef *server, HAPPlatformKeyValueStoreRef keyValueStore);

/**
 * Release the application.
 */
void AppRelease(void);

/**
 * Deinitialize the application.
 */
void AppDeinitialize();

/**
 * Start the accessory server for the app.
 */
void AppAccessoryServerStart(void);

/**
 * Handle the updated state of the Accessory Server.
 */
void AccessoryServerHandleUpdatedState(HAPAccessoryServerRef *server, void * _Nullable context);

void AccessoryServerHandleSessionAccept(HAPAccessoryServerRef *server, HAPSessionRef *session, void * _Nullable context);

void AccessoryServerHandleSessionInvalidate(HAPAccessoryServerRef *server, HAPSessionRef *session, void * _Nullable context);

/**
 * Returns pointer to accessory information.
 */
const HAPAccessory* AppGetAccessoryInfo();

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif
