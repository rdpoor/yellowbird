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

#include "definitions.h"
#include "yb_log.h"
#include "yb_rtc.h"

#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define APP_VERSION "0.1.1"

#define SD_DEVICE_NAME "/dev/mmcblka1"
#define SD_MOUNT_NAME "/mnt/mydrive"
#define MOUNT_TIMEOUT_MS 10000
#define CONFIG_FILE_NAME "config.txt"
#define MAX_CONFIG_LINE_LENGTH 80

#define TASK_STATES(M)                                                         \
  M(APP_STATE_INIT)                                                            \
  M(APP_STATE_AWAIT_FILESYS)                                                   \
  M(APP_SETTING_DRIVE)                                                         \
  M(APP_STATE_OPENING_FILE)                                                  \
  M(APP_STATE_READING_FILE)                                                  \
  M(APP_STATE_SUCCESS)                                                         \
  M(APP_STATE_ERROR)

#define EXPAND_ENUM(_name) _name,
typedef enum { TASK_STATES(EXPAND_ENUM) } app_state_t;

typedef struct {
  app_state_t state;
  yb_rtc_tics_t reboot_at; // time when system first woke up
  yb_rtc_tics_t event_at;  // time when we started trying to mount FS
  uint32_t mount_retries;  // # of times SYS_FS_Mount() was called)
  SYS_FS_HANDLE file_handle;
} app_ctx_t;

// *****************************************************************************
// Local (private, static) storage

#define EXPAND_NAME(_name) #_name,
static const char *s_app_state_names[] = {TASK_STATES(EXPAND_NAME)};

static char s_read_buf[MAX_CONFIG_LINE_LENGTH];

static app_ctx_t s_app_ctx;

// *****************************************************************************
// Local (private, static) forward declarations

static void app_set_state(app_state_t new_state);

static const char *app_state_name(app_state_t state);

static void print_banner(void);

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  yb_rtc_init();
  s_app_ctx.state = APP_STATE_INIT;
  s_app_ctx.reboot_at = yb_rtc_now();
}

void APP_Tasks(void) {
  switch (s_app_ctx.state) {

  case APP_STATE_INIT: {
    s_app_ctx.event_at = yb_rtc_now();
    print_banner();
    s_app_ctx.mount_retries = 1;
    app_set_state(APP_STATE_AWAIT_FILESYS);
  } break;

  case APP_STATE_AWAIT_FILESYS: {
    // Here while waiting for filesystem to initialize.  According to the docs,
    // the app should call SYS_FS_Mount() repeatedly until success or timeout.
    if (SYS_FS_Mount(SD_DEVICE_NAME, SD_MOUNT_NAME, FAT, 0, NULL) ==
        SYS_FS_RES_SUCCESS) {
      // mount succeeded
      YB_LOG_DEBUG("Mounted FS after %ld calls (%.0f ms) to SYS_FS_Mount()",
                   s_app_ctx.mount_retries,
                   yb_rtc_elapsed_ms(s_app_ctx.event_at));
      app_set_state(APP_SETTING_DRIVE);
    } else if (yb_rtc_elapsed_ms(s_app_ctx.event_at) >= MOUNT_TIMEOUT_MS) {
      // Timed out waiting for mount...
      YB_LOG_FATAL("Could not mount FS after %d ms - quit", MOUNT_TIMEOUT_MS);
      app_set_state(APP_STATE_ERROR);
    } else {
      // Remain in this state to retry mount
      s_app_ctx.mount_retries += 1;
    }
  } break;

  case APP_SETTING_DRIVE: {
    // Set current drive so that we do not have to use absolute path.
    if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      YB_LOG_ERROR("Unable to select drive, error %d", SYS_FS_Error());
      app_set_state(APP_STATE_ERROR);
    } else {
      app_set_state(APP_STATE_OPENING_FILE);
    }
  } break;

  case APP_STATE_OPENING_FILE: {
    // Arrive here with file system mounted.  Look for "config.txt"
    s_app_ctx.file_handle =
        SYS_FS_FileOpen(CONFIG_FILE_NAME, SYS_FS_FILE_OPEN_READ);
    if (s_app_ctx.file_handle == SYS_FS_HANDLE_INVALID) {
      YB_LOG_ERROR("Unable to open config file, error %d", SYS_FS_Error());
      app_set_state(APP_STATE_ERROR);
    } else {
      // debugging...
      SYS_FS_FSTAT file_stat;
      SYS_FS_RESULT ret = SYS_FS_FileStat(CONFIG_FILE_NAME, &file_stat);
      YB_LOG_INFO("FileStat of %s: ret=%d, siz=%d, fattrib=0x%x, name=%s",
                  CONFIG_FILE_NAME,
                  ret,
                  file_stat.fsize,
                  file_stat.fattrib,
                  file_stat.fname);
      app_set_state(APP_STATE_READING_FILE);
    }
  } break;

  case APP_STATE_READING_FILE: {
    // Arrive here with config.txt open for reading
    if (SYS_FS_FileEOF(s_app_ctx.file_handle)) {
      // We have processed the last line of config.txt -- all done.
      SYS_FS_FileClose(s_app_ctx.file_handle);
      YB_LOG_INFO("SYS_FS_FileEOF() returned true");
      app_set_state(APP_STATE_SUCCESS);
    } else {
      // read a line from the config file
      char *p = s_read_buf;
      // Stop at MAX_CONFIG_LINE_LENGTH so we can always null terminate
      for (int i = 0; i < MAX_CONFIG_LINE_LENGTH - 1; i++) {
        char c;
        size_t n_read = SYS_FS_FileRead(s_app_ctx.file_handle, &c, 1);
        if (n_read == 0) {
          break;
        } else if  (n_read == -1) {
          break;
        } else if (c == '\r') {
          // silently eat \r
        } else if (c == '\n') {
          break;
        } else {
          *p++ = c;
        }
      }
      *p = '\0'; // null terminate
      // Print the line
      YB_LOG_INFO("Read: '%s'", s_read_buf);
    }
  } break;

  case APP_STATE_SUCCESS: {
    // remain in this state
  } break;

  case APP_STATE_ERROR: {
    // remain in this state
  } break;

  } // switch
}

yb_rtc_ms_t app_uptime_ms(void) {
  return yb_rtc_elapsed_ms(s_app_ctx.reboot_at);
}

// *****************************************************************************
// Local (private, static) code

static void app_set_state(app_state_t new_state) {
  if (new_state != s_app_ctx.state) {
    YB_LOG_INFO(
        "%s => %s", app_state_name(s_app_ctx.state), app_state_name(new_state));
    s_app_ctx.state = new_state;
  }
}

static const char *app_state_name(app_state_t state) {
  return s_app_state_names[state];
}

static void print_banner(void) {
  printf("\n##############################"
         "\n# SD Read Test with FreeRTOS"
         "\n##############################");
}
