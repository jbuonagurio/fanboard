// Copyright (c) 2015-2019 The HomeKit ADK Contributors
//
// Copyright (c) 2020 John Buonagurio
// 
// Licensed under the Apache License, Version 2.0 (the “License”);
// you may not use this file except in compliance with the License.
// See [CONTRIBUTORS.md] for the list of HomeKit ADK project authors.

#include <stdlib.h>

#include "HAPPlatform.h"

HAP_NORETURN
void HAPPlatformAbort(void) {
    while (1) {}
}
