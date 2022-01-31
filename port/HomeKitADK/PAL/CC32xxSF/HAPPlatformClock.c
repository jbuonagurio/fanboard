// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2022 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>

#include "HAPPlatform.h"

static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Clock" };

HAPTime HAPPlatformClockGetCurrent(void)
{
    static HAPTime previousNow = 0;

    // The CC32xx provides an RTC mechanism using a set of HIB registers in
    // conjunction with a 48-bit always-on Slow Clock Counter (SCC) running at
    // 32.768 kHz. RTC registers can be accessed from the 32.768 kHz clock
    // domain (HIB1P2) or 40 MHz clock domain (HIB3P3). RTC registers in the
    // 40 MHz domain are automatically latched, but we need to read the SCC
    // three times and compare values to ensure we are correctly synchronized
    // with the 32.768 kHz RTC when both clocks are exactly aligned.
    uint64_t scc[3];
    for (size_t i = 0; i < 3; ++i)
        scc[i] = MAP_PRCMSlowClkCtrFastGet();

    // Select the SCC value which matches in at least two of the reads.
    HAPTime now = (scc[1] - scc[0] <= 1 ? scc[1] : scc[2]) * 1000ull / 32768ull;

    if (now < previousNow) {
        HAPLogFault(&logObject, "Time jumped backwards by %llu ms.", previousNow - now);
        HAPFatalError();
    }

    if (now & (1ull << 63)) {
        HAPLogFault(&logObject, "Time overflowed (capped at 2^63 - 1).");
        HAPFatalError();
    }

    previousNow = now;
    return now;
}
