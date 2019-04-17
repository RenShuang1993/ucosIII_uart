/*
 * main.c
 *
 *  Created on: 2015 Jul 21 15:47:01
 *  Author: ferreije
 */

#include <stdio.h>

#include <xmc_gpio.h>
#include <xmc_uart.h>

#define LED1 P1_1
#define UART_TX P1_5
#define UART_RX P1_4

XMC_UART_CH_CONFIG_t uart_config =
{
  .data_bits = 8U,
  .stop_bits = 1U,
  .baudrate = 115200U
};

void XMC_AssertHandler(const char *const msg, const char *const file, uint32_t line)
{
  printf("%s at line %u of %s\n", msg, (unsigned int)line, file);
  while(1);
}


/**

 * @brief main() - Application entry point
 *
 * <b>Details of function</b><br>
 * This routine is the application entry point. It is invoked by the device startup code. 
 */

int main(void)
{
  XMC_UART_CH_Init(XMC_UART0_CH0, &uart_config);

  XMC_GPIO_SetMode(UART_RX, XMC_GPIO_MODE_INPUT_TRISTATE);
  XMC_UART_CH_SetInputSource(XMC_UART0_CH0, XMC_UART_CH_INPUT_RXD, USIC0_C0_DX0_P1_4);

  XMC_UART_CH_Start(XMC_UART0_CH0);

  XMC_GPIO_SetMode(UART_TX, XMC_GPIO_MODE_OUTPUT_PUSH_PULL | P1_5_AF_U0C0_DOUT0);
  XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);

  printf("XMClib ASSERT demo\n");

  /* Next line will fail assertion and a message is printed out */
  XMC_GPIO_SetOutputLevel(LED1, 0);

  while(1U);
}
