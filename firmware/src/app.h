/**
 * @file app.h
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

#ifndef _APP_H_
#define _APP_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stdint.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

typedef struct {
  uint32_t reboot_count;
  uint32_t wake_at_tics;
} app_nv_data_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialization, called once at startup
 */
void APP_Initialize ( void );

/**
 * @brief Called repeatedly during execution
 */
void APP_Tasks( void );

bool app_is_cold_boot(void);
uint32_t app_get_wake_interval_s(void);
const char *app_get_ssid(void);
const char *app_get_pass(void);

#ifdef __cplusplus
}
#endif

#endif /* _APP_H */
