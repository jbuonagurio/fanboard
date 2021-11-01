//  Copyright 2021 John Buonagurio
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "Event.h"

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

void SimpleLinkFatalErrorEventHandler(SlDeviceFatal_t *pSlFatalErrorEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlDeviceFatal" };

    switch (pSlFatalErrorEvent->Id) {
    case SL_DEVICE_EVENT_FATAL_DEVICE_ABORT:
        HAPLogError(&logObject, "Device Abort (0x%lX)",
                    pSlFatalErrorEvent->Data.DeviceAssert.Code);
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_DRIVER_ABORT:
        HAPLogError(&logObject, "Driver Abort");
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_NO_CMD_ACK:
        HAPLogError(&logObject, "No Command Ack (0x%lX)",
                    pSlFatalErrorEvent->Data.NoCmdAck.Code);
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_SYNC_LOSS:
        HAPLogError(&logObject, "Sync Loss");
        HAPFatalError();
        break;
    case SL_DEVICE_EVENT_FATAL_CMD_TIMEOUT:
        HAPLogError(&logObject, "Command Timeout (0x%lX)",
                    pSlFatalErrorEvent->Data.CmdTimeout.Code);
        break;
    default:
        HAPLogError(&logObject, "Unknown Event (0x%lX)", pSlFatalErrorEvent->Id);
        HAPFatalError();
        break;
    }
}

void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pSlDeviceEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlDeviceEvent" };

    HAPLogInfo(&logObject, "Unknown Event (0x%lX)", pSlDeviceEvent->Id);
}

static void InitializeNetworkServices()
{
    SlNetIf_init(0);
    SlNetIf_add(SLNETIF_ID_1,                                 // ifID
                "wlan0",                                      // ifName
                (const SlNetIf_Config_t *)&SlNetIfConfigWifi, // ifConf
                kSimpleLink_InterfacePriority);               // priority
    SlNetSock_init(0);
    SlNetUtil_init(0);
}

void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pSlNetAppEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlNetAppEvent" };

    switch (pSlNetAppEvent->Id) {
    case SL_NETAPP_EVENT_IPV4_ACQUIRED:
        HAPLogInfo(&logObject, "IPv4 Acquired (%u.%u.%u.%u)",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpAcquiredV4.Ip, 0));
        InitializeNetworkServices();
        xTaskNotifyIndexed(mainTaskHandle, 0, kApplicationEvent_IPAcquired, eSetValueWithOverwrite);
        break;
    case SL_NETAPP_EVENT_IPV6_ACQUIRED:
        HAPLogInfo(&logObject, "IPv6 Acquired (%04lX:%04lX:%04lX:%04lX)",
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[0],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[1],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[2],
            (unsigned long)pSlNetAppEvent->Data.IpAcquiredV6.Ip[3]);
        InitializeNetworkServices();
        xTaskNotifyIndexed(mainTaskHandle, 0, kApplicationEvent_IPAcquired, eSetValueWithOverwrite);
        break;
    case SL_NETAPP_EVENT_IP_COLLISION:
        HAPLogInfo(&logObject, "IP Collision");
        break;
    case SL_NETAPP_EVENT_DHCPV4_LEASED:
        HAPLogInfo(&logObject, "DHCPv4 Leased (%u.%u.%u.%u)",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpLeased.IpAddress, 0));
        break;
    case SL_NETAPP_EVENT_DHCPV4_RELEASED:
        HAPLogInfo(&logObject, "DHCPv4 Released (%u.%u.%u.%u)",
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 3),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 2),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 1),
            (unsigned short)SL_IPV4_BYTE(pSlNetAppEvent->Data.IpReleased.IpAddress, 0));
        break;
    case SL_NETAPP_EVENT_HTTP_TOKEN_GET:
        HAPLogInfo(&logObject, "HTTP Token Get");
        break;
    case SL_NETAPP_EVENT_HTTP_TOKEN_POST:
        HAPLogInfo(&logObject, "HTTP Token Post");
        break;
    case SL_NETAPP_EVENT_IPV4_LOST:
        HAPLogInfo(&logObject, "IPv4 Lost");
        break;
    case SL_NETAPP_EVENT_DHCP_IPV4_ACQUIRE_TIMEOUT:
        HAPLogInfo(&logObject, "DHCP IPv4 Acquire Timeout");
        break;
    case SL_NETAPP_EVENT_IPV6_LOST:
        HAPLogInfo(&logObject, "IPv6 Lost");
        break;
    case SL_NETAPP_EVENT_NO_IPV4_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No IPv4 Collision Detected");
        break;
    case SL_NETAPP_EVENT_NO_LOCAL_IPV6_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No Local IPv6 Collision Detected");
        break;
    case SL_NETAPP_EVENT_NO_GLOBAL_IPV6_COLLISION_DETECTED:
        HAPLogInfo(&logObject, "No Global IPv6 Collision Detected");
        break;
    default:
        // Receiving spurious events with ID 0x10187B; SL_OPCODE_NETAPP_RECEIVE?
        HAPLogInfo(&logObject, "Unknown Event (0x%lX)", pSlNetAppEvent->Id);
        break;
    }
}

void SimpleLinkHttpServerEventHandler(SlNetAppHttpServerEvent_t *pSlHttpServerEvent,
                                      SlNetAppHttpServerResponse_t *pSlHttpServerResponse) {}

void SimpleLinkNetAppRequestEventHandler(SlNetAppRequest_t *pNetAppRequest,
                                         SlNetAppResponse_t *pNetAppResponse)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlNetAppRequest" };

    switch (pNetAppRequest->Type) {
    case SL_NETAPP_REQUEST_HTTP_GET:
        HAPLogInfo(&logObject, "HTTP GET");
        break;
    case SL_NETAPP_REQUEST_HTTP_POST:
        HAPLogInfo(&logObject, "HTTP POST");
        break;
    case SL_NETAPP_REQUEST_HTTP_PUT:
        HAPLogInfo(&logObject, "HTTP PUT");
        break;
    case SL_NETAPP_REQUEST_HTTP_DELETE:
        HAPLogInfo(&logObject, "HTTP DELETE");
        break;
    default:
        break;
    }
}

void SimpleLinkNetAppRequestMemFreeEventHandler(uint8_t *buffer) {}

void SimpleLinkWlanEventHandler(SlWlanEvent_t *pSlWlanEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlWlanEvent" };

    switch (pSlWlanEvent->Id) {
    case SL_WLAN_EVENT_CONNECT:
        pSlWlanEvent->Data.Connect.SsidName[pSlWlanEvent->Data.Connect.SsidLen] = '\0';
        HAPLogInfo(&logObject, "Connect (SSID=%s)",
                   pSlWlanEvent->Data.Connect.SsidName);
        xTaskNotifyIndexed(mainTaskHandle, 0, kApplicationEvent_Connect, eSetValueWithOverwrite);
        break;
    case SL_WLAN_EVENT_DISCONNECT:
        HAPLogInfo(&logObject, "Disconnect (ReasonCode=%d)",
                   pSlWlanEvent->Data.Disconnect.ReasonCode);
        xTaskNotifyIndexed(mainTaskHandle, 0, kApplicationEvent_Disconnect, eSetValueWithOverwrite);
        break;
    case SL_WLAN_EVENT_STA_ADDED:
        // Client connected in AP mode.
        HAPLogInfo(&logObject, "STA Added (%02X:%02X:%02X:%02X:%02X:%02X)",
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[0],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[1],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[2],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[3],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[4],
                   (unsigned short)pSlWlanEvent->Data.STAAdded.Mac[5]);
        break;
    case SL_WLAN_EVENT_STA_REMOVED:
        // Client disconnected in AP mode.
        HAPLogInfo(&logObject, "STA Removed (%02X:%02X:%02X:%02X:%02X:%02X)",
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[0],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[1],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[2],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[3],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[4],
                   (unsigned short)pSlWlanEvent->Data.STARemoved.Mac[5]);
        break;
    case SL_WLAN_EVENT_P2P_CONNECT:
        HAPLogInfo(&logObject, "P2P Connect");
        break;
    case SL_WLAN_EVENT_P2P_DISCONNECT:
        HAPLogInfo(&logObject, "P2P Disconnect");
        break;
    case SL_WLAN_EVENT_P2P_CLIENT_ADDED:
        HAPLogInfo(&logObject, "P2P Client Added");
        break;
    case SL_WLAN_EVENT_P2P_CLIENT_REMOVED:
        HAPLogInfo(&logObject, "P2P Client Removed");
        break;
    case SL_WLAN_EVENT_P2P_DEVFOUND:
        HAPLogInfo(&logObject, "P2P Device Found");
        break;
    case SL_WLAN_EVENT_P2P_REQUEST:
        HAPLogInfo(&logObject, "P2P Request");
        break;
    case SL_WLAN_EVENT_P2P_CONNECTFAIL:
        HAPLogInfo(&logObject, "P2P Connect Fail");
        break;
    case SL_WLAN_EVENT_RXFILTER:
        HAPLogInfo(&logObject, "RX Filter");
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
            HAPLogInfo(&logObject, "Provisioning Status ID 0x%02X",
                       (unsigned short)pSlWlanEvent->Data.ProvisioningStatus.ProvisioningStatus);
            break;
        }
        break;
    case SL_WLAN_EVENT_PROVISIONING_PROFILE_ADDED:
        pSlWlanEvent->Data.ProvisioningProfileAdded.Ssid[pSlWlanEvent->Data.ProvisioningProfileAdded.SsidLen] = '\0';
        HAPLogInfo(&logObject, "Provisioning Profile Added (SSID=%s)",
                   pSlWlanEvent->Data.ProvisioningProfileAdded.Ssid);
        break;
    case SL_WLAN_EVENT_LINK_QUALITY_TRIGGER:
        HAPLogInfo(&logObject, "Link Quality Trigger (RSSI=%u)",
                   (unsigned short)pSlWlanEvent->Data.LinkQualityTrigger.Data);
        break;
    default:
        HAPLogInfo(&logObject, "Unknown Event (0x%lX)", pSlWlanEvent->Id);
        break;
    }
}

void SimpleLinkSockEventHandler(SlSockEvent_t *pSlSockEvent)
{
    static const HAPLogObject logObject = { .subsystem = kHAPPlatform_LogSubsystem, .category = "SlSockEvent" };

    switch (pSlSockEvent->Event) {
    case SL_SOCKET_TX_FAILED_EVENT:
        HAPLogInfo(&logObject, "TX Failed (%d)",
                   pSlSockEvent->SocketAsyncEvent.SockTxFailData.Status);
        break;
    case SL_SOCKET_ASYNC_EVENT:
        HAPLogInfo(&logObject, "Async Event");
        break;
    default:
        HAPLogInfo(&logObject, "Unknown Event (0x%lX)", pSlSockEvent->Event);
        break;
    }
}
