#ifndef _PRESSURE_REGULATOR_H
#define _PRESSURE_REGULATOR_H
#include <Arduino.h>
#include "pregulator_limits.h"



class PressureRegulator {
    private:
        float setpoint;
        float hysteresis;
        float max_valve_frequency;
        bool  enabled;
        bool  keep_alive_enabled;
        float last_pressure;
        uint32_t last_valve_opened_time_ms;
        uint32_t min_time_ms_between_valve_opens;
        uint32_t keep_alive_ms;
        uint32_t last_keep_alive_time_ms;
        uint32_t valve_pin;

        bool pwm_enabled;
        bool valve_open;
        bool open_valve_manual;
        int pwm_duty_cycle; // 0 - 255 (0 - 100%)
        uint32_t pwm_delay_period_ms = DEFAULT_PWM_DELAY_PERIOD_MS; // valve will be fully powered for this period of time and then pwm_duty_cycle will be applied
    
    public:
        PressureRegulator(uint32_t valve_pin);

        bool  setSetpoint(float setpoint);
        float getSetpoint();

        float getPressure();

        void  enable();
        void  disable();
        bool  isEnabled();

        bool  setHysteresis(float hysteresis);
        float getHysteresis();

        bool  setMaxValveFrequency(float frequency);
        float getMaxValveFrequency();

        
        void  openValveManual();
        void  closeValve();
        bool  isValveOpen();

        void setKeepAlivePeriod(uint32_t keep_alive_ms);
        uint32_t getKeepAlivePeriod();
        void keepAliveTick(); // call this regularly to keep the pressure regulator enabled
        void setKeepAliveModeEnabled(bool enabled);
        bool isKeepAliveModeEnabled();


        void setPWMEnabled(bool enabled);
        bool isPWMEnabled();
        void setPWMDutyCycle(int duty_cycle);
        int  getPWMDutyCycle();

        void  update();

    private:
        bool  limitCheck(float value, float min, float max);
        float readPressureSensor();
        void  _handleValveOpen();
        bool  openValve();
};



#endif // _PRESSURE_REGULATOR_H
