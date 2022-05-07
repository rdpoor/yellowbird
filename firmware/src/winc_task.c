/**
 * @file winc_task.c
 *
 * MIT License
 *
 * Copyright (c) 2022 R. Dunbar Poor
 *
 * Permission is hereby granted, free of charge, to any
 * person obtaining a copy of this software and
 * associated documentation files (the "Software"), to
 * deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission
 * notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY
 * OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

// *****************************************************************************
// Includes

#include "winc_task.h"

#include "http_task.h"
#include "wdrv_winc_client_api.h"
#include "yb_utils.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define WINC_TASK_NTP_POOL_HOSTNAME "*.pool.ntp.org"

#define EXPAND_WINC_TASK_STATES                                                \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_INIT)                                 \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_REQ_OPEN)                             \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_REQ_DHCP)                             \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_SCANNING)                             \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_GETTING_SCAN_RESULTS)                 \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_CONFIGURING_STA)                      \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_REQ_CONNECTION)                       \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_AWAITING_CONNECTION)                  \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_REQ_NTP)                              \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_GET_NTP)                              \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_SUCCESS)                              \
  DEFINE_WINC_TASK_STATE(WINC_TASK_STATE_ERROR)

#undef DEFINE_WINC_TASK_STATE
#define DEFINE_WINC_TASK_STATE(_name) _name,
typedef enum { EXPAND_WINC_TASK_STATES } winc_task_state_t;

typedef struct {
  winc_task_state_t state;
  void (*on_completion)(void *arg);
  void *completion_arg;
  DRV_HANDLE wdrHandle;
  uint32_t dhcpAddr;
  uint32_t timeUTC;
} winc_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void winc_task_set_state(winc_task_state_t new_state);

static const char *winc_task_state_name(winc_task_state_t state);

static void winc_task_dhcp_cb(DRV_HANDLE handle, uint32_t ipAddress);

static void winc_task_ntp_cb(DRV_HANDLE handle, uint32_t timeUTC);

static void winc_task_wifi_notify_cb(DRV_HANDLE handle,
                                     WDRV_WINC_ASSOC_HANDLE assocHandle,
                                     WDRV_WINC_CONN_STATE currentState,
                                     WDRV_WINC_CONN_ERROR errorCode);

/**
 * @brief Set the state to final_state and invoke on_completion.
 */
static void endgame(winc_task_state_t final_state);

// *****************************************************************************
// Local (private, static) storage

#undef DEFINE_WINC_TASK_STATE
#define DEFINE_WINC_TASK_STATE(_name) #_name,
static const char *s_winc_task_state_names[] = {EXPAND_WINC_TASK_STATES};
static winc_task_ctx_t s_winc_task_ctx;
static WDRV_WINC_BSS_CONTEXT bssCtx;
static WDRV_WINC_AUTH_CONTEXT authCtx;

// *****************************************************************************
// Public code

void winc_task_init(void (*on_completion)(void *arg), void *completion_arg) {
  s_winc_task_ctx.state = WINC_TASK_STATE_INIT;
  s_winc_task_ctx.on_completion = on_completion;
  s_winc_task_ctx.completion_arg = completion_arg;
  s_winc_task_ctx.dhcpAddr = 0; // not yet set.
  s_winc_task_ctx.timeUTC = 0;  // not yet set.
}

void winc_task_step(void) {
  switch (s_winc_task_ctx.state) {

  case WINC_TASK_STATE_INIT: {
    winc_task_set_state(WINC_TASK_STATE_REQ_OPEN);
  } break;

  case WINC_TASK_STATE_REQ_OPEN: {
    s_winc_task_ctx.wdrHandle = WDRV_WINC_Open(0, 0);
    if (s_winc_task_ctx.wdrHandle != DRV_HANDLE_INVALID) {
      winc_task_set_state(WINC_TASK_STATE_REQ_DHCP);
    } else {
      // remain in this state until Open succeeds.
    }
  } break;

  case WINC_TASK_STATE_REQ_DHCP: {
    // Requesting DHCP from Access Point.  Note we don't need to do this --
    // the WINC handles it internally -- but we can specify a callback to see
    // when it happens.
    WDRV_WINC_IPUseDHCPSet(s_winc_task_ctx.wdrHandle, &winc_task_dhcp_cb);
    if (app_is_cold_boot()) {
      // Cold boot: perform a BSS scan for all access points.
      if (WDRV_WINC_STATUS_OK ==
          WDRV_WINC_BSSFindFirst(s_winc_task_ctx.wdrHandle,
                                 WDRV_WINC_ALL_CHANNELS,
                                 true,
                                 NULL,
                                 NULL)) {
        winc_task_set_state(WINC_TASK_STATE_SCANNING);
      } else {
        printf(
                        "\nCall to WDRV_WINC_BSSFindFirst() failed");
        endgame(WINC_TASK_STATE_ERROR);
      }
    } else {
      // Warm boot: to straight to contacting AP
      winc_task_set_state(WINC_TASK_STATE_CONFIGURING_STA);
    }
  } break;

  case WINC_TASK_STATE_SCANNING: {
    if (WDRV_WINC_BSSFindInProgress(s_winc_task_ctx.wdrHandle)) {
      // remain in this state
    } else {
      printf(
          "\nScan complete, %d AP(s) found",
          WDRV_WINC_BSSFindGetNumBSSResults(s_winc_task_ctx.wdrHandle));
      winc_task_set_state(WINC_TASK_STATE_GETTING_SCAN_RESULTS);
    }
  } break;

  case WINC_TASK_STATE_GETTING_SCAN_RESULTS: {
    WDRV_WINC_BSS_INFO BSSInfo;
    WDRV_WINC_STATUS status;

    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_BSSFindGetInfo(s_winc_task_ctx.wdrHandle, &BSSInfo)) {
      printf(
          "\nAP found: RSSI: %d %s", BSSInfo.rssi, BSSInfo.ctx.ssid.name);

      status = WDRV_WINC_BSSFindNext(s_winc_task_ctx.wdrHandle, NULL);

      if (WDRV_WINC_STATUS_BSS_FIND_END == status) {
        // finished printing scanned APs
        winc_task_set_state(WINC_TASK_STATE_CONFIGURING_STA);
      } else if (WDRV_WINC_STATUS_NOT_OPEN == status) {
        endgame(WINC_TASK_STATE_ERROR);
      } else if (WDRV_WINC_STATUS_INVALID_ARG == status) {
        endgame(WINC_TASK_STATE_ERROR);
      } else {
        // remain in this state to complete printing out scanned APs
      }
    } else {
      printf( "\nError in WDRV_WINC_BSSFindGetInfo");
      endgame(WINC_TASK_STATE_ERROR);
    }

  } break;

  case WINC_TASK_STATE_CONFIGURING_STA: {
    const char *err = NULL;
    const char *ssid;
    const char *pass;
    // use this do {} while(false) to handle a sequence of steps...
    do {
      if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetDefaults(&bssCtx)) {
        err = "WDRV_WINC_BSSCtxSetDefaults() failed";
        break;
      }
      ssid = app_get_ssid();
      if (WDRV_WINC_STATUS_OK !=
          WDRV_WINC_BSSCtxSetSSID(&bssCtx, (uint8_t *)ssid, strlen(ssid))) {
        err = "WDRV_WINC_BSSCtxSetSSID() failed";
        break;
      }
      pass = app_get_pass();
      if (WDRV_WINC_STATUS_OK !=
          WDRV_WINC_AuthCtxSetWPA(
              &authCtx, (uint8_t *)pass, strlen((const char *)pass))) {
        err = "WDRV_WINC_AuthCtxSetWPA() failed";
        break;
      }

    } while (false);

    if (err) {
      printf( "\n%s", err);
      endgame(WINC_TASK_STATE_ERROR);

    } else {
      winc_task_set_state(WINC_TASK_STATE_REQ_CONNECTION);
    }
  } break;

  case WINC_TASK_STATE_REQ_CONNECTION: {
    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_BSSConnect(s_winc_task_ctx.wdrHandle,
                             &bssCtx,
                             &authCtx,
                             &winc_task_wifi_notify_cb)) {
      winc_task_set_state(WINC_TASK_STATE_AWAITING_CONNECTION);
    } else {
      // Retry WDRV_WINC_BSSConnect() until STATUS_OK?!?
    }
  } break;

  case WINC_TASK_STATE_AWAITING_CONNECTION: {
    // wait for winc_task_wifi_notify_cb to advance state.
    asm("nop");
  } break;

  case WINC_TASK_STATE_REQ_NTP: {
    // winc_task_wifi_notify_cb triggered.  Request NTP.
    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_SystemTimeSNTPClientEnable(
            s_winc_task_ctx.wdrHandle, WINC_TASK_NTP_POOL_HOSTNAME, true)) {
      winc_task_set_state(WINC_TASK_STATE_GET_NTP);
    } else {
      printf( "\nNTP request failed");
      endgame(WINC_TASK_STATE_ERROR);
    }
  } break;

  case WINC_TASK_STATE_GET_NTP: {
    // Install NTP callback, to be triggered when NTP request completes
    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_SystemTimeGetCurrent(s_winc_task_ctx.wdrHandle,
                                       winc_task_ntp_cb)) {
      winc_task_set_state(WINC_TASK_STATE_SUCCESS);
    } else {
      printf( "\nNTP get failed");
      endgame(WINC_TASK_STATE_ERROR);
    }
  } break;

  case WINC_TASK_STATE_SUCCESS: {
    asm("nop");
  } break;

  case WINC_TASK_STATE_ERROR: {
    asm("nop");
  } break;

  } // switch(s_winc_task_ctx.state)
}

bool winc_task_succeeded(void) {
  return s_winc_task_ctx.state == WINC_TASK_STATE_SUCCESS;
}

bool winc_task_failed(void) {
  return s_winc_task_ctx.state == WINC_TASK_STATE_ERROR;
}

DRV_HANDLE winc_task_get_handle(void) { return s_winc_task_ctx.wdrHandle; }

uint32_t winc_task_get_dhcp_addr(void) { return s_winc_task_ctx.dhcpAddr; }

uint32_t winc_task_get_utc(void) { return s_winc_task_ctx.timeUTC; }

// *****************************************************************************
// Local (private, static) code

static void winc_task_set_state(winc_task_state_t new_state) {
  if (new_state != s_winc_task_ctx.state) {
    YB_LOG_STATE_CHANGE(winc_task_state_name(s_winc_task_ctx.state),
                        winc_task_state_name(new_state));
    s_winc_task_ctx.state = new_state;
  }
}

static const char *winc_task_state_name(winc_task_state_t state) {
  return s_winc_task_state_names[state];
}

static void endgame(winc_task_state_t final_state) {
  winc_task_set_state(final_state);
  if (s_winc_task_ctx.on_completion != NULL) {
    s_winc_task_ctx.on_completion(s_winc_task_ctx.completion_arg);
  }
}

static void winc_task_dhcp_cb(DRV_HANDLE handle, uint32_t dhcpAddr) {
  // Called asynchronously in response to WDRV_WINC_IPUseDHCPSet()
  (void)handle;
  s_winc_task_ctx.dhcpAddr = dhcpAddr;
}

static void winc_task_ntp_cb(DRV_HANDLE handle, uint32_t timeUTC) {
  (void)handle;
  s_winc_task_ctx.timeUTC = timeUTC;
  printf( "\ntimeUTC = %lu", timeUTC);
}

static void winc_task_wifi_notify_cb(DRV_HANDLE handle,
                                     WDRV_WINC_ASSOC_HANDLE assocHandle,
                                     WDRV_WINC_CONN_STATE currentState,
                                     WDRV_WINC_CONN_ERROR errorCode) {
  if (WDRV_WINC_CONN_STATE_CONNECTED == currentState) {
    printf( "\nConnected to AP");
    winc_task_set_state(WINC_TASK_STATE_REQ_NTP);

  } else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState) {
    printf( "\nDisconnected from AP");
    winc_task_set_state(WINC_TASK_STATE_REQ_CONNECTION);
  }
}
