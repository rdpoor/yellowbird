/**
 * @file http_task.c
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

#include "http_task.h"

#include "definitions.h"
#include "mu_str.h"
#include "mu_strbuf.h"
#include "wdrv_winc_client_api.h"
#include "winc_task.h" // should be app.h
#include "yb_log.h"
#include <stdbool.h>
#include <stddef.h>

// *****************************************************************************
// Local (private) types and definitions

#define HTTP_TASK_STATES(M)                                                    \
  M(HTTP_TASK_STATE_INIT)                                                      \
  M(HTTP_TASK_STATE_AWAIT_IP_LINK)                                             \
  M(HTTP_TASK_STATE_START_DNS)                                                 \
  M(HTTP_TASK_STATE_AWAIT_DNS)                                                 \
  M(HTTP_TASK_STATE_START_SOCKET)                                              \
  M(HTTP_TASK_STATE_AWAIT_SOCKET)                                              \
  M(HTTP_TASK_STATE_START_SEND)                                                \
  M(HTTP_TASK_STATE_AWAIT_SEND)                                                \
  M(HTTP_TASK_STATE_AWAIT_RESPONSE)                                            \
  M(HTTP_TASK_STATE_SUCCESS)                                                   \
  M(HTTP_TASK_STATE_ERROR)

#define EXPAND_TASK_STATE_ENUM(_name) _name,
typedef enum { HTTP_TASK_STATES(EXPAND_TASK_STATE_ENUM) } http_task_state_t;

typedef struct {
  http_task_state_t state;
  DRV_HANDLE winc_handle;
  const char *host_name;     // host name, triggers DNS lookup if host_ipv4 = 0
  uint32_t host_ipv4;        // host address, used in preference to host_name
  uint16_t host_port;        // host port
  bool use_tls;              // set to true to use SSL/TLS
  mu_strbuf_t *request_msg;  // HTTP request (header and body)
  mu_strbuf_t *response_msg; // Buffer to hold response
  SOCKET client_socket;      // socket...
} http_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void http_task_set_state(http_task_state_t new_state);
static const char *http_task_state_name(http_task_state_t state);

static void http_task_socket_callback(SOCKET socket,
                                      uint8_t msg_type,
                                      void *msg);

static void http_task_resolver_cb(uint8_t *pu8DomainName, uint32_t u32ServerIP);

// *****************************************************************************
// Local (private, static) storage

static http_task_ctx_t s_http_task_ctx;

#define EXPAND_TASK_STATE_NAME(_name) #_name,
static const char *s_http_task_state_names[] = {
    HTTP_TASK_STATES(EXPAND_TASK_STATE_NAME)};

// *****************************************************************************
// Public code

void http_task_init(DRV_HANDLE winc_handle,
                    const char *host_name,
                    const char *host_ipv4,
                    uint16_t host_port,
                    bool use_tls,
                    mu_strbuf_t *request_msg,
                    mu_strbuf_t *response_msg) {
  http_task_ctx_t *p = &s_http_task_ctx; // typing avoidance
  p->winc_handle = winc_handle;
  p->host_name = host_name;
  p->host_ipv4 = (host_ipv4 == NULL) ? 0 : inet_addr(host_ipv4);
  p->host_port = host_port;
  p->use_tls = use_tls;
  p->request_msg = request_msg;
  p->response_msg = response_msg;
  p->client_socket = -1;
  p->state = HTTP_TASK_STATE_INIT;

  socketInit();
  WDRV_WINC_SocketRegisterEventCallback(winc_handle, http_task_socket_callback);
}

void http_task_step(void) {
  switch (s_http_task_ctx.state) {

  case HTTP_TASK_STATE_INIT: {
    http_task_set_state(HTTP_TASK_STATE_AWAIT_IP_LINK);
  } break;

  case HTTP_TASK_STATE_AWAIT_IP_LINK: {
    if (WDRV_WINC_IPLinkActive(s_http_task_ctx.winc_handle) == false) {
      // remain in this state until the WINC reports IP Link active.
    } else if (s_http_task_ctx.host_ipv4 == 0) {
      // Have not resolved host address yet...
      http_task_set_state(HTTP_TASK_STATE_START_DNS);
    } else {
      // Host address was provided by the caller
      http_task_set_state(HTTP_TASK_STATE_START_SOCKET);
    }
  } break;

  case HTTP_TASK_STATE_START_DNS: {
    if (WDRV_WINC_STATUS_OK !=
        WDRV_WINC_SocketRegisterResolverCallback(s_http_task_ctx.winc_handle,
                                                 http_task_resolver_cb)) {
      http_task_set_state(HTTP_TASK_STATE_ERROR);
    } else {
      gethostbyname((const char *)s_http_task_ctx.host_name);
      http_task_set_state(HTTP_TASK_STATE_AWAIT_DNS);
    }
  } break;

  case HTTP_TASK_STATE_AWAIT_DNS: {
    // remain in this state until advanced by http_task_resolver_cb()
  } break;

  case HTTP_TASK_STATE_START_SOCKET: {
    const char *err = NULL;
    do {
      uint8_t flags = s_http_task_ctx.use_tls ? SOCKET_CONFIG_SSL_ON : 0;
      s_http_task_ctx.client_socket = socket(AF_INET, SOCK_STREAM, flags);
      if (s_http_task_ctx.client_socket < 0) {
        err = "Unable to open a socket";
        break;
      }

      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = _htons(s_http_task_ctx.host_port);
      addr.sin_addr.s_addr = s_http_task_ctx.host_ipv4;

      if (connect(s_http_task_ctx.client_socket,
                  (struct sockaddr *)&addr,
                  sizeof(struct sockaddr_in)) < 0) {
        err = "connect() failed";
        break;
      }
    } while (0);

    if (err != NULL) {
      // err is a string citing the cause of the error
      YB_LOG_ERROR("%s", err);
      http_task_set_state(HTTP_TASK_STATE_ERROR);
    } else {
      // success
      http_task_set_state(HTTP_TASK_STATE_AWAIT_SOCKET);
    }
  } break;

  case HTTP_TASK_STATE_AWAIT_SOCKET: {
    // remain here until http_task_socket_callback() advances state
  } break;

  case HTTP_TASK_STATE_START_SEND: {
    recv(s_http_task_ctx.client_socket,
         mu_strbuf_wdata(s_http_task_ctx.response_msg),
         mu_strbuf_capacity(s_http_task_ctx.response_msg),
         0);
    send(s_http_task_ctx.client_socket,
         (void *)mu_strbuf_rdata(s_http_task_ctx.request_msg),
         mu_strbuf_capacity(s_http_task_ctx.request_msg),
         0);
    YB_LOG_INFO("Sending\n==>>>\n%*s==>>>",
                mu_strbuf_capacity(s_http_task_ctx.request_msg),
                mu_strbuf_rdata(s_http_task_ctx.request_msg));
    http_task_set_state(HTTP_TASK_STATE_AWAIT_SEND);
  } break;

  case HTTP_TASK_STATE_AWAIT_SEND: {
    // remain here until http_task_socket_callback() advances state
  } break;

  case HTTP_TASK_STATE_AWAIT_RESPONSE: {
    // remain here until http_task_socket_callback() advances state
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

void http_task_shutdown(void) {
  if (s_http_task_ctx.client_socket >= 0) {
    shutdown(s_http_task_ctx.client_socket);
    s_http_task_ctx.client_socket = -1;
  }
}

// *****************************************************************************
// Local (private, static) code

static void http_task_set_state(http_task_state_t new_state) {
  if (new_state != s_http_task_ctx.state) {
    YB_LOG_INFO("%s => %s",
                http_task_state_name(s_http_task_ctx.state),
                http_task_state_name(new_state));
    s_http_task_ctx.state = new_state;
  }
}

static const char *http_task_state_name(http_task_state_t state) {
  return s_http_task_state_names[state];
}

static void http_task_socket_callback(SOCKET socket,
                                      uint8_t msg_type,
                                      void *msg) {
  if (socket != s_http_task_ctx.client_socket) {
    YB_LOG_DEBUG("Received socket callback for unknown socket");
    return;
  }

  switch (msg_type) {
  case SOCKET_MSG_CONNECT: {
    // arrive here in response to a connect() request
    tstrSocketConnectMsg *connect_msg = (tstrSocketConnectMsg *)msg;
    if (connect_msg != NULL && connect_msg->s8Error >= 0) {
      // successful connection -- initiate a TCP/IP exchange
      YB_LOG_INFO("Socket %d connected", socket);
      http_task_set_state(HTTP_TASK_STATE_START_SEND);
    }
  } break;

  case SOCKET_MSG_SEND: {
    // Arrive here when send() completes
    http_task_set_state(HTTP_TASK_STATE_AWAIT_RESPONSE);
  } break;

  case SOCKET_MSG_RECV: {
    // Arrive here when recv() completes
    tstrSocketRecvMsg *recv_msg = (tstrSocketRecvMsg *)msg;
    if (recv_msg != NULL && recv_msg->s16BufferSize > 0) {
      bool print_dots = false;
      uint32_t to_print = recv_msg->s16BufferSize;
      if (to_print > 300) {
        print_dots = true;
        recv_msg->pu8Buffer[300] = 0; // bam.  null terminate
        to_print = 300;
      }
      YB_LOG_INFO("Received response:\n==<<<\n%s%s\n==<<<",
                  (char *)recv_msg->pu8Buffer,
                  print_dots ? "..." : "");
      http_task_set_state(HTTP_TASK_STATE_SUCCESS);
    }

  } break;

  default: {
    YB_LOG_WARN("Unrecognized socket callback type %d", msg_type);
  }
  } // switch
}

static void http_task_resolver_cb(uint8_t *pu8DomainName,
                                  uint32_t u32ServerIP) {
  char s[20];

  YB_LOG_INFO("%s resolved to %s",
              pu8DomainName,
              inet_ntop(AF_INET, &u32ServerIP, s, sizeof(s)));
  s_http_task_ctx.host_ipv4 = u32ServerIP;
  http_task_set_state(HTTP_TASK_STATE_START_SOCKET);
}
