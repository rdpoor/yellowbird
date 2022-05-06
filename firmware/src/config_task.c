/**
 * @file config_task.c
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

#include "config_task.h"

#include "definitions.h"
#include "nv_data.h"
#include "winc_imager.h"
#include "yb_utils.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>

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
  void (*on_completion)(void *arg);
  void *completion_arg;
} config_task_ctx_t;

// *****************************************************************************
// Local (private, static) forward declarations

static void config_task_set_state(config_task_state_t new_state);
static const char *config_task_state_name(config_task_state_t state);

/**
 * @brief Set the state to final_state and invoke on_completion.
 */
static void endgame(config_task_state_t final_state);

// *****************************************************************************
// Local (private, static) storage

static char s_read_buf[MAX_CONFIG_LINE_LENGTH];

static config_task_ctx_t s_config_task_ctx;

#undef DEFINE_CONFIG_TASK_STATE
#define DEFINE_CONFIG_TASK_STATE(_name) #_name,
static const char *config_task_state_names[] = {EXPAND_CONFIG_TASK_STATES};

// *****************************************************************************
// Public code

void config_task_init(void (*on_completion)(void *arg), void *completion_arg) {
  s_config_task_ctx.state = CONFIG_TASK_STATE_INIT;
  s_config_task_ctx.on_completion = on_completion;
  s_config_task_ctx.completion_arg = completion_arg;
}

void config_task_step(void) {

  switch (s_config_task_ctx.state) {

  case CONFIG_TASK_STATE_INIT: {
    config_task_set_state(CONFIG_TASK_SETTING_DRIVE);
  } break;

  case CONFIG_TASK_SETTING_DRIVE: {
    // Set current drive so that we do not have to use absolute path.
    if (SYS_FS_CurrentDriveSet(SD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nUnable to select drive, error %d",
                      SYS_FS_Error());
      endgame(CONFIG_TASK_STATE_ERROR);
    } else {
      config_task_set_state(CONFIG_TASK_STATE_OPENING_CONFIG);
    }
  } break;

  case CONFIG_TASK_STATE_OPENING_CONFIG: {
    // Arrive here with file system mounted.  Look for "config.txt"
    s_config_task_ctx.file_handle =
        SYS_FS_FileOpen(CONFIG_FILE_NAME, SYS_FS_FILE_OPEN_READ);
    if (s_config_task_ctx.file_handle == SYS_FS_HANDLE_INVALID) {
      SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                      "\nUnable to open config file, error %d",
                      SYS_FS_Error());
      endgame(CONFIG_TASK_STATE_ERROR);
    } else {
      config_task_set_state(CONFIG_TASK_STATE_READING_CONFIG);
    }
  } break;

  case CONFIG_TASK_STATE_READING_CONFIG: {
    // Arrive here with config.txt open for reading
    if (FATFS_eof(s_config_task_ctx.file_handle)) {
      // We have processed the last line of config.txt -- all done.
      SYS_FS_FileClose(s_config_task_ctx.file_handle);
      // DEBUG STUB: hardcode the configuration parameters
      config_task_nv_data_t *config = &nv_data()->config_task_nv_data;
      strcpy(config->ssid, "pichincha");
      strcpy(config->pass, "robandmarisol");
      config->sleep_interval = 60;
      strcpy(config->winc_img_filename, "winc_19_7_6.img");
      // END OF STUB
      endgame(CONFIG_TASK_STATE_SUCCESS);
    } else {
      // read a line from the config file
      char *line = FATFS_gets(
          s_read_buf, sizeof(s_read_buf), s_config_task_ctx.file_handle);
      if (line == NULL) {
        SYS_DEBUG_PRINT(SYS_ERROR_ERROR,
                        "\nError while reading config file, error %d",
                        SYS_FS_Error());
        endgame(CONFIG_TASK_STATE_ERROR);
        // TODO:
        // } else if (parse_config_line(s_read_buf) == false) {
        //   // error while parsing config file...
        //   SYS_DEBUG_PRINT(SYS_ERROR_ERROR, "\nError while parsing config
        //   file"); endgame(CONFIG_TASK_STATE_ERROR);
      } else {
        // stay in this state to read and parse more lines
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

const char *config_task_get_ssid(void) {
  // STUB
  return "pichincha";
}

const char *config_task_get_password(void) {
  // STUB
  return "robandmarisol";
}

uint32_t config_task_get_sleep_interval_s(void) {
  return 15;
}

const char *config_task_get_winc_image_filename(void) {
  return NULL; // "winc_19_7_6.img";
}

bool config_task_succeeded(void) {
  return s_config_task_ctx.state == CONFIG_TASK_STATE_SUCCESS;
}

bool config_task_failed(void) {
  return s_config_task_ctx.state == CONFIG_TASK_STATE_ERROR;
}

// *****************************************************************************
// Local (private, static) code

static void config_task_set_state(config_task_state_t new_state) {
  if (new_state != s_config_task_ctx.state) {
    YB_LOG_STATE_CHANGE(config_task_state_name(s_config_task_ctx.state),
                        config_task_state_name(new_state));
    s_config_task_ctx.state = new_state;
  }
}

static const char *config_task_state_name(config_task_state_t state) {
  return config_task_state_names[state];
}

static void endgame(config_task_state_t final_state) {
  config_task_set_state(final_state);
  if (s_config_task_ctx.on_completion != NULL) {
    s_config_task_ctx.on_completion(s_config_task_ctx.completion_arg);
  }
}
