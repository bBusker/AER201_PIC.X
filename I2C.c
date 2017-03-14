/*
 * File:   I2C.c
 * Author: Administrator
 *
 * Created on August 4, 2016, 3:22 PM
 */


#include <xc.h>
#include "I2C.h"
#include "configBits.h"
#include "constants.h"

void I2C_Master_Init(const unsigned long c)
{
  // See Datasheet pg171, I2C mode configuration
  SSPSTAT = 0b00000000;
  SSPCON1 = 0b00101000;
  SSPCON2 = 0b00000000;
  SSPADD = (_XTAL_FREQ/(4*c))-1;
  TRISC3 = 1;        //Setting as input as given in datasheet
  TRISC4 = 1;        //Setting as input as given in datasheet
}

void I2C_Master_Wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));
}

void I2C_Master_Start()
{
  I2C_Master_Wait();
  SEN = 1;
}

void I2C_Master_RepeatedStart()
{
  I2C_Master_Wait();
  RSEN = 1;
}

void I2C_Master_Stop()
{
  I2C_Master_Wait();
  PEN = 1;
}

void I2C_Master_Write(unsigned d)
{
  I2C_Master_Wait();
  SSPBUF = d;
}

void I2C_ColorSens_Init(void){
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg + access enable reg
    I2C_Master_Write(0b00000001);   //Start POWER
    I2C_Master_Stop();
    
    __delay_ms(3);                  //TCS requires 2.4ms delay before other actions
    
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write
    I2C_Master_Write(0b10000000);   //Write to cmdreg + access enable reg
    I2C_Master_Write(0b00010011);   //Start RGBC and Interrupts
    I2C_Master_Stop();
    
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write
    I2C_Master_Write(0b10001111);   //Write to cmdreg + access control reg
    I2C_Master_Write(0b00000010);   //Set analog gain to 16
    I2C_Master_Stop();
    
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write 
    I2C_Master_Write(0b10100100);   //Write to cmdreg + access & increment interrupt threshold reg
    I2C_Master_Write(0x00);         //Write low and high interrupt values
    I2C_Master_Write(0x00);
    I2C_Master_Write(0x00);
    I2C_Master_Write(0b01001010);
    I2C_Master_Stop();
    
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write
    I2C_Master_Write(0b10001100);   //Write to cmdreg + access persistence reg
    I2C_Master_Write(0b00000001);   //persistence of 1 value out of range
    I2C_Master_Stop();
    
}

void I2C_ColorSens_ClearInt(void){
    I2C_Master_Start();             //Write Start condition
    I2C_Master_Write(0b01010010);   //7bit address for TCS (0x29) + Write
    I2C_Master_Write(0b11100110);   //Write to cmdreg + special func clear int
    I2C_Master_Write(0b11100110);   //TESTING 
    I2C_Master_Stop();
}

unsigned char I2C_Master_Read(unsigned char a)
{
  unsigned char temp;
  I2C_Master_Wait();
  RCEN = 1;
  I2C_Master_Wait();
  temp = SSPBUF;
  I2C_Master_Wait();
  ACKDT = (a)?0:1;
  ACKEN = 1;
  return temp;
}

void delay_10ms(unsigned char n) { 
    while (n-- != 0) { 
        __delay_ms(5); 
    } 
}