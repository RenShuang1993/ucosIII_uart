/** 
 * @file io_lib.h
 * @version  V0.1
 * @date     Oct 2014 
 * @author Martin Horauer
 *
 * @brief A simple library to abstract IO interfacing with LEDs, buttons, etc.
 */
#ifndef _io_lib_
#define _io_lib_

#include <stdint.h>
#include <stdbool.h>

/******************************************************************** DEFINES */
#define CBSIZE 10

/******************************************************** FUNCTION PROTOTYPES */
_Bool configureLeds(void);
_Bool configureButtons(void);

void cbInit (void);
_Bool cbGet (uint8_t *pitem);
_Bool cbPut (uint8_t item);

#endif
/** EOF */
