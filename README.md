# PIC18_I2C_RTC

Contains an I2C library, and a sample implementation that connects to the RTC module to set the time, read it, and display it on the LCD.

Devnote: The LCD module enters an undefined state every other reset. The cause of this is unknown, but can be fixed by adding an extra 8bit mode instruction. 