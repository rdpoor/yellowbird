/**
 * @file template.c
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

#include "template.h"

#include "yb_log.h"

#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Local (private) types and definitions

#define TASK_STATES(M)                                                         \
  M(TEMPLATE_TASK_STATE_INIT)                                                  \
  M(TEMPLATE_TASK_STATE_SUCCESS)                                               \
  M(TEMPLATE_TASK_STATE_ERROR)

#define EXPAND_ENUM(_name) _name,
typeef enum { TASK_STATES(EXPAND_ENUM} template_task_state_t;

typedef struct {
  template_task_state_t state;
  void (*on_completion)(void *arg);
  void *completion_arg;
} template_task_ctx_t;

// *****************************************************************************
// Local (private, static) storage

#define EXPAND_NAME(_name) #_name,
static const char *s_template_task_state_names[] = {TASK_STATES(EXPAND_NAME)};

// *****************************************************************************
// Local (private, static) forward declarations

static void template_task_set_state(template_task_state_t new_state);

static const char *template_task_state_name(template_task_state_t state);

// *****************************************************************************
// Public code

void template_task_init(void (*on_completion)(void *arg), void
                              *completion_arg) {
  s_template_task_ctx.state = TEMPLATE_TASK_STATE_INIT;
  s_template_task_ctx.on_completion = on_completion;
  s_template_task_ctx.completion_arg = completion_arg;
}

void template_task_step(void) {
  switch (template_task_ctx.state) {
    case TEMPLATE_TASK_STATE_INIT: {

    } break;
    case TEMPLATE_TASK_STATE_SUCCESS: {

    } break;
    case TEMPLATE_TASK_STATE_ERROR: {

    } break;
  } // switch
}

bool template_task_succeeded(void) {
  return s_template_task_ctx.state == TEMPLATE_TASK_STATE_SUCCESS;
}

bool template_task_failed(void) {
  return s_template_task_ctx.state == TEMPLATE_TASK_STATE_ERROR;
}

// *****************************************************************************
// Local (private, static) code

static void template_task_set_state(template_task_state_t new_state) {
  if (new_state != s_template_task_ctx.state) {
    yb_log("%s => %s", template_task_state_name(s_template_task_ctx.state),
                       template_task_state_name(new_state));
    s_template_task_ctx.state = new_state;
  }
}

static const char *template_task_state_name(template_task_state_t state) {
  return s_template_task_state_names[state];
}
