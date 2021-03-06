/**
 * @file winc_task.h
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

#ifndef _WINC_TASK_H_
#define _WINC_TASK_H_

// *****************************************************************************
// Includes

#include "definitions.h"
#include <stdbool.h>
#include <stdint.h>

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
 */
void winc_task_connect(const char *ssid, const char *pass);

/**
 * @brief Start the disconnect sequence.
 */
void winc_task_disconnect(void);

/**
 * @brief Advance the winc_task state machine
 */
void winc_task_step(void);

bool winc_task_succeeded(void);

bool winc_task_failed(void);

/**
 * @brief Release any resources allocated by winc_task.
 */
void winc_task_shutdown(void);

/**
 * @brief Return the driver handle for the WINC chip.
 *
 * NOTE: only valid when winc_task_succeeded() returns true;
 */
DRV_HANDLE winc_task_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _WINC_TASK_H_ */
