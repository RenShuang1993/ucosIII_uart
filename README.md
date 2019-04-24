# ucosIII_uart
PC → XMC
The layout of frames in the direction PC → XMC are as follows:


Every frame starts with a "start-of-frame" (SoF) character (#).

A message identifier (MID) that is simply an increasing number that wraps after 99 messages were sent (1-99).

Next is a separator (SEP) character (:).

This is followed by three "command" (CMD) characters (see below).

Next is a separator (SEP) character (:).

Following the separator are up to 10 "DATA" characters.

The frame is terminated by an "end-of-frame" (EoF) character ($).

The following examples shall detail the intended functionality.

'''#1:BL1:10$          // blink LED1 10 times with a default timing.'''
'#2:TL1:H20L10$      // set high-time of LED1 to 20 ms and low-time to 10 ms.'
'#3:RES:OFF$         // stop bot LEDs blinking and turn both LEDs OFF.'

XMC Application Requirements
Implement three application tasks:

Task Comm
Handles the PC→XMC interface and interprets commands from the PC and relays respective information to the other tasks using some message queue and/or mailbox system. In turn the communication task receives messages from the other tasks via its task message queue (or similar means) and sends them via the XMC→PC interface to the remote host PC.
In case a RES command is received and there are ongoing tasks executing by task LED1 and LED2, respectively, NOT DONE messages are sent back to the host PC.

Task LED1
This task receives commands from the communication task and controls the LED1. In addition it checks BUTTON1 — whenever it is pressed the execution of the task performed on LED1 is suspended until the button is released. When the command is completed the task notifies the communication task that it shall send a respective message back to the host PC.

Task LED2
This task receives commands from the communication task and controls the LED2. In addition it checks BUTTON2 — whenever it is pressed the execution of the task performed on LED2 is suspended until the button is released. When the command is completed the task notifies the communication task that it shall send a respective message back to the host PC.
