/*
 *  Platform-specific and custom entropy polling functions
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <mbedtls/platform.h>
#include <mbedtls/entropy.h>
#include <mbedtls/entropy_poll.h>

#include <ti/drivers/net/wifi/simplelink.h>

/*
 * Use the SimpleLink Host Driver for hardware-derived PRNG entropy.
 * MBEDTLS_ENTROPY_HARDWARE_ALT must be defined in Mbed TLS config.h
 */
int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen)
{
    int rc = 0;
    *olen = len;
    if (sl_NetUtilGet(SL_NETUTIL_TRUE_RANDOM,       // Option
                      0,                            // ObjID
                      output,                       // pValues
                      (unsigned short *)olen) != 0) // pValueLen
    {
        rc = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    return rc;
}
