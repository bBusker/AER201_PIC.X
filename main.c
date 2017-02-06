/*
 * Main file for PIC microcontroller
 */


#include <xc.h>
#include <stdio.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "macros.h"

void set_time(void);
void date_time(void);
void bottle_count(void);
void bottle_time(void);
void standby(void);

const char keys[] = "123A456B789C*0#D";
const char timeset[7] = {   0x50, //Seconds 
                            0x35, //Minutes
                            0x21, //Hour, 24 hour mode
                            0x08, //Day of the week, Monday = 1
                            0x05, //Day/Date
                            0x02, //Month
                            0x17};//Year, last two digits
enum state {
        STANDBY,
        DATETIME,
        BOTTLECOUNT,
        BOTTLETIME
    };
    
enum state curr_state;

unsigned char time[7];

void main(void) {
    
    // <editor-fold defaultstate="collapsed" desc=" STARTUP SEQUENCE ">
    
    TRISA = 0xFF; // Set Port A as all input
    TRISB = 0xFF; 
    TRISC = 0x00;
    TRISD = 0x00; //All output mode for LCD
    TRISE = 0x00;    

    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
    ADCON0 = 0x00;  //Disable ADC
    ADCON1 = 0xFF;  //Set PORTB to be digital instead of analog default  
    
    INT1IE = 1;
    
    nRBPU = 0;
    
    initLCD();
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock

    //</editor-fold>
    
    ei();

    curr_state = STANDBY;
    
    while(1){
        switch(curr_state){
            case STANDBY:
                standby();
                break;
            case DATETIME:
                date_time();
                break;
            case BOTTLECOUNT:
                bottle_count();
                break;
            case BOTTLETIME:
                bottle_time();
                break;
        }
        __delay_ms(200);
    }
    
    return;
}

void interrupt isr(void){
    if (INT1IF) {
        switch(PORTB){
            case 15:
                curr_state = STANDBY;
                break;
            case 63:
                curr_state = DATETIME;
                break;               
        }
    }
    else{
        while(1){
            __lcd_home();
            printf("bad interrupt");
            __delay_1s();
        }
    }
    INT1IF = 0;
    return;
}

void standby(void){
    __lcd_home();
    printf("standby");
    __lcd_newline();
    printf("PORTB: %d", PORTB);
    return;
}

void set_time(void){
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); //Set memory pointer to seconds
    for(char i=0; i<7; i++){
        I2C_Master_Write(timeset[i]);
    }    
    I2C_Master_Stop(); //Stop condition
}

void date_time(void){
    //Reset RTC memory pointer 
    I2C_Master_Start(); //Start condition
    I2C_Master_Write(0b11010000); //7 bit RTC address + Write
    I2C_Master_Write(0x00); //Set memory pointer to seconds
    I2C_Master_Stop(); //Stop condition

    //Read Current Time
    I2C_Master_Start();
    I2C_Master_Write(0b11010001); //7 bit RTC address + Read
    for(unsigned char i=0;i<0x06;i++){
        time[i] = I2C_Master_Read(1);
    }
    time[6] = I2C_Master_Read(0);       //Final Read without ack
    I2C_Master_Stop();

    //LCD Display
    __lcd_home();
    printf("%02x/%02x/%02x        ", time[5],time[4],time[6]);    //Print date in YY/MM/DD
    __lcd_newline();
    printf("%02x:%02x:%02x        ", time[2],time[1],time[0]);    //HH:MM:SS

    return;
}

void bottle_count(void){
    return;
}

void bottle_time(void){
    return;
}