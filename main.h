#ifndef MAIN_H
#define	MAIN_H

//FUNCTIONS
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
void read_colorsensor(void);

//VARIABLES
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


//For bottle count
//0 = Total
//1 = YOP + CAP
//2 = YOP - CAP
//3 = ESKA + CAP
//4 = ESKA - CAP
int bottle_count_disp = -1; //Data for bottle display screen
int bottle_count_array[5];

int operation_disp = 0;         //Data for operation running animation
int color[4];          //Stores TCS data in form clear, red, green, blue
int colorprev[4];

int testint[3];
int testflag = 0;

//Bottle Detection Logic
int flag_bottle;
int flag_bottle_high;
int flag_top_read;
int flag_yopNC;
int flag_picbug;
int bottle_read_top;
int bottle_read_bot;

//CONSTANTS
#define MAINPOLLINGDELAYMS  10
#define AMBIENTTCSCLEAR     22
#define TCSBOTTLEHIGH       30
#define NOCAPDISTINGUISH    300

#endif	/* MAIN_H */