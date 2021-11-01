//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "CRC.h"

#include <stdint.h>

#include <ti/devices/cc32xx/inc/hw_dthe.h>
#include <ti/devices/cc32xx/inc/hw_memmap.h>
#include <ti/devices/cc32xx/driverlib/crc.h>

// Calculate 16-bit CRC-CCITT (polynomial 0x1021, seed 0xFFFF) for serial packets.
uint16_t CalcCRC16(void *data, uint32_t len)
{
    CRCConfigSet(DTHE_BASE, CRC_CFG_INIT_1 | CRC_CFG_TYPE_P1021 | CRC_CFG_SIZE_8BIT);
    uint16_t crc = (uint16_t)CRCDataProcess(DTHE_BASE, data, len, CRC_CFG_SIZE_8BIT);
    return (crc >> 8) | (crc << 8); // Endian swap
}
