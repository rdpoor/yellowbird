/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

/*******************************************************************************
 * Copyright (C) 2020-21 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/

/**
 * @file template.c
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "configuration.h"
#include "definitions.h"
#include "example.h"
#include "wdrv_winc_client_api.h"
#include <stdbool.h>
#include <stdint.h>

// *****************************************************************************
// Private types and definitions

#define DEFINE_STATES                                                          \
  DEFINE_STATE(APP_STATE_INIT)                                                 \
  DEFINE_STATE(APP_STATE_WDRV_INIT_READY)                                      \
  DEFINE_STATE(APP_STATE_WDRV_OPEN)

#undef DEFINE_STATE
#define DEFINE_STATE(_name) _name,
typedef enum { DEFINE_STATES } APP_STATES;

/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    Application strings and buffers are be defined outside this structure.
 */
typedef struct {
  APP_STATES state;
} APP_DATA;

// *****************************************************************************
// Private (static, forward) declarations

/**
 * @brief Set the state to the new state.
 */
static void APP_SetState(APP_STATES new_state);

/**
 * @brief Return the state name as a string.
 */
static const char *APP_StateName(APP_STATES state);

// *****************************************************************************
// Private (static) storage

static DRV_HANDLE wdrvHandle;
static APP_DATA appData;

#undef DEFINE_STATE
#define DEFINE_STATE(_name) #_name,
static const char *appStateNames[] = {DEFINE_STATES};

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_DEBUG); // more chatty, please...
  APP_SetState(APP_STATE_INIT);
}

void APP_Tasks(void) {
  /* Check the application's current state. */
  switch (appData.state) {
  case APP_STATE_INIT: {

    if (SYS_STATUS_READY == WDRV_WINC_Status(sysObj.drvWifiWinc)) {
      APP_SetState(APP_STATE_WDRV_INIT_READY);
    }

    break;
  }

  case APP_STATE_WDRV_INIT_READY: {
    wdrvHandle = WDRV_WINC_Open(0, 0);

    if (DRV_HANDLE_INVALID != wdrvHandle) {
      APP_ExampleInitialize(wdrvHandle);
      APP_SetState(APP_STATE_WDRV_OPEN);
    }
    break;
  }

  case APP_STATE_WDRV_OPEN: {
    APP_ExampleTasks(wdrvHandle);
    break;
  }

  default: {
    /* TODO: Handle error in application's state machine. */
    break;
  }
  }
}

// *****************************************************************************
// Private (static) code

static void APP_SetState(APP_STATES new_state) {
  if (new_state != appData.state) {
    SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                    "\n%s => %s",
                    APP_StateName(appData.state),
                    APP_StateName(new_state));
    appData.state = new_state;
  }
}

static const char *APP_StateName(APP_STATES state) {
  if (state < sizeof(appStateNames) / sizeof(appStateNames[0])) {
    return appStateNames[state];
  } else {
    return "unknown state";
  }
}
