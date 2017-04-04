/*
 * Main file for PIC microcontroller
 */

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "configBits.h"
#include "constants.h"
#include "lcd.h"
#include "I2C.h"
#include "macros.h"
#include "main.h"

void main(void) {
    
    // <editor-fold defaultstate="collapsed" desc=" STARTUP SEQUENCE ">
    
    //TRIS Sets Input/Output
    //0 = output
    //1 = input
    TRISA = 0b11111011;         //Set Port A as all input, except A2 for motor
    TRISB = 0xFF;               //Keypad
    TRISC = 0x00;               //RC3 and RC4 output for I2C (?)
    TRISD = 0x00;               //All output mode for LCD
    TRISE = 0x00;    

    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
    ADCON0 = 0x00;              //Disable ADC
    ADCON1 = 0xFF;              //Set PORTB to be digital instead of analog default  
    
    //ei();                     //Global Interrupt Mask
    GIE = 1;
    INT1IE = 1;                 //Enable KP interrupts
    INT0IE = 0;                 //Disable external interrupts
    INT2IE = 0;
    
    nRBPU = 0;
    
    initLCD();
    I2C_Master_Init(10000);     //Initialize I2C Master with 100KHz clock
    I2C_ColorSens_Init();       //Initialize TCS34725 Color Sensor
    
    //Set Timer Properties
    TMR0 = 0;
    T08BIT = 0;
    T0CS = 0;
    PSA = 0;
    T0PS2 = 1;
    T0PS1 = 1;
    T0PS0 = 1;  
    //</editor-fold>
    
    curr_state = STANDBY;
    
    while(1){
        switch(curr_state){
            case STANDBY:
                standby();
                __delay_ms(500);
                break;
            case EMERGENCYSTOP:
                emergencystop();
                break;
            case OPERATION:
                operation();
                __delay_ms(3);
                break;
            case OPERATIONEND:
                operationend();
                __delay_ms(500);
                break;
            case DATETIME:
                date_time();
                __delay_ms(300);
                break;
            case BOTTLECOUNT:
                bottle_count();
                __delay_ms(300);
                break;
            case BOTTLETIME:
                bottle_time();
                __delay_ms(300);
                break;
        }
        //__delay_ms(MAINPOLLINGDELAYMS);
    }
    
    return;
}

void interrupt isr(void){
    if (INT1IF) {
        switch(PORTB>>4){
            case 0:    //KP_1 -- OPERATION START
                LATAbits.LATA2 = 1; //Start centrifuge motor
//                TMR0IE = 1;         //Start timer with interrupts
//                TMR0ON = 1;         //TESTING
//                TMR0 = 0;
                
                read_time();
                start_time[1] = time[1];
                start_time[0] = time[0];
                for(i=0;i<5;i++){
                    bottle_count_array[i] = 0;
                }
                __lcd_clear();
                __delay_ms(100);
                __lcd_home();
                printf("running");
                bottle_count_disp = -1;
                curr_state = OPERATION;
                break;
            case 1:    //KP_2 -- BOTTLECOUNT
                bottle_count_disp += 1;
                curr_state = BOTTLECOUNT;
                while(PORTB == 31){}
                break;
            case 2:    //KP_3
                operation_time = etime - stime;
                bottle_count_disp = -1;
                curr_state = BOTTLETIME;
                break;
            case 3:    //KP_A
                bottle_count_disp = -1;
                curr_state = DATETIME;
                break;
            case 4:    //KP_4
                LATAbits.LATA2 = 0; //Stop centrifuge motor
                TMR0IE = 0;         //Disable timer
                TMR0ON = 0;
                
                read_time();
                end_time[1] = time[1];
                end_time[0] = time[0];
                stime = 60*dec_to_hex(start_time[1])+dec_to_hex(start_time[0]);
                etime = 60*dec_to_hex(end_time[1])+dec_to_hex(end_time[0]);
                __lcd_clear();
                bottle_count_disp = -1;
                curr_state = OPERATIONEND;
                break;
            case 5:    //KP_5 -- TESTING
                read_colorsensor();
                __lcd_home();
                printf("C%u R%u                ", color[0], color[1]);
                __lcd_newline();
                printf("G%u B%u                ", color[2], color[3]);
                break;
            case 12:   //KP_*
                LATAbits.LATA2 = 0; //Stop centrifuge motor
                di();               //Disable all interrupts
                TMR0ON = 0;
                __lcd_clear();
                curr_state = EMERGENCYSTOP;
                break;
            case 14:   //KP_#
                bottle_count_disp = -1;
                curr_state = STANDBY;
                //I2C_ColorSens_Init(); TESTING
                break;
            case 7:   //KP_B -- TESTING
                servo_rotate1(8);
                break;
            case 11:   //KP_C -- TESTING
                servo_rotate1(18);
                break;
        }
        INT1IF = 0;
    }
    else if (TMR0IF){
        LATAbits.LATA2 = 0;
        TMR0ON = 0;
        read_time();
        end_time[1] = time[1];
        end_time[0] = time[0];
        stime = 60*dec_to_hex(start_time[1])+dec_to_hex(start_time[0]);
        etime = 60*dec_to_hex(end_time[1])+dec_to_hex(end_time[0]);
        curr_state = OPERATIONEND;
        bottle_count_disp = -1;
        TMR0IF = 0;
    }
    else{
        while(1){
            __lcd_home();
            printf("ERR: BAD ISR");
            __delay_1s();
        }
    }
    return;
}

void standby(void){
    __lcd_home();
    printf("standby");
    __lcd_newline();
    read_colorsensor();
    printf("%d      ", color[0]);
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

int dec_to_hex(int num) {                   //Convert decimal unsigned char to hexadecimal int
    int i = 0, quotient = num, temp, hexnum = 0;
 
    while (quotient != 0) {
        temp = quotient % 16;
        hexnum += temp*pow(10,i);
        quotient = quotient / 16;
        i += 1;
    }
    return hexnum;
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
        time[i] = I2C_Master_Read(1);   //Read with ack
    }
    time[6] = I2C_Master_Read(0);       //Final Read without ack
    I2C_Master_Stop();
    return;
}

void bottle_count(void){
    switch(bottle_count_disp % 3){
        case 0:
            __lcd_home();
            printf("Bottle Count    ");
            __lcd_newline();
            printf("Total: %d       ", bottle_count_array[0]);
            break;
        case 1:
            __lcd_home();
            printf("YOP W/ CAP: %d  ", bottle_count_array[1]);
            __lcd_newline();
            printf("YOP NO CAP: %d  ", bottle_count_array[2]);
            break;
        case 2:
            __lcd_home();
            printf("ESKA W/ CAP: %d ", bottle_count_array[3]);
            __lcd_newline();
            printf("ESKA NO CAP: %d ", bottle_count_array[4]);
            break;
        default:
            while(1){
                __lcd_home();
                printf("ERR: BAD BTLCNT");
            }
            break;
    }
    return;
}

void bottle_time(void){
    __lcd_home();
    printf("Total Operation       ");
    __lcd_newline();
    printf("Time: %d s       ", operation_time);
    return;
}

void operation(void){
//    switch(operation_disp){
//        case 0:
//            __lcd_home();
//            printf("Running~              ");
//            operation_disp = 1;
//            break;
//        case 1:
//            __lcd_home();
//            printf("Running~~              ");
//            operation_disp = 2;
//            break;
//        case 2:
//            __lcd_home();
//            printf("Running~~~              ");
//            operation_disp = 0;
//            break;
//    }
    colorprev[0] = color[0];
    colorprev[1] = color[1];
    colorprev[2] = color[2];
    colorprev[3] = color[3];
    
    GIE = 0;
    read_colorsensor();
    if(color[0]>AMBIENTTCSCLEAR){
        flag_bottle = 1;
//        flag_picbug = 0;      //PICBUG DISABLED FOR TESTING
        if(color[0]>NOCAPDISTINGUISH)flag_yopNC = 1;
        if(color[0]>flag_picbug) flag_picbug = color[0];
        if(color[0]>TCSBOTTLEHIGH){
            if(!flag_top_read){
                r = (float) color[1];
                b = (float) color[3];
//                __lcd_home();
//                printf("%f ", r/b);
                if(r/b > 2) bottle_read_top = 1;
                else if(r/b < 0.75) bottle_read_top = 2;
                else bottle_read_top = 0;
                flag_top_read = 1;
            }
            flag_bottle_high = 1;
        }
        else if(color[0]<TCSBOTTLEHIGH){
            if(flag_bottle_high){
                r_p = (float) colorprev[1];
                b_p = (float) colorprev[3];
                if(r_p/b_p > 3.2) bottle_read_bot = 1;
                else if(r_p/b_p < 0.75) bottle_read_bot = 2;
                else bottle_read_bot = 0;
                flag_bottle_high = 0;
            }
        }
    }

    else if(flag_bottle){
//        flag_picbug += 1;
//        if(flag_picbug>1){
            bottle_count_array[0] += 1;
            TMR0 = 0;
            if(bottle_read_top == 2 || bottle_read_bot == 2){
                bottle_count_array[3] += 1;
                servo_rotate1(11);
            }
            else if(bottle_read_top == 1 || bottle_read_bot == 1){
                bottle_count_array[1] += 1;
                servo_rotate0(10);
            }
            
            else if(flag_yopNC){
                bottle_count_array[2] += 1;
                servo_rotate0(14);
            }
            else{
                bottle_count_array[4] += 1;
                servo_rotate1(18);
            }
            flag_bottle = 0;
            flag_bottle_high = 0;
            flag_top_read = 0;
            flag_yopNC = 0;
//            printf("%f", r_p/b_p);
//            __lcd_newline();   //TESTING
//            printf("%d, %d", bottle_read_top, bottle_read_bot);
            __lcd_home();
            printf("%d", flag_picbug);
            flag_picbug = 0; //testing
//      }
    }
    GIE  = 1; //TESTING
    return;
}

void operationend(void){
    __lcd_home();
    printf("Operation Done!");
    return;
}

void emergencystop(void){
    di();
    PORTAbits.RA2 = 0;
    __lcd_clear();
    __lcd_home();
    printf("EMERGENCY STOP");
    while(1){}
    return;
}

void servo_rotate0(int degree){
//        PWM1_Start();
//        set_PWM1_duty(degree);
//        __lcd_home();
//        __lcd_newline();
//        printf("      %d", PWM_Max_Duty());

    int duty = degree;
    for (i=0; i<20; i++) {
        LATCbits.LATC0 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        LATCbits.LATC0 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void servo_rotate1(int degree){
//    PWM2_Stop();
//    set_PWM1_duty(degree);
//    PWM2_Start();
    
    int duty = degree;
    for (i=0; i<20; i++) {
        LATCbits.LATC1 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        LATCbits.LATC1 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void read_colorsensor(void){
//Reading Color
//    I2C_Master_Start();
//    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
//    I2C_Master_Write(0b10110100);   //Write to cmdreg + access&increment clear low reg
//    I2C_Master_Start();     //Repeated start command for combined I2C
//    I2C_Master_Write(0b01010011);   //7bit address 0x29 + Read
//    for(i=0; i<3; i++){
//        color_low[i] = I2C_Master_Read(1); //Reading with acknowledge, continuous
//        color_high[i] = I2C_Master_Read(1); 
//    }
//    color_low[3] = I2C_Master_Read(1); 
//    color_high[3] = I2C_Master_Read(0);    //Final read, no ack 
//    I2C_Master_Stop();              //Stop condition
//    
//    for(i=0; i<4; i++){
//        color[i] = (color_high[i] << 8)|(color_low[i]);
//    }
    
    I2C_Master_Start();                 //Repeated start command for combined I2C
    I2C_Master_Write(0b01010011);       //7bit address 0x29 + Read
    color_low[0] = I2C_Master_Read(1);  //Reading clear with acknowledge, continuous
    color_high[0] = I2C_Master_Read(1); 
    color_low[1] = I2C_Master_Read(1);  //Reading red with acknowledge, continuous
    color_high[1] = I2C_Master_Read(1); 
    color_low[2] = I2C_Master_Read(1);  //Reading green with acknowledge, continuous
    color_high[2] = I2C_Master_Read(1); 
    color_low[3] = I2C_Master_Read(1); 
    color_high[3] = I2C_Master_Read(0); //Final read for blue, no ack 
    I2C_Master_Stop();                  //Stop condition
    color[0] = (color_high[0] << 8)|(color_low[0]);
    color[1] = (color_high[1] << 8)|(color_low[1]);
    color[2] = (color_high[2] << 8)|(color_low[2]);
    color[3] = (color_high[3] << 8)|(color_low[3]);
    
    return;
}