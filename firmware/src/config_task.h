/**
 * @file config_task.h
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

#ifndef _CONFIG_TASK_H_
#define _CONFIG_TASK_H_

// *****************************************************************************
// Includes

#include "yb_rtc.h"
#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

#define MAX_CONFIG_KEY_LENGTH 20
#define MAX_CONFIG_VALUE_LENGTH 40
#define MAX_CONFIG_LINE_LENGTH (MAX_CONFIG_KEY_LENGTH + MAX_CONFIG_VALUE_LENGTH)

/**
 * @brief Results of parsing the config.txt file are stored here.
 *
 * In turn, this struct will be saved in nv ram (backup ram) so it is preserved
 * across reboots.
 */
typedef struct {
  char ssid[MAX_CONFIG_VALUE_LENGTH];
  char pass[MAX_CONFIG_VALUE_LENGTH];
  uint32_t sleep_interval;
  char winc_img_filename[MAX_CONFIG_VALUE_LENGTH];
} config_task_nv_data_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the config_task.
 *
 * If on_completion is non-null, it specifies a function to call when the
 * config_task completes (either successfully or on failure).
 */
void config_task_init(const char *filename);

/**
 * @brief Advance the config_task state machine.
 */
void config_task_step(void);

bool config_task_succeeded(void);

bool config_task_failed(void);

/**
* @brief Release any resources allocated by config_task.
*/
void config_task_shutdown(void);

const char *config_task_get_wifi_ssid(void);

const char *config_task_get_wifi_pass(void);

yb_rtc_ms_t config_task_get_wake_interval_ms(void);

const char *config_task_get_winc_image_filename(void);

yb_rtc_ms_t config_task_get_timeout_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef __H_ */
