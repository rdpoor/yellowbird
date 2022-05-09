/**
 * @file winc_task.c
 *
 * MIT License
 *
 * Copyright (c) 2022 Klatu Networks
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// *****************************************************************************
// Includes

#include "winc_task.h"

#include "http_task.h"
#include "wdrv_winc_client_api.h"
#include "yb_log.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define STATES(M)                                                              \
  M(WINC_TASK_STATE_INIT)                                                      \
  M(WINC_TASK_STATE_REQ_OPEN)                                                  \
  M(WINC_TASK_STATE_REQ_DHCP)                                                  \
  M(WINC_TASK_STATE_CONFIGURING_STA)                                           \
  M(WINC_TASK_STATE_START_CONNECT)                                             \
  M(WINC_TASK_STATE_AWAIT_CONNECT)                                             \
  M(WINC_TASK_STATE_START_HTTP_TASK)                                           \
  M(WINC_TASK_STATE_AWAIT_HTTP_TASK)                                           \
  M(WINC_TASK_STATE_START_DISCONNECT)                                          \
  M(WINC_TASK_STATE_AWAIT_DISCONNECT)                                          \
  M(WINC_TASK_STATE_SUCCESS)                                                   \
  M(WINC_TASK_STATE_ERROR)

#define EXPAND_STATE_ENUM_IDS(_name) _name,
typedef enum { STATES(EXPAND_STATE_ENUM_IDS) } winc_task_state_t;

typedef struct {
  winc_task_state_t state;
  const char *ssid;
  const char *pass;
  DRV_HANDLE wdrHandle;
  uint32_t timeUTC;
} winc_task_ctx_t;

// *****************************************************************************
// Local (private, static) storage

#define EXPAND_STATE_NAMES(_name) #_name,
static const char *s_winc_task_state_names[] = {STATES(EXPAND_STATE_NAMES)};

static winc_task_ctx_t s_winc_task_ctx;

static WDRV_WINC_BSS_CONTEXT s_bss;

static WDRV_WINC_AUTH_CONTEXT s_auth;

// *****************************************************************************
// Local (private, static) forward declarations

static void winc_task_set_state(winc_task_state_t new_state);

static const char *winc_task_state_name(winc_task_state_t state);

static void winc_task_dhcp_cb(DRV_HANDLE handle, uint32_t ipAddress);

static void winc_task_wifi_notify_cb(DRV_HANDLE handle,
                                     WDRV_WINC_ASSOC_HANDLE assocHandle,
                                     WDRV_WINC_CONN_STATE currentState,
                                     WDRV_WINC_CONN_ERROR errorCode);

// *****************************************************************************
// Public code

void winc_task_connect(const char *ssid, const char *pass) {
  s_winc_task_ctx.state = WINC_TASK_STATE_INIT;
  s_winc_task_ctx.ssid = ssid;
  s_winc_task_ctx.pass = pass;
}

void winc_task_disconnect(void) {
  s_winc_task_ctx.state = WINC_TASK_STATE_START_DISCONNECT;
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
    // Request DHCP from Access Point.  Although the WINC handles this
    // internally, registering a callback lets us report when it happens.
    WDRV_WINC_IPUseDHCPSet(s_winc_task_ctx.wdrHandle, &winc_task_dhcp_cb);
    winc_task_set_state(WINC_TASK_STATE_CONFIGURING_STA);
  } break;

  case WINC_TASK_STATE_CONFIGURING_STA: {
    const char *err = NULL;
    const char *ssid = s_winc_task_ctx.ssid;
    const char *pass = s_winc_task_ctx.pass;
    // use this do {} while(false) to handle a sequence of steps...
    do {
      if (WDRV_WINC_STATUS_OK != WDRV_WINC_BSSCtxSetDefaults(&s_bss)) {
        err = "WDRV_WINC_BSSCtxSetDefaults() failed";
        break;
      }
      if (WDRV_WINC_STATUS_OK !=
          WDRV_WINC_BSSCtxSetSSID(&s_bss, (uint8_t *)ssid, strlen(ssid))) {
        err = "WDRV_WINC_BSSCtxSetSSID() failed";
        break;
      }
      if (WDRV_WINC_STATUS_OK !=
          WDRV_WINC_AuthCtxSetWPA(
              &s_auth, (uint8_t *)pass, strlen((const char *)pass))) {
        err = "WDRV_WINC_AuthCtxSetWPA() failed";
        break;
      }

    } while (false);

    if (err) {
      YB_LOG_ERROR("%s", err);
      winc_task_set_state(WINC_TASK_STATE_ERROR);

    } else {
      winc_task_set_state(WINC_TASK_STATE_START_CONNECT);
    }
  } break;

  case WINC_TASK_STATE_START_CONNECT: {
    if (WDRV_WINC_STATUS_OK ==
        WDRV_WINC_BSSConnect(s_winc_task_ctx.wdrHandle,
                             &s_bss,
                             &s_auth,
                             &winc_task_wifi_notify_cb)) {
      winc_task_set_state(WINC_TASK_STATE_AWAIT_CONNECT);
    } else {
      // Retry WDRV_WINC_BSSConnect() until STATUS_OK?!?
    }
  } break;

  case WINC_TASK_STATE_AWAIT_CONNECT: {
    // wait for winc_task_wifi_notify_cb to advance state.
    asm("nop");
  } break;

  case WINC_TASK_STATE_START_HTTP_TASK: {
    YB_LOG_INFO("Starting HTTP task");
    http_task_init(winc_task_get_handle(),
                   APP_HOST_NAME,
                   APP_HOST_IP_ADDR,
                   APP_HOST_PORT,
                   APP_HOST_USE_TLS,
                   app_request_msg(),
                   app_response_msg());
    winc_task_set_state(WINC_TASK_STATE_AWAIT_HTTP_TASK);
  } break;

  case WINC_TASK_STATE_AWAIT_HTTP_TASK: {
    http_task_step();
    if (http_task_failed()) {
      YB_LOG_FATAL("HTTP task failed - quitting");
      winc_task_set_state(WINC_TASK_STATE_ERROR);
    } else if (http_task_succeeded()) {
      winc_task_set_state(WINC_TASK_STATE_START_DISCONNECT);
    } else {
      // http task has not completed -- remain in this state
    }
  } break;

  case WINC_TASK_STATE_START_DISCONNECT: {
    m2m_wifi_disconnect();
    winc_task_set_state(WINC_TASK_STATE_AWAIT_DISCONNECT);
  } break;

  case WINC_TASK_STATE_AWAIT_DISCONNECT: {
    // remain in this state until
  } break;

    // =============================
    // common endpoints

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

void winc_task_shutdown(void) { m2m_wifi_deinit(NULL); }

DRV_HANDLE winc_task_get_handle(void) { return s_winc_task_ctx.wdrHandle; }

// *****************************************************************************
// Local (private, static) code

static void winc_task_set_state(winc_task_state_t new_state) {
  if (new_state != s_winc_task_ctx.state) {
    YB_LOG_INFO("%s => %s",
                winc_task_state_name(s_winc_task_ctx.state),
                winc_task_state_name(new_state));
    s_winc_task_ctx.state = new_state;
  }
}

static const char *winc_task_state_name(winc_task_state_t state) {
  return s_winc_task_state_names[state];
}

static void winc_task_dhcp_cb(DRV_HANDLE handle, uint32_t dhcpAddr) {
  // Called asynchronously in response to WDRV_WINC_IPUseDHCPSet()
  (void)handle;
  char s[20];

  YB_LOG_INFO("DHCP address is %s",
              inet_ntop(AF_INET, &dhcpAddr, s, sizeof(s)));
}

static void winc_task_wifi_notify_cb(DRV_HANDLE handle,
                                     WDRV_WINC_ASSOC_HANDLE assocHandle,
                                     WDRV_WINC_CONN_STATE currentState,
                                     WDRV_WINC_CONN_ERROR errorCode) {
  if (WDRV_WINC_CONN_STATE_CONNECTED == currentState) {
    YB_LOG_INFO("Connected to AP");
    winc_task_set_state(WINC_TASK_STATE_START_HTTP_TASK);

  } else if (WDRV_WINC_CONN_STATE_DISCONNECTED == currentState) {
    YB_LOG_INFO("Disconnected from AP");
    m2m_wifi_deinit(NULL);
    winc_task_set_state(WINC_TASK_STATE_SUCCESS);

  } else {
    YB_LOG_WARN("winc_task_wifi_nofify_cb() received currenState = %d",
                currentState);
  }
}
