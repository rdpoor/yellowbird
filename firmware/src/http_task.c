/**
 * @file http_task.c
 *
 * MIT License
 *
 * Copyright (c) 2022 R. Dunbar Poor
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

#include "http_task.h"

#include "definitions.h"
#include "mu_strbuf.h"
#include "mu_str.h"
#include "winc_task.h"      // should be app.h
#include "yb_utils.h"
#include "wdrv_winc_client_api.h"
#include <stdbool.h>
#include <stddef.h>

// *****************************************************************************
// Local (private) types and definitions

#define EXPAND_HTTP_TASK_STATES                                                \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_INIT)                                 \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_AWAIT_NTP_AND_DHCP)                   \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_AWAIT_DHCP)                           \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_AWAIT_NTP)                            \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_SUCCESS)                              \
  DEFINE_HTTP_TASK_STATE(HTTP_TASK_STATE_ERROR)

#undef DEFINE_HTTP_TASK_STATE
#define DEFINE_HTTP_TASK_STATE(_name) _name,
typedef enum { EXPAND_HTTP_TASK_STATES } http_task_state_t;

// user-supplied parameters passed in winc_task_init()
typedef struct {
  const char *host_name;  // host name, triggers DNS lookup if host_ipv4 = 0
  uint32_t host_ipv4;     // host address, used in preference to host_name
  uint16_t host_port;     // host port
  bool use_tls;           // set to true to use SSL/TLS
  mu_strbuf_t req_strbuf; // buffer holding HTTP request (header and body)
  mu_strbuf_t rsp_strbuf; // buffer to receive HTTP response
  size_t rsp_length;      // # of bytes available in the response buffer
} user_params_t;

typedef struct {
  http_task_state_t state;
  user_params_t user_params;
  void (*on_completion)(void *arg);
  void *completion_arg;
} http_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void http_task_set_state(http_task_state_t new_state);
static const char *http_task_state_name(http_task_state_t state);

/**
 * @brief Set the state to final_state and invoke on_completion.
 */
static void endgame(http_task_state_t final_state);

/**
 * @brief Log the current time as returned by NTP
 */
static void log_utc(uint32_t utc);

/**
 * @brief Log the DHCP address returned by the AP
 */
static void log_dhcp(uint32_t dhcp);

// *****************************************************************************
// Local (private, static) storage

static http_task_ctx_t s_http_task_ctx;

#undef DEFINE_HTTP_TASK_STATE
#define DEFINE_HTTP_TASK_STATE(_name) #_name,
static const char *s_http_task_state_names[] = {EXPAND_HTTP_TASK_STATES};

// *****************************************************************************
// Public code

void http_task_init(const char *host_name,
                    uint32_t host_ipv4,
                    uint16_t host_port,
                    bool use_tls,
                    const char *req_str,
                    size_t req_length,
                    char *rsp_str,
                    size_t rsp_length,
                    void (*on_completion)(void *arg),
                    void *completion_arg) {
  user_params_t *p = &s_http_task_ctx.user_params;  // typing avoidance
  p->host_name = host_name;
  p->host_ipv4 = host_ipv4;
  p->host_port = host_port;
  p->use_tls = use_tls;
  mu_strbuf_init_ro(&p->req_strbuf, (const uint8_t *)req_str, req_length);
  mu_strbuf_init_rw(&p->rsp_strbuf, (uint8_t *)rsp_str, rsp_length);
  s_http_task_ctx.on_completion = on_completion;
  s_http_task_ctx.completion_arg = completion_arg;

  s_http_task_ctx.state = HTTP_TASK_STATE_INIT;
}

void http_task_step(DRV_HANDLE handle) {
  (void)handle;

  switch (s_http_task_ctx.state) {

  case HTTP_TASK_STATE_INIT: {
    http_task_set_state(HTTP_TASK_STATE_AWAIT_NTP_AND_DHCP);
  } break;

  case HTTP_TASK_STATE_AWAIT_NTP_AND_DHCP: {
    if (winc_task_get_utc() != 0) {
      log_utc(winc_task_get_utc());
      http_task_set_state(HTTP_TASK_STATE_AWAIT_DHCP);
    } else if (winc_task_get_dhcp_addr() != 0) {
      log_dhcp(winc_task_get_dhcp_addr());
      http_task_set_state(HTTP_TASK_STATE_AWAIT_NTP);
    } else {
      // still waiting for both...
    }
  } break;

  case HTTP_TASK_STATE_AWAIT_DHCP: {
    // NTP received.  Still waiting for DHCP.
    if (winc_task_get_dhcp_addr() != 0) {
      log_dhcp(winc_task_get_dhcp_addr());
      endgame(HTTP_TASK_STATE_SUCCESS);
    } else {
      // still waiting for DHCP.
    }
  } break;

  case HTTP_TASK_STATE_AWAIT_NTP: {
    // DHCP received.  Still waiting for NTP.
    if (winc_task_get_utc() != 0) {
      log_utc(winc_task_get_utc());
      endgame(HTTP_TASK_STATE_SUCCESS);
    } else {
      // still waiting for NTP.
    }
  } break;

  case HTTP_TASK_STATE_SUCCESS: {
    // remain in this state
  } break;

  case HTTP_TASK_STATE_ERROR: {
    // remain in this state
  } break;

  } // switch()
}

bool http_task_succeeded(void) {
  return s_http_task_ctx.state == HTTP_TASK_STATE_SUCCESS;
}

bool http_task_failed(void) {
  return s_http_task_ctx.state == HTTP_TASK_STATE_ERROR;
}

// *****************************************************************************
// Local (private, static) code

static void http_task_set_state(http_task_state_t new_state) {
  if (new_state != s_http_task_ctx.state) {
    YB_LOG_STATE_CHANGE(http_task_state_name(s_http_task_ctx.state),
                        http_task_state_name(new_state));
    s_http_task_ctx.state = new_state;
  }
}

static const char *http_task_state_name(http_task_state_t state) {
  return s_http_task_state_names[state];
}

static void endgame(http_task_state_t final_state) {
  http_task_set_state(final_state);
  if (s_http_task_ctx.on_completion != NULL) {
    s_http_task_ctx.on_completion(s_http_task_ctx.completion_arg);
  }
}

static void log_utc(uint32_t utc) {
  tstrSystemTime st;
  WDRV_WINC_UTCToLocalTime(utc, &st);
  if (st.u16Year > 2000) {
    YB_LOG("%4d-%02d-%02d %02d:%02d:%02d UTC",
           st.u16Year,
           st.u8Month,
           st.u8Day,
           st.u8Hour,
           st.u8Minute,
           st.u8Second);
  }
}

static void log_dhcp(uint32_t dhcp) {
    char s[20];
    YB_LOG("IP addr: %s", inet_ntop(AF_INET, &dhcp, s, sizeof(s)));
}
