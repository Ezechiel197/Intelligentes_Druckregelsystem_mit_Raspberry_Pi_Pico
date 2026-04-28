#include "pregulator.h"
#include "pregulator_limits.h"

#include <Arduino.h>


PressureRegulator::PressureRegulator(uint32_t valve_pin){
    this->valve_pin = valve_pin;
    
    // Set default values
    this->setSetpoint(MIN_SETPOINT);
    this->setHysteresis(MAX_HYSTERESIS);
    this->setMaxValveFrequency(MAX_VALVE_FREQUENCY);
    this->setKeepAliveModeEnabled(DEFAULT_KEEP_ALIVE_ENABLED);
    this->setKeepAlivePeriod(DEFAULT_KEEP_ALIVE_PERIOD_MS);
    this->setPWMEnabled(DEFAULT_PWM_ENABLED);
    this->setPWMDutyCycle(DEFAULT_PWM_DUTY_CYCLE);

    // Set the valve pin as output
    pinMode(this->valve_pin, OUTPUT);
}

bool PressureRegulator::setSetpoint(float setpoint) {
    if (!this->limitCheck(setpoint, MIN_SETPOINT, MAX_SETPOINT)) return false;
    this->setpoint = setpoint;
    return true;
}

float PressureRegulator::getSetpoint() {
    return this->setpoint;
}

bool PressureRegulator::setHysteresis(float hysteresis) {
    if (!this->limitCheck(hysteresis, MIN_HYSTERESIS, MAX_HYSTERESIS)) return false;
    this->hysteresis = hysteresis;
    return true;
}

float PressureRegulator::getHysteresis() {
    return this->hysteresis;
}

bool PressureRegulator::setMaxValveFrequency(float frequency) {
    if (!this->limitCheck(frequency, MIN_VALVE_FREQUENCY, MAX_VALVE_FREQUENCY)) return false;
    this->max_valve_frequency = frequency;
    this->min_time_ms_between_valve_opens = (uint32_t) (1000.0f / frequency);
    return true;
}

float PressureRegulator::getMaxValveFrequency() {
    return this->max_valve_frequency;
}


bool PressureRegulator::limitCheck(float value, float min, float max) {
    return (value >= min && value <= max);
}


void PressureRegulator::update() {

    float pressure = this->readPressureSensor();

    // keep alive mode
    if (this->isValveOpen() && this->isKeepAliveModeEnabled() && millis() - this->last_keep_alive_time_ms > this->keep_alive_ms) {
        this->disable();
        return;
    }

    // is the pressure regulator disabled?
    if(!this->isEnabled()) {

        // manual mode - only works if the pressure regulator is disabled
        if (this->open_valve_manual) {
            this->openValve();
        }
        return;
    }


    // pressure regulator is enabled -> check if the valve should be opened or closed
    float min_pressure = this->setpoint - this->hysteresis;
    float max_pressure = this->setpoint + this->hysteresis;

    if (pressure < min_pressure) {
        this->openValve();
    } else if (pressure > max_pressure) {
        this->closeValve();
    }

}

void PressureRegulator::enable() {
    this->enabled = true;
    this->keepAliveTick();
}

void PressureRegulator::disable() {
    this->enabled = false;
    this->open_valve_manual = false;
    this->closeValve();
}

bool PressureRegulator::isEnabled() {
    return this->enabled;
}

float PressureRegulator::readPressureSensor() {

    // read the latest pressure sensor data, which is given by the second processor via software-fifo
    int available = rp2040.fifo.available();

    if (available == 0) return this->last_pressure; // no new data available, TODO better handling ->sensormight be disconnected
    // stop the regulation if no new data is received after a certain time
    float pressure = this->last_pressure;
    for(int i = 0; i < available-1; i++) // skip to the latest data
    {
        rp2040.fifo.pop(); 
    }

    uint32_t pressure_bin = rp2040.fifo.pop();
    pressure = *(float*)&pressure_bin;// read the pressure sensor data from the other core
    this->last_pressure = pressure;
    return pressure;
}

float PressureRegulator::getPressure() {
    return this->last_pressure;
}


bool PressureRegulator::openValve() {

    // valve is closed
    if (!this->isValveOpen())
    {
        // check if the valve frequency is not exceeded
        if (millis() - this->last_valve_opened_time_ms < this->min_time_ms_between_valve_opens) return false;

        // valve will switch open -> therefore update the last valve open time
        this->last_valve_opened_time_ms = millis();
    }

    // open the valve
    this->_handleValveOpen();
    
    return true;
}


void PressureRegulator::openValveManual() {
    if (this->isEnabled()) return;
    this->open_valve_manual = true;
    this->openValve();
}

void PressureRegulator::setPWMEnabled(bool enabled) {
    this->pwm_enabled = enabled;

    if (enabled) {
        analogWriteFreq(DEFAULT_PWM_FREQUENCY); // set the PWM frequency to 100kHz
    }
}

bool PressureRegulator::isPWMEnabled() {
    return this->pwm_enabled;
}

void PressureRegulator::closeValve() {
    // set the valve pin to LOW
    digitalWrite(this->valve_pin, LOW);
    this->valve_open = false;
    this->open_valve_manual = false;
}

void PressureRegulator::setPWMDutyCycle(int duty_cycle) {
    this->pwm_duty_cycle = duty_cycle;
}

int PressureRegulator::getPWMDutyCycle() {
    return this->pwm_duty_cycle;
}

void PressureRegulator::_handleValveOpen() {
    this->valve_open = true; // set the valve state to open
    if (!this->isPWMEnabled()) {
        // if PWM is not enabled, just set the valve pin to HIGH
        digitalWrite(this->valve_pin, HIGH);
        return;
    }

    // if PWM is enabled, use the duty cycle to control the valve
    if (millis() - this->last_valve_opened_time_ms < this->pwm_delay_period_ms) {
        // open the valve fully for the first pwm_delay_period_ms
        //Serial.println("In FULL MODE");
        digitalWrite(this->valve_pin, HIGH);
        return;
    }
    else {
        //Serial.println("Switched to PWM mode");
        analogWrite(this->valve_pin, this->pwm_duty_cycle);
    }


}

bool PressureRegulator::isValveOpen() {
    // return the state of the valve pin
    return this->valve_open;
}

void PressureRegulator::setKeepAlivePeriod(uint32_t keep_alive_ms) {
    this->keep_alive_ms = keep_alive_ms;
}

uint32_t PressureRegulator::getKeepAlivePeriod() {
    return this->keep_alive_ms;
}

void PressureRegulator::keepAliveTick() {
    this->last_keep_alive_time_ms = millis();
}

void PressureRegulator::setKeepAliveModeEnabled(bool enabled) {
    this->keep_alive_enabled = enabled;
}

bool PressureRegulator::isKeepAliveModeEnabled() {
    return this->keep_alive_enabled;
}
