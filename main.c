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

void set_time(void);
int dec_to_hex(int num);
void date_time(void);
void read_time(void);
void bottle_count(void);
void bottle_time(int time);
void standby(void);
void operation(void);
void operationend(void);
void emergencystop(void);
void servo_rotate0(int degree);
void servo_rotate1(int degree);
void servo_rotate2(int degree);
void read_colorsensor(void);

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
int stime;
int etime;
char *ptr;
int bottle_count_disp = -1;
int operation_disp = 0;
unsigned short color[4];                           //Stores TCS data in form clear, red, green, blue

void main(void) {
    
    // <editor-fold defaultstate="collapsed" desc=" STARTUP SEQUENCE ">
    
    //TRIS Sets Input/Output
    //0 = output
    //1 = input
    TRISA = 0xFF;           // Set Port A as all input
    TRISB = 0xFF; 
    TRISC = 0b00011000;     //RC3 and RC4 for I2C
    TRISD = 0x00;           //All output mode for LCD
    TRISE = 0x00;    

    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
    ADCON0 = 0x00;          //Disable ADC
    ADCON1 = 0xFF;          //Set PORTB to be digital instead of analog default  
    
    INT1IE = 1;
    
    nRBPU = 0;
    
    initLCD();
    I2C_Master_Init(10000); //Initialize I2C Master with 100KHz clock
    I2C_ColorSens_Init();   //Initialize TCS34725 Color Sensor
    
    //Set Timer Properties
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
                bottle_time(etime - stime);
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
                stime = 60*dec_to_hex(start_time[1])+dec_to_hex(start_time[0]);
                etime = 60*dec_to_hex(end_time[1])+dec_to_hex(end_time[0]);
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
            case 127:   //KP_B
                servo_rotate0(1);
                break;
            case 191:   //KP_C
                servo_rotate0(180);
                break;
            case 175:   //KP_9
                read_colorsensor();
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
    printf("PORTB: ", PORTB);
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

void bottle_time(int time){
    __lcd_home();
    printf("Total Operation       ");
    __lcd_newline();
    printf("Time: %d s       ", time);
    return;
}

void operation(void){
    switch(operation_disp){
        case 0:
            __lcd_home();
            printf("Running~              ");
            __lcd_newline();
            read_colorsensor();
            printf("C: %d                ", color[0]);
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

void servo_rotate0(int degree){
    unsigned int i;
    unsigned int j;
    int duty = degree*10/90;
    for (i=0; i<50; i++) {
        PORTCbits.RC0 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        PORTCbits.RC0 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void servo_rotate1(int degree){
    unsigned int i;
    unsigned int j;
    int duty = degree*10/90;
    for (i=0; i<50; i++) {
        PORTCbits.RC1 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        PORTCbits.RC1 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void servo_rotate2(int degree){
    unsigned int i;
    unsigned int j;
    int duty = degree*10/90;
    for (i=0; i<50; i++) {
        PORTCbits.RC2 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        PORTCbits.RC2 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void read_colorsensor(void){
    unsigned short color_low;
    unsigned short color_high;
    unsigned short color_comb;
    short i;
    
    //Reading Color
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10110100);   //Write to cmdreg + access&increment clear low reg
    I2C_Master_Start();             //Repeated start command for combined I2C
    I2C_Master_Write(0b01010011);   //7bit address 0x29 + Read
    for(i=0; i<4; i++){
        color_low = I2C_Master_Read(1); //Reading with acknowledge, continuous
        color_high = I2C_Master_Read(1); 
        color_comb = (color_high << 8)||(color_low & 0xFF);
        color[i] = color_comb;
    }    
    I2C_Master_Stop();              //Stop condition
    return;
}