//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <FreeRTOS.h>
#include <queue.h>

#ifdef __cplusplus
extern "C" {
#endif

extern QueueHandle_t rxMessageQueue;
extern QueueHandle_t txMessageQueue;

void UARTTask(void *pvParameters);
void EnqueueMessage(uint8_t opcode, uint16_t payloadSize, void *payload);
void SendFanControlCommand(uint16_t value);
void SendLightControlCommand(uint16_t value);

#ifdef __cplusplus
}
#endif
