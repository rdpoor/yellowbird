/**
 * @file: template.h
 */

#ifndef _WINC_IMAGER_H_
#define _WINC_IMAGER_H_

// *****************************************************************************
// Includes

// =============================================================================
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

#define DEFINE_WINC_IMAGER_STATES                                              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_INIT)                             \
  DEFINE_WINC_IMAGER_STATE(WINC_PRINTING_VERSION)                              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_OPENING_WINC)                     \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_MOUNTING_DRIVE)                   \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_SETTING_DRIVE)                    \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_CHECKING_FILE_INFO)               \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_OPENING_IMAGE_FILE)               \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_COMPARING_SECTORS)                \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_READING_FILE_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_READING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_COMPARING_BUFFERS)                \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_REQUESTING_WINC_WRITE_PERMISSION) \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_ERASING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_WRITING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_INCREMENT_WRITE_SECTOR)           \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_REQUESTING_FILE_WRITE_PERMISSION) \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_OPENING_FILE_FOR_WRITE)           \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_READING_SECTORS)                  \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_LOADING_WINC_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_WRITING_FILE_SECTOR)              \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_INCREMENT_READ_SECTOR)            \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_SUCCESS)                          \
  DEFINE_WINC_IMAGER_STATE(WINC_IMAGER_STATE_ERROR)

#undef DEFINE_WINC_IMAGER_STATE
#define DEFINE_WINC_IMAGER_STATE(_name) _name,
typedef enum { DEFINE_WINC_IMAGER_STATES } winc_imager_state_t;

// *****************************************************************************
// Public declarations

void winc_imager_init(void);

/**
 * @brief Process the WINC imager state machine, return the resulting state.
 */
winc_imager_state_t winc_imager_step(void);

winc_imager_state_t winc_imager_state(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _WINC_IMAGER_H_ */
