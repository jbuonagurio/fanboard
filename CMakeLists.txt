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

project(fanboard LANGUAGES C ASM)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# TI POSIX compatibility headers require -std=c11, not -std=gnu11.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

#----------------------------------------------------------------------
# Project Configuration Options
#----------------------------------------------------------------------

set(SIMPLELINK_CC32XX_SDK_DIR "" CACHE PATH "Path to SimpleLink CC32xx SDK")
set(FREERTOS_DIR "${PROJECT_SOURCE_DIR}/external/FreeRTOS-Kernel" CACHE PATH "Path to FreeRTOS Kernel")
set(HOMEKIT_ADK_DIR "${PROJECT_SOURCE_DIR}/external/HomeKitADK" CACHE PATH "Path to HomeKit ADK")
set(SEGGER_DIR "${PROJECT_SOURCE_DIR}/external/SEGGER" CACHE PATH "Path to SEGGER RTT and SystemView")
set(MBEDTLS_DIR "${PROJECT_SOURCE_DIR}/external/mbedtls" CACHE PATH "Path to Mbed TLS")

set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/linker/cc32xxsf_freertos.lds")
set(STARTUP_FILE "${PROJECT_SOURCE_DIR}/startup/startup_cc32xx_gcc.c")

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

add_library(segger_rtt
    "${SEGGER_DIR}/RTT/SEGGER_RTT.c"
    "${SEGGER_DIR}/RTT/SEGGER_RTT_printf.c")

target_include_directories(segger_rtt PUBLIC
    "${SEGGER_DIR}/RTT"
    "${SEGGER_DIR}/Config")

target_compile_definitions(segger_rtt PUBLIC -DRTT_USE_ASM=0)

#----------------------------------------------------------------------
# Target: SEGGER SystemView
#----------------------------------------------------------------------

add_library(segger_sysview
    "${SEGGER_DIR}/SYSVIEW/SEGGER_SYSVIEW.c")

target_include_directories(segger_sysview PUBLIC
    "${SEGGER_DIR}/SYSVIEW"
    "${SEGGER_DIR}/Config")

target_link_libraries(segger_sysview PUBLIC segger_rtt)

#----------------------------------------------------------------------
# Target: FreeRTOS
#----------------------------------------------------------------------

# Object library must be used to link to pre-built TI archives.
# Includes TI POSIX, DPL (Driver Porting Layer) and SEGGER SystemView.
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
    # SEGGER SystemView
    "${SEGGER_DIR}/SYSVIEW/SEGGER_SYSVIEW_FreeRTOS.c"
    "${PROJECT_SOURCE_DIR}/port/SEGGER/SYSVIEW/SEGGER_SYSVIEW_Config_FreeRTOS.c"
    # TI POSIX
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
    # TI DPL
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
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/PowerCC32XX_freertos.c")

target_include_directories(freertos PUBLIC
    "${PROJECT_SOURCE_DIR}/port/FreeRTOS-Kernel/include"
    "${FREERTOS_DIR}/include"
    "${FREERTOS_DIR}/portable/GCC/ARM_CM3"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/gcc")

target_link_libraries(freertos PUBLIC segger_sysview)

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
    "${PROJECT_SOURCE_DIR}/port/mbedtls/library/entropy_poll.c")

target_include_directories(mbedcrypto PUBLIC
    "${PROJECT_SOURCE_DIR}/port/mbedtls/include"
    "${MBEDTLS_DIR}/include")

# For sl_NetUtilGet(SL_NETUTIL_TRUE_RANDOM).
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
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformOTA.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformRandomNumber.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformRunLoop.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformServiceDiscovery.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformSyslog.c"
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF/HAPPlatformTCPStreamManager.c")

target_include_directories(homekitadk PUBLIC
    "${PROJECT_SOURCE_DIR}/port/HomeKitADK/PAL/CC32xxSF"
    "${HOMEKIT_ADK_DIR}/External/Base64"
    "${HOMEKIT_ADK_DIR}/External/HTTP"
    "${HOMEKIT_ADK_DIR}/External/JSON"
    "${HOMEKIT_ADK_DIR}/PAL"
    "${HOMEKIT_ADK_DIR}/HAP")

target_link_libraries(homekitadk PUBLIC
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net_utils
    ti_net
    ti_devices_cc32xx_driverlib
    mbedcrypto)

# HAP_LOG_LEVEL:
# 0: kHAPPlatformLogEnabledTypes_None
# 1: kHAPPlatformLogEnabledTypes_Default
# 2: kHAPPlatformLogEnabledTypes_Info
# 3: kHAPPlatformLogEnabledTypes_Debug

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_LOG_LEVEL=3
        -DHAP_LOG_SENSITIVE=1
        -DHAP_LOG_REMOTE=1
        -DHAP_DISABLE_ASSERTS=0
        -DHAP_DISABLE_PRECONDITIONS=0)
else()
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_LOG_LEVEL=0
        -DHAP_LOG_SENSITIVE=0
        -DHAP_LOG_REMOTE=0
        -DHAP_DISABLE_ASSERTS=1
        -DHAP_DISABLE_PRECONDITIONS=1)
endif()

#----------------------------------------------------------------------
# Target: Application
#----------------------------------------------------------------------

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    "app/Board.c"
    "app/NWPEvent.c"
    "app/App.c"
    "app/DB.c"
    "app/UART.c"
    "app/Utilities.c"
    "app/Main.c"
    "${STARTUP_FILE}")

target_link_libraries(${PROJECT_NAME} PRIVATE
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net_utils
    ti_net
    ti_devices_cc32xx_driverlib
    homekitadk)

# __SF_DEBUG__ enables the .dbghdr section, which skips the integrity
# check phase of booting and signals to the bootloader that it should
# should not attempt an erase of the image. See SimpleLink Wi-Fi CC323x
# Technical Reference Manual, Section 21.10: Debugging Flash User
# Application Using JTAG.

target_compile_definitions(${PROJECT_NAME} PRIVATE -D__SF_DEBUG__)

target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-Map=${PROJECT_NAME}.map")
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-T,${LINKER_SCRIPT}")

# Require rebuild on linker script change.
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Generate binary and Intel Hex from ELF.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND ${CMAKE_OBJCOPY} -O ihex "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex")
