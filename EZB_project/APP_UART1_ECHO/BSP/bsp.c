/**
 * @file bsp.c
 * @brief Main board support package for uCOS-III targeting the RelaxKit board.
 *
 * @author Martin Horauer, UAS Technikum Wien
 * @revision 0.1
 * @date 02-2015
 */

/******************************************************************* INCLUDES */
#include  <bsp.h>
#include  <bsp_sys.h>
#include  <bsp_int.h>
#include  <bsp_uart.h>
#include "io_lib.h"
#include "io_driver.h"

/********************************************************* FILE LOCAL DEFINES */
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

/******************************************************* FILE LOCAL CONSTANTS */

/*********************************************************** FILE LOCAL TYPES */

/********************************************************* FILE LOCAL GLOBALS */
uint32_t p1_14_pressed;
uint32_t p1_15_pressed;
/****************************************************** FILE LOCAL PROTOTYPES */

/****************************************************************** FUNCTIONS */
/**
 * @function BSP_Init()
 * @params none
 * @returns none
 * @brief Initialization of the board support.
 */
void  BSP_Init (void)
{
	BSP_IntInit();
	BSP_UART_Init();
}
/**
 * @brief Configure the IOs where the buttons are connected to input mode.
 * @param button determines the button to configure
 *        P1_14 ... configure Port1 pin 14 -> input
 *        P1_15 ... configure Port1 pin 15 -> input
 * @return true on success, false otherwise
 */
_Bool setupButtonsOnRelaxkit(uint8_t button) {

  // declaration of locals
  XMC_GPIO_CONFIG_t config_in;
  
  // init ports connected to the buttons
  config_in.mode = XMC_GPIO_MODE_INPUT_SAMPLING;

  if (button == B1) {
    XMC_GPIO_Init(BUTTON1, &config_in);
    return true;
  }
  if (button == B2) {
    XMC_GPIO_Init(BUTTON2, &config_in);
    return true;
  }
  return false;
}
/**
 * @brief Read buttons based on timer service routine with debounce 
 *        functionality. The time needs to be selected in a way that ensures 
 *        that bouncing effects are negligible. The functions puts a BUTTON1 or 
 *        BUTTON2 click event in the circular buffer.
 */
void scanButtonsWithDebounce(void) 
{
  /*if ((XMC_GPIO_GetInput(BUTTON1) == 0) && (p1_14_pressed == 2))
  {
    if (cbPut(B1) == true)
      p1_14_pressed = 0;
  }*/
  if ((XMC_GPIO_GetInput(BUTTON1) == 0) && (p1_14_pressed == 1))
  {
      if (cbPut(B1) == true)
      p1_14_pressed = 0;
    //p1_14_pressed = 2;
  }
  if ((XMC_GPIO_GetInput(BUTTON1) == 0) && (p1_14_pressed == 0))
    p1_14_pressed = 1;
  
  if ((XMC_GPIO_GetInput(BUTTON2) == 0) && (p1_15_pressed == 2))
  {
    if (cbPut(B2) == true)
      p1_15_pressed = 0;
  }
  if ((XMC_GPIO_GetInput(BUTTON2) == 0) && (p1_15_pressed == 1))
    p1_15_pressed = 2;
  if ((XMC_GPIO_GetInput(BUTTON2) == 0) && (p1_15_pressed == 0))
    p1_15_pressed = 1;
}
/**
 * @brief Configure the IOs where the LEDs are connected to output mode using a 
 *        strong driver in push-pull mode.
 * @param led determines the LED to setup
 *        P1_0 ... configure Port1 pin 0 - LED2
 *        P1_1 ... configure Port1 pin 1 - LED1
 * @return true on success, false otherwise
 */
_Bool setupLedsOnRelaxkit(uint8_t led) {

  // declaration of locals
  XMC_GPIO_CONFIG_t config; 

  // init ports connected to LED1 and LED2
  config.mode = XMC_GPIO_MODE_OUTPUT_PUSH_PULL;
  config.output_level = XMC_GPIO_OUTPUT_LEVEL_HIGH;
  config.output_strength = XMC_GPIO_OUTPUT_STRENGTH_MEDIUM;
  config.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW;

  if (led == L2) {
    XMC_GPIO_Init(LED2, &config);
    return true;
  }
  if (led == L1) {
    XMC_GPIO_Init(LED1, &config);
    return true;
  }
  return false;
}

/**
 * @brief Initialize globals used for debouncing and configure the system timer.
 * @param mode determines the scan mode that is either
 *        SM_TIMER5MS ... 5ms period
 *        SM_TIMER10MS .. 10ms period
 * @return SM_TIMER50MS or SM_TIMER100MS on success, 0 otherwise
 */
uint8_t setupButtonScanMode(uint8_t mode) 
{
  p1_14_pressed = 0;
  p1_15_pressed = 0;

  /*if (mode == SM_TIMER5MS) {
    // 5ms tick period
    SysTick_Config(SystemCoreClock/200); 
    return SM_TIMER5MS;
  }
  if (mode == SM_TIMER10MS) {
    // 10ms tick period
    SysTick_Config(SystemCoreClock/100); 
    return SM_TIMER10MS;
  }*/
  return 1;
}
_Bool toggleLed(uint8_t led) 
{
  if (led == L1) {
    XMC_GPIO_ToggleOutput(P1_1);
    return true;
  }
  if (led == L2) {
    XMC_GPIO_ToggleOutput(P1_0);
    return true;
  }
  return false;
}
void set_high(uint8_t led)
{
  if (led == L1)
  {
      XMC_GPIO_SetOutputHigh(P1_1);
  }
  if (led == L2)
  {
      XMC_GPIO_SetOutputHigh(P1_0);
  }
}
void set_low(uint8_t led)
{
  if (led == L1)
  {
      XMC_GPIO_SetOutputLow(P1_1);
  }
  if (led == L2)
  {
      XMC_GPIO_SetOutputLow(P1_0);
  }
}
/** EOF */
