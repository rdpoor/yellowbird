/**
 * @file winc_task.h
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

#ifndef _WINC_TASK_H_
#define _WINC_TASK_H_

// *****************************************************************************
// Includes

#include "definitions.h"
#include <stdint.h>
#include <stdbool.h>

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the winc_task.
 *
 * If on_completion is non-null, it specifies a function to call when the
 * task completes (either successfully or on failure).
 */
void winc_task_init(void (*on_completion)(void *arg), void *completion_arg);

/**
 * @brief Advance the winc_task state machine
 */
void winc_task_step(void);

bool winc_task_succeeded(void);

bool winc_task_failed(void);

/**
 * @brief Return the driver handle for the WINC chip.
 */
DRV_HANDLE winc_task_get_handle(void);

/**
 * @brief Return the DHCP address of this STA.  Returns 0 if not yet set.
 */
uint32_t winc_task_get_dhcp_addr(void);

/**
 * @brief Return the time as reported by NTP.  Returns 0 if not yet set.
 */
uint32_t winc_task_get_utc(void);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _WINC_TASK_H_ */
