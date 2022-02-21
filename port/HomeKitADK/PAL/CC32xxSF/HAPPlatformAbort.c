// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2020 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <stdlib.h>

#include "HAPPlatform.h"

#include <FreeRTOS.h>
#include <task.h>

#include <ti/devices/cc32xx/inc/hw_nvic.h>
#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>
#include <ti/drivers/net/wifi/simplelink.h>

HAP_NORETURN
void HAPPlatformAbort(void)
{
    // Set a breakpoint if debugger is connected (DHCSR[C_DEBUGEN] == 1).
    if ((*(volatile uint32_t *)NVIC_DBG_CTRL) & NVIC_DBG_CTRL_C_DEBUGEN)
        __asm("bkpt 1");

    // Stop the NWP.
    sl_Stop(200U);

    // Short delay for debug log output before reset.
    vTaskDelay(pdMS_TO_TICKS(500UL));

    // Trigger a hibernate cycle for the device using RTC.
    MAP_PRCMHibernateCycleTrigger();

    while (1) {}
}
