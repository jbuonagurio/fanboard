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

void PrintDeviceInfo(void);

void PrintStorageInfo(void);

void PrintFileList(void);

void RemoveInvalidFiles(uint32_t token);

void RestoreFactoryImage(void);

#ifdef __cplusplus
}
#endif
