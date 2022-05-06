/**
 * @file app.c
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

#include "app.h"

#include "config_task.h"
#include "definitions.h"
#include "m2m_hif.h"
#include "nv_data.h"
#include "http_task.h"
#include "spi_flash_map.h"
#include "system/console/sys_console.h"
#include "wdrv_winc_client_api.h"
#include "winc_imager.h"
#include "winc_task.h"
#include "yb_utils.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local types and definitions

#define APP_VERSION "0.0.7"

#define MIN_SLEEP_TICS 3               // 90 microseonds should be plenty...
#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"
#define FILE_IMAGE_NAME "winc.img"

#define HTTP_HOST_NAME "www.example.com"
#define HTTP_HOST_ADDR 0
#define HTTP_HOST_PORT 443
#define HTTP_USE_TLS true
#define HTTP_RESPONSE_BUFLEN 1500

#define EXPAND_APP_STATES                                                      \
  DEFINE_APP_STATE(APP_STATE_INIT)                                             \
  DEFINE_APP_STATE(APP_STATE_AWAIT_WINC)                                       \
  DEFINE_APP_STATE(APP_STATE_PRINTING_BANNER)                                  \
  DEFINE_APP_STATE(APP_STATE_AWAIT_FILESYS)                                    \
  DEFINE_APP_STATE(APP_STATE_AWAIT_CONFIG_TASK)                                \
  DEFINE_APP_STATE(APP_STATE_AWAIT_WINC_IMAGER)                                \
  DEFINE_APP_STATE(APP_STATE_CONNECT_TO_AP)                                    \
  DEFINE_APP_STATE(APP_STATE_AWAITING_WINC_TASK)                               \
  DEFINE_APP_STATE(APP_STATE_START_HTTP)                                       \
  DEFINE_APP_STATE(APP_STATE_AWAIT_HTTP_TASK)                                  \
  DEFINE_APP_STATE(APP_STATE_HIBERNATING)                                      \
  DEFINE_APP_STATE(APP_STATE_ERROR)

#undef DEFINE_APP_STATE
#define DEFINE_APP_STATE(_name) _name,
typedef enum { EXPAND_APP_STATES } APP_STATES;

typedef struct {
  APP_STATES state;
  SYS_CONSOLE_HANDLE consoleHandle;
  uint32_t mount_retries;
} APP_DATA;

// *****************************************************************************
// Local (static, forward) declarations

static void app_set_state(APP_STATES new_state);
static const char *app_state_name(APP_STATES state);
static void print_banner(void);
static void app_go_to_sleep(void);
static bool system_is_busy(void);
static void print_winc_version(void);

// *****************************************************************************
// Local (static) storage

static APP_DATA appData;

#undef DEFINE_APP_STATE
#define DEFINE_APP_STATE(_name) #_name,
static const char *app_state_names[] = {EXPAND_APP_STATES};

static const char HTTP_REQUEST_BUF[] = {
"GET / undefined\r\n"
"Host: www.example.com\r\n"
"User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:97.0) Gecko/20100101 Firefox/97.0\r\n"
"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n"
"Accept-Language: en-US,en;q=0.5\r\n"
"Accept-Encoding: gzip, deflate, br\r\n"
"DNT: 1\r\n"
"Connection: keep-alive\r\n"
"Upgrade-Insecure-Requests: 1\r\n"
"Sec-Fetch-Dest: document\r\n"
"Sec-Fetch-Mode: navigate\r\n"
"Sec-Fetch-Site: none\r\n"
"Sec-Fetch-User: ?1\r\n"
"TE: trailers\r\n"
"\r\n"
};

static char HTTP_RESPONSE_BUF[HTTP_RESPONSE_BUFLEN];

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_INFO);
  appData.state = APP_STATE_INIT;
  if (app_is_cold_boot()) {
    nv_data_clear();     // forget everything you knew...
    RTC_Initialize();    // Re-initialize only on cold boot
    RTC_Timer32Start();  // start RTC
  }
  config_task_init(NULL, NULL);
}

void APP_Tasks(void) {
  switch (appData.state) {

  case APP_STATE_INIT: {
    appData.consoleHandle = SYS_CONSOLE_HandleGet(SYS_CONSOLE_INDEX_0);
    nv_data()->app_nv_data.reboot_count += 1;
    app_set_state(APP_STATE_AWAIT_WINC);
  } break;

  case APP_STATE_AWAIT_WINC: {
    if (SYS_STATUS_READY != WDRV_WINC_Status(sysObj.drvWifiWinc)) {
      // remain in this state until WINC is ready
    } else {
      app_set_state(APP_STATE_PRINTING_BANNER);
    }
  } break;

  case APP_STATE_PRINTING_BANNER: {
    print_banner();
    if (app_is_cold_boot()) {
      print_winc_version();
      appData.mount_retries = 0;
      app_set_state(APP_STATE_AWAIT_FILESYS);
    } else {
      app_set_state(APP_STATE_CONNECT_TO_AP);
    }
  } break;

  case APP_STATE_AWAIT_FILESYS: {
    // According to the documents, SYS_FS_Mount() should be calle repeatedly
    // until it returns success.
    appData.mount_retries += 1;
    // if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) ==
    //     SYS_FS_RES_SUCCESS) {
    //   SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
    //                   "\nSD card mounted after %ld attempts",
    //                   appData.mount_retries);
      app_set_state(APP_STATE_AWAIT_CONFIG_TASK);
    // } else if (appData.mount_retries % 100000 == 0) {
    //   // remain in this state, but print out a message now and then...
    //   SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
    //                   "\nSD card not ready after %ld attempts",
    //                   appData.mount_retries);
    // }
  } break;

  case APP_STATE_AWAIT_CONFIG_TASK: {
    // repeatedly call config_task state machine until it completes
    // config_task_step();
    // if (config_task_succeeded()) {
    //   const char *winc_image_filename = config_task_get_winc_image_filename();
    //   // If a winc_image filename was provided, start the winc_imager task.
    //   if (winc_image_filename != NULL) {
    //     winc_imager_init(winc_image_filename, NULL, NULL);
    //     app_set_state(APP_STATE_AWAIT_WINC_IMAGER);
    //   } else {
        app_set_state(APP_STATE_CONNECT_TO_AP);
    //   }
    // } else if (config_task_failed()) {
    //   app_set_state(APP_STATE_ERROR);
    // } else {
    //   // remain in this state
    // }
  } break;

  case APP_STATE_AWAIT_WINC_IMAGER: {
    // Repeatedly call the winc_imager state machine until it completes.
    winc_imager_step();
    if (winc_imager_succeeded()) {
        app_set_state(APP_STATE_CONNECT_TO_AP);
    } else if (winc_imager_failed()) {
      app_set_state(APP_STATE_ERROR);
    } else {
      // remain in this state
    }
  } break;

  case APP_STATE_CONNECT_TO_AP: {
    winc_task_init(NULL, NULL);
    app_set_state(APP_STATE_AWAITING_WINC_TASK);
  } break;

  case APP_STATE_AWAITING_WINC_TASK: {
    // Repeatedly call the winc_task state machine until it completes.
    winc_task_step();
    if (winc_task_succeeded()) {
      app_set_state(APP_STATE_START_HTTP);
    } else if (winc_task_failed()) {
      app_set_state(APP_STATE_ERROR);
    } else {
      // remain in this state
    }
  } break;

  case APP_STATE_START_HTTP: {
    // APP_WiFiConnectNotifyCallback triggered.  Wait for IP link.
    http_task_init(HTTP_HOST_NAME,
                   HTTP_HOST_ADDR,
                   HTTP_HOST_PORT,
                   HTTP_USE_TLS,
                   HTTP_REQUEST_BUF,
                   strlen(HTTP_REQUEST_BUF),
                   HTTP_RESPONSE_BUF,
                   HTTP_RESPONSE_BUFLEN,
                   0,
                   0);
    app_set_state(APP_STATE_AWAIT_HTTP_TASK);
  } break;

  case APP_STATE_AWAIT_HTTP_TASK: {
    // Repeatedly call the http_task state machine until it completes.
    http_task_step(winc_task_get_handle());
    if (http_task_succeeded()) {
      // TODO: Mak sure WINC is shut down
      app_set_state(APP_STATE_HIBERNATING);
    } else if (http_task_failed()) {
      // TODO: Log error
      app_set_state(APP_STATE_HIBERNATING);
    } else {
      // remain in this state
    }
  } break;

  case APP_STATE_HIBERNATING: {
    // don't hibernate until other actions complete (e.g. printing).
    if (system_is_busy()) {
      // stay in this state
    } else {
      app_go_to_sleep();
    }
  } break;

  case APP_STATE_ERROR: {
    // here on some error
  } break;

  default: {
    /* TODO: Handle error in application's state machine. */
    break;
  }
  }
}

bool app_is_cold_boot(void) {
  // In this application a warm boot is any form of wakeup from hibernation.
  // Anything else is a cold boot.
  RSTC_RESET_CAUSE rcause = RSTC_ResetCauseGet();
  bool is_warm_boot = rcause & 0x80; // msb signifies backup reset (e.g. RTC)
  return !is_warm_boot;
}

uint32_t app_get_wake_interval_s(void) {
  return config_task_get_sleep_interval_s();
}

const char *app_get_ssid(void) {
  return config_task_get_ssid();
}

const char *app_get_pass(void) {
  return config_task_get_password();
}

// *****************************************************************************
// Local (static) code

static void app_set_state(APP_STATES new_state) {
  if (appData.state != new_state) {
    YB_LOG_STATE_CHANGE(app_state_name(appData.state),
                        app_state_name(new_state));
    appData.state = new_state;
  }
}

static const char *app_state_name(APP_STATES state) {
  return app_state_names[state];
}

static void print_banner(void) {
  SYS_CONSOLE_PRINT("\n##############################");
  SYS_CONSOLE_PRINT("\n# Klatu Networks Yellowbird, v %s (%s boot) #%d",
                    APP_VERSION,
                    app_is_cold_boot() ? "cold" : "warm",
                    nv_data()->app_nv_data.reboot_count);
  SYS_CONSOLE_PRINT("\n##############################");
}

static void app_go_to_sleep(void) {
    // Set the RTC match counter to bring the processor out of hibernation.
    // Arrive here with app_nv_dta.wake_at_tics set to the time at which the
    // system started.
    uint32_t now = RTC_Timer32CounterGet();
    uint32_t wake_interval_tics =
      app_get_wake_interval_s() * RTC_COUNTER_CLOCK_FREQUENCY;
    uint32_t then = nv_data()->app_nv_data.wake_at_tics += wake_interval_tics;
    // Preventing "sleep for a long time" errors:
    if (then - now < MIN_SLEEP_TICS) {
      // wake time is "too soon in the future" -- push it out a bit so the
      // counter hasn't passed the compare register by the time we sleep.
      then = now + MIN_SLEEP_TICS;
    } else if (then - now > wake_interval_tics) {
      // the only way then - now can be greater than wake_interval_tics is if
      // `then` is in the past.  Restart wake_at_tics in the future...
      then = now + wake_interval_tics;
    }
    nv_data()->app_nv_data.wake_at_tics = then;
    RTC_Timer32Compare0Set(then);
    RTC_Timer32InterruptEnable(RTC_TIMER32_INT_MASK_CMP0);


  PM_HibernateModeEnter();
}

static bool system_is_busy(void) {
  bool is_busy = false;
  if (SERCOM2_USART_WriteCountGet() > 0) {
    is_busy = true;
  }
  return is_busy;
}

static void print_winc_version(void) {
  return;  // not ready for prime time -- causes faults (DMA not setup?)

  // general violation of abstraction layers.  hopefully won't mess up too much.
  int8_t ret = M2M_SUCCESS;                  // nm_common.h
  uint8_t u8WifiMode = M2M_WIFI_MODE_NORMAL; // m2m_types.h
  tstrM2mRev strtmp;                         // nmdrv.h

  ret = nm_drv_init_start(&u8WifiMode); // nmdrv.h
  if (ret == M2M_SUCCESS) {
    ret = hif_init(NULL); // m2m_hif.h
    if (ret == M2M_SUCCESS) {
      ret = nm_get_firmware_full_info(&strtmp); // nmdrv.h
      SYS_CONSOLE_PRINT("\nWINC1500 Info:");
      SYS_CONSOLE_PRINT("\n  Chip ID: %ld", strtmp.u32Chipid);
      SYS_CONSOLE_PRINT("\n  Firmware Ver: %u.%u.%u SVN Rev %u",
                        strtmp.u8FirmwareMajor,
                        strtmp.u8FirmwareMinor,
                        strtmp.u8FirmwarePatch,
                        strtmp.u16FirmwareSvnNum);
      SYS_CONSOLE_PRINT("\n  Firmware Built at %s Time %s",
                        strtmp.BuildDate,
                        strtmp.BuildTime);
      SYS_CONSOLE_PRINT("\n  Firmware Min Driver Ver: %u.%u.%u",
                        strtmp.u8DriverMajor,
                        strtmp.u8DriverMinor,
                        strtmp.u8DriverPatch);
      if (M2M_ERR_FW_VER_MISMATCH == ret) {
        SYS_CONSOLE_PRINT("\n  Mismatch Firmware Version");
      }
    }
    hif_deinit(NULL); // nmdrv.h
  }
  nm_drv_deinit(NULL); // nmdrv.h
}
