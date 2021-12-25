//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "App.h"
#include "Board.h"
#include "FanControl.h"
#include "UART.h"

#include <HAP.h>

#include <ti/drivers/UART.h>
#include <ti/drivers/uart/UARTCC32XX.h>
#include <ti/devices/cc32xx/inc/hw_dthe.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/driverlib/crc.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

// Block time used used for UART RX and TX.
#define kUART_BlockTime pdMS_TO_TICKS((TickType_t) 10000UL)

// Maximum number of messages in RX amd TX queues.
#define kUART_RXQueueDepth ((size_t) 10)
#define kUART_TXQueueDepth ((size_t) 10)

// Maximum payload size observed is 34 bytes. Allow extra space for unknown
// message types, for a total maximum message size of 64 bytes.
#define kUART_MaxPayloadSize ((size_t) 58)

// Serial message format.
typedef struct __attribute__((packed)) {
    uint8_t som;
    uint8_t opcode;
    uint16_t payloadSize;
} MessageHeader_t;

typedef struct __attribute__((packed)) {
    MessageHeader_t header;
    uint8_t payload[kUART_MaxPayloadSize];
    uint16_t crc;
} Message_t;

HAP_STATIC_ASSERT(sizeof(Message_t) == 64, InvalidMessageSize);

// UART RX message status for task notifications.
HAP_ENUM_BEGIN(uint8_t, MessageStatus) {
    kMessageStatus_OK = 0,
    kMessageStatus_InvalidSOM,
    kMessageStatus_InvalidPayloadSize,
    kMessageStatus_InvalidCRC
} HAP_ENUM_END(uint8_t, MessageStatus);

// UART device handle.
static UART_Handle uartHandle = NULL;

// FreeRTOS task handle.
static TaskHandle_t uartTaskHandle = NULL;

// UART RX message data.
static Message_t rxBuffer;
static volatile size_t rxTotalBytes;

// Queues used to send and receive complete message structures.
QueueHandle_t rxMessageQueue = NULL;
QueueHandle_t txMessageQueue = NULL;

// Initialization state.
static uint16_t initFlags;

static inline bool IsTXOpcodeValid(uint8_t opcode)
{
    switch (opcode) {
    case 0x04: // Initialization 1; expected payload size 0
    case 0x12: // Initialization 2; expected payload size 1
    case 0x21: // Initialization 3; expected payload size 0
    case 0x30: // Initialization 4; expected payload size 1
    case 0x36: // Initialization 5; expected payload size 1
    case 0x53: // Initialization 6; expected payload size 0
    case 0x55: // Initialization 7; expected payload size 0
    case 0x63: // Initialization 8; expected payload size 0
    case 0x57: // Initialization 9; expected payload size 1
    case 0x33: // Unknown Command; expected payload size 2
    case 0x34: // Unknown Command; expected payload size 4
    case 0x50: // Fan Speed Command; expected payload size 2
    case 0x60: // Light Level Command; expected payload size 2
        return true;
    default:
        return false;
    }
}

static inline bool IsRXOpcodeValid(uint8_t opcode)
{
    switch (opcode) {
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
        return true;
    default:
        return false;
    }
}

static void ProcessIncomingMessages()
{
    static Message_t message;

    while (xQueueReceive(rxMessageQueue, (void *)&message, 0) == pdPASS) {
        size_t messageSize = sizeof(message.header) + message.header.payloadSize + sizeof(message.crc);
        HAPLogBufferDebug(&kHAPLog_Default, &message, messageSize, "RX message size %d", messageSize);

        switch (message.header.opcode) {
        case 0x00: // Initialization 1 (0x04) response; send Initialization 2 (0x12)
            initFlags |= 0b000000001;
            UARTSendMessage(0x12, 1, &(uint8_t[1]){ 0x01 });
            break;
        case 0x13: // Initialization 2 (0x12) response; send Initialization 3 (0x30)
            initFlags |= 0b000000010;
            UARTSendMessage(0x30, 0, NULL);
            break;
        case 0x31: // Initialization 3 (0x30) response; send Initialization 4 (0x21)
            initFlags |= 0b000000100;
            UARTSendMessage(0x21, 1, &(uint8_t[1]){ 0x01 });
            break;
        case 0x22: // Initialization 4 (0x21) response; send Initialization 5 (0x36)
            initFlags |= 0b000001000;
            UARTSendMessage(0x36, 1, &(uint8_t[1]){ 0x01 });
            break;
        case 0x37: // Initialization 5 (0x36) response; send Initialization 6 (0x53)
            initFlags |= 0b000010000;
            UARTSendMessage(0x53, 0, NULL);
            break;
        case 0x54: // Initialization 6 (0x53) response; send Initialization 7 (0x55)
            initFlags |= 0b000100000;
            UARTSendMessage(0x55, 0, NULL);
            break;
        case 0x56: // Initialization 7 (0x55) response; send Initialization 8 (0x63)
            initFlags |= 0b001000000;
            UARTSendMessage(0x63, 0, NULL);
            break;
        case 0x64: // Initialization 8 (0x63) response; send Initialization 9 (0x57)
            initFlags |= 0b010000000;
            UARTSendMessage(0x57, 1, &(uint8_t[1]){ 0x00 });
            break;
        case 0x59: // Initialization 9 (0x57) response
            initFlags |= 0b100000000;
            HAPLogInfo(&kHAPLog_Default, "Initialization sequence complete (0x%04X).", initFlags);
            break;
        case 0x32: // Remote control data received
            if (message.header.payloadSize == sizeof(RemoteControlRXPayload)) {
                uint16_t event = ((RemoteControlRXPayload *)(&message.payload))->event;
                HAPLogDebug(&kHAPLog_Default, "Remote control event: 0x%04X.", event);
                switch (event) {
                case kRemoteControlEvent_FanOnOff:
                    HAPLogDebug(&kHAPLog_Default, "kRemoteControlEvent_FanOnOff");
                    SendFanControlCommand(0xFFFF);
                    break;
                case kRemoteControlEvent_LightOnOff:
                    HAPLogDebug(&kHAPLog_Default, "RemoteControlEvent_LightOnOff");
                    SendLightControlCommand(0xFFFF);
                    break;
                case kRemoteControlEvent_FanPlus:
                    HAPLogDebug(&kHAPLog_Default, "RemoteControlEvent_FanPlus");
                    break;
                case kRemoteControlEvent_FanMinus:
                    HAPLogDebug(&kHAPLog_Default, "RemoteControlEvent_FanMinus");
                    break;
                case kRemoteControlEvent_LightPlus:
                    HAPLogDebug(&kHAPLog_Default, "RemoteControlEvent_LightPlus");
                    break;
                case kRemoteControlEvent_LightMinus:
                    HAPLogDebug(&kHAPLog_Default, "RemoteControlEvent_LightMinus");
                    break;
                default:
                    break;
                }
            }
            break;
        case 0x52: // Fan control 0x50 response
            if (message.header.payloadSize == sizeof(FanControlRXPayload)) {
                uint16_t fanSpeed = ((FanControlRXPayload *)(&message.payload))->value;
                HAPLogInfo(&kHAPLog_Default, "Fan speed changed: 0x%04X.", fanSpeed);
                // Update accessory state.
            }
            break;
        case 0x62: // Light control 0x60 response
            if (message.header.payloadSize == sizeof(LightControlRXPayload)) {
                uint16_t lightLevel = ((LightControlRXPayload *)(&message.payload))->value;
                HAPLogInfo(&kHAPLog_Default, "Light level changed: 0x%04X.", lightLevel);
                // Update accessory state.
            }
            break;
        default:
            break;
        }
    }
}

// Remove all data from the UART ring buffer and RX FIFO.
static void FlushBuffers(UART_Handle handle)
{
    UARTCC32XX_Object *object = handle->object;
    UARTCC32XX_HWAttrsV1 const *hwAttrs = handle->hwAttrs;
    RingBuf_flush(&object->ringBuffer);
    while (((int32_t)MAP_UARTCharGetNonBlocking(hwAttrs->baseAddr)) != -1);
    MAP_UARTRxErrorClear(hwAttrs->baseAddr);
}

// Calculate 16-bit CRC-CCITT (polynomial 0x1021, seed 0xFFFF) for serial packets.
static inline uint16_t CRC16(void *data, uint32_t len)
{
    CRCConfigSet(DTHE_BASE, CRC_CFG_INIT_1 | CRC_CFG_TYPE_P1021 | CRC_CFG_SIZE_8BIT);
    uint16_t crc = (uint16_t)CRCDataProcess(DTHE_BASE, data, len, CRC_CFG_SIZE_8BIT);
    return (crc >> 8) | (crc << 8); // Endian swap
}

// Callback function used by UART driver. Callback occurs in interrupt context.
static void ReadCallback(UART_Handle handle, void *buffer, size_t count)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    UBaseType_t uxSavedInterruptStatus;

    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();

    rxTotalBytes += count;

    if (rxTotalBytes == 0) {
        // Read SOM.
        UART_read(handle, &rxBuffer.header.som, sizeof(rxBuffer.header.som));
    }
    else if (rxTotalBytes == sizeof(rxBuffer.header.som)) {
        // Check SOM; logic low pulse for approx. 35us, interpreted as 0xf8.
        if (rxBuffer.header.som == 0xf8) {
            // Valid SOM; read opcode and payload size.
            UART_read(handle, &rxBuffer.header.opcode, sizeof(rxBuffer.header.opcode) + sizeof(rxBuffer.header.payloadSize));
        }
        else {
            xTaskNotifyFromISR(uartTaskHandle, kMessageStatus_InvalidSOM, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        }
    }
    else if (rxTotalBytes == sizeof(rxBuffer.header)) {
        // Check payload size.
        if (rxBuffer.header.payloadSize <= kUART_MaxPayloadSize) {
            // Valid payload size; read payload and CRC.
            UART_read(handle, &rxBuffer.payload, rxBuffer.header.payloadSize + sizeof(rxBuffer.crc));
        }
        else {
            xTaskNotifyFromISR(uartTaskHandle, kMessageStatus_InvalidPayloadSize, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        }
    }
    else if (rxTotalBytes == sizeof(rxBuffer.header) + rxBuffer.header.payloadSize + sizeof(rxBuffer.crc)) {
        // Copy CRC from payload buffer.
        HAPRawBufferCopyBytes(&rxBuffer.crc, (uint8_t *)(&rxBuffer.payload) + rxBuffer.header.payloadSize, sizeof(rxBuffer.crc));

        // Check CRC.
        void *crcptr = &rxBuffer.header.opcode;
        const uint32_t crclen = sizeof(rxBuffer.header.opcode) + sizeof(rxBuffer.header.payloadSize) + rxBuffer.header.payloadSize;
        if (CRC16(crcptr, crclen) != rxBuffer.crc) {
            xTaskNotifyFromISR(uartTaskHandle, kMessageStatus_InvalidCRC, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        }

        // Complete message received; post to queue.
        xQueueSendToBackFromISR(rxMessageQueue, (void *)&rxBuffer, &xHigherPriorityTaskWoken);
        xTaskNotifyFromISR(uartTaskHandle, kMessageStatus_OK, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
    }

    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
}

void UARTTask(void *pvParameters)
{
    uartTaskHandle = xTaskGetCurrentTaskHandle();

    rxMessageQueue = xQueueCreate(kUART_RXQueueDepth, sizeof(Message_t));
    if (rxMessageQueue == NULL) {
        HAPLogError(&kHAPLog_Default, "Failed to create RX message queue.");
        HAPFatalError();
    }

    txMessageQueue = xQueueCreate(kUART_TXQueueDepth, sizeof(Message_t));
    if (txMessageQueue == NULL) {
        HAPLogError(&kHAPLog_Default, "Failed to create TX message queue.");
        HAPFatalError();
    }

    vQueueAddToRegistry(rxMessageQueue, "rxqueue");
    vQueueAddToRegistry(txMessageQueue, "txqueue");

    // SimpleLink™ Wi-Fi® CC323x Technical Reference Manual (SWRU543A), 6.2.3.3:
    // The receive time-out interrupt is asserted when the RX FIFO is not empty,
    // and no further data are received over a 32-bit period when the HSE bit is
    // clear, or over a 64-bit period when the HSE bit is set.
    //
    // HSE is not set (115200 * 16 <= 80000000)
    // UART clock period = 1E6 / 80 MHz = 0.0125 μs
    // Timeout = 0.0125 μs * 32 = 0.4 μs

    UART_Params uartParams = {
        .readMode = UART_MODE_CALLBACK,
        .writeMode = UART_MODE_BLOCKING,
        .readTimeout = UART_WAIT_FOREVER,
        .writeTimeout = UART_WAIT_FOREVER,
        .readCallback = ReadCallback,
        .writeCallback = NULL,
        .readReturnMode = UART_RETURN_FULL,
        .readDataMode = UART_DATA_BINARY,
        .writeDataMode = UART_DATA_BINARY,
        .readEcho = UART_ECHO_OFF,
        .baudRate = 115200,
        .dataLength = UART_LEN_8,
        .stopBits = UART_STOP_ONE,
        .parityType = UART_PAR_NONE,
        .custom = NULL
    };

    uartHandle = UART_open(BOARD_UART0, &uartParams);
    if (uartHandle == NULL) {
        HAPLogError(&kHAPLog_Default, "Failed to initialize UART0.");
        HAPFatalError();
    }

    HAPLogInfo(&kHAPLog_Default, "Starting UART loop.");
    FlushBuffers(uartHandle);

    // Start the initialization sequence.
    UARTSendMessage(0x04, 0, NULL);

    for (;;) {
        // Send the next message in the TX queue, if present.
        Message_t message;
        BaseType_t messagePending = xQueueReceive(txMessageQueue, (void *)&message, 0);
        if (messagePending == pdPASS) {
            size_t messageSize = sizeof(message.header) + message.header.payloadSize + sizeof(message.crc);
            UART_write(uartHandle, &message, messageSize);
        }

        // Start reading the next message.
        rxTotalBytes = 0;
        HAPRawBufferZero(&rxBuffer, sizeof rxBuffer);
        UART_read(uartHandle, &rxBuffer.header.som, sizeof(rxBuffer.header.som));

        // Block until notification from RX callback.
        uint32_t notifyValue = kMessageStatus_OK;
        if (xTaskNotifyWait(0x00, ULONG_MAX, &notifyValue, kUART_BlockTime) == pdFAIL) {
            // Receive timeout; cancel read and resend last message.
            UART_readCancel(uartHandle);
            if (messagePending == pdPASS) {
                xQueueSendToBack(txMessageQueue, (void *)&message, (TickType_t)0);
            }
        }

        switch (notifyValue) {
        case kMessageStatus_OK:
            break;
        case kMessageStatus_InvalidSOM:
            HAPLogBufferError(&kHAPLog_Default, &rxBuffer, rxTotalBytes, "Invalid SOM (0x%02X).", rxBuffer.header.som);
            FlushBuffers(uartHandle);
            break;
        case kMessageStatus_InvalidPayloadSize:
            HAPLogBufferError(&kHAPLog_Default, &rxBuffer, rxTotalBytes, "Invalid payload size (%u).", rxBuffer.header.payloadSize);
            FlushBuffers(uartHandle);
            break;
        case kMessageStatus_InvalidCRC:
            HAPLogBufferError(&kHAPLog_Default, &rxBuffer, rxTotalBytes, "Invalid CRC (0x%04X).", rxBuffer.crc);
            FlushBuffers(uartHandle);
            break;
        default:
            break;
        }

        ProcessIncomingMessages();
    }
}

// Post a message to the TX queue. Don't block if the queue is full.
void UARTSendMessage(uint8_t opcode, uint16_t payloadSize, void *payload)
{
    HAPAssert(payloadSize <= kUART_MaxPayloadSize);
    HAPAssert(IsTXOpcodeValid(opcode));

    Message_t message;
    message.header.som = 0xf8;
    message.header.opcode = opcode;
    message.header.payloadSize = payloadSize;
    if (payloadSize > 0) {
        HAPRawBufferCopyBytes(&message.payload, payload, payloadSize);
    }

    void *crcbuf = &message.header.opcode;
    const uint32_t crclen = sizeof(message.header.opcode) + sizeof(message.header.payloadSize) + message.header.payloadSize;
    message.crc = CRC16(crcbuf, crclen);

    // Copy CRC to the end of the payload buffer.
    HAPRawBufferCopyBytes((uint8_t *)(&message.payload) + message.header.payloadSize, &message.crc, sizeof(message.crc));
    
    if (xQueueSendToBack(txMessageQueue, (void *)&message, (TickType_t)0) != pdTRUE) {
        HAPLogError(&kHAPLog_Default, "Failed to post message to TX queue.");
    }
}

void SendFanControlCommand(uint16_t value)
{
    FanControlTXPayload payload = { .value = value };
    UARTSendMessage(0x50, sizeof(payload), &payload);
}

void SendLightControlCommand(uint16_t value)
{
    LightControlTXPayload payload = { .value = value };
    UARTSendMessage(0x60, sizeof(payload), &payload);
}