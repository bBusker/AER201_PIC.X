/*
 * Main file for PIC microcontroller
 */


#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "macros.h"

void set_time(void);
void date_time(void);
void read_time(void);
void bottle_count(void);
void bottle_time(void);
void standby(void);
void operation(void);
void operationend(void);
void emergencystop(void);

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
        EMERGENCYSTOP,
        OPERATION,
        OPERATIONEND,
        DATETIME,
        BOTTLECOUNT,
        BOTTLETIME
    };
    
enum state curr_state;

unsigned char time[7];
unsigned char start_time[2];
unsigned char end_time[2];
char *ptr;
int bottle_count_disp = -1;
int operation_disp = 0;

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
    
    TMR0 = 0;
    T08BIT = 0;
    T0CS = 0;
    PSA = 0;
    T0PS2 = 1;
    T0PS1 = 1;
    T0PS0 = 1;  

    //</editor-fold>
    
    ei();

    curr_state = STANDBY;
    
    while(1){
        switch(curr_state){
            case STANDBY:
                standby();
                break;
            case EMERGENCYSTOP:
                emergencystop();
                break;
            case OPERATION:
                operation();
                break;
            case OPERATIONEND:
                operationend();
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
            case 239:   //KP_#
                curr_state = STANDBY;
                bottle_count_disp = 0;
                break;
            case 15:    //KP_1
                TMR0IE = 1;
                TMR0ON = 1;
                TMR0 = 0;
                read_time();
                start_time[1] = time[1];
                start_time[0] = time[0];
                __lcd_clear();
                curr_state = OPERATION;
                bottle_count_disp = -1;
                break;
            case 31:    //KP_2
                curr_state = BOTTLECOUNT;
                bottle_count_disp += 1;
                while(PORTB == 31){}
                break;
            case 47:    //KP_3
                curr_state = BOTTLETIME;
                bottle_count_disp = -1;
                break;
            case 63:    //KP_A
                curr_state = DATETIME;
                bottle_count_disp = -1;
                break;
            case 79:    //KP_4
                read_time();
                end_time[1] = time[1];
                end_time[0] = time[0];
                __lcd_clear();
                curr_state = OPERATIONEND;
                bottle_count_disp = -1;
                break;
            case 207:   //KP_*
                __lcd_clear();
                curr_state = EMERGENCYSTOP;
                bottle_count_disp = -1;
                TMR0ON = 0;
                break;
        }
        INT1IF = 0;
    }
    else if (TMR0IF) {
        TMR0ON = 0;
        read_time();
        end_time[1] = time[1];
        end_time[0] = time[0];
        __lcd_clear();
        curr_state = OPERATIONEND;
        bottle_count_disp = -1;
        TMR0IF = 0;
    }
    else{
        while(1){
            __lcd_home();
            printf("bad interrupt");
            __delay_1s();
        }
    }
    return;
}

void standby(void){
    __lcd_home();
    printf("standby         ");
    __lcd_newline();
    printf("PORTB: %d       ", PORTB);
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
    printf("Date: %02x/%02x/%02x  ", time[5],time[4],time[6]);    //Print date in MM/DD/YY
    __lcd_newline();
    printf("Time: %02x:%02x:%02x  ", time[2],time[1],time[0]);    //HH:MM:SS

    return;
}

void read_time(void){
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
    return;
}

void bottle_count(void){
    switch(bottle_count_disp % 5){
        case 0:
            __lcd_home();
            printf("Bottle Count    ");
            __lcd_newline();
            printf("Total: 10       ");
            break;
        case 1:
            __lcd_home();
            printf("YOP+CAP+LBL: 3  ");
            __lcd_newline();
            printf("YOP+CAP-LBL: 1  ");
            break;
        case 2:
            __lcd_home();
            printf("YOP-CAP+LBL: 1  ");
            __lcd_newline();
            printf("YOP-CAP-LBL: 0  ");
            break;
        case 3:
            __lcd_home();
            printf("ESKA+CAP+LBL: 1 ");
            __lcd_newline();
            printf("ESKA+CAP-LBL: 1 ");
            break;
        case 4:
            __lcd_home();
            printf("ESKA-CAP+LBL: 1 ");
            __lcd_newline();
            printf("ESKA-CAP-LBL: 2 ");
            break;
        default:
            while(1){
                __lcd_home();
                printf("ERROR: %d", bottle_count_disp);
            }
            break;
    }
    return;
}

void bottle_time(void){
    //long stime = 60*strtoul(start_time[1], &ptr, 16)+strtoul(start_time[0], &ptr, 16);
    //long etime = 60*strtoul(end_time[1], &ptr, 16)+strtoul(end_time[0], &ptr, 16);
    __lcd_home();
    printf("Total Operation       ");
    __lcd_newline();
    //printf("Time: %d s       ", etime-stime);
    printf("Time: 7s             ");
    return;
}

void operation(void){
    switch(operation_disp){
        case 0:
            __lcd_home();
            printf("Running~              ");
            operation_disp = 1;
            break;
        case 1:
            __lcd_home();
            printf("Running~~              ");
            operation_disp = 2;
            break;
        case 2:
            __lcd_home();
            printf("Running~~~              ");
            operation_disp = 0;
            break;
    }
    return;
}

void operationend(void){
    __lcd_home();
    printf("Operation Done!");
    return;
}

void emergencystop(void){
    __lcd_home();
    printf("EMERGENCY STOP");
    di();
    while(1){}
    return;
}