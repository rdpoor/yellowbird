/**
 * @file main.c
 */

// *****************************************************************************
// Includes

#include "definitions.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static, forward) declarations

// *****************************************************************************
// Private (static) storage

// *****************************************************************************
// Public code

int main(void) {
  SYS_Initialize(NULL);

  while (true) {
    SYS_Tasks();
  }
  // Execution should not come here during normal operation
  return (EXIT_FAILURE);
}

// *****************************************************************************
// Private (static) code
