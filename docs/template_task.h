/**
 * @file template.h
 *
 * MIT License
 *
 * Copyright (c) 2022 Klatu Networks, Inc.
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

#ifndef _TEMPLATE_TASK_H_
#define _TEMPLATE_TASK_H_

// *****************************************************************************
// Includes

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
 * @brief Initialize the template_task.
 *
 * If on_completion is non-null, it specifies a function to call when the
 * task completes (either successfully or on failure).
 */
void template_task_init(void (*on_completion)(void *arg), void *completion_arg);

/**
 * @brief Return true if the task has completed successfully.
 */
bool template_task_succeeded(void);

/**
 * @brief Return true if the task has completed with an error.
 */
bool template_task_failed(void);

/**
 * @brief Advance the template_task state machine
 */
void template_task_step(void);

// *****************************************************************************
// EOF

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _TEMPLATE_TASK_H_ */
