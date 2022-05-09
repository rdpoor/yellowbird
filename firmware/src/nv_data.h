/**
 * @file nv_data.h
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

#ifndef _NV_DATA_H_
#define _NV_DATA_H_

// *****************************************************************************
// Includes

#include "app.h"
#include "config_task.h"
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
  app_nv_data_t app_nv_data;
  config_task_nv_data_t config_task_nv_data;
} nv_data_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Return a pointer to the non-volatile nv_data structure
 *
 * Data written in the nv_data structure will be preserved across reboots.
 */
nv_data_t *nv_data(void);

/**
 * @brief Clear the non-volatile storage by setting it to 0
 */
void nv_data_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _NV_DATA_H_ */
