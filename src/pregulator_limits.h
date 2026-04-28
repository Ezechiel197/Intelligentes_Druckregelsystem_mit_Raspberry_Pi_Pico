#ifndef _PRESSURE_REGULATOR_SYSTEM_LIMITS_H
#define _PRESSURE_REGULATOR_SYSTEM_LIMITS_H



#define MAX_SETPOINT 16.0f // (bar) maximum setpoint
#define MIN_SETPOINT 0.0f  // (bar)
#define MAX_HYSTERESIS 3.0f // (bar) maximum hysteresis
#define MIN_HYSTERESIS 0.001f // (bar)
#define MAX_VALVE_FREQUENCY 10.0f // (Hz) maximum valve switching frequency
#define MIN_VALVE_FREQUENCY 0.01f  // (Hz) 

#define DEFAULT_KEEP_ALIVE_PERIOD_MS 4000 // (ms) default keep alive period -> how long will the valve stay open if no serial command is received within this period
#define DEFAULT_KEEP_ALIVE_ENABLED true // default keep alive mode enabled
#define DEFAULT_PWM_DUTY_CYCLE 15 // default PWM duty cycle (0-255)
#define DEFAULT_PWM_DELAY_PERIOD_MS 20 // (ms) valve will be fully powered for this period of time and then pwm_duty_cycle will be applied
#define DEFAULT_PWM_ENABLED true // default PWM mode enabled
#define DEFAULT_PWM_FREQUENCY 10000 // should be at least 1kHz for the valve to stay open (80kHz cutoff frequency for the opto-coupler of the valve driver)
/*
DEFAULT_PWM_FREQUENCY 1kHz
24 V in, HP-Valve
Duty cycle (0-255)   voltage (V) RMS Multimeter
127                  12.7
80                   8.2
40                   4.3
DEFAULT_PWM_FREQUENCY 10kHz
24 V in, HP-Valve
Duty cycle (0-255)   voltage (V) RMS Multimeter
127                  23.8
40                   13
20                   8.2
15                   6.7  <- a minimum of 6V is needed to keep the valve open with a safe margin 
10                   2.7
*/

#endif // _SYSTEM_LIMITS_H

