#ifndef PWM_H
#define	PWM_H

//#define _XTAL_FREQ 32000000
#define TMR2PRESCALE 16

int PWM_Max_Duty();
void set_PWM_freq(long fre);
void set_PWM1_duty(unsigned int duty);
void set_PWM2_duty(unsigned int duty);
void PWM1_Start();
void PWM2_Start();
void PWM1_Stop();
void PWM2_Stop();


#endif	/* PWM_H */