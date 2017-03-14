/* 
 * File:   constants.h
 * Author: Administrator
 *
 * Created on August 11, 2016, 2:41 PM
 */

#ifndef CONSTANTS_H
#define	CONSTANTS_H         //Prevent multiple inclusion 

//LCD Control Registers
#define RS          LATDbits.LATD2          
#define E           LATDbits.LATD3
#define	LCD_PORT    LATD   //On LATD[4,7] to be specific
#define LCD_DELAY   30

//PIC Constants
#define TCSLOWINT   3000
#define TCSHIGHINT  2000

#endif	/* CONSTANTS_H */

