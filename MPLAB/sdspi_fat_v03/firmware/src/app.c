/**
 * @file template.c
 */

// *****************************************************************************
// Includes

#include "app.h"

#include "bsp/bsp.h"
#include "configuration.h"
#include "definitions.h"
#include "system/fs/sys_fs.h"
#include "user.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// *****************************************************************************
// Private types and definitions

#define WINC_IMAGER_VERSION "0.3.0"
#define SDCARD_MOUNT_NAME "/mnt/mydrive"
#define SDCARD_DEV_NAME "/dev/mmcblka1"
#define SDCARD_FILE_NAME "FILE_TOO_LONG_NAME_EXAMPLE_123.JPG"
#define SDCARD_DIR_NAME "Dir1"

#define APP_DATA_LEN 1024

#define SECTOR_TO_OFFSET(_sector) ((_sector)*FLASH_SECTOR_SZ)
#define WINC_SECTOR_COUNT 128 // TODO: look up dynamically
#define WINC_IMAGE_SIZE SECTOR_TO_OFFSET(WINC_SECTOR_COUNT)

#define DEFINE_STATES                                                          \
  DEFINE_STATE(APP_STATE_INIT)                                                 \
  DEFINE_STATE(APP_WAIT_SWITCH_PRESS)                                          \
  DEFINE_STATE(APP_MOUNT_DISK)                                                 \
  DEFINE_STATE(APP_UNMOUNT_DISK)                                               \
  DEFINE_STATE(APP_MOUNT_DISK_AGAIN)                                           \
  DEFINE_STATE(APP_SET_CURRENT_DRIVE)                                          \
  DEFINE_STATE(APP_OPEN_FIRST_FILE)                                            \
  DEFINE_STATE(APP_CREATE_DIRECTORY)                                           \
  DEFINE_STATE(APP_OPEN_SECOND_FILE)                                           \
  DEFINE_STATE(APP_READ_WRITE_TO_FILE)                                         \
  DEFINE_STATE(APP_CLOSE_FILE)                                                 \
  DEFINE_STATE(APP_IDLE)                                                       \
  DEFINE_STATE(APP_ERROR)

#undef DEFINE_STATE
#define DEFINE_STATE(_name) _name,
typedef enum { DEFINE_STATES } APP_STATES;

typedef struct {
  SYS_FS_HANDLE srcFileHandle;
  SYS_FS_HANDLE dstFileHandle;
  APP_STATES state;
  int32_t nBytesRead;
} APP_DATA;

// *****************************************************************************
// Private (static, forward) declarations

static void set_state(APP_STATES new_state);
static const char *state_name(APP_STATES state);

// *****************************************************************************
// Private (static) storage

APP_DATA appData;

static uint8_t BUFFER_ATTRIBUTES readWriteBuffer[APP_DATA_LEN];

// static uint8_t s_file_buffer[FLASH_SECTOR_SZ];
// static uint8_t s_winc_buffer[FLASH_SECTOR_SZ];

#undef DEFINE_STATE
#define DEFINE_STATE(_name) #_name,
static const char *s_state_names[] = {DEFINE_STATES};

// *****************************************************************************
// Public code

void APP_Initialize(void) {
  /* Place the App state machine in its initial state. */
  appData.state = APP_STATE_INIT;
  SYS_DEBUG_ErrorLevelSet(SYS_ERROR_DEBUG);

  /* Wait for the switch status to return to the default "not pressed" state */
  while (SWITCH_GET() == SWITCH_STATUS_PRESSED)
    ;
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks(void) {
  /* Check the application's current state. */
  switch (appData.state) {
  case APP_STATE_INIT:
    SYS_CONSOLE_PRINT("\n========== SD FATFS Test v %s=========="
                      "\nto start, presss user button: ",
                    );
    set_state(APP_WAIT_SWITCH_PRESS);
    break;
  case APP_WAIT_SWITCH_PRESS:
    if (SWITCH_GET() == SWITCH_STATUS_PRESSED) {
      SYS_CONSOLE_PRINT("\nmounting disk");
      set_state(APP_MOUNT_DISK);
    }
    break;
  case APP_MOUNT_DISK:
    if (SYS_FS_Mount(SDCARD_DEV_NAME, SDCARD_MOUNT_NAME, FAT, 0, NULL) != 0) {
      /* The disk could not be mounted. Try
       * mounting again until success. */
      set_state(APP_MOUNT_DISK);
    } else {
      /* Mount was successful. Unmount the disk, for testing. */
      SYS_CONSOLE_PRINT("\nunmounting disk");
      set_state(APP_UNMOUNT_DISK);
    }
    break;

  case APP_UNMOUNT_DISK:
    if (SYS_FS_Unmount(SDCARD_MOUNT_NAME) != 0) {
      /* The disk could not be un mounted. Try
       * un mounting again untill success. */

      set_state(APP_UNMOUNT_DISK);
    } else {
      /* UnMount was successful. Mount the disk again */
      SYS_CONSOLE_PRINT("\nmounting disk again");
      set_state(APP_MOUNT_DISK_AGAIN);
    }
    break;

  case APP_MOUNT_DISK_AGAIN:
    if (SYS_FS_Mount(SDCARD_DEV_NAME, SDCARD_MOUNT_NAME, FAT, 0, NULL) != 0) {
      /* The disk could not be mounted. Try
       * mounting again until success. */
      set_state(APP_MOUNT_DISK_AGAIN);
    } else {
      /* Mount was successful. Set current drive so that we do not have to use
       * absolute path. */
      SYS_CONSOLE_PRINT("\nsetting current drive");
      set_state(APP_SET_CURRENT_DRIVE);
    }
    break;

  case APP_SET_CURRENT_DRIVE:
    if (SYS_FS_CurrentDriveSet(SDCARD_MOUNT_NAME) == SYS_FS_RES_FAILURE) {
      /* Error while setting current drive */
      SYS_CONSOLE_PRINT("\nerror %d while setting drive", SYS_FS_Error());
      set_state(APP_ERROR);
    } else {
      /* Open a file for reading. */
      SYS_CONSOLE_PRINT("\nopening source file");
      set_state(APP_OPEN_FIRST_FILE);
    }
    break;

  case APP_OPEN_FIRST_FILE:
    appData.srcFileHandle =
        SYS_FS_FileOpen(SDCARD_FILE_NAME, (SYS_FS_FILE_OPEN_READ));
    if (appData.srcFileHandle == SYS_FS_HANDLE_INVALID) {
      /* Could not open the file. Error out*/
      SYS_CONSOLE_PRINT("\nerror %d while opening source file", SYS_FS_Error());
      set_state(APP_ERROR);
    } else {
      /* Create a directory. */
      SYS_CONSOLE_PRINT("\ncreating directory");
      set_state(APP_CREATE_DIRECTORY);
    }
    break;

  case APP_CREATE_DIRECTORY:
    if (SYS_FS_DirectoryMake(SDCARD_DIR_NAME) == SYS_FS_RES_FAILURE) {
      /* Error while creating a new drive */
      SYS_CONSOLE_PRINT("\nerror %d while creating directory", SYS_FS_Error());
      set_state(APP_ERROR);
    } else {
      /* Open a second file for writing. */
      SYS_CONSOLE_PRINT("\nopening target file");
      set_state(APP_OPEN_SECOND_FILE);
    }
    break;

  case APP_OPEN_SECOND_FILE:
    /* Open a second file inside "Dir1" */
    appData.dstFileHandle = SYS_FS_FileOpen(SDCARD_DIR_NAME "/" SDCARD_FILE_NAME,
                                          (SYS_FS_FILE_OPEN_WRITE));

    if (appData.dstFileHandle == SYS_FS_HANDLE_INVALID) {
      /* Could not open the file. Error out*/
      SYS_CONSOLE_PRINT("\nerror %d while opening destination file",
                        SYS_FS_Error());
      set_state(APP_ERROR);
    } else {
      /* Read from one file and write to another file */
      SYS_CONSOLE_PRINT("\ncopying file contents from source to destination"
                        "\n(each . represents %d bytes copied): ",
                        APP_DATA_LEN);
      set_state(APP_READ_WRITE_TO_FILE);
    }
    break;
  case APP_READ_WRITE_TO_FILE:

    appData.nBytesRead = SYS_FS_FileRead(
        appData.srcFileHandle, (void *)readWriteBuffer, APP_DATA_LEN);

    if (appData.nBytesRead == -1) {
      /* There was an error while reading the file.
       * Close the file and error out. */
      SYS_CONSOLE_PRINT("\nerror %d while reading source file", SYS_FS_Error());
      SYS_FS_FileClose(appData.srcFileHandle);
      set_state(APP_ERROR);
    } else {
      /* If read was success, try writing to the new file */
      if (SYS_FS_FileWrite(appData.dstFileHandle,
                           (const void *)readWriteBuffer,
                           appData.nBytesRead) == -1) {
        /* Write was not successful. Close the file
         * and error out.*/
        SYS_CONSOLE_PRINT("\nerror %d while writing destination file",
                          SYS_FS_Error());
        SYS_FS_FileClose(appData.dstFileHandle);
        set_state(APP_ERROR);
      } else if (SYS_FS_FileEOF(appData.srcFileHandle) ==
                 1) /* Test for end of file */
      {
        /* Continue the read and write process, until the end of file is reached
         */
        SYS_CONSOLE_PRINT("\nclosing source and destination files");
        set_state(APP_CLOSE_FILE);
      } else {
        SYS_CONSOLE_MESSAGE("."); // show progress
      }
    }
    break;

  case APP_CLOSE_FILE:
    /* Close both files */
    SYS_FS_FileClose(appData.srcFileHandle);
    SYS_FS_FileClose(appData.dstFileHandle);

    /* The test was successful. Lets idle. */
    SYS_CONSOLE_PRINT("\nsuccess!");
    set_state(APP_IDLE);
    break;

  case APP_IDLE:
    /* The application comes here when the demo has completed
     * successfully. Glow LED1. */
    LED_ON();
    break;

  case APP_ERROR:
    /* The application comes here when the demo has failed. */
    break;

  default:
    break;
  }
}

// *****************************************************************************
// Private (static) code

static void set_state(APP_STATES new_state) {
  SYS_DEBUG_PRINT(
      SYS_ERROR_DEBUG, "\n%s => %s", state_name(appData.state), state_name(new_state));
  appData.state = new_state;
}

static const char *state_name(APP_STATES state) {
  if (state < sizeof(s_state_names) / sizeof(s_state_names[0])) {
    return s_state_names[state];
  } else {
    return "unknown state";
  }
}
