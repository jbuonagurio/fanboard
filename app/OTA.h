//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include "HTTPServer.h"

#include <ti/drivers/net/wifi/netapp.h>

#ifdef __cplusplus
extern "C" {
#endif

void OTAPutCallback(HTTPRequest *pRequest);
void OTAGetCallback(HTTPRequest *pRequest);

#ifdef __cplusplus
}
#endif
