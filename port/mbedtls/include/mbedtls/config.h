/**
 * \file config.h
 *
 * \brief Configuration options (set of defines)
 *
 *  This set of compile-time options may be used to enable
 *  or disable features selectively, and reduce the global
 *  memory footprint.
 */
/*
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

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <stddef.h>

/* Forward declarations */
void vPortFree(void * pv);
void * pvPortCalloc(size_t xNumElements, size_t xSize);

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_FREE_MACRO vPortFree
#define MBEDTLS_PLATFORM_CALLOC_MACRO pvPortCalloc
#define MBEDTLS_PLATFORM_SNPRINTF_MACRO snprintf

/* mbed TLS feature support */
#define MBEDTLS_CIPHER_MODE_CTR
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED

/* mbed TLS modules */
#define MBEDTLS_AES_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_CHACHAPOLY_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C
#define MBEDTLS_HKDF_C
#define MBEDTLS_OID_C
#define MBEDTLS_MD_C
#define MBEDTLS_PKCS5_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C

/* mbed TLS platform modules */
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BASE64_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_VERSION_C

/* Miscellaneous options */
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_ECP_NIST_OPTIM

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
