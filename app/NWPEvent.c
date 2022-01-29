//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "NWPEvent.h"
#include "HTTPServer.h"

#include <stdint.h>

#include <HAP.h>
#include <HAPPlatform+Init.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#define kSimpleLink_InterfacePriority (5)

extern TaskHandle_t mainTaskHandle;

static void InitializeNetworkModules()
{
    SlNetIf_init(0);
    SlNetIf_add(SLNETIF_ID_1, "wlan0", (const SlNetIf_Config_t *)&SlNetIfConfigWifi, kSimpleLink_InterfacePriority);
    SlNetSock_init(0);
    SlNetUtil_init(0);
}

void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *pSlFatalErrorEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Device" };

    switch (pSlFatalErrorEvent->Id) {
    case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
        HAPLogFault(&logObject, "Device abort (0x%lX).",
                    pSlFatalErrorEvent->Data.DeviceAssert.Code);
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
        HAPLogFault(&logObject, "Driver abort.");
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
        HAPLogFault(&logObject, "No command ack (0x%lX).",
                    pSlFatalErrorEvent->Data.NoCmdAck.Code);
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
        HAPLogFault(&logObject, "Sync loss.");
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
        HAPLogFault(&logObject, "Command timeout (0x%lX).",
                    pSlFatalErrorEvent->Data.CmdTimeout.Code);
        break;
    default:
        HAPLogFault(&logObject, "Unknown event (0x%lX).", pSlFatalErrorEvent->Id);
        HAPFatalError();
        break;
    }
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pSlDeviceEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Device" };
    
    switch (pSlDeviceEvent->Id) {
    case SL_DEVICE_EVENT_RESET_REQUEST:
        HAPLogInfo(&logObject, "Device reset request (Status=%d, Caller=%u).",
                   pSlDeviceEvent->Data.ResetRequest.Status,
                   pSlDeviceEvent->Data.ResetRequest.Caller);
        break;
    case SL_DEVICE_EVENT_ERROR:
        HAPLogError(&logObject, "Device error (Source=0x%02X, Code=%d).",
                    (unsigned short)pSlDeviceEvent->Data.Error.Source,
                    pSlDeviceEvent->Data.Error.Code);
        break;
    default:
        HAPLogInfo(&logObject, "Unknown event (0x%lX).", pSlDeviceEvent->Id);
        break;
    }
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pSlNetAppEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "NetApp" };
    
    switch (pSlNetAppEvent->Id) {
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        HAPLogInfo(&logObject, "IPv4 acquired (%u.%u.%u.%u).",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 0));
        InitializeNetworkModules();
        xTaskNotifyIndexed(mainTaskHandle, 0, kHAPPlatformEvent_IPAcquired, eSetValueWithOverwrite);
        break;
    case SL_NETAPP_EVENT_IPV6_ACQUIRED:
        HAPLogInfo(&logObject, "IPv6 acquired (%04lX:%04lX:%04lX:%04lX).",
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[0],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[1],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[2],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[3]);
        InitializeNetworkModules();
        xTaskNotifyIndexed(mainTaskHandle, 0, kHAPPlatformEvent_IPAcquired, eSetValueWithOverwrite);
        break;
    case SL_NETAPP_EVENT_IP_COLLISION:
        HAPLogInfo(&logObject, "IP collision.");
        break;
    case SL_NETAPP_EVENT_DHCPV4_LEASED:
        HAPLogInfo(&logObject, "DHCPv4 leased (%u.%u.%u.%u).",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 0));
        break;
    case SL_NETAPP_EVENT_DHCPV4_RELEASED:
        HAPLogInfo(&logObject, "DHCPv4 released (%u.%u.%u.%u).",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 0));
        break;
    case SL_NETAPP_EVENT_HTTP_TOKEN_GET:
        break;
    case SL_NETAPP_EVENT_HTTP_TOKEN_POST:
        break;
    case SL_NETAPP_EVENT_IPV4_LOST:
        HAPLogInfo(&logObject, "IPv4 lost.");
        break;
    case SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT:
        HAPLogInfo(&logObject, "DHCP IPv4 acquire timeout.");
        break;
    case SL_NETAPP_EVENT_IPV6_LOST:
        HAPLogInfo(&logObject, "IPv6 lost.");
        break;
    case SL_NETAPP_EVENT_NO_IPV4_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No IPv4 collision detected.");
        break;
    case SL_NETAPP_EVENT_NO_LOCAL_IPV6_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No local IPv6 collision detected.");
        break;
    case SL_NETAPP_EVENT_NO_GLOBAL_IPV6_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No global IPv6 collision detected.");
        break;
    default:
        // Receiving spurious events with ID 0x10187B; SL_OPCODE_NETAPP_RECEIVE?
        HAPLogInfo(&logObject, "Unknown event (0x%lX).", pSlNetAppEvent->Id);
        break;
    }
}

void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pSlHttpServerEvent,
                                      SlNetAppHttpServerResponse_t *pSlHttpServerResponse)
{
    // Not implemented.
}

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse)
{
    // Pass event to application context.
    HTTPRequestHandler(pNetAppRequest, pNetAppResponse);
}

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer)
{
    // Not implemented.
}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "WLAN" };

    switch (pSlWlanEvent->Id) {
    case SL_WLAN_EVENT_CONNECT:
        pSlWlanEvent->Data.Connect.SsidName[pSlWlanEvent->Data.Connect.SsidLen] = '\0';
        HAPLogInfo(&logObject, "Connect (SSID=%s).",
                   pSlWlanEvent->Data.Connect.SsidName);
        xTaskNotifyIndexed(mainTaskHandle, 0, kHAPPlatformEvent_Connected, eSetValueWithOverwrite);
        break;
    case SL_WLAN_EVENT_DISCONNECT:
        HAPLogInfo(&logObject, "Disconnect (ReasonCode=%d).",
                   pSlWlanEvent->Data.Disconnect.ReasonCode);
        xTaskNotifyIndexed(mainTaskHandle, 0, kHAPPlatformEvent_Disconnected, eSetValueWithOverwrite);
        break;
    case SL_WLAN_EVENT_STA_ADDED:
        // Client connected in AP mode.
        HAPLogInfo(&logObject, "STA added (%02X:%02X:%02X:%02X:%02X:%02X).",
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[0],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[1],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[2],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[3],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[4],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[5]);
        break;
    case SL_WLAN_EVENT_STA_REMOVED:
        // Client disconnected in AP mode.
        HAPLogInfo(&logObject, "STA removed (%02X:%02X:%02X:%02X:%02X:%02X).",
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[0],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[1],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[2],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[3],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[4],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[5]);
        break;
    case SL_WLAN_EVENT_P2P_CONNECT:
        HAPLogInfo(&logObject, "P2P connect.");
        break;
    case SL_WLAN_EVENT_P2P_DISCONNECT:
        HAPLogInfo(&logObject, "P2P disconnect.");
        break;
    case SL_WLAN_EVENT_P2P_CLIENT_ADDED:
        HAPLogInfo(&logObject, "P2P client added.");
        break;
    case SL_WLAN_EVENT_P2P_CLIENT_REMOVED:
        HAPLogInfo(&logObject, "P2P client removed.");
        break;
    case SL_WLAN_EVENT_P2P_DEVFOUND:
        HAPLogInfo(&logObject, "P2P device found.");
        break;
    case SL_WLAN_EVENT_P2P_REQUEST:
        HAPLogInfo(&logObject, "P2P request.");
        break;
    case SL_WLAN_EVENT_P2P_CONNECTFAIL:
        HAPLogInfo(&logObject, "P2P connect failed.");
        break;
    case SL_WLAN_EVENT_RXFILTER:
        HAPLogInfo(&logObject, "RX filter.");
        break;
    case SL_WLAN_EVENT_PROVISIONING_STATUS:
        switch (pSlWlanEvent->Data.ProvisioningStatus.ProvisioningStatus) {
        case SL_WLAN_PROVISIONING_GENERAL_ERROR:
        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_NETWORK_NOT_FOUND:
        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_FAIL_CONNECTION_FAILED:
        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_CONNECTION_SUCCESS_IP_NOT_ACQUIRED:
        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS_FEEDBACK_FAILED:
        case SL_WLAN_PROVISIONING_CONFIRMATION_STATUS_SUCCESS:
        case SL_WLAN_PROVISIONING_ERROR_ABORT:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_INVALID_PARAM:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_HTTP_SERVER_DISABLED:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_PROFILE_LIST_FULL:
        case SL_WLAN_PROVISIONING_ERROR_ABORT_PROVISIONING_ALREADY_STARTED:
        case SL_WLAN_PROVISIONING_AUTO_STARTED:
        case SL_WLAN_PROVISIONING_STOPPED:
        case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNCED:
        case SL_WLAN_PROVISIONING_SMART_CONFIG_SYNC_TIMEOUT:
        case SL_WLAN_PROVISIONING_CONFIRMATION_WLAN_CONNECT:
        case SL_WLAN_PROVISIONING_CONFIRMATION_IP_ACQUIRED:
        case SL_WLAN_PROVISIONING_EXTERNAL_CONFIGURATION_READY:
        default:
            HAPLogInfo(&logObject, "Provisioning status (0x%02X).",
                       (unsigned short)pSlWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
            break;
        }
        break;
    case SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED:
        pSlWlanEvent->Data.ProvisioningProfileAdded.Ssid[pSlWlanEvent->Data.ProvisioningProfileAdded.SsidLen] = '\0';
        HAPLogInfo(&logObject, "Provisioning profile added (SSID=%s).",
                   pSlWlanEvent->Data.ProvisioningProfileAdded.Ssid);
        break;
    case SL_WLAN_EVENT_LINK_QUALITY_TRIGGER:
        HAPLogInfo(&logObject, "Link quality trigger (RSSI=%u).",
                   (unsigned short)pSlWlanEvent->Data.LinkQualityTrigger.Data);
        break;
    default:
        HAPLogInfo(&logObject, "Unknown event (0x%lX).", pSlWlanEvent->Id);
        break;
    }
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSlSockEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "Socket" };

    switch (pSlSockEvent->Event) {
    case SL_SOCKET_TX_FAILED_EVENT:
        HAPLogInfo(&logObject, "TX failed (%d).",
                   pSlSockEvent->SocketAsyncEvent.SockTxFailData.Status);
        break;
    case SL_SOCKET_ASYNC_EVENT:
        HAPLogInfo(&logObject, "Async event.");
        break;
    default:
        HAPLogInfo(&logObject, "Unknown event (0x%lX)", pSlSockEvent->Event);
        break;
    }
}
