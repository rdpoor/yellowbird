/**
 * @file winc_mgr.c
 */

// =============================================================================
// Includes

#include "stdio.h"
#include "winc_mgr.h"

#include "FreeRTOS.h"
#include "console.h"
#include "hal_rtos.h"
#include "string.h"
#include "task.h"
#include "winc_init.h"
#include <stddef.h>

#include "osa.h"
#include "trace.h"

// =============================================================================
// Private types and definitions

#define WINC_MGR_STACK_SIZE (500/ sizeof(portSTACK_TYPE))
#define WINC_MGR_PRIORITY (tskIDLE_PRIORITY + 5)
#define console_printf printf

// Define the states for the WiFi callbacks
typedef enum {
  WIFI_STATE_ENTRY,
  WIFI_STATE_DISCONNECTED,
  WIFI_STATE_REQUESTING_DHCP,
  WIFI_STATE_CONNECTED
} wifi_state_t;

// Define the states for the Socket callbacks
typedef enum {
  SOCKET_STATE_ENTRY,
  SOCKET_STATE_CONNECTION_SUCCEEDED,
  SOCKET_STATE_CONNECTION_FAILED,
  SOCKET_STATE_SEND_SUCCEEDED,
  SOCKET_STATE_SEND_FAILED,
  SOCKET_STATE_RECEIVE_SUCCEEDED,
  SOCKET_STATE_RECEIVE_FAILED
} socket_state_t;

typedef enum {
  WINC_MGR_NOTIFY_START,
  WINC_MGR_NOTIFY_STEP,
  WINC_MGR_MOTIFY_STOP,
} winc_mgr_notification_t;

/** Using IP address. */
#define IPV4_BYTE(val, index) ((val >> (index * 8)) & 0xFF)

// LOGGING TBD
#define LOG_INFO(x)                                                            \
  do {                                                                         \
  } while (false);
typedef enum {
  LOG_WINC_MGR_AP_CONNECT_FAILURE,
  LOG_WINC_MGR_AWAITING_AP_CONNECTION,
  LOG_WINC_MGR_AWAITING_DNS_RESOLUTION,
  LOG_WINC_MGR_AWAITING_SOCKET_CONNECTION,
  LOG_WINC_MGR_CONNECT_FAILURE,
  LOG_WINC_MGR_DNS_RESOLUTION_FAILURE,
  LOG_WINC_MGR_INITIATING_AP_CONNECTION,
  LOG_WINC_MGR_INITIATING_DNS_RESOLUTION,
  LOG_WINC_MGR_INITIATING_SOCKET_CONNECTION,
  LOG_WINC_MGR_SOCKET_CONNECTED,
  LOG_WINC_MGR_SOCKET_FAILURE,
} log_state_t;

// =============================================================================
// Private declarations (static)

/**
 * @brief Send a notification to the winc task.
 *
 * Note: The winc task will normally be blocked until it receives a
 * notification.
 */
static winc_mgr_err_t winc_mgr_notify(uint32_t notification);

/**
 * @brief Send a notification to the winc task from interrupt level.
 */
// static winc_mgr_err_t winc_mgr_notify_from_isr(uint32_t notification,
// BaseType_t *pxHigherPriorityTaskWoken);

/**
 * @brief Top-level function of the winc task.
 */
static void winc_task(void *p);

/**
 * @brief Run the state machine until it needs to wait for external change.
 */
static winc_mgr_state_t winc_step_state(winc_mgr_state_t state);

/**
 * @brief Run the state macine for one step.
 */
static winc_mgr_state_t winc_step_state_aux(winc_mgr_state_t state);

/**
 * @brief
 */
static void reset(void);

/**
 * @brief Trigger the callback with the socket and final state as arguments.
 */
static void endgame(winc_mgr_state_t state);

/**
 * @brief Update the state.
 *
 * Note: this function does not modify the internal state variable.  Rather,
 * it simply calls the instrumentation functions before returning the new
 * state.
 */
static winc_mgr_state_t update_state(winc_mgr_state_t from,
                                     winc_mgr_state_t to);

/**
 * @brief
 */
static void wifi_cb(uint8_t u8MsgType, void *pvMsg);

/**
 * @brief
 */
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg);

/**
 * @brief Callback triggered upon completion of DNS resolution.
 */
static void resolve_cb(uint8_t *hostName, uint32_t hostIp);

// =============================================================================
// Private storage (static)

// Expand DEFINE_STATES to generate a table of state names
#undef DEFINE_STATE
#define DEFINE_STATE(x) #x,
static const char *s_state_names[] = {DEFINE_STATES};

// user supplied callback
static winc_mgr_cb_t s_cb;

// state captured from winc_mgr_init()
static winc_mgr_params_t *s_params;

// internal state
static uint32_t s_host_addr;
static winc_mgr_state_t s_winc_mgr_state;
static SOCKET s_socket = -1;
static socket_state_t s_socket_state;
static struct sockaddr_in s_addr_in;
static wifi_state_t s_wifi_state;

static TickType_t s_start_time; // time when process started

static TaskHandle_t s_winc_mgr_task_handle;

// =============================================================================
// Public code

/**
 * \brief Create the WINC task
 */
winc_mgr_err_t winc_mgr_init(void) {
	
#ifdef FREERTOS_STATIC_ALLOCATION 
  static StackType_t xWincMgrStack[WINC_MGR_STACK_SIZE];
  static StaticTask_t xWincMgrBuffer;

  s_winc_mgr_task_handle = xTaskCreateStatic(winc_task,
                                             "WINC",
                                             WINC_MGR_STACK_SIZE,
                                             NULL,
                                             WINC_MGR_PRIORITY,
                                             xWincMgrStack,
                                             &xWincMgrBuffer);
											 
	#else										 
	OSA_ThreadCreate(&s_winc_mgr_task_handle,
					 WINC_MGR_PRIORITY ,
					 winc_task,
					 NULL,
					"winc_thread",
					NULL,
					WINC_MGR_STACK_SIZE,
					0);
	#endif
  if (s_winc_mgr_task_handle == NULL) {
    return WINC_MGR_ERR_INIT_FAILED;
  }
  return WINC_MGR_ERR_NONE;
}

winc_mgr_err_t winc_mgr_open_socket(winc_mgr_params_t *params,
                                    winc_mgr_cb_t cb) {
  s_params = params;
  s_cb = cb;

 TRACE_DBG("%s() Entry\n", __FUNCTION__);

  winc_mgr_notify(WINC_MGR_NOTIFY_START);

    TRACE_DBG("Starting Call back scheduler \n");

    while(1)
    {
        /* Handle the app state machine plus the WINC event handler */
        while(m2m_wifi_handle_events(NULL) != M2M_SUCCESS) {
    }
}


}

winc_mgr_err_t winc_mgr_reset(void) {
  return winc_mgr_notify(WINC_MGR_MOTIFY_STOP);
}

SOCKET winc_mgr_get_socket(void) { return s_socket; }

winc_mgr_state_t winc_mgr_get_state(void) { return s_winc_mgr_state; }

const char *winc_mgr_get_state_name(winc_mgr_state_t state) {
  size_t n_states = sizeof(s_state_names) / sizeof(s_state_names[0]);
  if (state < n_states) {
    return s_state_names[state];
  } else {
    return "WINC_MGR state unknown";
  }
}

// =============================================================================
// Private code (static)

static winc_mgr_err_t winc_mgr_notify(uint32_t notification) {
  BaseType_t ret;
  // By passing eSetValueWithoutOverwrite as the action, xTaskNotify will return
  // an error if the previous notification hasn't yet been processed.
  taskYIELD();
  ret = xTaskNotify(
      s_winc_mgr_task_handle, notification, eSetValueWithoutOverwrite);
  if (ret != pdPASS) {
    return WINC_MGR_ERR_RUNTIME;
  }
  return WINC_MGR_ERR_NONE;
}

// static winc_mgr_err_t winc_mgr_notify_from_isr(uint32_t notification,
// BaseType_t *pxHigherPriorityTaskWoken) {
//   BaseType_t ret;
//   ret = xTaskNotifyFromISR(s_winc_mgr_task_handle, notification,
//   eSetValueWithoutOverwrite, pxHigherPriorityTaskWoken); if (ret != pdPASS) {
//     return WINC_MGR_ERR_RUNTIME;
//   }
//   return WINC_MGR_ERR_NONE;
// }

static void winc_task(void *p) {
  (void)p;
  uint32_t notification;
  
  TRACE_DBG("%s() Entry\n",__FUNCTION__);

  // winc_msg_setup();  // one-time setup
  while (true) {
    // wait for a notification from an interrupt or other task.
    xTaskNotifyWait(0, 0, &notification, portMAX_DELAY);
    TRACE_DBG("%s() Notification = %lu\n", __FUNCTION__,notification);
    switch (notification) {
    case WINC_MGR_NOTIFY_START:
      reset();
	  TRACE_DBG("%s() After Reset \n", __FUNCTION__);
      s_winc_mgr_state = winc_step_state(s_winc_mgr_state);
      break;
    case WINC_MGR_NOTIFY_STEP:
      // winc_step_state() will advance the state machine as far as it can,
      // then will return here to wait for an external interrupt or task to
      // generate a notification.
      s_winc_mgr_state = winc_step_state(s_winc_mgr_state);
      break;
    case WINC_MGR_MOTIFY_STOP:
      reset();
      break;
    }
  }
}

static winc_mgr_state_t winc_step_state(winc_mgr_state_t state) {
  char buf[80];
  winc_mgr_state_t prev_state = state;

  TRACE_DBG("%s() Entry\n", __FUNCTION__);

  while (true) {
    winc_mgr_state_t next_state = winc_step_state_aux(prev_state);
    snprintf(buf,
             80,
             "%s => %s\n",
             winc_mgr_get_state_name(prev_state),
             winc_mgr_get_state_name(next_state));
    //console_puts(buf);
    if (next_state == prev_state) {
      // state machine could not advance any further.  Return now.
      break;
    }
    prev_state = next_state;
  }

  TRACE_DBG("%s() Exit\n", __FUNCTION__);
  return prev_state;
}

static winc_mgr_state_t winc_step_state_aux(winc_mgr_state_t state) {
  winc_mgr_state_t next_state = state;

  TRACE_DBG("%s() Entry\n", __FUNCTION__);

  switch (state) {

  case WINC_MGR_ENTRY:
    s_start_time = xTaskGetTickCount();
    next_state = update_state(state, WINC_MGR_INITIATING_AP_CONNECTION);
    LOG_INFO(LOG_WINC_MGR_INITIATING_AP_CONNECTION);
    break;

  case WINC_MGR_INITIATING_AP_CONNECTION:
    if (s_wifi_state != WIFI_STATE_CONNECTED) {
      m2m_wifi_connect((char *)s_params->wifi_ssid,
                       strlen(s_params->wifi_ssid) + 1, // include null byte!
                       s_params->wifi_sectype,
                       s_params->wifi_auth,
                       s_params->wifi_channels);
      next_state = update_state(state, WINC_MGR_AWAITING_AP_CONNECTION);
      LOG_INFO(LOG_WINC_MGR_AWAITING_AP_CONNECTION);
    } else {
      next_state = update_state(state, WINC_MGR_INITIATING_DNS_RESOLUTION);
      LOG_INFO(LOG_WINC_MGR_INITIATING_DNS_RESOLUTION);
    }
    break;

  case WINC_MGR_AWAITING_AP_CONNECTION:
    if (s_wifi_state == WIFI_STATE_CONNECTED) {
      next_state = update_state(state, WINC_MGR_INITIATING_DNS_RESOLUTION);
      LOG_INFO(LOG_WINC_MGR_INITIATING_DNS_RESOLUTION);
    } else if (s_wifi_state == WIFI_STATE_DISCONNECTED) {
      next_state = update_state(state, WINC_MGR_AP_CONNECT_FAILURE);
      LOG_INFO(LOG_WINC_MGR_AP_CONNECT_FAILURE);
    } else {
      console_printf(
          "In WINC_MGR_AWAITING_AP_CONNECTION, awaiting WIFI connection\n");
      next_state = update_state(state, WINC_MGR_AWAITING_AP_CONNECTION);
      // no change yet: keep waiting for notification from wifi_cb
    }
    break;

  case WINC_MGR_INITIATING_DNS_RESOLUTION:
    if ((s_params->host_name == NULL) && (s_host_addr == 0)) {
      // either hostname or hostaddr must be set
      next_state = update_state(state, WINC_MGR_DNS_RESOLUTION_FAILURE);
      LOG_INFO(LOG_WINC_MGR_DNS_RESOLUTION_FAILURE);
    } else if (s_params->host_name != NULL) {
      // initiate DNS lookup
      console_printf("Host name is %s\r\n", s_params->host_name);
      gethostbyname((uint8_t *)s_params->host_name);
      next_state = update_state(state, WINC_MGR_AWAITING_DNS_RESOLUTION);
      LOG_INFO(LOG_WINC_MGR_AWAITING_DNS_RESOLUTION);
    } else {
      // already have hostaddr
      next_state = update_state(state, WINC_MGR_AWAITING_DNS_RESOLUTION);
      LOG_INFO(LOG_WINC_MGR_AWAITING_DNS_RESOLUTION);
    }
    break;

  case WINC_MGR_AWAITING_DNS_RESOLUTION:
    if (s_host_addr == 0) {
      // still awaiting DNS resolution -- stay in this state
      console_printf("In WINC_MGR_AWAITING_DNS_RESOLUTION, awaiting DNS\n");
      next_state = update_state(state, WINC_MGR_AWAITING_DNS_RESOLUTION);
    } else {
      // hostaddr has been resolved (see dns_resolve_callback) or was provided
      // a priori
      console_printf("Host IP is %d.%d.%d.%d\r\n",
                     (int)IPV4_BYTE(s_host_addr, 0),
                     (int)IPV4_BYTE(s_host_addr, 1),
                     (int)IPV4_BYTE(s_host_addr, 2),
                     (int)IPV4_BYTE(s_host_addr, 3));
      // we now have enough info to fill in the sockaddr_in structure.
      s_addr_in.sin_family = AF_INET;
      s_addr_in.sin_port = s_params->host_port;
      s_addr_in.sin_addr.s_addr = s_host_addr;
      next_state = update_state(state, WINC_MGR_INITIATING_SOCKET_CONNECTION);
      LOG_INFO(LOG_WINC_MGR_INITIATING_SOCKET_CONNECTION);
    }
    break;

  case WINC_MGR_INITIATING_SOCKET_CONNECTION:
    if (s_socket < 0) {
      // need to connect: initialize socket address structure.
      uint8_t flags = s_params->use_ssl ? SOCKET_FLAGS_SSL : 0;
      if ((s_socket = socket(AF_INET, SOCK_STREAM, flags)) < 0) {
        // failed to create TCP client socket
        next_state = update_state(state, WINC_MGR_SOCKET_FAILURE);
        LOG_INFO(LOG_WINC_MGR_SOCKET_FAILURE);
      } else if (connect(s_socket,
                         (struct sockaddr *)&s_addr_in,
                         sizeof(struct sockaddr_in)) < 0) {
        // failed to connect TCP client socket.
        next_state = update_state(state, WINC_MGR_CONNECT_FAILURE);
        LOG_INFO(LOG_WINC_MGR_CONNECT_FAILURE);
      } else {
        // successfully created and connected socket
        next_state = update_state(state, WINC_MGR_AWAITING_SOCKET_CONNECTION);
        LOG_INFO(LOG_WINC_MGR_AWAITING_SOCKET_CONNECTION);
      }
    } else {
      // already have a socket connection
      next_state = update_state(state, WINC_MGR_AWAITING_SOCKET_CONNECTION);
      LOG_INFO(LOG_WINC_MGR_AWAITING_SOCKET_CONNECTION);
    }
    break;

  case WINC_MGR_AWAITING_SOCKET_CONNECTION:
    if (s_socket_state == SOCKET_STATE_CONNECTION_SUCCEEDED) {
      next_state = update_state(state, WINC_MGR_CONNECT_SUCCESS);
      LOG_INFO(LOG_WINC_MGR_SOCKET_CONNECTED);
      // Since the WINC hasn't been contacted, it won't generate an interrupt.
    } else if (s_socket_state == SOCKET_STATE_CONNECTION_FAILED) {
      next_state = update_state(state, WINC_MGR_CONNECT_FAILURE);
      LOG_INFO(LOG_WINC_MGR_CONNECT_FAILURE);
    } else {
      console_printf(
          "In WINC_MGR_AWAITING_SOCKET_CONNECTION, awaiting socket\n");
      next_state = update_state(state, WINC_MGR_AWAITING_SOCKET_CONNECTION);
      // no change yet: keep waiting for notification from socket_cb
    }
    break;

  case WINC_MGR_CONNECT_FAILURE:
  case WINC_MGR_SOCKET_FAILURE:
    if (s_socket >= 0) {
      close(s_socket);
      s_socket = -1;
      s_socket_state = SOCKET_STATE_ENTRY;
      endgame(state);
    }
    break;

  case WINC_MGR_DNS_RESOLUTION_FAILURE:
  case WINC_MGR_AP_CONNECT_FAILURE:
    endgame(state);
    break;

  case WINC_MGR_CONNECT_SUCCESS:
    // success.
    endgame(state);
    break;

  } // switch

  TRACE_DBG("%s() Exit\n", __FUNCTION__);
  return next_state;
}

static void reset(void) {
  tstrWifiInitParam wifi_params;

  TRACE_DBG("%s() Entry\n", __FUNCTION__);

  // probably heavier handed than it needs to be...
 // socketDeinit();
  // m2m_wifi_deinit(NULL);
  //nm_bsp_deinit();

  s_winc_mgr_state = WINC_MGR_ENTRY;
  s_socket = -1;
  s_socket_state = SOCKET_STATE_ENTRY;
  s_wifi_state = WIFI_STATE_ENTRY;

  // nm_bsp_init() is called inside wifi_init()
  // nm_bsp_init();

  // Initialize wifi with the wifi parameters structure.
  memset((uint8_t *)&wifi_params, 0, sizeof(tstrWifiInitParam));
  wifi_params.pfAppWifiCb = wifi_cb;
  wifi_init(&wifi_params);
  m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);

  /* EXPERIMENT: disable logs and SNTP to see if it reduces transaction time */
  // m2m_wifi_enable_firmware_logs(true);
  // m2m_wifi_enable_sntp(true);

  /* Initialize socket module */
  socketInit();
  registerSocketCallback(socket_cb, resolve_cb);
}

static void endgame(winc_mgr_state_t state) {
  if (s_cb != NULL) {
    s_cb(winc_mgr_get_socket(), s_winc_mgr_state);
  }
}

static winc_mgr_state_t update_state(winc_mgr_state_t from,
                                     winc_mgr_state_t to) {
  // TickType_t dt = xTaskGetTickCount() - s_start_time;
  // TBD: log state changes
  // console_printf("%05ld: %s => %s\r\n",
  //                dt,
  //                winc_mgr_state_name(from),
  //                winc_mgr_state_name(to));
  return to;
}

static void wifi_cb(uint8_t u8MsgType, void *pvMsg) {

  TRACE_DBG("%s() Entry\n", __FUNCTION__);

  switch (u8MsgType) {

  case M2M_WIFI_RESP_CON_STATE_CHANGED: {
    tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;

    if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
      console_printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
      s_wifi_state = WIFI_STATE_REQUESTING_DHCP;
      m2m_wifi_request_dhcp_client();

    } else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
      console_printf(
          "wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
      s_wifi_state = WIFI_STATE_DISCONNECTED;
    }
  } break;

  case M2M_WIFI_REQ_DHCP_CONF: {
    uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
    s_wifi_state = WIFI_STATE_CONNECTED;
    // use this state to request rssi - responds with M2M_WIFI_RESP_CURRENT_RSSI
    m2m_wifi_req_curr_rssi();
    console_printf(
        "wifi_cb: M2M_WIFI_REQ_DHCP_CONF: Client IP is %u.%u.%u.%u\r\n",
        pu8IPAddress[0],
        pu8IPAddress[1],
        pu8IPAddress[2],
        pu8IPAddress[3]);
  } break;

  case M2M_WIFI_RESP_GET_SYS_TIME: {
    tstrSystemTime *systime = (tstrSystemTime *)pvMsg;
    console_printf("wifi_cb: time is %4d-%02d-%02d %02d:%02d:%02d UTC\r\n",
                   systime->u16Year,
                   systime->u8Month,
                   systime->u8Day,
                   systime->u8Hour,
                   systime->u8Minute,
                   systime->u8Second);
  } break;

  case M2M_WIFI_RESP_CURRENT_RSSI: {
    int8_t *rssi = (int8_t *)pvMsg;
    console_printf("wifi_cb: rssi = %d\r\n", (int8_t)(*rssi));
  } break;

  default: {
    console_printf("wifi_cb: got 0x%x\r\n", u8MsgType);
  } break;
  }
}

static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg) {

  TRACE_DBG("%s() Entry\n", __FUNCTION__);
  switch (u8Msg) {
  /* Socket connected */
  case SOCKET_MSG_CONNECT: {
    tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *)pvMsg;
    if (pstrConnect && pstrConnect->s8Error >= 0) {
      console_printf("socket_cb: connected.\r\n");
      s_socket_state = SOCKET_STATE_CONNECTION_SUCCEEDED;
    } else {
      console_printf("socket_cb: connect error.\r\n");
      s_socket_state = SOCKET_STATE_CONNECTION_FAILED;
    }
  } break;

  /* Message send */
  case SOCKET_MSG_SEND: {
    console_printf("socket_cb: send ok.\r\n");
    s_socket_state = SOCKET_STATE_SEND_SUCCEEDED;
  } break;

  /* Message receive */
  case SOCKET_MSG_RECV: {
    tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;
    if (pstrRecv && pstrRecv->s16BufferSize > 0) {
      console_printf("socket_cb: recv ok.\r\n");
      s_socket_state = SOCKET_STATE_RECEIVE_SUCCEEDED;

    } else {
      console_printf("socket_cb: recv error.\r\n");
      s_socket_state = SOCKET_STATE_RECEIVE_FAILED;
    }
  } break;

  default: {
    console_printf("socket_cb: got 0x%x\r\n", u8Msg);
  } break;
  }
}

static void resolve_cb(uint8_t *hostName, uint32_t hostIp) {
  // main state machine will notice that host_addr has been set...
  s_host_addr = hostIp;
}
