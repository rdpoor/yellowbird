/**
 * @file template.c
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "bsp/bsp.h"
#include "configuration.h"
#include "definitions.h"
#include "user.h"
#include "wdrv_winc_client_api.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// *****************************************************************************
// Private types and definitions

#define WINC_IMAGER_VERSION "0.3.1"
#define MOUNT_NAME "/mnt/mydrive"
#define DEV_NAME "/dev/mmcblka1"
#define FILE_NAME "FILE_TOO_LONG_NAME_EXAMPLE_123.JPG"
#define DIR_NAME "Dir1"
#define MAX_MOUNT_RETRIES 10
#define APP_DATA_LEN 1024

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
#define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

#define DEFINE_STATES                                                          \
  DEFINE_STATE(STATE_INIT)                                                     \
  DEFINE_STATE(STATE_INIT1)                                                    \
  DEFINE_STATE(STATE_INIT2)                                                    \
  DEFINE_STATE(STATE_INIT3)                                                    \
  DEFINE_STATE(STATE_INITIALIZING_WINC)                                        \
  DEFINE_STATE(STATE_MOUNTING_FILESYSTEM)                                      \
  DEFINE_STATE(STATE_SET_CURRENT_DRIVE)                                        \
  DEFINE_STATE(STATE_OPEN_FIRST_FILE)                                          \
  DEFINE_STATE(STATE_CREATE_DIRECTORY)                                         \
  DEFINE_STATE(STATE_OPEN_SECOND_FILE)                                         \
  DEFINE_STATE(STATE_READ_WRITE_TO_FILE)                                       \
  DEFINE_STATE(STATE_CLOSE_FILE)                                               \
  DEFINE_STATE(STATE_IDLE)                                                     \
  DEFINE_STATE(STATE_ERROR)

#undef DEFINE_STATE
#define DEFINE_STATE(_name) _name,
typedef enum { DEFINE_STATES } app_state_t;

typedef struct {
  app_state_t state;
  SYS_FS_HANDLE srcFileHandle;
  SYS_FS_HANDLE dstFileHandle;
  DRV_HANDLE winc_handle;
  uint8_t sector;
  int32_t nBytesRead;
  uint8_t mount_retries;
} app_ctx_t;

// *****************************************************************************
// Private (static, forward) declarations

static void set_state(app_state_t new_state);
static const char *state_name(app_state_t state);

// *****************************************************************************
// Private (static) storage

app_ctx_t s_app_ctx;

static uint8_t BUFFER_ATTRIBUTES s_xfer_buffer[APP_DATA_LEN];

// static uint8_t CACHE_ALIGN s_file_buffer[FLASH_SECTOR_SZ];
// static uint8_t CACHE_ALIGN s_winc_buffer[FLASH_SECTOR_SZ];

#undef DEFINE_STATE
#define DEFINE_STATE(_name) #_name,
static const char *s_state_names[] = {DEFINE_STATES};

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  /* Place the App state machine in its initial state. */
  s_app_ctx.state = STATE_INIT;
  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_DEBUG);
}

void APP_Tasks(void) {
  switch (s_app_ctx.state) {

  case STATE_INIT: {
    // Arrive here on startup.  Print an introductory message...
    SYS_CONSOLE_PRINT("\n======== WINC imaging tool, version %s ========",
                      WINC_IMAGER_VERSION);
    set_state(STATE_INIT1);
  } break;

  case STATE_INIT1: {
    // Arrive here on startup.  Print an introductory message...
    SYS_CONSOLE_PRINT(
        "\nRead and write raw WINC image from and to a file on the SD card.");
    set_state(STATE_INIT2);
  } break;

  case STATE_INIT2: {
    // Arrive here on startup.  Print an introductory message...
    SYS_CONSOLE_PRINT("\nRequires:"
                      "\n  SAME54 Xplained Pro Development board");
    set_state(STATE_INIT3);
  } break;

  case STATE_INIT3: {
    // Arrive here on startup.  Print an introductory message...
    SYS_CONSOLE_PRINT("\n  WINC1500 Xplained Pro Expansion board (in EXT-1)"
                      "\n  IO/1 Xplained Pro Expansion board (in EXT-2)");
    set_state(STATE_INITIALIZING_WINC);
  } break;

  case STATE_INITIALIZING_WINC: {
    // Open the WINC150 driver and store handle in s_app_ctx.winc_handle
    s_app_ctx.winc_handle = WDRV_WINC_Open(0, DRV_IO_INTENT_EXCLUSIVE);
    if (s_app_ctx.winc_handle != DRV_HANDLE_INVALID) {
      SYS_DEBUG_MESSAGE(SYS_ERROR_INFO, "\nOpened WINC1500");
      s_app_ctx.mount_retries = MAX_MOUNT_RETRIES;
      set_state(STATE_MOUNTING_FILESYSTEM);
    } else {
      SYS_DEBUG_MESSAGE(SYS_ERROR_ERROR, "\nUnable to open WINC1500");
      set_state(STATE_ERROR);
    }
  } break;

  case STATE_MOUNTING_FILESYSTEM: {
    if (s_app_ctx.mount_retries == 0) {
      // failed to mount disk after MAX_MOUNT_RETRIES attempts
      SYS_CONSOLE_PRINT("\nfailed to mount SD card after %d attempts",
                        MAX_MOUNT_RETRIES);
      set_state(STATE_ERROR);
    } else if (SYS_FS_Mount(DEV_NAME, MOUNT_NAME, FAT, 0, NULL) != 0) {
      /* The disk was not mounted -- try again until success. */
      s_app_ctx.mount_retries -= 1;
      set_state(STATE_MOUNTING_FILESYSTEM);
    } else {
      SYS_CONSOLE_PRINT("\nsetting current drive");
      set_state(STATE_SET_CURRENT_DRIVE);
    }
  } break;

  case STATE_SET_CURRENT_DRIVE:
    /* Set current drive so that we do not have to use absolute path. */
    if (SYS_FS_CurrentDriveSet(MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      /* Error while setting current drive */
      SYS_CONSOLE_PRINT("\nerror %d while setting drive", SYS_FS_Error());
      set_state(STATE_ERROR);
    } else {
      /* Open a file for reading. */
      SYS_CONSOLE_PRINT("\nopening source file");
      set_state(STATE_OPEN_FIRST_FILE);
    }
    break;

  case STATE_OPEN_FIRST_FILE:
    s_app_ctx.srcFileHandle =
        SYS_FS_FileOpen(FILE_NAME, (SYS_FS_FILE_OPEN_READ));
    if (s_app_ctx.srcFileHandle == SYS_FS_HANDLE_INVALID) {
      /* Could not open the file. Error out*/
      SYS_CONSOLE_PRINT("\nerror %d while opening source file", SYS_FS_Error());
      set_state(STATE_ERROR);
    } else {
      /* Create a directory. */
      SYS_CONSOLE_PRINT("\ncreating directory");
      set_state(STATE_CREATE_DIRECTORY);
    }
    break;

  case STATE_CREATE_DIRECTORY:
    if (SYS_FS_DirectoryMake(DIR_NAME) == SYS_FS_RES_FAILURE) {
      /* Error while creating a new drive */
      SYS_CONSOLE_PRINT("\nerror %d while creating directory", SYS_FS_Error());
      set_state(STATE_ERROR);
    } else {
      /* Open a second file for writing. */
      SYS_CONSOLE_PRINT("\nopening target file");
      set_state(STATE_OPEN_SECOND_FILE);
    }
    break;

  case STATE_OPEN_SECOND_FILE:
    /* Open a second file inside "Dir1" */
    s_app_ctx.dstFileHandle =
        SYS_FS_FileOpen(DIR_NAME "/" FILE_NAME, (SYS_FS_FILE_OPEN_WRITE));

    if (s_app_ctx.dstFileHandle == SYS_FS_HANDLE_INVALID) {
      /* Could not open the file. Error out*/
      SYS_CONSOLE_PRINT("\nerror %d while opening destination file",
                        SYS_FS_Error());
      set_state(STATE_ERROR);
    } else {
      /* Read from one file and write to another file */
      SYS_CONSOLE_PRINT("\ncopying file contents from source to destination"
                        "\n(each . represents %d bytes copied): ",
                        APP_DATA_LEN);
      set_state(STATE_READ_WRITE_TO_FILE);
    }
    break;

  case STATE_READ_WRITE_TO_FILE:

    s_app_ctx.nBytesRead = SYS_FS_FileRead(
        s_app_ctx.srcFileHandle, (void *)s_xfer_buffer, APP_DATA_LEN);

    if (s_app_ctx.nBytesRead == -1) {
      /* There was an error while reading the file.
       * Close the file and error out. */
      SYS_CONSOLE_PRINT("\nerror %d while reading source file", SYS_FS_Error());
      SYS_FS_FileClose(s_app_ctx.srcFileHandle);
      set_state(STATE_ERROR);
    } else {
      /* If read was success, try writing to the new file */
      if (SYS_FS_FileWrite(s_app_ctx.dstFileHandle,
                           (const void *)s_xfer_buffer,
                           s_app_ctx.nBytesRead) == -1) {
        /* Write was not successful. Close the file
         * and error out.*/
        SYS_CONSOLE_PRINT("\nerror %d while writing destination file",
                          SYS_FS_Error());
        SYS_FS_FileClose(s_app_ctx.dstFileHandle);
        set_state(STATE_ERROR);
      } else if (SYS_FS_FileEOF(s_app_ctx.srcFileHandle) ==
                 1) /* Test for end of file */
      {
        /* Continue the read and write process, until the end of file is reached
         */
        SYS_CONSOLE_PRINT("\nclosing source and destination files");
        set_state(STATE_CLOSE_FILE);
      } else {
        SYS_CONSOLE_MESSAGE("."); // show progress
      }
    }
    break;

  case STATE_CLOSE_FILE:
    /* Close both files */
    SYS_FS_FileClose(s_app_ctx.srcFileHandle);
    SYS_FS_FileClose(s_app_ctx.dstFileHandle);

    /* The test was successful. Lets idle. */
    SYS_CONSOLE_PRINT("\nsuccess!");
    set_state(STATE_IDLE);
    break;

  case STATE_IDLE:
    /* The application comes here when the demo has completed
     * successfully. Glow LED1. */
    LED_ON();
    break;

  case STATE_ERROR:
    /* The application comes here when the demo has failed. */
    break;

  default:
    break;
  }
}

// *****************************************************************************
// Private (static) code

static void set_state(app_state_t new_state) {
  SYS_DEBUG_PRINT(SYS_ERROR_DEBUG,
                  "\n%s => %s",
                  state_name(s_app_ctx.state),
                  state_name(new_state));
  s_app_ctx.state = new_state;
}

static const char *state_name(app_state_t state) {
  if (state < sizeof(s_state_names) / sizeof(s_state_names[0])) {
    return s_state_names[state];
  } else {
    return "unknown state";
  }
}
