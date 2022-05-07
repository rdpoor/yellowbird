/**
 * @file winc_mgr.h
 */

#ifndef _WINC_MGR_H_
#define _WINC_MGR_H_

// =============================================================================
// Includes

#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// Public types and definitions

typedef enum {
  WINC_MGR_ERR_NONE = 0,
  WINC_MGR_ERR_INIT_FAILED,
  WINC_MGR_ERR_BAD_PARAM,
  WINC_MGR_ERR_RUNTIME,
} winc_mgr_err_t;

typedef struct {
  const char *wifi_ssid; // SSID of the Access Point
  uint8_t wifi_sectype;  // M2M_WIFI_SEC_[OPEN, WEP, WPA_PSK, 802_1X]
  void *wifi_auth;
  uint16_t wifi_channels;
  uint16_t host_port;
  uint32_t host_addr;
  const char *host_name;
  bool use_ssl;
} winc_mgr_params_t;

// This block defines the possible states for the process_transaction() state
// machine.
#define DEFINE_STATES                                                          \
  DEFINE_STATE(WINC_MGR_ENTRY)                                                 \
  DEFINE_STATE(WINC_MGR_INITIATING_AP_CONNECTION)                              \
  DEFINE_STATE(WINC_MGR_AWAITING_AP_CONNECTION)                                \
  DEFINE_STATE(WINC_MGR_AP_CONNECT_FAILURE)                                    \
  DEFINE_STATE(WINC_MGR_INITIATING_DNS_RESOLUTION)                             \
  DEFINE_STATE(WINC_MGR_AWAITING_DNS_RESOLUTION)                               \
  DEFINE_STATE(WINC_MGR_DNS_RESOLUTION_FAILURE)                                \
  DEFINE_STATE(WINC_MGR_INITIATING_SOCKET_CONNECTION)                          \
  DEFINE_STATE(WINC_MGR_AWAITING_SOCKET_CONNECTION)                            \
  DEFINE_STATE(WINC_MGR_SOCKET_FAILURE)                                        \
  DEFINE_STATE(WINC_MGR_CONNECT_FAILURE)                                       \
  DEFINE_STATE(WINC_MGR_CONNECT_SUCCESS)

// Expand DEFINE_STATES to generate an enum for each state
#undef DEFINE_STATE
#define DEFINE_STATE(x) x,
typedef enum { DEFINE_STATES } winc_mgr_state_t;

// Ths signature of the completion callback function.
typedef void (*winc_mgr_cb_t)(SOCKET socket, winc_mgr_state_t final_state);

// =============================================================================
// Public declarations

/**
 * @brief Initialize the WINC manager.
 */
winc_mgr_err_t winc_mgr_init(void);

/**
 * @brief Launch an asynchronous task to open a socket.
 *
 * @param params Input parameters.
 * @param cb Callbeck triggered upon completion (success or failure).
 * @return Error code if call to winc_mgr_open_socket itself was successful.
 */
winc_mgr_err_t winc_mgr_open_socket(winc_mgr_params_t *params,
                                    winc_mgr_cb_t cb);

/**
 * @brief Close any open socket and reset the WINC.
 */
winc_mgr_err_t winc_mgr_reset(void);

/**
 * @brief Return the current open socket, or -1 if socket is not open.
 */
SOCKET winc_mgr_get_socket(void);

/**
 * @brief Return the current winc_mgr state.
 */
winc_mgr_state_t winc_mgr_get_state(void);

/**
 * @brief Return the current winc_mgr state as a string.
 */
const char *winc_mgr_get_state_name(winc_mgr_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _WINC_MGR_H_ */
