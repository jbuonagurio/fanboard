##  Copyright 2022 John Buonagurio
##
##  Distributed under the Boost Software License, Version 1.0.
##
##  See accompanying file LICENSE_1_0.txt or copy at
##  http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.18)

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
set(FREERTOS_KERNEL_DIR "${PROJECT_SOURCE_DIR}/external/FreeRTOS-Kernel" CACHE PATH "Path to FreeRTOS Kernel")
set(HOMEKIT_ADK_DIR "${PROJECT_SOURCE_DIR}/external/HomeKitADK" CACHE PATH "Path to HomeKit ADK")
set(SEGGER_DIR "${PROJECT_SOURCE_DIR}/external/SEGGER" CACHE PATH "Path to SEGGER RTT and SystemView")
set(MBEDTLS_DIR "${PROJECT_SOURCE_DIR}/external/mbedtls" CACHE PATH "Path to Mbed TLS")
set(MULTIPART_PARSER_DIR "${PROJECT_SOURCE_DIR}/external/multipart-parser" CACHE PATH "Path to multipart-parser")

# HAP_LOG_LEVEL:
# 0: kHAPPlatformLogEnabledTypes_None
# 1: kHAPPlatformLogEnabledTypes_Default
# 2: kHAPPlatformLogEnabledTypes_Info
# 3: kHAPPlatformLogEnabledTypes_Debug
set(HAP_LOG_LEVEL Debug CACHE STRING "Logging level for HomeKit ADK")
set(HAP_LOG_LEVEL_VALUES "None;Default;Info;Debug" CACHE INTERNAL "")
set_property(CACHE HAP_LOG_LEVEL PROPERTY STRINGS ${HAP_LOG_LEVEL_VALUES})

# Enable logging to a remote syslog server. This option must not be
# used when J-Link might also do RTT.
option(HAP_LOG_REMOTE "Enable syslog logging in HomeKit ADK" OFF)

# Include sensitive information in logs.
option(HAP_LOG_SENSITIVE "Enable sensitive information logging in HomeKit ADK" OFF)

# __SF_DEBUG__ enables the .dbghdr section, which skips the integrity
# check phase of booting and signals to the bootloader that it should
# should not attempt an erase of the image. Production images must not
# have a .dbghdr section. See SimpleLink Wi-Fi CC323x Technical
# Reference Manual (SWRU543A), Section 21.10: Debugging Flash User
# Application Using JTAG.
option(ENABLE_SF_DEBUG "Enable the .dbghdr section" OFF)

message(STATUS "CMAKE_C_COMPILER_ID: ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_SYSROOT: ${CMAKE_SYSROOT}")
message(STATUS "HAP_LOG_LEVEL: ${HAP_LOG_LEVEL}")
message(STATUS "HAP_LOG_REMOTE: ${HAP_LOG_REMOTE}")
message(STATUS "HAP_LOG_SENSITIVE: ${HAP_LOG_SENSITIVE}")
message(STATUS "ENABLE_SF_DEBUG: ${ENABLE_SF_DEBUG}")

#----------------------------------------------------------------------
# Target: SEGGER RTT
#----------------------------------------------------------------------

add_library(segger_rtt
    "${SEGGER_DIR}/RTT/SEGGER_RTT_ASM_ARMv7M.S"
    "${SEGGER_DIR}/RTT/SEGGER_RTT.c"
    "${SEGGER_DIR}/RTT/SEGGER_RTT_printf.c")
target_include_directories(segger_rtt
    PUBLIC
        "${SEGGER_DIR}/RTT"
        "${SEGGER_DIR}/Config")
target_compile_definitions(segger_rtt PRIVATE -DRTT_USE_ASM=1)
set_property(TARGET segger_rtt PROPERTY PREFIX "")

#----------------------------------------------------------------------
# Target: SEGGER SystemView
#----------------------------------------------------------------------

add_library(segger_sysview)
target_sources(segger_sysview
    PRIVATE
        "${SEGGER_DIR}/SYSVIEW/SEGGER_SYSVIEW.c"
    INTERFACE
        "${SEGGER_DIR}/SYSVIEW/SEGGER_SYSVIEW_FreeRTOS.c"
        "${PROJECT_SOURCE_DIR}/port/SEGGER/SYSVIEW/SEGGER_SYSVIEW_Config_FreeRTOS.c")
target_include_directories(segger_sysview INTERFACE
    "${SEGGER_DIR}/SYSVIEW"
    "${SEGGER_DIR}/Config")
target_link_libraries(segger_sysview PUBLIC segger_rtt)
set_property(TARGET segger_sysview PROPERTY PREFIX "")

#----------------------------------------------------------------------
# Target: TI-POSIX
#----------------------------------------------------------------------

# Interface target for FreeRTOS.
add_library(ti_posix INTERFACE)
target_sources(ti_posix INTERFACE
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/clock.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/mqueue.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/freertos/memory.c"
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
    "${SIMPLELINK_CC32XX_SDK_DIR}/kernel/freertos/dpl/PowerCC32XX_freertos.c")
target_include_directories(ti_posix INTERFACE
    "${SIMPLELINK_CC32XX_SDK_DIR}/source"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/posix/gcc")

#----------------------------------------------------------------------
# Target: TI DPL (Driver Porting Layer)
#----------------------------------------------------------------------

# Interface target for FreeRTOS.
add_library(ti_dpl INTERFACE)
target_sources(ti_dpl INTERFACE
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
target_include_directories(ti_dpl INTERFACE
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")

#----------------------------------------------------------------------
# Target: FreeRTOS
#----------------------------------------------------------------------

# Includes SEGGER SystemView, TI POSIX and DPL.
add_library(freertos
    "${FREERTOS_KERNEL_DIR}/croutine.c"
    "${FREERTOS_KERNEL_DIR}/event_groups.c"
    "${FREERTOS_KERNEL_DIR}/list.c"
    "${FREERTOS_KERNEL_DIR}/queue.c"
    "${FREERTOS_KERNEL_DIR}/stream_buffer.c"
    "${FREERTOS_KERNEL_DIR}/tasks.c"
    "${FREERTOS_KERNEL_DIR}/timers.c"
    "${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM3/port.c"
    "${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c")
target_include_directories(freertos PUBLIC
    "${PROJECT_SOURCE_DIR}/port/FreeRTOS-Kernel/include"
    "${FREERTOS_KERNEL_DIR}/include"
    "${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM3")
target_link_libraries(freertos PUBLIC segger_sysview ti_posix ti_dpl)

#----------------------------------------------------------------------
# CC32xx Driverlib
#----------------------------------------------------------------------

add_library(ti_driverlib
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/adc.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/adc.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/aes.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/camera.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/cpu.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/crc.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/des.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/flash.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/gpio.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/hwspinlock.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/i2c.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/interrupt.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/i2s.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/pin.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/prcm.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/sdhost.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/shamd5.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/spi.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/systick.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/timer.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/uart.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/udma.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/utils.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib/wdt.c")
target_include_directories(ti_driverlib
    PUBLIC
        "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti"
    PRIVATE
        "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx/driverlib"
        "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/devices/cc32xx")

# cpu.c, utils.c
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(ti_driverlib PRIVATE -Dgcc)
elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_definitions(ti_driverlib PRIVATE -Dticlang)
endif()

set_property(TARGET ti_driverlib PROPERTY COMPILE_OPTIONS "-O0")
set_property(TARGET ti_driverlib PROPERTY PREFIX "")

#----------------------------------------------------------------------
# CC32xx Drivers
#----------------------------------------------------------------------

add_library(ti_drivers
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/ADC.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/adc/ADCCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/apps/Button.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/apps/LED.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/Camera.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/camera/CameraCC32XXDMA.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/Capture.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/capture/CaptureCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/crypto/CryptoCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/cryptoutils/utils/CryptoUtils.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/dma/UDMACC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/GPIO.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/gpio/GPIOCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/I2C.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/i2c/I2CCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/I2S.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/i2s/I2SCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/ITM.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/itm/ITMCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/NVS.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/nvs/NVSRAM.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/nvs/NVSSPI25X.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/power/PowerCC32XX_asm.sm4"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/power/PowerCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/PWM.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/pwm/PWMTimerCC32XX.c"
    #"${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/SD.c"
    #"${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/sd/SDHostCC32XX.c"
    #"${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/sd/SDSPI.c"
    #"${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/SDFatFS.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/SPI.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/spi/SPICC32XXDMA.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/Timer.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/timer/TimerCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/UART.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/uart/UARTCC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/uart/UARTCC32XXDMA.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/UART2.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/uart2/UART2CC32XX.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/utils/List.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/utils/Random.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/utils/RingBuf.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/utils/StructRingBuf.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/Watchdog.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/watchdog/WatchdogCC32XX.c")
set_property(SOURCE
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/power/PowerCC32XX_asm.sm4"
    APPEND PROPERTY LANGUAGE ASM)
target_include_directories(ti_drivers PUBLIC
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")
target_compile_definitions(ti_drivers PUBLIC
    -DUSE_CC3220_ROM_DRV_API
    -DDeviceFamily_CC3220)
set_property(TARGET ti_drivers PROPERTY PREFIX "")

#----------------------------------------------------------------------
# CC32xx Wi-Fi Drivers
#----------------------------------------------------------------------

add_library(ti_drivers_net_wifi
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/slnetif/slnetifwifi.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/slnetif/slwificonn.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/device.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/driver.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/flowcont.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/fs.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/netapp.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/netcfg.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/netutil.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/nonos.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/sl_socket.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/spawn.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/wlan.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/porting/cc_pal.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/eventreg.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/drivers/net/wifi/source/wlanconfig.c")
target_include_directories(ti_drivers_net_wifi PUBLIC
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")
target_compile_definitions(ti_drivers_net_wifi PUBLIC
    -DSL_SUPPORT_IPV6
    -DSL_PLATFORM_MULTI_THREADED)
target_link_libraries(ti_drivers_net_wifi PUBLIC freertos)
set_property(TARGET ti_drivers_net_wifi PROPERTY PREFIX "")

#----------------------------------------------------------------------
# CC32xx Network Drivers
#----------------------------------------------------------------------

add_library(ti_net
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/bsd/netdb.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/bsd/socket.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/bsd/errnoutil.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/slnetsock.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/slnetconn.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/slnetutils.c"
    "${SIMPLELINK_CC32XX_SDK_DIR}/source/ti/net/slnetif.c")
target_include_directories(ti_net PUBLIC
    "${SIMPLELINK_CC32XX_SDK_DIR}/source")
target_link_libraries(ti_net PUBLIC freertos)
set_property(TARGET ti_net PROPERTY PREFIX "")

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
    "${PROJECT_SOURCE_DIR}/port/mbedtls/library/platform_memory.c")

target_include_directories(mbedcrypto PUBLIC
    "${PROJECT_SOURCE_DIR}/port/mbedtls/include"
    "${MBEDTLS_DIR}/include")

# entropy_poll.c: ti_drivers_net_wifi (sl_NetUtilGet)
# platform_memory.c: freertos (pvPortCalloc, vPortFree)
target_link_libraries(mbedcrypto PUBLIC ti_drivers_net_wifi freertos)

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
    ti_driverlib
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net
    mbedcrypto)

if(HAP_LOG_LEVEL STREQUAL "None")
    target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_LEVEL=0)
elseif(HAP_LOG_LEVEL STREQUAL "Default")
    target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_LEVEL=1)
elseif(HAP_LOG_LEVEL STREQUAL "Info")
    target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_LEVEL=2)
elseif(HAP_LOG_LEVEL STREQUAL "Debug")
    target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_LEVEL=3)
else()
    message(FATAL_ERROR "Invalid HAP_LOG_LEVEL." )
endif()

target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_REMOTE=$<IF:$<BOOL:${HAP_LOG_REMOTE}>,1,0>)
target_compile_definitions(homekitadk PUBLIC -DHAP_LOG_SENSITIVE=$<IF:$<BOOL:${HAP_LOG_SENSITIVE}>,1,0>)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_DISABLE_ASSERTS=0
        -DHAP_DISABLE_PRECONDITIONS=0)
else()
    target_compile_definitions(homekitadk PUBLIC
        -DHAP_DISABLE_ASSERTS=1
        -DHAP_DISABLE_PRECONDITIONS=1)
endif()

#----------------------------------------------------------------------
# Target: multipart_parser
#----------------------------------------------------------------------

add_library(multipart_parser "${MULTIPART_PARSER_DIR}/multipartparser.c")
target_include_directories(multipart_parser PUBLIC "${MULTIPART_PARSER_DIR}")

#----------------------------------------------------------------------
# Target: Application
#----------------------------------------------------------------------

set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/linker/cc32xxsf_freertos.lds")
set(STARTUP_FILE "${PROJECT_SOURCE_DIR}/startup/startup_cc32xx.c")

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/app/App.c"
    "${PROJECT_SOURCE_DIR}/app/Board.c"
    "${PROJECT_SOURCE_DIR}/app/DB.c"
    "${PROJECT_SOURCE_DIR}/app/HTTPServer.c"
    "${PROJECT_SOURCE_DIR}/app/Main.c"
    "${PROJECT_SOURCE_DIR}/app/NWPEvent.c"
    "${PROJECT_SOURCE_DIR}/app/OTA.c"
    "${PROJECT_SOURCE_DIR}/app/UART.c"
    "${PROJECT_SOURCE_DIR}/app/Utilities.c"
    "${PROJECT_SOURCE_DIR}/app/Syscalls.c"
    "${STARTUP_FILE}")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ti_driverlib
    freertos
    ti_drivers_net_wifi
    ti_drivers
    ti_net
    homekitadk
    multipart_parser)

if(ENABLE_SF_DEBUG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -D__SF_DEBUG__)
endif()

target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-Map=${PROJECT_NAME}.map")
target_link_options(${PROJECT_NAME} PRIVATE "LINKER:-T,${LINKER_SCRIPT}")

# Require rebuild on linker script change.
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

# Generate binary and Intel Hex from ELF.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin"
    COMMAND ${CMAKE_OBJCOPY} -O ihex "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}" "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex"
    COMMAND ${CMAKE_SIZE} "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
