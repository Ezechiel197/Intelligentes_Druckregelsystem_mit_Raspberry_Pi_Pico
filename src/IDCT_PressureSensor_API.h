#ifndef ICS_PRESSURESENSOR_API_H
#define ICS_PRESSURESENSOR_API_H
#include <Arduino.h>
#include <map>
#include "DFRobot_RTU.h"








#define MAXIMUM_PRESSURE_SENSOR_READ_ATTEMPTS 10 // if the sensor fails to read the pressure this many times in a row it is assumed that it has disconnected. A reconnect will be attempted then.
#define COLD_START_RS485_TIMEOUT_MS 1000 // time (ms) to wait after a cold start of the RS485 bus (for a first connection attempt)
#define NOMINAL_OPERATION_RS485_TIMEOUT_MS 100 // time (ms) to wait after a nominal operation of the RS485 bus (if sensor is already connected)



// registers of the pressure sensor
// see ICS Schneider IDCT User manual for the decription of the input register etc.
enum class IDCT_HOLDING_REGISTERS : uint16_t {
    PRESSURE_UNIT = 0x0000,
    TEMPERATURE_UNIT = 0x0001,
    DEVICE_ADDRESS = 0x0002,
    BAUD_RATE = 0x0003,
    PARITY = 0x0004
};

// Array to store all the attributes of IDCT_HOLDING_REGISTERS
const IDCT_HOLDING_REGISTERS ALL_HOLDING_REGISTERS[] = {
    IDCT_HOLDING_REGISTERS::PRESSURE_UNIT,
    IDCT_HOLDING_REGISTERS::TEMPERATURE_UNIT, // unused by the IDCT531 (only for the IDCT531i with temperature compensation)
    IDCT_HOLDING_REGISTERS::DEVICE_ADDRESS,
    IDCT_HOLDING_REGISTERS::BAUD_RATE,
    IDCT_HOLDING_REGISTERS::PARITY
};

enum class IDCT_PRESSURE_UNITS : uint16_t {
    MM_H2O = 0x0003, 
    MM_HG = 0x0004, 
    PSI = 0x0005, 
    BAR = 0x0006, 
    MBAR = 0x0007, 
    G_CM2=0x0008, 
    KG_CM2 = 0x0009, 
    PA = 0x000A,
    KPA = 0x000B,
    TORR = 0x000C,
    ATM = 0x000D,
    MH2O = 0x000E,
    MPA = 0x000F
};

const std::map<uint16_t, String> PRESSURE_UNIT_STRINGS = {
    {(uint16_t)IDCT_PRESSURE_UNITS::MM_H2O, "mmH2O"},
    {(uint16_t)IDCT_PRESSURE_UNITS::MM_HG, "mmHg"},
    {(uint16_t)IDCT_PRESSURE_UNITS::PSI, "psi"},
    {(uint16_t)IDCT_PRESSURE_UNITS::BAR, "bar"},
    {(uint16_t)IDCT_PRESSURE_UNITS::MBAR, "mbar"},
    {(uint16_t)IDCT_PRESSURE_UNITS::G_CM2, "g/cm2"},
    {(uint16_t)IDCT_PRESSURE_UNITS::KG_CM2, "kg/cm2"},
    {(uint16_t)IDCT_PRESSURE_UNITS::PA, "Pa"},
    {(uint16_t)IDCT_PRESSURE_UNITS::KPA, "kPa"},
    {(uint16_t)IDCT_PRESSURE_UNITS::TORR, "torr"},
    {(uint16_t)IDCT_PRESSURE_UNITS::ATM, "atm"},
    {(uint16_t)IDCT_PRESSURE_UNITS::MH2O, "mH2O"},
    {(uint16_t)IDCT_PRESSURE_UNITS::MPA, "MPa"}
};

enum class IDCT_DATA_TYPES: uint8_t {
    UINT32,
    IEEE754,
    Date
};

struct IDCT_REGISTER_INFO {
    IDCT_DATA_TYPES dataType;
    uint16_t startAddress;
    uint16_t endAddress;
};

std::map<String, IDCT_REGISTER_INFO> IDCT_INPUT_REGISTERS = {
    {"SerialNumber", {IDCT_DATA_TYPES::UINT32, 0x0000, 0x0001}},
    {"LastCalibrationDate", {IDCT_DATA_TYPES::Date, 0x0002, 0x0003}},
    {"PressureUpperRange", {IDCT_DATA_TYPES::IEEE754, 0x0004, 0x0005}},
    {"PressureLowerRange", {IDCT_DATA_TYPES::IEEE754, 0x0006, 0x0007}},
    {"ActualPressure", {IDCT_DATA_TYPES::IEEE754, 0x0008, 0x0009}},
    {"MaximalPressure", {IDCT_DATA_TYPES::IEEE754, 0x000A, 0x000B}},
    {"MinimalPressure", {IDCT_DATA_TYPES::IEEE754, 0x000C, 0x000D}},
    {"TemperatureUpperRange", {IDCT_DATA_TYPES::IEEE754, 0x000E, 0x000F}},
    {"TemperatureLowerRange", {IDCT_DATA_TYPES::IEEE754, 0x0010, 0x0011}},
    {"ActualTemperature", {IDCT_DATA_TYPES::IEEE754, 0x0012, 0x0013}},
    {"MaximalTemperature", {IDCT_DATA_TYPES::IEEE754, 0x0014, 0x0015}},
    {"MinimalTemperature", {IDCT_DATA_TYPES::IEEE754, 0x0016, 0x0017}}
};

// the baud rates that are tried to connect to the pressure sensor
const uint32_t PRESSURE_SENSOR_BAUD_RATES [] = {9600, 38400, 19200, 4800}; // sorted by probability
const uint8_t PRESSURE_SENSOR_ADDRESSES [] = {1, 4}; // from , to

const uint16_t SERIAL_CONFIG0 = (SERIAL_STOP_BIT_1 | SERIAL_PARITY_NONE | SERIAL_DATA_8);
const uint16_t SERIAL_CONFIG1 = (SERIAL_STOP_BIT_1 | SERIAL_PARITY_ODD | SERIAL_DATA_8);
const uint16_t SERIAL_CONFIG2 = (SERIAL_STOP_BIT_1 | SERIAL_PARITY_EVEN | SERIAL_DATA_8);
const uint16_t SERIAL_CONFIGS[] = {SERIAL_CONFIG0, SERIAL_CONFIG1, SERIAL_CONFIG2}; // sorted by probability



class IDCT531_PressureSensor {
private:
    SerialUART* serial;
    DFRobot_RTU* modbus;
    uint8_t address;
    uint16_t last_baud;
    
    bool _connected = false;
    uint16_t _failed_reads = 0;

    // Sensor Info
    uint16_t pressure_unit;
    uint32_t serial_number;
    float pressure_upper_range;
    float pressure_lower_range;
    uint32_t last_calibration_date;


    
public:
    IDCT531_PressureSensor(SerialUART*serial) {
        this->serial = serial;
        //this->serial->setFIFOSize(256);
        this->modbus = new DFRobot_RTU(serial);
        
    }
    
    void connect(bool try_specific_address_first=false, uint8_t specific_address=1, uint32_t specific_baud=9600, uint16_t specific_config=SERIAL_CONFIG0) {
        /*  Try to connect to the pressure sensor:
            */

        this->modbus->setTimeoutTimeMs(COLD_START_RS485_TIMEOUT_MS);

        if (try_specific_address_first) {
            _beginSerial(specific_baud, specific_config);
            address = specific_address;
            if (this->_readSensorInfo()) return; // if the sensor was found, return
        }


        // Iterate throug the typical address range and possible baud rates to find the sensor 
        while(1){
            for (uint16_t config : SERIAL_CONFIGS) {
                for (uint32_t baud : PRESSURE_SENSOR_BAUD_RATES) {
                    _beginSerial(baud, config);

                    for (uint8_t addr = PRESSURE_SENSOR_ADDRESSES[0]; addr <= PRESSURE_SENSOR_ADDRESSES[1]; addr++) {
                        // Try to read the sensor information -> if it fails, try the next address
                        address = addr;
                        // Serial.println("Trying address " + String(address) + " with baud " + String(baud));
                        if (this->_readSensorInfo()){
                            // Serial.println("Sensor found at address " + String(address) + " with baud " + String(baud) + "!!!!");
                            this->modbus->setTimeoutTimeMs(NOMINAL_OPERATION_RS485_TIMEOUT_MS);
                            this->_connected = true;
                            this->_failed_reads = 0;
                            return; // if the sensor was found, return
                        }
                    }

                    close();
                }
            }

            // If no sensor was found, wait for a while and try again
            delay(1000);
        }
    }

    void close() {
        serial->end();
        _connected = false;
        _failed_reads = 0;
        delay(10);
    }

    bool isConnected() {
        return _connected;
    }

    uint32_t getSerialNumber() {
        return serial_number;
    }

    void reconnect() {
        close();
        connect(true, address, last_baud);
    }
    
    float readPressure() {
        // Read the raw pressure value from the sensor
        uint16_t data[2];
        if(!_readInputRegister("ActualPressure", data)) {
            _failed_reads++;

            if(_failed_reads > MAXIMUM_PRESSURE_SENSOR_READ_ATTEMPTS) { 
                // if the sensor fails to read MAXIMUM_PRESSURE_SENSOR_READ_ATTEMPTS times in a row it is assumed that it has disconnected, try to reconnect
                reconnect();
            }
            return -1e7f;
        }
        _failed_reads = 0;

        // Convert raw pressure to actual pressure value
        uint32_t pressure_bin = (uint32_t)data[0] << 16 | data[1];
        float pressure = *(float*)&pressure_bin;
        
        return pressure;
    }

    String getPressureUnit() {
        return PRESSURE_UNIT_STRINGS.at(pressure_unit);
    }

    float getPressureUpperRange() {
        return pressure_upper_range;
    }

    float getPressureLowerRange() {
        return pressure_lower_range;
    }

    String getLastCalibrationDate() {
        // TODO: convert the date to a human readable format
        uint16_t year = (last_calibration_date & 0x7F) + 2000;
        uint8_t month = (last_calibration_date >> 24)  & 0xF;
        uint8_t day   = (last_calibration_date >> 16)  & 0x1F;


        return String(day) + "." + String(month) + "." + String(year);
    }

private:
    void _beginSerial(uint32_t baud, uint16_t SERIAL_CONFIG) {
        last_baud = baud;
        this->serial->begin(baud, SERIAL_CONFIG);

        delay(600); // give the sensor some time to wake up
    }

    bool _readSensorInfo() {
        if(!_readAllHoldingRegisters()) return false;
        if(!_readAllInputRegisters()) return false;
        return true;
    }

    bool _readAllHoldingRegisters() {
        uint16_t successfull_reads = 0;

        for(IDCT_HOLDING_REGISTERS register_id : ALL_HOLDING_REGISTERS)
        {
            uint16_t data;
            // Serial.println("Try to read register " + String((uint16_t)register_id));
            uint8_t result =modbus->readHoldingRegister(address, (uint16_t)register_id, &data, 1);
            if(result != 0) // if reading the register fails
            { 
                // Serial.println("Failed to read register " + String((uint16_t)register_id) + " with error " + String(result));
            }
            else {
                // Serial.println("Read register " + String((uint16_t)register_id) + " with value " + String(data));
                successfull_reads++;
            }
            if(register_id == IDCT_HOLDING_REGISTERS::PRESSURE_UNIT) pressure_unit = data;
        }

        if(successfull_reads == 0) return false;

        return true;
    }

    bool _readInputRegister(String register_name, uint16_t* data) {
        IDCT_REGISTER_INFO info = IDCT_INPUT_REGISTERS.at(register_name);
        // Serial.println("Try to read input register " + register_name);
        if(modbus->readInputRegister(address, info.startAddress, data, info.endAddress - info.startAddress + 1) != 0) // if reading the register fails
        { 
            // Serial.println("Failed to input read register " + register_name);
            return false;
        }
        return true;
    }

    bool _readAllInputRegisters() {
        uint16_t successfull_reads = 0;
        for(auto const& [register_name, info] : IDCT_INPUT_REGISTERS)
        {
            delay(100);
            uint16_t data[2];
            
            if(!_readInputRegister(register_name, data)) {

                continue;
            }

            uint32_t data_bin = (uint32_t)data[0] << 16 | data[1];

            
            
            successfull_reads++;
            float data_float;


            switch(info.dataType) {
                case IDCT_DATA_TYPES::UINT32:
                    // Serial.println("Read input register " + register_name + " with value " + String(data_bin));
                    if(register_name == "SerialNumber") serial_number = data_bin;
                    break;
                case IDCT_DATA_TYPES::IEEE754:
                    data_float = *(float*)&data_bin;
                    
                    // Serial.println("Read input register " + register_name + " with value " + String(data_float));
                    if(register_name == "PressureUpperRange") {
                        pressure_upper_range = data_float;
                    }
                    else if(register_name == "PressureLowerRange") {
                        pressure_lower_range = data_float;
                    }
                    break;
                case IDCT_DATA_TYPES::Date:
                    last_calibration_date = data_bin;
                    break;
            }
        }


        return successfull_reads != 0;
    }
};

#endif // ICS_PRESSURESENSOR_API_H
