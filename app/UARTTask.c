//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "UARTTask.h"
#include "Board.h"
#include "CRC.h"

#include <HAP.h>

#include <ti/drivers/UART.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

// Maximum number of bytes in UART RX message payload.
#define kApp_MaxPayloadSize ((size_t) 64)

// UART message structure.
typedef struct __attribute__((packed)) {
    uint8_t som;
    uint8_t opcode;
    uint16_t payloadSize;
    uint8_t payload[kApp_MaxPayloadSize];
    uint16_t crc;
} UARTMessage;

// UART message receive errors.
HAP_ENUM_BEGIN(uint8_t, UARTMessageError) {
    kUARTMessageError_None = 0,
    kUARTMessageError_InvalidSOM,
    kUARTMessageError_UnknownOpcode,
    kUARTMessageError_InvalidPayloadSize
} HAP_ENUM_END(uint8_t, UARTMessageError);

// FreeRTOS semaphore.
SemaphoreHandle_t uartSemaphore;

// UART message data.
static UARTMessage rxMessage;
static UARTMessageError rxMessageError;
static size_t rxTotalBytes;

static void UARTReadCallback(UART_Handle handle, void *buffer, size_t count)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UBaseType_t uxSavedInterruptStatus;

    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    rxTotalBytes += count;

    if (rxTotalBytes == 0) {
        // Read SOM.
        UART_read(handle, &rxMessage.som, sizeof(rxMessage.som));
    }
    else if (rxTotalBytes == offsetof(UARTMessage, opcode)) {
        // Check SOM; logic low pulse for approx. 35us, interpreted as 0xf8.
        switch (rxMessage.som) {
        case 0xf8:
            // Valid SOM; read opcode.
            UART_read(handle, &rxMessage.opcode, sizeof(rxMessage.opcode));
            break;
        default:
            // Invalid SOM; unblock the calling thread.
            rxMessageError = kUARTMessageError_InvalidSOM;
            xSemaphoreGiveFromISR(uartSemaphore, &xHigherPriorityTaskWoken);
            break;
        }
    }
    else if (rxTotalBytes == offsetof(UARTMessage, payloadSize)) {
        // Check opcode.
        switch (rxMessage.opcode) {
        case 0x00: // Initialization 1, 0x04 Response; expected payload size 2
        case 0x13: // Initialization 2, 0x12 Response; expected payload size 5
        case 0x31: // Initialization 3, 0x30 Response; expected payload size 2
        case 0x22: // Initialization 4, 0x21 Response; expected payload size 2
        case 0x37: // Initialization 5, 0x36 Response; expected payload size 3
        case 0x54: // Initialization 6, 0x53 Response; expected payload size 16
        case 0x56: // Initialization 7, 0x55 Response; expected payload size 10
        case 0x64: // Initialization 8, 0x63 Response; expected payload size 34
        case 0x59: // Initialization 9, 0x57 Response; expected payload size 2
        case 0x32: // Remote Control Data Received; expected payload size 6
        case 0x52: // Fan Control 0x50 Response; expected payload size 3
        case 0x62: // Light Control 0x60 Response; expected payload size 3
            // Valid opcode; read payload length.
            UART_read(handle, &rxMessage.payloadSize, sizeof(rxMessage.payloadSize));
            break;
        default:
            // Invalid opcode.
            rxMessageError = kUARTMessageError_UnknownOpcode;
            xSemaphoreGiveFromISR(uartSemaphore, &xHigherPriorityTaskWoken);
            break;
        }
    }
    else if (rxTotalBytes == offsetof(UARTMessage, payload)) {
        // Check payload size.
        if (rxMessage.payloadSize <= kApp_MaxPayloadSize) {
            // Valid payload size; read payload.
            UART_read(handle, &rxMessage.payload, rxMessage.payloadSize);
        }        
        else {
            // Invalid payload size.
            rxMessageError = kUARTMessageError_InvalidPayloadSize;
            xSemaphoreGiveFromISR(uartSemaphore, &xHigherPriorityTaskWoken);
        }
    }
    else if (rxTotalBytes == offsetof(UARTMessage, payload) + rxMessage.payloadSize) {
        // Read CRC.
        UART_read(handle, &rxMessage.crc, sizeof(rxMessage.crc));
    }
    else if (rxTotalBytes == offsetof(UARTMessage, payload) + rxMessage.payloadSize + sizeof(rxMessage.crc)) {
        // Complete message received.
        xSemaphoreGiveFromISR(uartSemaphore, &xHigherPriorityTaskWoken);
    }

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

void UARTTask(void *pvParameters)
{
    // Initialize binary semaphore for UART callback.
    uartSemaphore = xSemaphoreCreateBinary();
    if (uartSemaphore == NULL) {
        HAPLogError(&kHAPLog_Default, "Failed to create UART semaphore.");
        HAPFatalError();
    }

    // Initialize UART driver.
    UART_Params uartParams = {
        .readMode = UART_MODE_CALLBACK,
        .writeMode = UART_MODE_BLOCKING,
        .readTimeout = UART_WAIT_FOREVER,
        .writeTimeout = UART_WAIT_FOREVER,
        .readCallback = UARTReadCallback,
        .writeCallback = NULL,
        .readReturnMode = UART_RETURN_FULL,
        .readDataMode = UART_DATA_BINARY,
        .writeDataMode = UART_DATA_BINARY,
        .readEcho = UART_ECHO_ON,
        .baudRate = 115200,
        .dataLength = UART_LEN_8,
        .stopBits = UART_STOP_ONE,
        .parityType = UART_PAR_NONE,
        .custom = NULL
    };

    UART_Handle uartHandle = UART_open(BOARD_UART0, &uartParams);
    if (uartHandle == NULL) {
        HAPLogError(&kHAPLog_Default, "Failed to initialize UART0.");
        HAPFatalError();
    }

    //const char txMessage[] = { 0xf8, 0x04, 0x00, 0x00, 0x10, 0x5c };
    //size_t txTotalBytes = UART_write(uartHandle, txMessage, sizeof(txMessage));
    //HAPLogInfo(&kHAPLog_Default, "UART0 wrote %d bytes.", txTotalBytes);

    HAPLogInfo(&kHAPLog_Default, "Starting UART RX loop.");

    for (;;) {
        // Start reading the next message.
        rxTotalBytes = 0;
        rxMessageError = kUARTMessageError_None;
        UART_read(uartHandle, &rxMessage.som, 1);
        
        // Block until the RX callback posts the semaphore.
        if (xSemaphoreTake(uartSemaphore, portMAX_DELAY) != pdTRUE)
            continue;

        if (rxMessageError == kUARTMessageError_None) {
            // Calculate CRC16.
            void *crcptr = &rxMessage.opcode;
            const uint32_t crclen = sizeof(rxMessage.opcode) + sizeof(rxMessage.payloadSize) + rxMessage.payloadSize;
            uint16_t crc = CalcCRC16(crcptr, crclen);
            if (crc != rxMessage.crc) {
                HAPLogError(&kHAPLog_Default, "Invalid CRC %04X; expected %04X.", rxMessage.crc, crc);
            }
            
            // Log the message buffer (opcode, payload size, payload).
            HAPLogBufferDebug(&kHAPLog_Default, crcptr, crclen, "Message received");
        }
        else {
            // Log any other errors.
            switch (rxMessageError) {
            case kUARTMessageError_InvalidSOM:
                HAPLogError(&kHAPLog_Default, "Invalid SOM: %02X.", rxMessage.som);
                break;
            case kUARTMessageError_UnknownOpcode:
                HAPLogError(&kHAPLog_Default, "Unknown opcode: %02X.", rxMessage.opcode);
                break;
            case kUARTMessageError_InvalidPayloadSize:
                HAPLogError(&kHAPLog_Default, "Invalid payload size: %lu.", (unsigned long)rxMessage.payloadSize);
                break;
            default:
                break;
            }
        }
    }
}