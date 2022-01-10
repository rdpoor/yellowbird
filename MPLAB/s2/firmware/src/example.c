/*******************************************************************************
  WINC Example Application

  File Name:
    example.c

  Summary:
    WINC scan and connection example

  Description:
     This example performs the following steps:
        1) Scans all channels looking for the specified BSS
        2) Displays all found BSSs
        3) Connects to the specified BSS
        4) Sends one or more ICMP echo requests to a known IP address

    The configuration options for this example are:
        WLAN_SSID           -- BSS to search for
        WLAN_AUTH           -- Security for the BSS
        WLAN_PSK            -- Passphrase for WPA security
        ICMP_ECHO_TARGET    -- IP address to send ICMP echo request to
        ICMP_ECHO_COUNT     -- Number of ICMP echo requests to send
        ICMP_ECHO_INTERVAL  -- Time between ICMP echo requests, in milliseconds
*******************************************************************************/

/*******************************************************************************
 * Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/

#include "example.h"

#include "app.h"
#include "wdrv_winc_client_api.h"

// *****************************************************************************
// Private types and definitions

#define WLAN_SSID "pichincha"
#define WLAN_AUTH_WPA_PSK M2M_WIFI_SEC_WPA_PSK
#define WLAN_PSK "robandmarisol"

//#define ICMP_ECHO_TARGET    "192.168.1.1"
//#define ICMP_ECHO_COUNT     3
//#define ICMP_ECHO_INTERVAL  100

#define DEFINE_STATES                                                          \
  DEFINE_STATE(EXAMP_STATE_INIT)                                               \
  DEFINE_STATE(EXAMP_STATE_SCANNING)                                           \
  DEFINE_STATE(EXAMP_STATE_SCAN_GET_RESULTS)                                   \
  DEFINE_STATE(EXAMP_STATE_SCAN_DONE)                                          \
  DEFINE_STATE(EXAMP_STATE_CONNECTING)                                         \
  DEFINE_STATE(EXAMP_STATE_CONNECTED)                                          \
  DEFINE_STATE(EXAMP_STATE_ICMP_ECHO_REQUEST_SENT)                             \
  DEFINE_STATE(EXAMP_STATE_DISCONNECTED)                                       \
  DEFINE_STATE(EXAMP_STATE_ERROR)

#undef DEFINE_STATE
#define DEFINE_STATE(_name) _name,
typedef enum { DEFINE_STATES } EXAMP_STATES;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the state to the new state.
 */
static void APP_ExampleSetState(EXAMP_STATES new_state);

/**
 * @brief Return the state name as a string.
 */
static const char *APP_ExampleStateName(EXAMP_STATES state);

#ifdef ICMP_ECHO_TARGET
static void
APP_ExampleICMPEchoResponseCallback(DRV_HANDLE handle,
                                    uint32_t ipAddress,
                                    uint32_t RTT,
                                    WDRV_WINC_ICMP_ECHO_STATUS statusCode);
#endif

static void APP_ExampleDHCPAddressEventCallback(DRV_HANDLE handle,
                                                uint32_t ipAddress);

static void APP_ExampleConnectNotifyCallback(DRV_HANDLE handle,
                                             WDRV_WINC_ASSOC_HANDLE assocHandle,
                                             WDRV_WINC_CONN_STATE currentState,
                                             WDRV_WINC_CONN_ERROR errorCode);

// *****************************************************************************
// Private (static) storage

#undef DEFINE_STATE
#define DEFINE_STATE(_name) #_name,
static const char *exampStateNames[] = {DEFINE_STATES};

static EXAMP_STATES s_state;
static bool foundBSS;
#ifdef ICMP_ECHO_TARGET
static uint8_t icmpEchoCount;
static uint32_t icmpEchoReplyTime;
#endif

// *****************************************************************************
// Public code

void APP_ExampleInitialize(DRV_HANDLE handle) {
  SYS_CONSOLE_PRINT("\n=========================");
  SYS_CONSOLE_PRINT("\nWINC AP Scan Example");
  SYS_CONSOLE_PRINT("\n=========================");

  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_DEBUG); // more chatty, please...
  APP_ExampleSetState(EXAMP_STATE_INIT);
}

void APP_ExampleTasks(DRV_HANDLE handle) {
  WDRV_WINC_STATUS status;

  switch (s_state) {
  case EXAMP_STATE_INIT: {
    /* Enable use of DHCP for network configuration, DHCP is the default
     but this also registers the callback for notifications. */

    WDRV_WINC_IPUseDHCPSet(handle, &APP_ExampleDHCPAddressEventCallback);

    /* Start a BSS find operation on all channels. */

    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_BSSFindFirst(
            handle, WDRV_WINC_ALL_CHANNELS, true, NULL, NULL)) {
      APP_ExampleSetState(EXAMP_STATE_SCANNING);
      foundBSS = false;
    }
    break;
  }

  case EXAMP_STATE_SCANNING: {
    /* Wait for BSS find operation to complete, then report the number
     of results found. */

    if (false == WDRV_WINC_BSSFindInProgress(handle)) {
      SYS_CONSOLE_PRINT("\nScan complete, %d AP(s) found",
                        WDRV_WINC_BSSFindGetNumBSSResults(handle));
      APP_ExampleSetState(EXAMP_STATE_SCAN_GET_RESULTS);
    }
    break;
  }

  case EXAMP_STATE_SCAN_GET_RESULTS: {
    WDRV_WINC_BSS_INFO BSSInfo;

    /* Request the current BSS find results. */

    if (WDRV_WINC_STATUS_OK == WDRV_WINC_BSSFindGetInfo(handle, &BSSInfo)) {
      SYS_CONSOLE_PRINT(
          "\nAP found: RSSI: %d %s", BSSInfo.rssi, BSSInfo.ctx.ssid.name);

      /* Check if this SSID matches the search target SSID. */

      if (((sizeof(WLAN_SSID) - 1) == BSSInfo.ctx.ssid.length) &&
          (0 ==
           memcmp(BSSInfo.ctx.ssid.name, WLAN_SSID, BSSInfo.ctx.ssid.length))) {
        foundBSS = true;
      }

      /* Request the next set of BSS find results. */

      status = WDRV_WINC_BSSFindNext(handle, NULL);

      if (WDRV_WINC_STATUS_BSS_FIND_END == status) {
        /* If there are no more results available check if the target
         SSID has been found. */

        if (true == foundBSS) {
          SYS_CONSOLE_PRINT("\nTarget AP found, trying to connect");
          APP_ExampleSetState(EXAMP_STATE_SCAN_DONE);
        } else {
          SYS_CONSOLE_PRINT("\nTarget BSS not found");
          APP_ExampleSetState(EXAMP_STATE_ERROR);
        }
      } else if ((WDRV_WINC_STATUS_NOT_OPEN == status) ||
                 (WDRV_WINC_STATUS_INVALID_ARG == status)) {
        /* An error occurred requesting results. */

        APP_ExampleSetState(EXAMP_STATE_ERROR);
      }
    }
    break;
  }

  case EXAMP_STATE_SCAN_DONE: {
    WDRV_WINC_AUTH_CONTEXT authCtx;
    WDRV_WINC_BSS_CONTEXT bssCtx;

    /* Initialize the BSS context to use default values. */

    if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetDefaults(&bssCtx)) {
      APP_ExampleSetState(EXAMP_STATE_ERROR);
      break;
    }

    /* Update BSS context with target SSID for connection. */

    if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetSSID(&bssCtx,
                                                       (uint8_t *)WLAN_SSID,
                                                       strlen(WLAN_SSID))) {
      APP_ExampleSetState(EXAMP_STATE_ERROR);
      break;
    }

#if defined(WLAN_AUTH_OPEN)
    /* Initialize the authentication context for open mode. */

    if (WDRV_WINC_STATUS_OK != WDRV_WINC_AuthCtxSetOpen(&authCtx)) {
      APP_ExampleSetState(EXAMP_STATE_ERROR);
      break;
    }
#elif defined(WLAN_AUTH_WPA_PSK)
    /* Initialize the authentication context for WPA. */

    if (WDRV_WINC_STATUS_OK != WDRV_WINC_AuthCtxSetWPA(&authCtx,
                                                       (uint8_t *)WLAN_PSK,
                                                       strlen(WLAN_PSK))) {
      APP_ExampleSetState(EXAMP_STATE_ERROR);
      break;
    }
#endif

    /* Connect to the target BSS with the chosen authentication. */

    if (WDRV_WINC_STATUS_OK !=
        WDRV_WINC_BSSConnect(
            handle, &bssCtx, &authCtx, &APP_ExampleConnectNotifyCallback)) {
      APP_ExampleSetState(EXAMP_STATE_ERROR);
      break;
    }
    APP_ExampleSetState(EXAMP_STATE_CONNECTING);
    break;
  }

  case EXAMP_STATE_CONNECTING: {
    /* Wait for the BSS connect to trigger the callback and update
     the connection state. */
    break;
  }

  case EXAMP_STATE_CONNECTED: {
    /* Wait for the IP link to become active. */

    if (false == WDRV_WINC_IPLinkActive(handle)) {
      break;
    }
#ifdef ICMP_ECHO_TARGET
    /* Send an ICMP echo request to the target IP address. */

    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_ICMPEchoRequest(handle,
                                  inet_addr(ICMP_ECHO_TARGET),
                                  0,
                                  &APP_ExampleICMPEchoResponseCallback)) {
      SYS_CONSOLE_PRINT("\nICMP echo request sent");

      APP_ExampleSetState(EXAMP_STATE_ICMP_ECHO_REQUEST_SENT);
    }
#endif
    break;
  }

  case EXAMP_STATE_ICMP_ECHO_REQUEST_SENT: {
#ifdef ICMP_ECHO_TARGET
    /* Wait for the callback to set the ICMP echo reply time. */

    if (0 != icmpEchoReplyTime) {
      /* Wait for the requested number of milliseconds before returning
       to connected state to send the next ICMP echo request. */

      uint32_t icmpEchoIntervalTime =
          (ICMP_ECHO_INTERVAL * SYS_TMR_TickCounterFrequencyGet()) / 1000ul;

      if ((SYS_TMR_TickCountGet() - icmpEchoReplyTime) >=
          icmpEchoIntervalTime) {
        APP_ExampleSetState(EXAMP_STATE_CONNECTED);
        icmpEchoReplyTime = 0;
      }
    }
#endif
    break;
  }

  case EXAMP_STATE_DISCONNECTED: {
    /* If we become disconnected, trigger a reconnection. */

    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_BSSReconnect(handle, &APP_ExampleConnectNotifyCallback)) {
      APP_ExampleSetState(EXAMP_STATE_CONNECTING);
    }
    break;
  }

  case EXAMP_STATE_ERROR: {
    break;
  }

  default: {
    break;
  }
  }
}

// *****************************************************************************
// Private (static) code

static void APP_ExampleSetState(EXAMP_STATES new_state) {
  if (new_state != s_state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "\n%s => %s",
                    APP_ExampleStateName(s_state),
                    APP_ExampleStateName(new_state));
    s_state = new_state;
  }
}

static const char *APP_ExampleStateName(EXAMP_STATES state) {
  if (state < sizeof(exampStateNames) / sizeof(exampStateNames[0])) {
    return exampStateNames[state];
  } else {
    return "unknown state";
  }
}

#ifdef ICMP_ECHO_TARGET
static void
APP_ExampleICMPEchoResponseCallback(DRV_HANDLE handle,
                                    uint32_t ipAddress,
                                    uint32_t RTT,
                                    WDRV_WINC_ICMP_ECHO_STATUS statusCode) {
  /* Report the results of the ICMP echo request. */

  switch (statusCode) {
  case WDRV_WINC_ICMP_ECHO_STATUS_SUCCESS: {
    SYS_CONSOLE_PRINT("\nICMP echo request successful; RTT = %ld ms", RTT);
    break;
  }

  case WDRV_WINC_ICMP_ECHO_STATUS_UNREACH: {
    SYS_CONSOLE_PRINT("\nICMP echo request failed; destination unreachable");
    break;
  }

  case WDRV_WINC_ICMP_ECHO_STATUS_TIMEOUT: {
    SYS_CONSOLE_PRINT("\nICMP echo request failed; timeout");
    break;
  }
  }

  /* Reduce the number of ICMP echo requests remaining and store the time
   this response was received. */

  icmpEchoCount--;

  if (icmpEchoCount > 0) {
    icmpEchoReplyTime = SYS_TMR_TickCountGet();
  }
}
#endif

static void APP_ExampleDHCPAddressEventCallback(DRV_HANDLE handle,
                                                uint32_t ipAddress) {
  char s[20];

  SYS_CONSOLE_PRINT("\nIP address is %s",
                    inet_ntop(AF_INET, &ipAddress, s, sizeof(s)));
}

static void APP_ExampleConnectNotifyCallback(DRV_HANDLE handle,
                                             WDRV_WINC_ASSOC_HANDLE assocHandle,
                                             WDRV_WINC_CONN_STATE currentState,
                                             WDRV_WINC_CONN_ERROR errorCode) {
  if (WDRV_WINC_CONN_STATE_CONNECTED == currentState) {
    /* When connected reset the ICMP echo request counter and state. */
    SYS_CONSOLE_PRINT("\nConnected");

#ifdef ICMP_ECHO_TARGET
    icmpEchoCount = ICMP_ECHO_COUNT;
    icmpEchoReplyTime = 0;
#endif

    APP_ExampleSetState(EXAMP_STATE_CONNECTED);
  } else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState) {
    SYS_CONSOLE_PRINT("\nDisconnected");

    APP_ExampleSetState(EXAMP_STATE_DISCONNECTED);
  }
}
