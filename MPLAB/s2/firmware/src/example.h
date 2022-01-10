/**
 * @file: template.h
 */

#ifndef _EXAMPLE_H_
#define _EXAMPLE_H_

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

void APP_ExampleInitialize(DRV_HANDLE handle);

void APP_ExampleTasks(DRV_HANDLE handle);

// *****************************************************************************
// Public declarations

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _EXAMPLE_H_ */
