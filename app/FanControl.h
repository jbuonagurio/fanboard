//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// UART Protocol:

// Packet Structure:

// uint8_t som; // Logic low pulse for approx. 35us. Interpreted as 0xF8.
// uint8_t opcode;
// uint16_t payloadSize;
// char payload[];
// uint16_t crc;

typedef struct __attribute__((packed)) {
    uint8_t[4] header;
    uint16_t eventID;
} RemoteControlEventPayload;

typedef struct {
    uint16_t fanSpeed;
} FanControlCommandPayload;

typedef struct __attribute__((packed)) {
    uint8_t padding;
    uint16_t fanSpeed;
} FanControlResponsePayload;

typedef struct {
    uint16_t lightLevel;
} LightControlCommandPayload;

typedef struct __attribute__((packed)) {
    uint8_t padding;
    uint16_t lightLevel;
} LightControlResponsePayload;



// TX to Fan
//
// 0x01 Hard Reset
// 0x33 Generic Command
// 0x34 Generic Command
// 0x50 Fan Control Command
// 0x60 Light Control Command

//#define kAppOpcode_FanControlCommand ((uint8_t) 0x50)
//#define kAppOpcode_LightControlCommand ((uint8_t) 0x60)

// RX from Fan
//
// 0x32 Remote Control Data Received
// 0x52 Fan Control Command (0x50) Response
// 0x62 Light Control Command (0x60) Response

//#define kAppOpcode_RemoteControlEvent ((uint8_t) 0x32)
//#define kAppOpcode_FanControlResponse ((uint8_t) 0x52)
//#define kAppOpcode_LightControlResponse ((uint8_t) 0x62)

// 0x32 Payload
// "0000BAF0" or "0001BAF0" followed by two byte button ID.
//
// ((uint16_t) 0xFC03) // Fan On/Off
// ((uint16_t) 0xBC43) // Light On/Off
// ((uint16_t) 0xAF50) // Fan Plus
// ((uint16_t) 0xBE41) // Fan Minus
// ((uint16_t) 0xBD42) // Light Plus
// ((uint16_t) 0xAE51) // Light Minus

// 0x50/0x52 Payload
//
// ((uint16_t) 0x0000) // 0     Speed 0
// ((uint16_t) 0x0001) // 1     Speed 1
// ((uint16_t) 0x2AAB) // 10923 Speed 2
// ((uint16_t) 0x5556) // 21846 Speed 3
// ((uint16_t) 0x8000) // 32768 Speed 4
// ((uint16_t) 0xAAAA) // 43690 Speed 5
// ((uint16_t) 0xD555) // 54613 Speed 6
// ((uint16_t) 0xFFFF) // 65535 Speed 7

// 0x60/0x62 Payload
//
// ((uint16_t) 0x0000) // 0     Light Level 0
// ((uint16_t) 0x0001) // 1     Light Level 1
// ((uint16_t) 0x0124) // 292   Light Level 2
// ((uint16_t) 0x048E) // 1166  Light Level 3
// ((uint16_t) 0x0A3D) // 2621  Light Level 4
// ((uint16_t) 0x1236) // 4662  Light Level 5
// ((uint16_t) 0x1C74) // 7284  Light Level 6
// ((uint16_t) 0x28F7) // 10487 Light Level 7
// ((uint16_t) 0x37C1) // 14273 Light Level 8
// ((uint16_t) 0x48D2) // 18642 Light Level 9
// ((uint16_t) 0x5C28) // 23592 Light Level 10
// ((uint16_t) 0x71C5) // 29125 Light Level 11
// ((uint16_t) 0x89A9) // 35241 Light Level 12
// ((uint16_t) 0xA3DA) // 41946 Light Level 13
// ((uint16_t) 0xC04A) // 49226 Light Level 14
// ((uint16_t) 0xDF01) // 57089 Light Level 15
// ((uint16_t) 0xFFFF) // 65535 Light Level 16

#ifdef __cplusplus
}
#endif
