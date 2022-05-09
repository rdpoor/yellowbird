/**
 * @file app.c
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

#include "app.h"

#include "config_task.h"
#include "definitions.h"
#include "http_task.h"
#include "imager_task.h"
#include "mu_strbuf.h"
#include "nv_data.h"
#include "winc_task.h"
#include "yb_log.h"
#include "yb_rtc.h"

#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define APP_VERSION "0.1.0"

#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"
#define MOUNT_TIMEOUT_MS 10000
#define CONFIG_FILE_NAME "config.txt"

#define TCP_BUFFER_SIZE 1460
#define TCP_SEND_MESSAGE                                                       \
  "GET /index.html HTTP/1.1\r\n"                                               \
  "Host: example.com\r\n"                                                      \
  "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:95.0)\r\n"         \
  "Accept: */*;q=0.8\r\n"                                                      \
  "\r\n"

#define TASK_STATES(M)                                                         \
  M(APP_STATE_INIT)                                                            \
  M(APP_STATE_AWAIT_FILESYS)                                                   \
  M(APP_STATE_START_CONFIG_TASK)                                               \
  M(APP_STATE_AWAIT_CONFIG_TASK)                                               \
  M(APP_STATE_START_IMAGER_TASK)                                               \
  M(APP_STATE_AWAIT_IMAGER_TASK)                                               \
  M(APP_STATE_WARM_BOOT)                                                       \
  M(APP_STATE_AWAIT_WINC)                                                      \
  M(APP_STATE_START_WINC_TASK)                                                 \
  M(APP_STATE_AWAIT_WINC_TASK)                                                 \
  M(APP_STATE_START_HIBERNATION)                                               \
  M(APP_STATE_TIMED_OUT)                                                       \
  M(APP_STATE_ERROR)

#define EXPAND_ENUM(_name) _name,
typedef enum { TASK_STATES(EXPAND_ENUM) } app_state_t;

typedef struct {
  app_state_t state;
  yb_rtc_tics_t reboot_at;        // time when system first woke up
  yb_rtc_tics_t timeout_start_at; // start time for timeout timer
  bool timeout_is_active;         // true when timeout timer is running
  uint32_t mount_retries;         // # of times SYS_FS_Mount() was called)
} app_ctx_t;

// *****************************************************************************
// Local (private, static) storage

static uint8_t s_response_buf[TCP_BUFFER_SIZE];

static mu_strbuf_t s_request_msg;
static mu_strbuf_t s_response_msg;

#define EXPAND_NAME(_name) #_name,
static const char *s_app_state_names[] = {TASK_STATES(EXPAND_NAME)};

static app_ctx_t s_app_ctx;

// *****************************************************************************
// Local (private, static) forward declarations

static void app_set_state(app_state_t new_state);

static const char *app_state_name(app_state_t state);

/**
 * @brief Called when a sub-task completes: run state machine.
 */
void app_step(void *arg);

/**
 * @brief Print the startup banner.
 */
static void print_banner(void);

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  s_app_ctx.state = APP_STATE_INIT;
  s_app_ctx.timeout_is_active = false;
  if (app_is_cold_boot()) {
    nv_data_clear(); // forget everything you knew...
    yb_rtc_init();
  }
  mu_strbuf_init_from_cstr(&s_request_msg, TCP_SEND_MESSAGE);
  mu_strbuf_init_rw(&s_response_msg, s_response_buf, TCP_BUFFER_SIZE);
  s_app_ctx.reboot_at = yb_rtc_now();
}

void APP_Tasks(void) {

  if (s_app_ctx.timeout_is_active &&
      yb_rtc_elapsed_ms(s_app_ctx.timeout_start_at) >
          config_task_get_timeout_ms()) {
    // timed out before completing HTTP exchange
    app_set_state(APP_STATE_TIMED_OUT);
  }

  switch (s_app_ctx.state) {

  case APP_STATE_INIT: {
    nv_data()->app_nv_data.reboot_count += 1;
    // Dubious coding practice: we are using timeout_start_at for two things:
    // timeout for mounting FS and for failure to complete HTTP exchange.  But
    // they're mutually exclusive, so we can get away with it.  See also
    // APP_STATE_WARM_BOOT
    s_app_ctx.timeout_start_at = yb_rtc_now();
    print_banner();
    if (app_is_cold_boot()) {
      s_app_ctx.mount_retries = 1;
      app_set_state(APP_STATE_AWAIT_FILESYS);
    } else {
      app_set_state(APP_STATE_WARM_BOOT);
    }
  } break;

  case APP_STATE_AWAIT_FILESYS: {
    // Here while waiting for filesystem to initialize.  According to the docs,
    // the app should call SYS_FS_Mount() repeatedly until success or timeout.
    if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) ==
        SYS_FS_RES_SUCCESS) {
      // mount succeeded
      YB_LOG_DEBUG("Mounted FS after %ld calls to SYS_FS_Mount()",
                   s_app_ctx.mount_retries);
      app_set_state(APP_STATE_START_CONFIG_TASK);
    } else if (yb_rtc_elapsed_ms(s_app_ctx.timeout_start_at) >=
               MOUNT_TIMEOUT_MS) {
      // Timed out waiting for mount...
      YB_LOG_FATAL("Could not mount FS after %d ms - quitting",
                   MOUNT_TIMEOUT_MS);
      app_set_state(APP_STATE_ERROR);
    } else {
      // Remain in this state to retry mount
      s_app_ctx.mount_retries += 1;
    }
  } break;

  case APP_STATE_START_CONFIG_TASK: {
    YB_LOG_INFO("Reading configuration file from %s", CONFIG_FILE_NAME);
    config_task_init(CONFIG_FILE_NAME);
    app_set_state(APP_STATE_AWAIT_CONFIG_TASK);
  } break;

  case APP_STATE_AWAIT_CONFIG_TASK: {
    config_task_step();
    if (config_task_failed()) {
      YB_LOG_FATAL("Unable to open configuration file '%s' - quitting",
                   CONFIG_FILE_NAME);
      app_set_state(APP_STATE_ERROR);
    } else if (config_task_succeeded()) {
      if (config_task_get_winc_image_filename() != NULL) {
        // A WINC image filename was found -- update WINC firmware with it.
        app_set_state(APP_STATE_START_IMAGER_TASK);
      } else {
        // Skip updating the WINC
        app_set_state(APP_STATE_WARM_BOOT);
      }
    } else {
      // still processing config file -- remain in this state.
    }
  } break;

  case APP_STATE_START_IMAGER_TASK: {
    YB_LOG_INFO("Reflashing WINC from %s", CONFIG_FILE_NAME);
    imager_task_init(config_task_get_winc_image_filename());
    app_set_state(APP_STATE_AWAIT_IMAGER_TASK);
  } break;

  case APP_STATE_AWAIT_IMAGER_TASK: {
    imager_task_step();
    if (imager_task_failed()) {
      YB_LOG_FATAL("Unable to reflash WINC from %s - quitting",
                   config_task_get_winc_image_filename());
      app_set_state(APP_STATE_ERROR);
    } else if (imager_task_succeeded()) {
      app_set_state(APP_STATE_WARM_BOOT);
    } else {
      // still running imager_task() - remain in this state
    }
  } break;

  case APP_STATE_WARM_BOOT: {
    // Arrive here on warm boot or after cold boot tasks complete.
    if (app_is_cold_boot()) {
      nv_data()->app_nv_data.wake_at = yb_rtc_now();
    }
    s_app_ctx.timeout_start_at = yb_rtc_now();
    s_app_ctx.timeout_is_active = true;
    app_set_state(APP_STATE_AWAIT_WINC);
  } break;

  case APP_STATE_AWAIT_WINC: {
    if (SYS_STATUS_READY == WDRV_WINC_Status(sysObj.drvWifiWinc)) {
      app_set_state(APP_STATE_START_WINC_TASK);
    } else {
      // remain in this state until WINC is ready
    }
  } break;

  case APP_STATE_START_WINC_TASK: {
    YB_LOG_INFO("Starting WINC task");
    winc_task_connect(config_task_get_wifi_ssid(), config_task_get_wifi_pass());
    app_set_state(APP_STATE_AWAIT_WINC_TASK);
  } break;

  case APP_STATE_AWAIT_WINC_TASK: {
    winc_task_step();
    if (winc_task_failed()) {
      YB_LOG_FATAL("WINC task failed - quitting");
      app_set_state(APP_STATE_ERROR);
    } else if (winc_task_succeeded()) {
      nv_data()->app_nv_data.success_count += 1;
      app_set_state(APP_STATE_START_HIBERNATION);
    } else {
      // winc task has not completed -- remain in this state
    }
  } break;

  case APP_STATE_TIMED_OUT: {
    // software timed out before completion.
    YB_LOG_INFO("timed out");
    app_set_state(APP_STATE_START_HIBERNATION);
  } break;

  case APP_STATE_START_HIBERNATION: {
    // TODO: release WINC and any other resources.
    YB_LOG_INFO("Preparing to hibernate");
    http_task_shutdown();
    winc_task_shutdown();
    imager_task_shutdown();
    config_task_shutdown();
    SYS_FS_Unmount(SD_MOUNT_NAME);
    yb_rtc_tics_t wake_at = nv_data()->app_nv_data.wake_at;
    wake_at = yb_rtc_offset(wake_at, config_task_get_wake_interval_ms());
    // record the time at which we next want to wake...
    nv_data()->app_nv_data.wake_at = wake_at;
    yb_rtc_hibernate_until(wake_at);
  } break;

  case APP_STATE_ERROR: {
    // Cannot proceed due to some error.
    // TODO: blink LED or other error indicator.
  } break;
  } // switch
}

bool app_is_cold_boot(void) {
  // A warm boot is any form of wakeup from hibernation; all else is cold boot.
  RSTC_RESET_CAUSE rcause = RSTC_ResetCauseGet();
  bool is_warm_boot = rcause & 0x80; // msb signifies backup reset (e.g. RTC)
  return !is_warm_boot;
}

yb_rtc_ms_t app_uptime_ms(void) {
  return yb_rtc_elapsed_ms(s_app_ctx.reboot_at);
}

mu_strbuf_t *app_request_msg() { return &s_request_msg; }

mu_strbuf_t *app_response_msg() { return &s_response_msg; }


// *****************************************************************************
// Local (private, static) code

static void app_set_state(app_state_t new_state) {
  if (new_state != s_app_ctx.state) {
    YB_LOG_INFO("%s => %s", app_state_name(s_app_ctx.state), app_state_name(new_state));
    s_app_ctx.state = new_state;
  }
}

static const char *app_state_name(app_state_t state) {
  return s_app_state_names[state];
}

static void print_banner(void) {
  printf("\n##############################");
  printf("\n# Klatu Networks Yellowbird, v %s (%s boot) #%lu",
         APP_VERSION,
         app_is_cold_boot() ? "cold" : "warm",
         nv_data()->app_nv_data.reboot_count);
  printf("\n##############################");
}
