/** 
 * @file io_lib.h
 * @version  V0.1
 * @date     Oct 2014 
 * @author Martin Horauer
 *
 * @brief Configure IOs for use with LEDs, buttons, etc.
 */
#ifndef _io_driver_
#define _io_driver_

#include <stdint.h>
#include <stdbool.h>
#include <xmc_gpio.h>
#include "io_lib.h"

/******************************************************************** DEFINES */
/*enum pins {P1_0=0, P1_1, P1_2, P1_3, P1_4, P1_5, P1_6, P1_7, \*/
/*           P1_8, P1_9, P1_10, P1_11, P1_12, P1_13, P1_14, P1_15};*/

#define LED1 P1_1
#define LED2 P1_0

#define BUTTON1 P1_14
#define BUTTON2 P1_15

#define SM_TIMER5MS 5
#define SM_TIMER10MS 10

#define L1 1
#define L2 0
#define B1 14
#define B2 15

/******************************************************** FUNCTION PROTOTYPES */
_Bool setupButtonsOnRelaxkit(uint8_t button);
uint8_t setupButtonScanMode(uint8_t mode);
void scanButtonsWithDebounce(void);

_Bool setupLedsOnRelaxkit(uint8_t pin);
_Bool toggleLed(uint8_t led);

void enterSleepmode(void);

#endif
/** EOF */
