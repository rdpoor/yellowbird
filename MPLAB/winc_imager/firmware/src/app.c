/**
 * @file: app.c
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "definitions.h"
#include "winc_imager.h"
#include "wdrv_winc_client_api.h"
#include <stdint.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

#define WINC_IMAGER_VERSION "1.0.0"
#define IO1_LED_ON() IO1_LED_Clear()
#define IO1_LED_OFF() IO1_LED_Set()
#define IO1_LED_TOGGLE() IO1_LED_Toggle()

typedef enum {
  APP_STATE_INIT,
  APP_STATE_RUNNING_IMAGER,
  APP_STATE_SUCCESS,
  APP_STATE_ERROR,
} app_state_t;

typedef struct {
  app_state_t state;
} app_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

static void set_state(app_state_t new_state);

// *****************************************************************************
// Private (static) storage

app_ctx_t s_app_ctx;

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  /* Place the App state machine in its initial state. */
  s_app_ctx.state = APP_STATE_INIT;
  winc_imager_init();
}

void APP_Tasks(void) {
  /* Check the application's current state. */
  switch (s_app_ctx.state) {
  /* Application's initial state. */
  case APP_STATE_INIT: {
    IO1_LED_OFF(); // IO1 LED off
    LED_Off();     // LED on E54 XPRO board off
    SYS_CONSOLE_PRINT("\n======"
                      "\nWINC Imager v %s"
                      "\n  Connect WINC1500 XPRO to SAME54 XPRO EXT1"
                      "\n  Connect IO1 XPRO to SAME54 XPRO EXT2"
                      "\n======",
                      WINC_IMAGER_VERSION);
    SYS_DEBUG_ErrorLevelSet(SYS_ERROR_INFO);
    SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nAllocating system resources...");
    s_app_ctx.state = APP_STATE_RUNNING_IMAGER;
  } break;

  case APP_STATE_RUNNING_IMAGER: {
    // run the winc imager state machine
    winc_imager_state_t wi_state = winc_imager_step();

    if (wi_state == WINC_IMAGER_STATE_SUCCESS) {
      SYS_CONSOLE_PRINT("\nwinc_imager completed successfully.");
      set_state(APP_STATE_SUCCESS);
    } else if (wi_state == WINC_IMAGER_STATE_ERROR) {
      SYS_CONSOLE_PRINT("\nwinc_imager terminated with an error.");
      set_state(APP_STATE_ERROR);
    } else {
      // remain in this state.
    }
  } break;

  case APP_STATE_SUCCESS: {
    // On success, turn off IO1 LED and turn on LED on E54 XPRO
    IO1_LED_OFF();
    LED_On();
  } break;

  case APP_STATE_ERROR:
  default: {
    // On failure, turn on IO1 LED and turn off LED on E54 XPRO
    IO1_LED_ON();
    LED_Off();
  } break;

  } // switch()
}

// *****************************************************************************
// Private (static) code

static void set_state(app_state_t new_state) {
  s_app_ctx.state = new_state;
}
