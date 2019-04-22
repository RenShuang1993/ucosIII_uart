/**
 * \file app.c
 *
 * \mainpage Application for UART communication tests
 *
 * Build: make debug OR make flash
 * Connect a TTL USB cable to UART1, launch a terminal program and initiate
 * communication, e.g. (without the quotes):
 *         PC -> uC: "#Hello$"
 *         uC -> PC: "XMC: Hello"
 *
 * @author Beneder Roman, Martin Horauer, UAS Technikum Wien
 * @revision 0.2
 * @date 02-2018
 */

/******************************************************************* INCLUDES */
#include <app_cfg.h>
#include <cpu_core.h>
#include <os.h>

#include <bsp.h>
#include <bsp_sys.h>
#include <bsp_int.h>
#include <io_lib.h>
#include <io_driver.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <xmc_uart.h>
#include <xmc_gpio.h>
#include <xmc4_gpio.h>
#include <GPIO.h>

#include <lib_math.h>

#if SEMI_HOSTING
#include <debug_lib.h>
#endif

#if JLINK_RTT
#include <SEGGER_RTT.h>
#include <SEGGER_RTT_Conf.h>
#endif

/******************************************************************** DEFINES */
#define ACK  0x6
#define MAX_MSG_LENGTH 20
#define NUM_MSG        3
#define WAIT_DELAY     5000000

/********************************************************* FILE LOCAL GLOBALS */
static  CPU_STK  AppStartTaskStk[APP_CFG_TASK_START_STK_SIZE];            // <1>
static  OS_TCB   AppStartTaskTCB;

static  CPU_STK  AppTaskComStk[APP_CFG_TASK_COM_STK_SIZE];
static  OS_TCB   AppTaskComTCB;

static  CPU_STK  AppTaskLED_1Stk[APP_CFG_TASK_COM_STK_SIZE];
static  OS_TCB   AppTaskLED_1_TCB;

static  CPU_STK  AppTaskLED_2Stk[APP_CFG_TASK_COM_STK_SIZE];
static  OS_TCB   AppTaskLED_2_TCB;

// Memory Block                                                           // <2>
OS_MEM      Mem_Partition;
CPU_CHAR    MyPartitionStorage[NUM_MSG - 1][MAX_MSG_LENGTH];
// Memory Block                                                           // <2>
OS_MEM      Mem_LED1;
CPU_CHAR    Mem_LED1Storage[NUM_MSG - 1][MAX_MSG_LENGTH];
// Message Queue
OS_Q        UART_ISR;
OS_Q        DATA_Msg;
uint8_t     keyPress;
/****************************************************** FILE LOCAL PROTOTYPES */
static  void AppTaskStart (void  *p_arg);
static  void AppTaskCreate (void);
static  void AppObjCreate (void);
static  void AppTaskCom (void  *p_arg);
static  void AppTaskLED_1 (void  *p_arg);
static  void AppTaskLED_2 (void  *p_arg);


/************************************************************ FUNCTIONS/TASKS */

/*********************************************************************** MAIN */
/**
 * \function main
 * \params none
 * \returns 0 always
 *
 * \brief This is the standard entry point for C code.
 */
int main (void)
{
  OS_ERR  err;

  // Disable all interrupts                                               // <3>
  BSP_IntDisAll();
  // Enable Interrupt UART
  BSP_IntEn (BSP_INT_ID_USIC1_01); //**
  BSP_IntEn (BSP_INT_ID_USIC1_00); //**

// init SEMI Hosting DEBUG Support                                        // <4>
#if SEMI_HOSTING
  initRetargetSwo();
#endif

// init JLINK RTT DEBUG Support
#if JLINK_RTT
  SEGGER_RTT_ConfigDownBuffer (0, NULL, NULL, 0,
             SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
  SEGGER_RTT_ConfigUpBuffer (0, NULL, NULL, 0,
           SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
#endif

  // Init uC/OS-III
  OSInit (&err);                                                          // <5>
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSInit: main\n");

  /* Create the start task */                                             // <6>
  OSTaskCreate ( (OS_TCB     *) &AppStartTaskTCB,
           (CPU_CHAR   *) "Startup Task",
           (OS_TASK_PTR) AppTaskStart,
           (void       *) 0,
           (OS_PRIO) APP_CFG_TASK_START_PRIO,
           (CPU_STK    *) &AppStartTaskStk[0],
           (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE / 10u,
           (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
           (OS_MSG_QTY) 0u,
           (OS_TICK) 0u,
           (void       *) 0,
           (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
           (OS_ERR     *) &err);

  // Start multitasking (i.e., give control to uC/OS-III)
  OSStart (&err);                                                         // <7>
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSStart: main\n");

  while (1) {                                                             // <8>
    APP_TRACE_DBG ("Should never be output! Bug?\n");
  }
  return 0;
}

/*************************************************************** STARTUP TASK */
/**
 * \function AppTaskStart
 * \ params p_arg ... argument passed to AppTaskStart() by
 *                    OSTaskCreate()
 * \returns none
 *
 * \brief Startup (init) task that loads board support functions,
 *        initializes CPU services, the memory, the systick timer,
 *        etc. and finally invokes other application tasks.
 */
static void AppTaskStart (void *p_arg)
{
  CPU_INT32U  cpu_clk_freq;
  CPU_INT32U  cnts;
  OS_ERR      err;

  (void) p_arg;
  // initialize BSP functions
  BSP_Init();                                                             // <9>
  // initialize the uC/CPU services
  CPU_Init();
  // determine SysTick reference frequency
  cpu_clk_freq = BSP_SysClkFreqGet();
  // determine nbr SysTick increments
  cnts = cpu_clk_freq / (CPU_INT32U) OSCfg_TickRate_Hz;
  // init uCOS-III periodic time src (SysTick)
  OS_CPU_SysTickInit (cnts);
  // initialize memory management module
  Mem_Init();
  // initialize mathematical module
  Math_Init();
  // init circular buffer
  cbInit();
  // init ports connected to LED1 and LED2
  configureLeds();
  // init ports connected to the buttons (configures scanning using the timer)
  configureButtons();


// compute CPU capacity with no task running
#if (OS_CFG_STAT_TASK_EN > 0u)                                           // <10>
  OSStatTaskCPUUsageInit (&err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSStatTaskCPUUsageInit: AppTaskStart\n");
#endif

  APP_TRACE_INFO ("Creating Application Objects...\n");                  // <11>
  // create application objects
  AppObjCreate();

  APP_TRACE_INFO ("Creating Application Tasks...\n");                    // <12>
  // create application tasks
  AppTaskCreate();

  while (DEF_TRUE) {                                                     // <13>
    // Suspend current task
    OSTaskSuspend ( (OS_TCB *) 0, &err);
    if (err != OS_ERR_NONE)
      APP_TRACE_DBG ("Error OSTaskSuspend: AppTaskStart\n");
  }
}

/************************************************* Create Application Objects */
/**
 * \function AppObjCreate()
 * \brief Creates application objects.
 * \params none
 * \returns none
 */
static void AppObjCreate (void)
{
  OS_ERR      err;

  // Create Shared Memory
  OSMemCreate ( (OS_MEM    *) &Mem_Partition,
          (CPU_CHAR  *) "Mem Partition",
          (void      *) &MyPartitionStorage[0][0],
          (OS_MEM_QTY)  NUM_MSG,
          (OS_MEM_SIZE) MAX_MSG_LENGTH * sizeof (CPU_CHAR),
          (OS_ERR    *) &err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSMemCreate: AppObjCreate\n");
   // Create Shared Memory
  OSMemCreate ( (OS_MEM    *) &Mem_LED1,
          (CPU_CHAR  *) "Mem LED1",
          (void      *) &Mem_LED1Storage[0][0],
          (OS_MEM_QTY)  NUM_MSG,
          (OS_MEM_SIZE) MAX_MSG_LENGTH * sizeof (CPU_CHAR),
          (OS_ERR    *) &err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSMemCreate: AppObjCreate\n");
  // Create Message Queue
  OSQCreate ( (OS_Q *)     &UART_ISR,
        (CPU_CHAR *) "ISR Queue",
        (OS_MSG_QTY) NUM_MSG,
        (OS_ERR   *) &err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSQCreate: AppObjCreate\n");
  // Create Message Queue
  OSQCreate ( (OS_Q *)     &DATA_Msg,
        (CPU_CHAR *) "DATA Msg",
        (OS_MSG_QTY) NUM_MSG,
        (OS_ERR   *) &err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSQCreate: AppObjCreate\n");
}

/*************************************************** Create Application Tasks */
/**
 * \function AppTaskCreate()
 * \brief Creates one application task.
 * \params none
 * \returns none
 */
static void  AppTaskCreate (void)
{
  OS_ERR      err;

  // create AppTask_COM
  OSTaskCreate ( (OS_TCB     *) &AppTaskComTCB,
           (CPU_CHAR   *) "TaskCOM",
           (OS_TASK_PTR) AppTaskCom,
           (void       *) 0,
           (OS_PRIO) 2,
           (CPU_STK    *) &AppTaskComStk[0],
           (CPU_STK_SIZE) APP_CFG_TASK_COM_STK_SIZE / 10u,
           (CPU_STK_SIZE) APP_CFG_TASK_COM_STK_SIZE,
           (OS_MSG_QTY) 0u,
           (OS_TICK) 0u,
           (void       *) 0,
           (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
           (OS_ERR     *) &err);
  // create AppTaskLED_1
  OSTaskCreate ( (OS_TCB     *) &AppTaskLED_1_TCB,
           (CPU_CHAR   *) "LED_1",
           (OS_TASK_PTR) AppTaskLED_1,
           (void       *) 0,
           (OS_PRIO) 3,
           (CPU_STK    *) &AppTaskLED_1Stk[0],
           (CPU_STK_SIZE) APP_CFG_TASK_COM_STK_SIZE / 10u,
           (CPU_STK_SIZE) APP_CFG_TASK_COM_STK_SIZE,
           (OS_MSG_QTY) 0u,
           (OS_TICK) 0u,
           (void       *) 0,
           (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
           (OS_ERR     *) &err);
  if (err != OS_ERR_NONE)
    APP_TRACE_DBG ("Error OSTaskCreate: AppTaskCreate\n");
}

/*********************************** Communication Application Task */
/**
 * \function AppTaskCom
 * \ params p_arg ... argument passed to AppTaskCom() by
 *                    AppTaskCreate()
 * \returns none
 *
 * \brief Task for Communication between UART_ISR (see BSP/bsp_int.c)
 *        and AppTaskCom. Communication from the ISR to the AppTaskCom is
 *        facilitated using a message queue.
 *
 *        Debug trace mesages are output to the SEGGER J-Link GDB Server.
 *
 *        (1) Debug or Flash the application.
 *        (2) Connect a TTL-USB UART cable: 
 *            GND (BLACK) - GND, TX (GREEN) - P0.0, RX (WHITE) - P0.1
 *        (3) Launch a terminal program and connect with 9600-8N1
 *            Enter strings like: #12345$, #abc$, etc.
 *            The XMC will respond with: XMC: 12345, XMC: abc, etc.
 */
static void AppTaskCom (void *p_arg)
{
  void        *p_msg;
  OS_ERR      err;
  OS_MSG_SIZE msg_size;
  CPU_TS      ts;
  CPU_CHAR    msg[MAX_MSG_LENGTH];
  CPU_INT08U  i = 0;
  CPU_CHAR    debug_msg[MAX_MSG_LENGTH + 30];
  //
  CPU_CHAR    *pbuf=NULL;

  (void) p_arg;                                                          // <14>
  APP_TRACE_INFO ("Entering AppTaskCom ...\n");
  while (DEF_TRUE) {
    // empty the message buffer
    memset (&msg, 0, MAX_MSG_LENGTH);                                    // <15>

    // wait until a message is received
    p_msg = OSQPend (&UART_ISR,                                          // <16>
         0,
         OS_OPT_PEND_BLOCKING,
         &msg_size,
         &ts,
         &err);
    if (err != OS_ERR_NONE)
      APP_TRACE_DBG ("Error OSQPend: AppTaskCom\n");

    // obtain message we received
    memcpy (msg, (CPU_CHAR*) p_msg, msg_size - 1);                       // <17>

    // release the memory partition allocated in the UART service routine
    OSMemPut (&Mem_Partition, p_msg, &err);                              // <18>
    if (err != OS_ERR_NONE)
      APP_TRACE_DBG ("Error OSMemPut: AppTaskCom\n");

    // send ACK in return
    XMC_UART_CH_Transmit (XMC_UART1_CH1, ACK);                           // <19>

    // print the received message to the debug interface
    sprintf (debug_msg, "Msg: %s\tLength: %d\n", msg, msg_size - 1);     // <20>
    APP_TRACE_INFO (debug_msg);

    // send the received message back via the UART pre-text with "XMC: "
    XMC_UART_CH_Transmit (XMC_UART1_CH1, 'X');                           // <21>
    XMC_UART_CH_Transmit (XMC_UART1_CH1, 'M');
    XMC_UART_CH_Transmit (XMC_UART1_CH1, 'C');
    XMC_UART_CH_Transmit (XMC_UART1_CH1, ':');
    XMC_UART_CH_Transmit (XMC_UART1_CH1, ' ');
    if(strcmp(msg,"BEL1")==0)
    {
      XMC_UART_CH_Transmit (XMC_UART1_CH1, 'Y');
    }
    for (i = 0; i <= msg_size; i++) {
      XMC_UART_CH_Transmit (XMC_UART1_CH1, msg[i]);
    }
    XMC_UART_CH_Transmit (XMC_UART1_CH1, '\n');
    //message queue
    pbuf = (CPU_CHAR *) OSMemGet (&Mem_LED1, &err);
    pbuf = msg;
    // the memory block into the queue to the application task
		OSQPost ( (OS_Q      *) &DATA_Msg,
			  (void      *) pbuf,
			  (OS_MSG_SIZE) sizeof(msg),
			  (OS_OPT)      OS_OPT_POST_FIFO,
			  (OS_ERR    *) &err);
		if (err != OS_ERR_NONE)
			APP_TRACE_DBG ("Error OSQPost: BSP_IntHandler_Uart_Recive\n");

		// clear the receive pointer and counter                              // <5>
		pbuf = NULL;
    OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_HMSM_STRICT,&err);
  }
}
static  void AppTaskLED_1 (void  *p_arg)
{
    OS_ERR err;
    void        *p_msg;
    p_arg = p_arg;
    CPU_TS      ts;
    OS_MSG_SIZE msg_size;
    CPU_CHAR    msg[MAX_MSG_LENGTH];
    CPU_CHAR    *cmd;
    uint16_t    mid;
    bool       pause = false;
    bool       bel_busy = false;
    bool       tel_busy = false;
    int        bel_number = 0;
    CPU_INT32U        h_time;
    CPU_INT32U        l_time;
    while(DEF_TRUE)
    {
      // empty the message buffer
      memset (&msg, 0, MAX_MSG_LENGTH);
      scanButtonsWithDebounce();
      // check for available button events in the circular buffer
      if (cbGet(&keyPress)) {
        switch (keyPress) {
        case B1:
          pause = true;
          break;
        case B2:
          toggleLed(L2);
         break;
      default:
        break;
      }
    }
    // wait until a message is received
    p_msg = OSQPend (&DATA_Msg,                                          
         1,
         OS_OPT_PEND_BLOCKING,
         &msg_size,
         &ts,
         &err);
    if (err != OS_ERR_NONE)
      APP_TRACE_DBG ("Error OSQPend: AppTaskCom\n");
    if(p_msg != NULL)
    {
    // obtain message we received
    memcpy (msg, (CPU_CHAR*) p_msg, msg_size - 1);
    // get cmd from msg
    cmd = strtok(msg,":");
    mid = atoi(cmd);
    XMC_UART_CH_Transmit (XMC_UART1_CH1,'M');
    XMC_UART_CH_Transmit (XMC_UART1_CH1,'i');
    XMC_UART_CH_Transmit (XMC_UART1_CH1,'d');
    XMC_UART_CH_Transmit (XMC_UART1_CH1,':');
    XMC_UART_CH_Transmit (XMC_UART1_CH1,mid);
    XMC_UART_CH_Transmit (XMC_UART1_CH1,':');
    //get BEL1,TL1
    cmd = strtok( NULL,":");
    if(strcmp(cmd,"BEL1")==0)
    {
      bel_busy = true;
      cmd = strtok(NULL,":");
      bel_number = atoi(cmd);
      cmd = NULL;
    }
    /*else if (strcmp(cmd,"TL1")==0) {
      tel_busy = true;
      cmd = strtok(NULL,":");
      cmd = strtok(cmd,"H");
      cmd = strtok(cmd,"L");
      h_time = atoi(cmd);
      cmd = strtok(NULL,"L");
      l_time = atoi(cmd);
      cmd = NULL;
    }*/
    
    // release the memory partition allocated in the UART service routine
    OSMemPut (&Mem_LED1, p_msg, &err);
    }
    if(!pause)
    {
      //EBL1
      if(bel_busy)
      {
        while(bel_number != 0)
        {
          
          bel_number--;
          OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_HMSM_STRICT,&err);
          scanButtonsWithDebounce();
          if (cbGet(&keyPress))
          {
            switch (keyPress)
            {
                case B1:
                pause = true;
                break;
                default:
                break;
            }
          }
          if(pause)
          {
            break;
          }
          toggleLed(L1);
        }
        if(bel_number == 0)
        {
          bel_busy = false;
          XMC_UART_CH_Transmit (XMC_UART1_CH1, 'D');                           // <21>
          XMC_UART_CH_Transmit (XMC_UART1_CH1, '0');
          XMC_UART_CH_Transmit (XMC_UART1_CH1, 'N');
          XMC_UART_CH_Transmit (XMC_UART1_CH1, 'E');
          XMC_UART_CH_Transmit (XMC_UART1_CH1, '\n');
        }
      }
      //TL1
     /* if(tel_busy)
      {
        set_high(L1);
        while(h_time != 0)
        {
          h_time--;
          scanButtonsWithDebounce();
          if (cbGet(&keyPress))
          {
            switch (keyPress)
            {
                case B1:
                pause = true;
                break;
                default:
                break;
            }
          }
          if(pause)
          {
            break;
          }
          OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_HMSM_STRICT,&err);//delay 1ms
        }
        if(!pause)
        {
          set_low(L1);
          while(l_time !=0)
          {
            l_time--;
            scanButtonsWithDebounce();
            if (cbGet(&keyPress))
            {
              switch (keyPress)
              {
                case B1:
                pause = true;
                break;
                default:
                break;
              }
            }
            if(pause)
            {
              break;
            }
            OSTimeDlyHMSM(0,0,0,1,OS_OPT_TIME_HMSM_STRICT,&err);//delay 1ms
          }
        }
        if((l_time == 0) && (h_time == 0))
        {
          tel_busy = false;
        }
      }*/
    }
    pause = false;
    }
}
/************************************************************************ EOF */
/******************************************************************************/
