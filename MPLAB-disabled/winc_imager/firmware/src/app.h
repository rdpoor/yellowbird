/**
 * @file: app.h
 *
 * rdpoor@klatunetworks.com Jan 2022
 *
 * @brief WINC1500 Imaging application: read and write WINC flash contents.
 *
 * This program extracts and updates the flash memory of a Microchip WINC1500
 * WiFi module.  See adjacent ReadMe.md file for full information.
 */

#ifndef _APP_H_
#define _APP_H_

// *****************************************************************************
// Includes

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

// *****************************************************************************
// Public declarations

void APP_Initialize(void);

/**
 * @brief toplevel "superloop" state machine function.
 *
 * APP_Tasks allocates the resources required by WINC_Imager (Mounts the FAT
 * filesystem and opens the WINC1500 driver) and then repeatedly calls
 * winc_imager_step() until the WINC_Imager completes.
 */
void APP_Tasks(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _APP_H_ */
