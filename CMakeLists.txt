##  Copyright 2021 John Buonagurio
##
##  Distributed under the Boost Software License, Version 1.0.
##
##  See accompanying file LICENSE_1_0.txt or copy at
##  http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/cmake/arm-gcc-toolchain.cmake")
endif()

project(fanboard)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

 # TI POSIX compatibility headers require -std=c11, not -std=gnu11
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

#----------------------------------------------------------------------
# Project Configuration Options
#----------------------------------------------------------------------

set(SIMPLELINK_CC32XX_SDK_DIR "" CACHE PATH "Path to SimpleLink CC32xx SDK")
set(FREERTOS_DIR "${PROJECT_SOURCE_DIR}/external/FreeRTOS-Kernel" CACHE PATH "Path to FreeRTOS Kernel")
set(HOMEKIT_ADK_DIR "${PROJECT_SOURCE_DIR}/external/HomeKitADK" CACHE PATH "Path to HomeKit ADK")
set(SEGGER_RTT_DIR "${PROJECT_SOURCE_DIR}/external/SEGGER_RTT" CACHE PATH "Path to SEGGER RTT")
set(MBEDTLS_DIR "${PROJECT_SOURCE_DIR}/external/mbedtls" CACHE PATH "Path to Mbed TLS")

# Platform Configuration

set(PLATFORM_FLAGS -mcpu=cortex-m4+nofp -mfloat-abi=soft -mthumb --specs=nano.specs)
set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/linker/cc32xxsf_freertos.lds")
set(STARTUP_FILE "${PROJECT_SOURCE_DIR}/startup/startup_cc32xx_gcc.c")

# Global Compile Options:
#
# -ffunction-sections
# -fdata-sections
#     Place each function or data item into its own section in the
#     output file. Used with linker `--gc-sections` option.
# -ffreestanding
#     Asserts that program startup is not necessarily at `main`.
#     Implies `-fno-builtin`.
# -fmerge-constants
#     Attempt to merge identical constants (string constants and
#     floating-point constants) across compilation units. 
# -fstack-usage
#     Generate an extra file that specifies the maximum amount of
#     stack used, on a per-function basis.

add_compile_options(${PLATFORM_FLAGS} -ffunction-sections -fdata-sections -ffreestanding -fmerge-constants -fstack-usage)

#----------------------------------------------------------------------
# Imported Targets
#----------------------------------------------------------------------

# CC32xx Driverlib
add_library(ti_devices_cc32xx_driverlib INTERFACE IMPORTED)
set_property(TARGET ti_devices_cc32xx_driverlib PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/gcc/Release/driverlib.a")
set_property(TARGET ti_devices_cc32xx_driverlib PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti")

# Libraries required for /ti/drivers
add_library(ti_drivers INTERFACE IMPORTED)
set_property(TARGET ti_drivers PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/lib/gcc/m4/drivers_cc32xx.a")
set_property(TARGET ti_drivers PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/drivers/net/wifi
add_library(ti_drivers_net_wifi INTERFACE IMPORTED)
set_property(TARGET ti_drivers_net_wifi PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/slnetif/gcc/Release/slnetifwifi.a"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/gcc/rtos/simplelink.a")
set_property(TARGET ti_drivers_net_wifi PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/bsd")

# Libraries required for /ti/net (SlNetSock)
add_library(ti_net INTERFACE IMPORTED)
set_property(TARGET ti_net PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/lib/gcc/m4/slnetsock_release.a")
set_property(TARGET ti_net PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/net/mqtt
add_library(ti_net_mqtt INTERFACE IMPORTED)
set_property(TARGET ti_net_mqtt PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/mqtt/lib/gcc/m4/mqtt_release.a")
set_property(TARGET ti_net_mqtt PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/net/sntp
add_library(ti_net_sntp INTERFACE IMPORTED)
set_property(TARGET ti_net_sntp PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/sntp/lib/gcc/m4/sntp_release.a")
set_property(TARGET ti_net_sntp PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/net/utils (ClockSync, Map, StrMpl)
add_library(ti_net_utils INTERFACE IMPORTED)
set_property(TARGET ti_net_utils PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/utils/gcc/Release/net_utils.a")
set_property(TARGET ti_net_utils PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/display (Display)
add_library(ti_display INTERFACE IMPORTED)
set_property(TARGET ti_display PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/grlib/lib/gcc/m4/grlib.a"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/display/lib/gcc/m4/display.a")
set_property(TARGET ti_display PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

# Libraries required for /ti/utils/runtime (Log)
add_library(ti_utils_runtime INTERFACE IMPORTED)
set_property(TARGET ti_utils_runtime PROPERTY INTERFACE_LINK_LIBRARIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/loggers/drivers/lib/gcc/m4/loggers.a"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/loggers/utils/lib/gcc/m4/loggers_release.a"
    "${SIMPLELINK_CC32XX_SDK_DIR}/ti/utils/runtime/lib/gcc/m4/runtime_release.a")
set_property(TARGET ti_utils_runtime PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

#----------------------------------------------------------------------
# Target: SEGGER RTT
#----------------------------------------------------------------------

add_library(segger_rtt OBJECT
    "${SEGGER_RTT_DIR}/RTT/SEGGER_RTT.c"
    "${SEGGER_RTT_DIR}/RTT/SEGGER_RTT_printf.c"
)

target_include_directories(segger_rtt PUBLIC "${SEGGER_RTT_DIR}/RTT")
target_include_directories(segger_rtt PUBLIC "${SEGGER_RTT_DIR}/Config")

#----------------------------------------------------------------------
# Target: FreeRTOS
#----------------------------------------------------------------------

# Object library required for linking to pre-built TI archives.
# Includes TI POSIX compability layer and DPL (Driver Porting Layer).
add_library(freertos OBJECT
    "${FREERTOS_DIR}/croutine.c"
    "${FREERTOS_DIR}/event_groups.c"
    "${FREERTOS_DIR}/list.c"
    "${FREERTOS_DIR}/queue.c"
    "${FREERTOS_DIR}/stream_buffer.c"
    "${FREERTOS_DIR}/tasks.c"
    "${FREERTOS_DIR}/timers.c"
    "${FREERTOS_DIR}/portable/GCC/ARM_CM3/port.c"
    "${FREERTOS_DIR}/portable/MemMang/heap_4.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/clock.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/memory.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/mqueue.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/pthread_barrier.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/pthread_cond.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/pthread.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/pthread_mutex.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/pthread_rwlock.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/sched.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/semaphore.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/sleep.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/timer.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/AppHooks_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/ClockP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/DebugP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/MutexP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/QueueP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/SemaphoreP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/StaticAllocs_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/SwiP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/SystemP_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/HwiPCC32XX_freertos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/PowerCC32XX_freertos.c"
)

target_include_directories(freertos PUBLIC
    "${PROJECT_SOURCE_DIR}/port/FreeRTOS-Kernel/include"
    "${FREERTOS_DIR}/include"
    "${FREERTOS_DIR}/portable/GCC/ARM_CM3"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/gcc"
)

# SEGGER RTT is used for tracing; see FreeRTOSConfigTrace.h.
target_link_libraries(freertos PUBLIC segger_rtt)

#----------------------------------------------------------------------
# Target: Mbed TLS
#----------------------------------------------------------------------

add_library(mbedcrypto
    "${MBEDTLS_DIR}/library/aes.c"
    "${MBEDTLS_DIR}/library/aesni.c"
    "${MBEDTLS_DIR}/library/arc4.c"
    "${MBEDTLS_DIR}/library/aria.c"
    "${MBEDTLS_DIR}/library/asn1parse.c"
    "${MBEDTLS_DIR}/library/asn1write.c"
    "${MBEDTLS_DIR}/library/base64.c"
    "${MBEDTLS_DIR}/library/bignum.c"
    "${MBEDTLS_DIR}/library/blowfish.c"
    "${MBEDTLS_DIR}/library/camellia.c"
    "${MBEDTLS_DIR}/library/ccm.c"
    "${MBEDTLS_DIR}/library/chacha20.c"
    "${MBEDTLS_DIR}/library/chachapoly.c"
    "${MBEDTLS_DIR}/library/cipher.c"
    "${MBEDTLS_DIR}/library/cipher_wrap.c"
    "${MBEDTLS_DIR}/library/cmac.c"
    "${MBEDTLS_DIR}/library/ctr_drbg.c"
    "${MBEDTLS_DIR}/library/des.c"
    "${MBEDTLS_DIR}/library/dhm.c"
    "${MBEDTLS_DIR}/library/ecdh.c"
    "${MBEDTLS_DIR}/library/ecdsa.c"
    "${MBEDTLS_DIR}/library/ecjpake.c"
    "${MBEDTLS_DIR}/library/ecp.c"
    "${MBEDTLS_DIR}/library/ecp_curves.c"
    "${MBEDTLS_DIR}/library/entropy.c"
    "${MBEDTLS_DIR}/library/entropy_poll.c"
    "${MBEDTLS_DIR}/library/error.c"
    "${MBEDTLS_DIR}/library/gcm.c"
    "${MBEDTLS_DIR}/library/havege.c"
    "${MBEDTLS_DIR}/library/hkdf.c"
    "${MBEDTLS_DIR}/library/hmac_drbg.c"
    "${MBEDTLS_DIR}/library/md.c"
    "${MBEDTLS_DIR}/library/md2.c"
    "${MBEDTLS_DIR}/library/md4.c"
    "${MBEDTLS_DIR}/library/md5.c"
    "${MBEDTLS_DIR}/library/memory_buffer_alloc.c"
    "${MBEDTLS_DIR}/library/nist_kw.c"
    "${MBEDTLS_DIR}/library/oid.c"
    "${MBEDTLS_DIR}/library/padlock.c"
    "${MBEDTLS_DIR}/library/pem.c"
    "${MBEDTLS_DIR}/library/pk.c"
    "${MBEDTLS_DIR}/library/pk_wrap.c"
    "${MBEDTLS_DIR}/library/pkcs12.c"
    "${MBEDTLS_DIR}/library/pkcs5.c"
    "${MBEDTLS_DIR}/library/pkparse.c"
    "${MBEDTLS_DIR}/library/pkwrite.c"
    "${MBEDTLS_DIR}/library/platform.c"
    "${MBEDTLS_DIR}/library/platform_util.c"
    "${MBEDTLS_DIR}/library/poly1305.c"
    "${MBEDTLS_DIR}/library/psa_crypto.c"
    "${MBEDTLS_DIR}/library/psa_crypto_driver_wrappers.c"
    "${MBEDTLS_DIR}/library/psa_crypto_se.c"
    "${MBEDTLS_DIR}/library/psa_crypto_slot_management.c"
    "${MBEDTLS_DIR}/library/psa_crypto_storage.c"
    "${MBEDTLS_DIR}/library/psa_its_file.c"
    "${MBEDTLS_DIR}/library/ripemd160.c"
    "${MBEDTLS_DIR}/library/rsa.c"
    "${MBEDTLS_DIR}/library/rsa_internal.c"
    "${MBEDTLS_DIR}/library/sha1.c"
    "${MBEDTLS_DIR}/library/sha256.c"
    "${MBEDTLS_DIR}/library/sha512.c"
    "${MBEDTLS_DIR}/library/threading.c"
    "${MBEDTLS_DIR}/library/timing.c"
    "${MBEDTLS_DIR}/library/version.c"
    "${MBEDTLS_DIR}/library/version_features.c"
    "${MBEDTLS_DIR}/library/xtea.c"

    # Port
    "${PROJECT_SOURCE_DIR}/port/mbedtls/library/entropy_poll.c"
)

target_include_directories(mbedcrypto PUBLIC
    "${PROJECT_SOURCE_DIR}/port/mbedtls/include"
    "${MBEDTLS_DIR}/include"
)

target_link_libraries(mbedcrypto PUBLIC ti_drivers_net_wifi)

#----------------------------------------------------------------------
# Target: HomeKit ADK
#----------------------------------------------------------------------

add_library(homekitadk
    "${HOMEKIT_ADK_DIR}/External/Base64/util_base64.c"
    "${HOMEKIT_ADK_DIR}/External/HTTP/util_http_reader.c"
    "${HOMEKIT_ADK_DIR}/External/JSON/util_json_reader.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessory+Info.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessoryServer+Reset.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessoryServer.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessorySetup.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessorySetupInfo.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPAccessoryValidation.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEAccessoryServer+Advertising.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEAccessoryServer+Broadcast.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEAccessoryServer.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristic+Broadcast.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristic+Configuration.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristic+Signature.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristic.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristicParseAndWriteValue.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLECharacteristicReadAndSerializeValue.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEPDU+TLV.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEPDU.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEPeripheralManager.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEProcedure.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEProtocol+Configuration.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLEService+Signature.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLESession.c"
    #"${HOMEKIT_ADK_DIR}/HAP/HAPBLETransaction.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPBitSet.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPCharacteristic.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPCharacteristicTypes.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPDeviceID.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIP+ByteBuffer.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPAccessory.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPAccessoryProtocol.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPAccessoryServer.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPCharacteristic.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPSecurityProtocol.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPIPServiceDiscovery.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPJSONUtils.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPLegacyImport.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPMACAddress.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPMFiHWAuth.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPMFiTokenAuth.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPDU.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPairing.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPairingBLESessionCache.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPairingPairSetup.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPairingPairVerify.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPPairingPairings.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPRequestHandlers+AccessoryInformation.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPRequestHandlers+HAPProtocolInformation.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPRequestHandlers+Pairing.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPRequestHandlers.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPServiceTypes.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPSession.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPStringBuilder.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPTLV.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPTLVMemory.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPTLVReader.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPTLVWriter.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPUUID.c"
    "${HOMEKIT_ADK_DIR}/HAP/HAPVersion.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPAssert.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+Crypto.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+Double.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+Float.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+Int.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+MACAddress.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+RawBuffer.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+Sha1Checksum.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+String.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPBase+UTF8.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPLog.c"
    "${HOMEKIT_ADK_DIR}/PAL/HAPPlatformSystemInit.c"
    "${HOMEKIT_ADK_DIR}/PAL/Crypto/MbedTLS/HAPMbedTLS.c"
    
    # Port
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatform.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformAbort.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformAccessorySetup.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformAccessorySetupDisplay.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformAccessorySetupNFC.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformBLEPeripheralManager.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformClock.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformKeyValueStore.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformLog.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformMFiHWAuth.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformMFiTokenAuth.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformRandomNumber.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformRunLoop.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformServiceDiscovery.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformTCPStreamManager.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformTimer.c"
)

target_include_directories(homekitadk PUBLIC
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF"
    "${HOMEKIT_ADK_DIR}/External/Base64"
    "${HOMEKIT_ADK_DIR}/External/HTTP"
    "${HOMEKIT_ADK_DIR}/External/JSON"
    "${HOMEKIT_ADK_DIR}/PAL"
    "${HOMEKIT_ADK_DIR}/HAP"
)

target_link_libraries(homekitadk PUBLIC
    segger_rtt
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net_utils
    ti_net
    ti_devices_cc32xx_driverlib
    mbedcrypto
)

# HAP_LOG_LEVEL:
# 0: kHAPPlatformLogEnabledTypes_None
# 1: kHAPPlatformLogEnabledTypes_Default
# 2: kHAPPlatformLogEnabledTypes_Info
# 3: kHAPPlatformLogEnabledTypes_Debug

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_LOG_LEVEL=3
        -DHAP_DISABLE_ASSERTS=0
        -DHAP_DISABLE_PRECONDITIONS=0)
else()
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_LOG_LEVEL=3
        -DHAP_DISABLE_ASSERTS=1
        -DHAP_DISABLE_PRECONDITIONS=1)
endif()

#----------------------------------------------------------------------
# Target: Application
#----------------------------------------------------------------------

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    "app/Board.c"
    "app/Event.c"
    "app/App.c"
    "app/DB.c"
    "app/UART.c"
    "app/Utilities.c"
    "app/Main.c"
    "${STARTUP_FILE}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    segger_rtt
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net_utils
    ti_net
    ti_devices_cc32xx_driverlib
    homekitadk
)

# __SF_DEBUG__ enables the .dbghdr section, which skips the integrity
# check phase of booting and signals to the bootloader that it should
# should not attempt an erase of the image. See SimpleLink Wi-Fi CC323x
# Technical Reference Manual, Section 21.10: Debugging Flash User
# Application Using JTAG.

target_compile_definitions(${PROJECT_NAME} PRIVATE -D__SF_DEBUG__)

# Target Link Options:
#
# -nostartfiles
#     Do not use the standard system startup files when linking.
#     The standard system libraries are used normally, unless
#     `-nostdlib`, `-nolibc`, or `-nodefaultlibs` is used.
# --gcsections
#     Enable garbage collection of unused input sections. Used with
#     compiler `-fdata-sections` option.
# --omagic
#     Set the text and data sections to be readable and writable.
#     Also, do not page-align the data segment, and disable linking
#     against shared libraries.
# --sort-section=alignment
#     This option will apply "SORT_BY_ALIGNMENT" to all wildcard
#     section patterns in the linker script.
# --cref
#     Output a cross reference table.
# --print-memory-usage
#     Print used size, total size and used size of memory regions
#     created with the MEMORY command.

target_link_options(${PROJECT_NAME} PRIVATE ${PLATFORM_FLAGS} -nostartfiles -lg -lgcc -lc -lm)
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--gc-sections,--omagic,--sort-section=alignment,--cref,--print-memory-usage")
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-Map=${PROJECT_NAME}.map,-T,${LINKER_SCRIPT}")

# Require rebuild on linker script change
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Generate binary and Intel Hex from ELF
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND ${CMAKE_OBJCOPY} -O ihex "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
)
