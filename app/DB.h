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

#include "HAP.h"

#if __has_feature(nullability)
#pragma clang assume_nonnull begin
#endif

/**
 * Total number of services and characteristics contained in the accessory.
 */
#define kAttributeCount ((size_t) 26)

/**
 * HomeKit Accessory Information service.
 */
extern const HAPService accessoryInformationService;

/**
 * Characteristics to expose accessory information and configuration (associated with Accessory Information service).
 */
extern const HAPBoolCharacteristic accessoryInformationIdentifyCharacteristic;
extern const HAPStringCharacteristic accessoryInformationManufacturerCharacteristic;
extern const HAPStringCharacteristic accessoryInformationModelCharacteristic;
extern const HAPStringCharacteristic accessoryInformationNameCharacteristic;
extern const HAPStringCharacteristic accessoryInformationSerialNumberCharacteristic;
extern const HAPStringCharacteristic accessoryInformationFirmwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationHardwareRevisionCharacteristic;
extern const HAPStringCharacteristic accessoryInformationADKVersionCharacteristic;

/**
 * HAP Protocol Information service.
 */
extern const HAPService hapProtocolInformationService;

/**
 * Pairing service.
 */
extern const HAPService pairingService;

/**
 * Fan service.
 */
extern const HAPService fanService;
extern const HAPUInt8Characteristic fanActiveCharacteristic;
extern const HAPFloatCharacteristic fanRotationSpeedCharacteristic;

/**
 * Lightbulb Service.
 */
extern const HAPService lightBulbService;
extern const HAPBoolCharacteristic lightBulbOnCharacteristic;
extern const HAPIntCharacteristic lightBulbBrightnessCharacteristic;

#if __has_feature(nullability)
#pragma clang assume_nonnull end
#endif

#ifdef __cplusplus
}
#endif
