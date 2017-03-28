/*
 * File:   pwm.c
 * Author: Michael Ding
 *
 * Created on July 14, 2016, 10:25 AM
 */


#include <xc.h>
#include "pwm.h"
#include "configBits.h"
//#include <./delay_100ms.h>



//<editor-fold defaultstate="collapsed" desc="Variable Defs">
long freq; // Selected PWM Frequency
unsigned int i = 0; // Keep track of current PWM duty cycle
//</editor-fold>

int PWM_Max_Duty()
// Returns the register values to be set for 100% duty cycle, this is dependent
// on the PWM frequency and oscillator frequency
// https://electrosome.com/pwm-pic-microcontroller-mplab-xc8/
{
  return(10000000/(freq*TMR2PRESCALE);
}
  
void set_PWM_freq(long fre)
// Sets PR2 register to match the frequency desired
// See datasheet pg 149, equation 16-1
// https://electrosome.com/pwm-pic-microcontroller-mplab-xc8/
{
  PR2 = (10000000/(fre*4*TMR2PRESCALE)) - 1;
  freq = fre;
}

void set_PWM1_duty(unsigned int duty)
// Sets the duty cycle of PWM1, from 1024 to 0
// See datasheet pg 150, equation 16-1
// https://electrosome.com/pwm-pic-microcontroller-mplab-xc8/
{
  if(duty<1024)
  {
    duty = ((float)duty/1023)*PWM_Max_Duty();
    CCP1X = duty & 2; // Set the 2 lest significant bit in CCP1CON register
    CCP1Y = duty & 1;
    CCPR1L = duty>>2; // Set rest of the duty cycle bits in CCPR1L
  }
    
}

void set_PWM2_duty(unsigned int duty)
// Sets the duty cycle of PWM2, from 1024 to 0
// See datasheet pg 150, equation 16-1
// https://electrosome.com/pwm-pic-microcontroller-mplab-xc8/
{
  if(duty<1024)
  {
    duty = ((float)duty/1023)*PWM_Max_Duty();
    CCP2X = duty & 2; // Set the 2 lest significant bit in CCP2CON register
    CCP2Y = duty & 1;
    CCPR2L = duty>>2; // Set rest of the duty cycle bits in CCPR2L
  }
}

void PWM1_Start()
// START PWM1 OUTPUT, PWM1 have enhanced features, see datasheet
{
  //Configure CCP1CON, single output mode, all active high
  P1M1 = 0;
  P1M0 = 0;
  CCP1M3 = 1;
  CCP1M2 = 1;
  CCP1M1 = 0;
  CCP1M0 = 0;
  
  //Configure prescale values for Timer2, according to TMR2PRESCALAR
  #if TMR2PRESCALAR == 1
    T2CKPS0 = 0;
    T2CKPS1 = 0;
  #elif TMR2PRESCALAR == 4
    T2CKPS0 = 1;
    T2CKPS1 = 0;
  #elif TMR2PRESCALAR == 16
    T2CKPS0 = 1;
    T2CKPS1 = 1;
  #endif

  // Enable timer 2
  TMR2ON = 1;
  
  // Enable PWM output pins
  TRISCbits.TRISC2 = 0;
}

void PWM2_Start()
// START PWM2 OUTPUT
{
  //Configure CCP2CON, enter PWM mode
  CCP1M3 = 1;
  CCP1M2 = 1;
  
  //Configure prescale values for Timer2, according to TMR2PRESCALAR
  #if TMR2PRESCALAR == 1
    T2CKPS0 = 0;
    T2CKPS1 = 0;
  #elif TMR2PRESCALAR == 4
    T2CKPS0 = 1;
    T2CKPS1 = 0;
  #elif TMR2PRESCALAR == 16
    T2CKPS0 = 1;
    T2CKPS1 = 1;
  #endif

  // Enable timer 2
  TMR2ON = 1;
  
  // Enable PWM output pins
  TRISCbits.TRISC1 = 0;
}

void PWM1_Stop()
// Stop PWM1 output
{
  CCP1M3 = 0;
  CCP1M2 = 0;
  CCP1M1 = 0;
  CCP1M0 = 0;
}

void PWM2_Stop()
{
  CCP2M3 = 0;
  CCP2M2 = 0;
  CCP2M1 = 0;
  CCP2M0 = 0;
}

//void main(void) {
//    // Set internal oscillator to run at 8 MHZ
//    OSCCON = OSCCON | 0b01110000; 
//    // Enable PLL for the internal oscillator, Processor now runs at 32MHZ
//    OSCTUNEbits.PLLEN = 1; 
//     
//    // Configure PWM frequency, 50khz
//    // Scale for pwm is not linear, see datasheet pg150
//    set_PWM_freq (3100);
//    
//    PWM1_Start();
//    
//    // Infinite loop, cycle state on each pin
//    TRISC = 0x11110001;
//    set_PWM1_duty(512);
//    TRISB = 0x00;
//    while(1){
//        // LATCbits.LATC3 = 1 - LATCbits.LATC3 ; // This line of close flashes RC3 every iteration of the loop
//        set_PWM1_duty(i);
//        LATB = i>>2;
//        if(i <= 1020)
//          i= i+10;
//        else
//            i = 0;
//        delay_10ms(10);
//    }
//  }