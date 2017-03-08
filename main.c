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
void bottle_time(void);
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
int operation_time;


int bottle_count_disp = -1; //Data for bottle display screen
int yopcaplbl_count = 0;
int yopcap_count = 0;
int yoplbl_count = 0;
int yop_count = 0;
int eskacaplbl_count = 0;
int eskacap_count = 0;
int eskalbl_count = 0;
int eska_count = 0;
int total_bottle_count = 0;

int operation_disp = 0;         //Data for operation running animation
int color[4];          //Stores TCS data in form clear, red, green, blue


//For queue       0 = YOP + CAP + LBL
//                1 = YOP + CAP - LBL
//                2 = YOP - CAP + LBL
//                3 = YOP - CAP - LBL
//                4 = ESKA + CAP + LBL
//                5 = ESKA + CAP - LBL
//                6 = ESKA - CAP + LBL
//                7 = ESKA - CAP - LBL
int bottlequeue[11];
int bottlequeue_tail;
int bottlequeue_head;
int nodedata;

void main(void) {
    
    // <editor-fold defaultstate="collapsed" desc=" STARTUP SEQUENCE ">
    
    //TRIS Sets Input/Output
    //0 = output
    //1 = input
    TRISA = 0b11111011;           //Set Port A as all input
    TRISB = 0xFF;           //Keypad
    TRISC = 0x00;           //RC3 and RC4 output for I2C (?)
    TRISD = 0x00;           //All output mode for LCD
    TRISE = 0x00;    

    LATA = 0x00;
    LATB = 0x00; 
    LATC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    
    ADCON0 = 0x00;          //Disable ADC
    ADCON1 = 0xFF;          //Set PORTB to be digital instead of analog default  
    
    //ei();                   //Global Interrupt Mask
    GIE = 1;
    INT1IE = 1;             //Enable KP interrupts
    INT0IE = 0;             //Disable external interrupts
    INT2IE = 0;
    
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
                bottle_count_disp = -1;
                curr_state = STANDBY;
                break;
            case 15:    //KP_1
                LATAbits.LATA2 = 1; //Start centrifuge motor
                INT0IE = 1;     //Enable external interrupts
                INT2IE = 1;
                TMR0IE = 1;     //Start timer with interrupts
                TMR0ON = 1;
                TMR0 = 0;
                
                read_time();
                start_time[1] = time[1];
                start_time[0] = time[0];
                
                bottlequeue_head = bottlequeue_tail = 0; //Initiate queue
                
                __lcd_clear();
                bottle_count_disp = -1;
                curr_state = OPERATION;
                break;
            case 31:    //KP_2
                bottle_count_disp += 1;
                curr_state = BOTTLECOUNT;
                while(PORTB == 31){}
                break;
            case 47:    //KP_3
                operation_time = etime - stime;
                bottle_count_disp = -1;
                curr_state = BOTTLETIME;
                break;
            case 63:    //KP_A
                bottle_count_disp = -1;
                curr_state = DATETIME;
                break;
            case 79:    //KP_4
                LATAbits.LATA2 = 0; //Stop centrifuge motor
                INT0IE = 0;         //Disable external interrupts
                INT2IE = 0;
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
            case 95:    //KP_5 -- TESTING
                read_colorsensor();
                __lcd_home();
                printf("C%u R%u                ", color[0], color[1]);
                __lcd_newline();
                printf("G%u B%u                ", color[2], color[3]);
                break;
            case 207:   //KP_*
                LATAbits.LATA2 = 0; //Stop centrifuge motor
                di();               //Disable all interrupts
                TMR0ON = 0;
                __lcd_clear();
                curr_state = EMERGENCYSTOP;
                break;
            case 127:   //KP_B
                servo_rotate0(1);
                break;
            case 191:   //KP_C
                servo_rotate0(2);
                break;
        }
        INT1IF = 0;
    }
    else if (INT0IF){   //Interrupt for first laser sensor at RB0
        if(PORTAbits.RA3){
            read_colorsensor();
            if (color[0]>10000 && color[1]>10000 && color[2]>10000 && color[3]>10000) bottlequeue[bottlequeue_tail] = 2;
            else if (color[0]<3000 && color[1]<1100 && color[2]<1100 && color[3]<1200) bottlequeue[bottlequeue_tail] = 4;
            else if (color[0]<5200 && color[1]<3200 && color[3]<1400 && color[3]<1300) bottlequeue[bottlequeue_tail] = 0;
            else if (color[0]>10000 && color[1]>3600 && color[2]>3900 && color[3]>3400) bottlequeue[bottlequeue_tail] = 6;
            __delay_ms(150);
            read_colorsensor();
            if (color[0]>1000) bottlequeue[bottlequeue_tail] += 1;
            bottlequeue_tail += 1;
        }
        INT0IF = 0;
    }
    else if (INT2IF){   //Interrupt for second laser sensor at RB2
        if(PORTAbits.RA4){
            nodedata = bottlequeue[bottlequeue_head];
            bottlequeue_head += 1;
            total_bottle_count += 1;
            switch (nodedata){
                case 0:
                    servo_rotate0(0);
                    servo_rotate2(0);
                    yopcaplbl_count += 1;
                    break;
                case 1:
                    servo_rotate0(0);
                    servo_rotate2(0);
                    yopcap_count += 1;
                    break;
                case 2:
                    servo_rotate0(0);
                    servo_rotate2(120);
                    yoplbl_count += 1;
                    break;
                case 3:
                    servo_rotate0(0);
                    servo_rotate2(120);
                    yop_count += 1;
                    break;
                case 4:
                    servo_rotate0(120);
                    servo_rotate1(0);
                    eskacaplbl_count += 1;
                    break;
                case 5:
                    servo_rotate0(120);
                    servo_rotate1(0);
                    eskacap_count += 1;
                    break;
                case 6:
                    servo_rotate0(120);
                    servo_rotate1(120);
                    eskalbl_count += 1;
                    break;
                case 7:
                    servo_rotate0(120);
                    servo_rotate1(120);
                    eska_count += 1;
                    break;
            }
        }
        INT2IF = 0;
    }
    else if (TMR0IF){
        LATAbits.LATA2 = 0;
        TMR0ON = 0;
        read_time();
        end_time[1] = time[1];
        end_time[0] = time[0];
        stime = 60*dec_to_hex(start_time[1])+dec_to_hex(start_time[0]);
        etime = 60*dec_to_hex(end_time[1])+dec_to_hex(end_time[0]);
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
    //printf("standby         ");
    //__lcd_newline();
    //printf("PORTB: %d", PORTB);
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
            printf("Total: %d       ", total_bottle_count);
            break;
        case 1:
            __lcd_home();
            printf("YOP+CAP+LBL: %d  ", yopcaplbl_count);
            __lcd_newline();
            printf("YOP+CAP-LBL: %d  ", yopcap_count);
            break;
        case 2:
            __lcd_home();
            printf("YOP-CAP+LBL: %d  ", yoplbl_count);
            __lcd_newline();
            printf("YOP-CAP-LBL: %d  ", yop_count);
            break;
        case 3:
            __lcd_home();
            printf("ESKA+CAP+LBL: %d ", eskacaplbl_count);
            __lcd_newline();
            printf("ESKA+CAP-LBL: %d ", eskacap_count);
            break;
        case 4:
            __lcd_home();
            printf("ESKA-CAP+LBL: %d ", eskalbl_count);
            __lcd_newline();
            printf("ESKA-CAP-LBL: %d ", eska_count);
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
    __lcd_home();
    printf("Total Operation       ");
    __lcd_newline();
    printf("Time: %d s       ", operation_time);
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
    
    __lcd_newline();
    read_colorsensor();
    printf("R%d G%d B%d                ", color[1], color[2], color[3]);
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
    unsigned int i;
    unsigned int j;
    int duty = degree;
    for (i=0; i<50; i++) {
        LATCbits.LATC0 = 1;
        for(j=0; j<duty; j++) __delay_ms(1);
        LATCbits.LATC0 = 0;
        for(j=0; j<(20 - duty); j++) __delay_ms(1);
    }
    return;
}

void servo_rotate1(int degree){
    unsigned int i;
    unsigned int j;
    int duty = ((degree+90)*5/90)+10;
    for (i=0; i<50; i++) {
        LATCbits.LATC1 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        LATCbits.LATC1 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void servo_rotate2(int degree){
    unsigned int i;
    unsigned int j;
    int duty = ((degree+90)*5/90)+10;
    for (i=0; i<50; i++) {
        LATCbits.LATC2 = 1;
        for(j=0; j<duty; j++) __delay_us(100);
        LATCbits.LATC2 = 0;
        for(j=0; j<(200 - duty); j++) __delay_us(100);
    }
    return;
}

void read_colorsensor(void){
    //Set correct TRISC bits for TCS I2C
//    TRISCbits.RC3 = 1;
//    TRISCbits.RC4 = 1;
    
    unsigned char color_low;
    unsigned char color_high;
    int color_comb;
    int i;
    
    //Reading Color
    I2C_Master_Start();
    I2C_Master_Write(0b01010010);   //7bit address 0x29 + Write
    I2C_Master_Write(0b10110100);   //Write to cmdreg + access&increment clear low reg
    I2C_Master_RepeatedStart();             //Repeated start command for combined I2C
    I2C_Master_Write(0b01010011);   //7bit address 0x29 + Read
    for(i=0; i<3; i++){
        color_low = I2C_Master_Read(1); //Reading with acknowledge, continuous
        color_high = I2C_Master_Read(1); 
        color_comb = (color_high << 8)|(color_low);
        color[i] = color_comb;
    }
    color_low = I2C_Master_Read(1); 
    color_high = I2C_Master_Read(0);    //Final read, no ack 
    color_comb = (color_high << 8)|(color_low);
    color[3] = color_comb;
    I2C_Master_Stop();              //Stop condition
    return;
}