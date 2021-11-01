//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <stdint.h>

#ifndef DeviceFamily_CC3220
#define DeviceFamily_CC3220
#endif

#include <ti/devices/DeviceFamily.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum BOARD_CryptoName {
    BOARD_CRYPTO0 = 0,
    BOARD_CRYPTOCOUNT
} BOARD_CryptoName;

typedef enum BOARD_I2CName {
    BOARD_I2C0 = 0,
    BOARD_I2CCOUNT
} BOARD_I2CName;

typedef enum BOARD_GPIOName {
    BOARD_GPIO0,        // GPIO09, Yellow LED
    BOARD_GPIO1,        // GPIO10, Blue LED
    BOARD_GPIOCOUNT
} BOARD_GPIOName;

typedef enum BOARD_PWMName {
    BOARD_PWM0,         // GPIO09, uses Timer2B for PWM.
    BOARD_PWM1,         // GPIO10, uses Timer3A for PWM.
    BOARD_PWMCOUNT
} BOARD_PWMName;

typedef enum BOARD_LEDName {
    BOARD_LED0,         // GPIO09, Yellow LED
    BOARD_LED1,         // GPIO10, Blue LED
    BOARD_LEDCOUNT
} BOARD_LEDName;

typedef enum BOARD_SPIName {
    BOARD_SPI0 = 0,     // Network Processor SPI Bus
    BOARD_SPI1,         // GSPI
    BOARD_SPICOUNT
} BOARD_SPIName;

typedef enum BOARD_TimerName {
    BOARD_TIMER0 = 0,   // Timer 0 subtimer A, 32-bit
    BOARD_TIMER1,       // Timer 1 subtimer A, 16-bit
    BOARD_TIMER2,       // Timer 1 subtimer B, 16-bit
    BOARD_TIMERCOUNT
} BOARD_TimerName;

typedef enum BOARD_UARTName {
    BOARD_UART0 = 0,
    BOARD_UARTCOUNT
} BOARD_UARTName;

typedef enum BOARD_WatchdogName {
    BOARD_WATCHDOG0 = 0,
    BOARD_WATCHDOGCOUNT
} BOARD_WatchdogName;

void InitializeBoard(void);

#ifdef __cplusplus
}
#endif
