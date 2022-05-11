/**
 * @file config_task.c
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

#include "config_task.h"

#include "definitions.h"
#include "mu_cfg_parser.h"
#include "mu_str.h"
#include "mu_strbuf.h"
#include "nv_data.h"
#include "winc_imager.h"
#include "yb_log.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
// #include <strings.h>
#include <stdio.h>

// *****************************************************************************
// Local (private) types and definitions

#define SD_MOUNT_NAME "/mnt/mydrive"
#define CONFIG_FILE_NAME "config.txt"

#define EXPAND_CONFIG_TASK_STATES                                              \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_STATE_INIT)                             \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_SETTING_DRIVE)                          \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_STATE_OPENING_CONFIG)                   \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_STATE_READING_CONFIG)                   \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_STATE_SUCCESS)                          \
  DEFINE_CONFIG_TASK_STATE(CONFIG_TASK_STATE_ERROR)

#undef DEFINE_CONFIG_TASK_STATE
#define DEFINE_CONFIG_TASK_STATE(_name) _name,
typedef enum { EXPAND_CONFIG_TASK_STATES } config_task_state_t;

typedef struct {
  config_task_state_t state;
  SYS_FS_HANDLE file_handle;
  const char *file_name;
  mu_cfg_parser_t parser;
  char winc_image[MAX_CONFIG_VALUE_LENGTH];
} config_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void config_task_set_state(config_task_state_t new_state);

static const char *config_task_state_name(config_task_state_t state);

static void on_match(mu_str_t *key, mu_str_t *value);

/**
 * @brief Return true if cstr equals the contents of mu_str.
 */
static bool match_cstring(mu_str_t *mu_str, const char *cstr);

/**
 * @brief Convert a mu_str to a floating point value.
 */
static float mu_str_to_float(mu_str_t *mu_str);

// *****************************************************************************
// Local (private, static) storage

static char s_read_buf[MAX_CONFIG_LINE_LENGTH];

static config_task_ctx_t s_config_task_ctx;

#undef DEFINE_CONFIG_TASK_STATE
#define DEFINE_CONFIG_TASK_STATE(_name) #_name,
static const char *config_task_state_names[] = {EXPAND_CONFIG_TASK_STATES};

// *****************************************************************************
// Public code

void config_task_init(const char *filename) {
  s_config_task_ctx.state = CONFIG_TASK_STATE_INIT;
  s_config_task_ctx.file_name = filename;
  mu_cfg_parser_init(&s_config_task_ctx.parser, on_match);
}

void config_task_step(void) {

  switch (s_config_task_ctx.state) {

  case CONFIG_TASK_STATE_INIT: {
    config_task_set_state(CONFIG_TASK_SETTING_DRIVE);
  } break;

  case CONFIG_TASK_SETTING_DRIVE: {
    // Set current drive so that we do not have to use absolute path.
    if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      YB_LOG_ERROR("Unable to select drive, error %d", SYS_FS_Error());
      config_task_set_state(CONFIG_TASK_STATE_ERROR);
    } else {
      config_task_set_state(CONFIG_TASK_STATE_OPENING_CONFIG);
    }
  } break;

  case CONFIG_TASK_STATE_OPENING_CONFIG: {
    // Arrive here with file system mounted.  Look for "config.txt"
    s_config_task_ctx.file_handle =
        SYS_FS_FileOpen(s_config_task_ctx.file_name, SYS_FS_FILE_OPEN_READ);
    if (s_config_task_ctx.file_handle == SYS_FS_HANDLE_INVALID) {
      YB_LOG_ERROR("Unable to open config file, error %d", SYS_FS_Error());
      config_task_set_state(CONFIG_TASK_STATE_ERROR);
    } else {
      // debugging...
      SYS_FS_FSTAT file_stat;
      SYS_FS_RESULT ret =
          SYS_FS_FileStat(s_config_task_ctx.file_name, &file_stat);
      YB_LOG_INFO("FileStat of %s: ret=%d, siz=%d, fattrib=0x%x, name=%s",
                  s_config_task_ctx.file_name,
                  ret,
                  file_stat.fsize,
                  file_stat.fattrib,
                  file_stat.fname);
      config_task_set_state(CONFIG_TASK_STATE_READING_CONFIG);
    }
  } break;

  case CONFIG_TASK_STATE_READING_CONFIG: {
    // Arrive here with config.txt open for reading
    if (FATFS_eof(s_config_task_ctx.file_handle)) {
      // We have processed the last line of config.txt -- all done.
      SYS_FS_FileClose(s_config_task_ctx.file_handle);
      config_task_set_state(CONFIG_TASK_STATE_SUCCESS);
    } else {
      // read a line from the config file
      char *line = FATFS_gets(
          s_read_buf, sizeof(s_read_buf), s_config_task_ctx.file_handle);
      if (line == NULL) {
        YB_LOG_ERROR("Error while reading config file, error %d",
                     SYS_FS_Error());
        config_task_set_state(CONFIG_TASK_STATE_ERROR);
      } else {
        // parse line and remain in this state to parse more lines.
        // Be permissive: ignore parse errors.
        mu_cfg_parser_read_line(&s_config_task_ctx.parser, line);
      }
    }
  } break;

  case CONFIG_TASK_STATE_SUCCESS: {
    // remain in this state
  } break;

  case CONFIG_TASK_STATE_ERROR: {
    // remain in this state
  } break;

  } // switch()
}

bool config_task_succeeded(void) {
  return s_config_task_ctx.state == CONFIG_TASK_STATE_SUCCESS;
}

bool config_task_failed(void) {
  return s_config_task_ctx.state == CONFIG_TASK_STATE_ERROR;
}

void config_task_shutdown(void) {}

const char *config_task_get_wifi_ssid(void) {
  return nv_data()->config_task_nv_data.wifi_ssid;
}

const char *config_task_get_wifi_pass(void) {
  return nv_data()->config_task_nv_data.wifi_pass;
}

yb_rtc_ms_t config_task_get_wake_interval_ms(void) {
  return nv_data()->config_task_nv_data.wake_interval_ms;
}

yb_rtc_ms_t config_task_get_timeout_ms(void) {
  return nv_data()->config_task_nv_data.timeout_ms;
}

const char *config_task_get_winc_image_filename(void) {
  return s_config_task_ctx.winc_image;
}

// *****************************************************************************
// Local (private, static) code

static void config_task_set_state(config_task_state_t new_state) {
  if (new_state != s_config_task_ctx.state) {
    YB_LOG_INFO("%s => %s",
                config_task_state_name(s_config_task_ctx.state),
                config_task_state_name(new_state));
    s_config_task_ctx.state = new_state;
  }
}

static const char *config_task_state_name(config_task_state_t state) {
  return config_task_state_names[state];
}

static void on_match(mu_str_t *key, mu_str_t *val) {
  config_task_nv_data_t *nv = &nv_data()->config_task_nv_data;
  YB_LOG_INFO("cfg parse: key='%*s', value='%*s'",
              mu_str_available_rd(key),
              mu_str_ref_rd(key),
              mu_str_available_rd(val),
              mu_str_ref_rd(val));

  if (match_cstring(key, "wifi_ssid") == 0) {
    mu_str_to_cstr(val, nv->wifi_ssid, MAX_CONFIG_VALUE_LENGTH);
  } else if (match_cstring(key, "wifi_pass") == 0) {
    mu_str_to_cstr(val, nv->wifi_pass, MAX_CONFIG_VALUE_LENGTH);
  } else if (match_cstring(key, "wake_interval_ms") == 0) {
    nv->wake_interval_ms = mu_str_to_float(val);
  } else if (match_cstring(key, "timeout_ms") == 0) {
    nv->timeout_ms = mu_str_to_float(val);
  } else if (match_cstring(key, "winc_image") == 0) {
    // unlike the other config params, winc_image is not stored in nv ram since
    // it is only used once at cold boot
    mu_str_to_cstr(val, s_config_task_ctx.winc_image, MAX_CONFIG_VALUE_LENGTH);
  }
}

static bool match_cstring(mu_str_t *mu_str, const char *cstr) {
  size_t len = mu_str_available_rd(mu_str);
  if (len != strlen(cstr)) {
    return false;
  } else {
    return strncmp(cstr, (const char *)mu_str_ref_rd(mu_str), len) == 0;
  }
}

static float mu_str_to_float(mu_str_t *mu_str) {
  float val;
  char fmt[MAX_CONFIG_VALUE_LENGTH];
  // Dynamically create a format string for sscanf since there appears to be no
  // other way to dynamically specify a field length.
  size_t len = mu_str_available_rd(mu_str);
  snprintf(fmt, sizeof(fmt), "%%%df", len); // eg '%6f'
  sscanf((const char *)mu_str_ref_rd(mu_str), fmt, &val);
  return val;
}
