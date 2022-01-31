//  Copyright 2022 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "Config.h"

#include "App.h"
#include "AppDomains.h"
#include "Board.h"
#include "DB.h"
#include "HTTPServer.h"
#include "NWPEvent.h"
#include "UART.h"
#include "Utilities.h"
#include "Version.h"

#include <HAP.h>
#include <HAPLog.h>
#include <HAPPlatform+Init.h>
#include <HAPPlatformAccessorySetup+Init.h>
#include <HAPPlatformKeyValueStore+Init.h>
#include <HAPPlatformMFiTokenAuth+Init.h>
#include <HAPPlatformOTA+Init.h>
#include <HAPPlatformRunLoop+Init.h>
#include <HAPPlatformServiceDiscovery+Init.h>
#include <HAPPlatformSyslog+Init.h>
#include <HAPPlatformTCPStreamManager+Init.h>

#include <ti/devices/cc32xx/inc/hw_types.h>
#include <ti/devices/cc32xx/driverlib/prcm.h>
#include <ti/devices/cc32xx/driverlib/rom_map.h>

#include <ti/drivers/apps/LED.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

// The Host Driver task must have the highest priority in the system.
#define kApp_HostTaskPriority (tskIDLE_PRIORITY + 1)
#define kApp_HostTaskStackSize (1024) // 32K

#define kApp_MainTaskPriority (tskIDLE_PRIORITY + 2)
#define kApp_MainTaskStackSize (2048) // 64K

#define kApp_UARTTaskPriority (tskIDLE_PRIORITY + 3)
#define kApp_UARTTaskStackSize (1024) // 32K

#define kApp_HTTPTaskPriority (tskIDLE_PRIORITY + 4)
#define kApp_HTTPTaskStackSize (2048) // 64K

// NWP stop timeout in milliseconds.
#define kSimpleLink_StopTimeout (200)

// Unused port number from the ephemeral port range, or kHAPNetworkPort_Any.
#define kHAPNetworkPort_Default (10000)

// HomeKit ADK Integration Guide for ADK 2.0, Section 3.2.4.
#define kHAPIPSessionStorage_NumElements ((size_t) 9)
#define kHAPIPSession_InboundBufferSize ((size_t) 768)
#define kHAPIPSession_OutboundBufferSize ((size_t) 1536)
#define kHAPIPSession_ScratchBufferSize ((size_t) 1536)

static bool requestedFactoryReset = false;
static bool clearPairings = false;

// FreeRTOS handles.
TaskHandle_t hostTaskHandle;
TaskHandle_t mainTaskHandle;
TaskHandle_t uartTaskHandle;
TaskHandle_t httpTaskHandle;

// Global platform objects.
static struct {
    HAPPlatformKeyValueStore keyValueStore;
    HAPAccessoryServerOptions hapAccessoryServerOptions;
    HAPPlatform hapPlatform;
    HAPAccessoryServerCallbacks hapAccessoryServerCallbacks;
    HAPPlatformTCPStreamManager tcpStreamManager;
    HAPPlatformMFiTokenAuth mfiTokenAuth;
} platform;

static HAPPlatformAccessorySetup accessorySetup;
static HAPPlatformServiceDiscovery serviceDiscovery;
static HAPAccessoryServerRef accessoryServer;

void HandleUpdatedState(HAPAccessoryServerRef *_Nonnull server, void *_Nullable context);

// Start the SimpleLink Network Processor (NWP).
static void StartNetworkProcessor()
{
    HAPLogInfo(&kHAPLog_Default, "Starting NWP.");

    // Create the SimpleLink Host Driver task.
    BaseType_t rc = xTaskCreate((TaskFunction_t)sl_Task,  // pvTaskCode
                                "Host Driver",            // pcName
                                kApp_HostTaskStackSize,   // usStackDepth
                                NULL,                     // pvParameters
                                kApp_HostTaskPriority,    // uxPriority
                                &hostTaskHandle);         // pxCreatedTask
    if (rc != pdPASS) { 
        HAPLogFault(&kHAPLog_Default, "Failed to create SimpleLink host driver task.");
        HAPFatalError();
    }

    // Block for a short period of time to allow sl_Task to start.
    while (eTaskGetState(hostTaskHandle) == eReady) {
        vTaskDelay(1);
    }

    // Initialize the SimpleLink NWP. sl_Start callback cannot be used
    // for task synchronization because SL_SET_DEVICE_STARTED is not
    // set before the callback is executed.
    int16_t mode = sl_Start(NULL, NULL, NULL);
    if (mode < 0) {
        HAPLogFault(&kHAPLog_Default, "Failed to start the NWP: %d.", mode);
        HAPFatalError();
    }
}

// Initialize global platform objects.
static void PlatformInitialize()
{
    // Syslog.
    HAPPlatformSyslogInitialize(
        &(const HAPPlatformSyslogOptions){ .ip = SYSLOG_IP, .port = SYSLOG_PORT });

    // Service discovery.
    HAPPlatformServiceDiscoveryCreate(&serviceDiscovery);
    platform.hapPlatform.ip.serviceDiscovery = &serviceDiscovery;

    // Key-value store.  
    HAPPlatformKeyValueStoreCreate(&platform.keyValueStore,
        &(const HAPPlatformKeyValueStoreOptions){ .rootDirectory = ".homekitstore" });
    platform.hapPlatform.keyValueStore = &platform.keyValueStore;

    // Accessory setup manager. Depends on key-value store.
    HAPPlatformAccessorySetupCreate(
        &accessorySetup, &(const HAPPlatformAccessorySetupOptions){ .keyValueStore = &platform.keyValueStore });
    platform.hapPlatform.accessorySetup = &accessorySetup;

    // TCP stream manager.
    HAPPlatformTCPStreamManagerCreate(&platform.tcpStreamManager,
        &(const HAPPlatformTCPStreamManagerOptions){ .interfaceName = NULL,
                                                     .port = kHAPNetworkPort_Default,
                                                     .maxConcurrentTCPStreams = kHAPIPSessionStorage_NumElements });

    // Software Token provider. Depends on key-value store.
    HAPPlatformMFiTokenAuthCreate(&platform.mfiTokenAuth,
        &(const HAPPlatformMFiTokenAuthOptions){ .keyValueStore = &platform.keyValueStore });
    platform.hapPlatform.authentication.mfiTokenAuth =
        HAPPlatformMFiTokenAuthIsProvisioned(&platform.mfiTokenAuth) ? &platform.mfiTokenAuth : NULL;
    
    // Run loop.
    HAPPlatformRunLoopCreate(&(const HAPPlatformRunLoopOptions){ .keyValueStore = &platform.keyValueStore });

    // Accessory server.
    platform.hapAccessoryServerOptions.maxPairings = kHAPPairingStorage_MinElements;
    platform.hapAccessoryServerCallbacks.handleUpdatedState = HandleUpdatedState;

    static HAPIPSession ipSessions[kHAPIPSessionStorage_NumElements];
    static uint8_t ipInboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_InboundBufferSize];
    static uint8_t ipOutboundBuffers[HAPArrayCount(ipSessions)][kHAPIPSession_OutboundBufferSize];
    static HAPIPEventNotificationRef ipEventNotifications[HAPArrayCount(ipSessions)][kAttributeCount];
    for (size_t i = 0; i < HAPArrayCount(ipSessions); i++) {
        ipSessions[i].inboundBuffer.bytes = ipInboundBuffers[i];
        ipSessions[i].inboundBuffer.numBytes = sizeof ipInboundBuffers[i];
        ipSessions[i].outboundBuffer.bytes = ipOutboundBuffers[i];
        ipSessions[i].outboundBuffer.numBytes = sizeof ipOutboundBuffers[i];
        ipSessions[i].eventNotifications = ipEventNotifications[i];
        ipSessions[i].numEventNotifications = HAPArrayCount(ipEventNotifications[i]);
    }

    static HAPIPReadContextRef ipReadContexts[kAttributeCount];
    static HAPIPWriteContextRef ipWriteContexts[kAttributeCount];
    static uint8_t ipScratchBuffer[kHAPIPSession_ScratchBufferSize];
    static HAPIPAccessoryServerStorage ipAccessoryServerStorage = {
        .sessions = ipSessions,
        .numSessions = HAPArrayCount(ipSessions),
        .readContexts = ipReadContexts,
        .numReadContexts = HAPArrayCount(ipReadContexts),
        .writeContexts = ipWriteContexts,
        .numWriteContexts = HAPArrayCount(ipWriteContexts),
        .scratchBuffer = {.bytes = ipScratchBuffer, .numBytes = sizeof ipScratchBuffer}};

    platform.hapAccessoryServerOptions.ip.transport = &kHAPAccessoryServerTransport_IP;
    platform.hapAccessoryServerOptions.ip.accessoryServerStorage = &ipAccessoryServerStorage;

    platform.hapPlatform.ip.tcpStreamManager = &platform.tcpStreamManager;
}

// Deinitialize global platform objects.
static void PlatformDeinitialize()
{
    HAPPlatformTCPStreamManagerRelease(&platform.tcpStreamManager);
    HAPPlatformRunLoopRelease();
}

// Restore platform specific factory settings.
void PlatformRestoreFactorySettings(void)
{
    // Ensure NWP is in station mode.
    sl_Start(NULL, NULL, NULL);
    sl_WlanSetMode(ROLE_STA);
    sl_Stop(kSimpleLink_StopTimeout);
    sl_Start(NULL, NULL, NULL);

    // Restore WLAN defaults.
    sl_WlanSet(SL_WLAN_CFG_GENERAL_PARAM_ID, SL_WLAN_GENERAL_PARAM_OPT_COUNTRY_CODE, 2, (unsigned char *)"US");
    sl_WlanPolicySet(SL_WLAN_POLICY_CONNECTION, SL_WLAN_CONNECTION_POLICY(1,1,0,0), NULL, 0); // Auto, Fast
    sl_WlanProvisioning(SL_WLAN_PROVISIONING_CMD_STOP, 0xFF, 0, NULL, 0x0);
    sl_NetCfgSet(SL_NETCFG_IPV4_STA_ADDR_MODE, SL_NETCFG_ADDR_DHCP, 0, NULL);
    sl_WlanPolicySet(SL_WLAN_POLICY_PM, SL_WLAN_NORMAL_POLICY, NULL, 0);

    // Remove stored profiles.
    sl_WlanProfileDel(SL_WLAN_DEL_ALL_PROFILES);

    // Add default connection profiles. WLAN macros are defined in config.h.
    sl_WlanProfileAdd((const signed char *)WLAN0_SSID, strlen(WLAN0_SSID), NULL,
                      &(const SlWlanSecParams_t) {
                          .Key = (signed char *)WLAN0_KEY,
                          .KeyLen = strlen(WLAN0_KEY),
                          .Type = SL_WLAN_SEC_TYPE_WPA_WPA2
                      }, NULL, 0, 0);
    
    sl_WlanProfileAdd((const signed char *)WLAN1_SSID, strlen(WLAN1_SSID), NULL,
                      &(const SlWlanSecParams_t) {
                          .Key = (signed char *)WLAN1_KEY,
                          .KeyLen = strlen(WLAN1_KEY),
                          .Type = SL_WLAN_SEC_TYPE_WPA_WPA2
                      }, NULL, 1, 0);
    
    // Restore NetApp defaults.
    const char *urn = "simplelink";
    sl_NetAppSet(SL_NETAPP_DEVICE_ID, SL_NETAPP_DEVICE_URN, strlen(urn), (uint8_t *)urn);

    // Flush the DNS cache.
    sl_NetAppSet(SL_NETAPP_DNS_CLIENT_ID, SL_NETAPP_DNS_CLIENT_CACHE_CLEAR, 0, NULL);

    // Restart the NWP.
    sl_Stop(kSimpleLink_StopTimeout);
    sl_Start(NULL, NULL, NULL);
}

// Either simply passes State handling to app, or processes Factory Reset
void HandleUpdatedState(HAPAccessoryServerRef *_Nonnull server, void *_Nullable context)
{
    HAPError err;

    if (HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && requestedFactoryReset) {
        HAPPrecondition(server);
        HAPLogInfo(&kHAPLog_Default, "A factory reset has been requested.");

        // Purge app state.
        err = HAPPlatformKeyValueStorePurgeDomain(&platform.keyValueStore, kAppKeyValueStoreDomain_Configuration);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Reset HomeKit state.
        // .homekitstore/90.10: Configuration_FirmwareVersion
        // .homekitstore/90.21: Configuration_LTSK
        // .homekitstore/90.20: Configuration_ConfigurationNumber
        // .homekitstore/a0.*:  Pairings
        err = HAPRestoreFactorySettings(&platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }

        // Restore platform specific factory settings.
        PlatformRestoreFactorySettings();
        AppRelease();
        requestedFactoryReset = false;
        AppCreate(server, &platform.keyValueStore);
        AppAccessoryServerStart();
        return;
    }
    else if (HAPAccessoryServerGetState(server) == kHAPAccessoryServerState_Idle && clearPairings) {
        HAPLogInfo(&kHAPLog_Default, "Removing pairings.");
        err = HAPRemoveAllPairings(&platform.keyValueStore);
        if (err) {
            HAPAssert(err == kHAPError_Unknown);
            HAPFatalError();
        }
        AppAccessoryServerStart();
    } else {
        AccessoryServerHandleUpdatedState(server, context);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Main task.
//----------------------------------------------------------------------------------------------------------------------

void MainTask(void *pvParameters)
{
    LED_Handle ledHandle = LED_open(BOARD_BLUE_LED, NULL);
    LED_startBlinking(ledHandle, 150, LED_BLINK_FOREVER);

    StartNetworkProcessor();

    // Wait for DHCP acquire.
    for (;;) {
        uint32_t notificationValue = 0;
        if (xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &notificationValue, portMAX_DELAY) == pdPASS) {
            if ((notificationValue & kHAPPlatformEvent_IPAcquired) != 0) {
                break;
            }
        }
    }
    
    PrintDeviceInfo();

    PlatformInitialize();

    // Perform Application-specific initializations such as setting up callbacks
    // and configure any additional unique platform dependencies.
    AppInitialize(&platform.hapAccessoryServerOptions, &platform.hapPlatform, &platform.hapAccessoryServerCallbacks);

    // Initialize accessory server.
    HAPAccessoryServerCreate(
        &accessoryServer,
        &platform.hapAccessoryServerOptions,
        &platform.hapPlatform,
        &platform.hapAccessoryServerCallbacks,
        NULL);
    
    AppCreate(&accessoryServer, &platform.keyValueStore);
    AppAccessoryServerStart();

    // Image should be operational at this point. If we have an OTA image pending
    // commit, stop the watchdog timer and accept.
    switch (HAPPlatformOTAGetImageState(NULL)) {
    case kHAPPlatformOTAPALImageState_PendingCommit:
        HAPPlatformOTASetImageState(NULL, kHAPPlatformOTAImageState_Accepted);
        break;
    default:
        break;
    }

    LED_stopBlinking(ledHandle);

    // Run main loop until explicitly stopped.
    HAPPlatformRunLoopRun();

    // Run loop stopped. Suspend the UART task.
    vTaskSuspend(uartTaskHandle);

    // Cleanup.
    AppAccessoryServerStop();
    AppRelease();
    AppDeinitialize();
    PlatformDeinitialize();

    // Unblock the HTTP task for OTA.
    xTaskNotifyGive(httpTaskHandle);

    vTaskSuspend(NULL);
}

//----------------------------------------------------------------------------------------------------------------------
// FreeRTOS hooks.
//----------------------------------------------------------------------------------------------------------------------

void vApplicationMallocFailedHook(void)
{
    HAPLogFault(&kHAPLog_Default, "pvPortMalloc failed (%u bytes free).", xPortGetFreeHeapSize());
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    while (1) {}
}

//----------------------------------------------------------------------------------------------------------------------
// Application entry point.
//----------------------------------------------------------------------------------------------------------------------

int main(void)
{
    BaseType_t rc;
    Board_init();
    
    unsigned long sysResetCause = PRCMSysResetCauseGet();
    switch (sysResetCause) {
    case PRCM_POWER_ON:   HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_POWER_ON).");      break;
    case PRCM_LPDS_EXIT:  HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_LPDS_EXIT).");     break;
    case PRCM_CORE_RESET: HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_CORE_RESET).");    break;
    case PRCM_MCU_RESET:  HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_MCU_RESET).");     break;
    case PRCM_WDT_RESET:  HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_WDT_RESET).");     break;
    case PRCM_SOC_RESET:  HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_SOC_RESET).");     break;
    case PRCM_HIB_EXIT:   HAPLogInfo(&kHAPLog_Default, "Reset (PRCM_HIB_EXIT).");      break;
    default:              HAPLogInfo(&kHAPLog_Default, "Reset (%lu).", sysResetCause); break;
    }

    // Enable SEGGER SystemView.
    //traceSTART();

    // Create the main application task.
    rc = xTaskCreate((TaskFunction_t)MainTask, // pvTaskCode
                     "Main",                   // pcName
                     kApp_MainTaskStackSize,   // usStackDepth
                     NULL,                     // pvParameters
                     kApp_MainTaskPriority,    // uxPriority
                     &mainTaskHandle);         // pxCreatedTask
    
    if (rc != pdPASS) {
        HAPLogFault(&kHAPLog_Default, "Failed to create main task.");
        HAPFatalError();
    }
    
    // Create the UART manager task.
    rc = xTaskCreate((TaskFunction_t)UARTTask, // pvTaskCode
                     "UART",                   // pcName
                     kApp_UARTTaskStackSize,   // usStackDepth
                     NULL,                     // pvParameters
                     kApp_UARTTaskPriority,    // uxPriority
                     &uartTaskHandle);         // pxCreatedTask

    if (rc != pdPASS) {
        HAPLogFault(&kHAPLog_Default, "Failed to create UART task.");
        HAPFatalError();
    }

    // Create the HTTP server task.
    rc = xTaskCreate((TaskFunction_t)HTTPTask, // pvTaskCode
                     "HTTP",                   // pcName
                     kApp_HTTPTaskStackSize,   // usStackDepth
                     NULL,                     // pvParameters
                     kApp_HTTPTaskPriority,    // uxPriority
                     &httpTaskHandle);         // pxCreatedTask

    if (rc != pdPASS) {
        HAPLogFault(&kHAPLog_Default, "Failed to create HTTP task.");
        HAPFatalError();
    }

    vTaskStartScheduler();

    while (1) {}
}
