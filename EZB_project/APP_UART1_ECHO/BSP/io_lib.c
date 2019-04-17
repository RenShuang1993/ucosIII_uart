/** 
 * @file io_lib.c
 * @version  V0.1
 * @date     Oct 2014 
 * @author Martin Horauer
 *
 * @brief A simple library to abstract IO interfacing with LEDs, buttons, etc.
 */
#include "io_lib.h"
#include "io_driver.h"
/******************************************************************** GLOBALS */
_Bool full, empty;
uint8_t inix, outix;
uint8_t cb[CBSIZE];

/**
 * @brief Configures the IOs for both LEDs on the RelaxKit board.
 * @return true on success, false otherwise
 */
_Bool configureLeds(void) {
  if (setupLedsOnRelaxkit(L1) == false) {
    return false;
  }
  if (setupLedsOnRelaxkit(L2) == false) {
    return false;
  }
  return true;
}

/**
 * @brief Configures the IOs for both buttons on the RelaxKit board. 
 *        Additionally, configures the scan mode how to read the keys.
 *        - SM_TIMER5MS .... scan every 5ms system timer based
 *        - SM_TIMER10MS ... scan every 10ms system timer based
 *
 * @return true on success, false otherwise
 */
_Bool configureButtons(void) {
  if (setupButtonsOnRelaxkit(B1) == false) {
    return false;
  }
  if (setupButtonsOnRelaxkit(B2) == false) {
    return false;
  }
  setupButtonScanMode(SM_TIMER5MS);
  return true;
}

/**
 * @brief Initialize the variables used for the circular buffer between the
 *        timer service routine (button scan) and the application.
 */
void cbInit (void) {
  inix = 0;
  outix = 0;
  full = false;
  empty = true;
}

/**
 * @brief Function to put one key in the circular buffer.
 * @param item holds the value of the key to put into the buffer.
 * @return true on success, false otherwise
 */
_Bool cbPut (uint8_t item) {
  if (full) {
    return false;
  }
  inix = (inix + 1) % CBSIZE;
  cb[inix] = item;
  if (inix == outix) {
    full = true;
  }
  empty = false;
  return true;
}

/**
 * @brief Function to obtain a key from the circular buffer.
 * @param *pItem holds the address where to store the obtained value.
 * @return true on success, false otherwise
 */
_Bool cbGet (uint8_t *pItem) {
  if (empty) {
    return false;
  }
  outix = (outix + 1) % CBSIZE;
  *pItem = cb[outix];
  if (outix == inix) {
    empty = true;
  }
  full = false;
  return true;
}
/** EOF */
