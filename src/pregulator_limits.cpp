#include <Arduino.h> // !!!!install board from https://github.com/earlephilhower/arduino-pico
#include "SerialCommands.h"
#include "pregulator.h"
#include "IDCT_PressureSensor_API.h"
#include "HardwareSerial.h"



#define PRESSURE_REGULATOR_SOFTWARE_VERSION "0.9.8" // the software version of the pressure regulator
#define PRESSURE_REGULATOR_SOFTWARE_BUILD_DATE "07.03.2024" // last build date of the pressure regulator software
#define PRESSURE_REGULATOR_SOFTWARE_AUTHOR "Jan-René Haferkamp" // the author of the pressure regulator software

// #############################    PIN Layout    ##################################################################
#define RS485_TX0_PIN 0 // the pin that the RS485 HAT is connected to (UART0)
#define RS485_RX0_PIN 1 // the pin that the RS485 HAT is connected to (UART0)
#define RS485_TX1_PIN 4 // the pin that the RS485 HAT is connected to (UART1) -> unused (only UART0 is connected to a pressure sensor)
#define RS485_RX1_PIN 5 // the pin that the RS485 HAT is connected to (UART1) -> unused
#define VALVE_PIN 17    // GP17 the pin that the valve is connected to


// #############################    PRESSURE REGULATOR    ##################################################################
PressureRegulator pregulator(VALVE_PIN);
IDCT531_PressureSensor* pressure_sensor = NULL; // the pressure sensor object



// #############################    SERIAL COMMUNICATION TO PC    ##################################################################
#define CMD_BUFFER_SIZE 64    // maximum length of a command
#define CMD_SUCCESS_STR "OK"  // the string that is returned when a command is successful
#define CMD_MISSING_ARGS_STR "ERROR MISSING PARAMETER" // the string that is returned when a command requires a parameter
#define NEWLINE "\r\n"        // this new line is used to terminate the command
#define DELIM "="             // this is the delimiter between the command and the parameters
#define PC_SERIAL_BAUD_RATE 115200 // the baud rate of the serial connection to the PC

char serial_command_buffer_[CMD_BUFFER_SIZE];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), NEWLINE, DELIM);

void cmd_unrecognized(SerialCommands* sender, const char* cmd);
void cmd_set_p_setpoint(SerialCommands* sender);
void cmd_get_p_setpoint(SerialCommands* sender);
void cmd_get_pressure(SerialCommands* sender);
void cmd_set_hysteresis(SerialCommands* sender);
void cmd_get_hysteresis(SerialCommands* sender);
void cmd_set_max_valve_frequency(SerialCommands* sender);
void cmd_get_max_valve_frequency(SerialCommands* sender);
void cmd_enable(SerialCommands* sender);
void cmd_is_enabled(SerialCommands* sender);
void cmd_set_keep_alive_mode(SerialCommands* sender);
void cmd_is_keep_alive_mode(SerialCommands* sender);
void cmd_set_keep_alive_period(SerialCommands* sender);
void cmd_get_keep_alive_period(SerialCommands* sender);
void cmd_reset_system(SerialCommands* sender);
void cmd_help(SerialCommands* sender);
void cmd_valve_manual(SerialCommands* sender);
void cmd_is_valve_open(SerialCommands* sender);
void cmd_get_version(SerialCommands* sender);
void cmd_get_pressure_sensor_info(SerialCommands* sender); // TODO
void cmd_is_pwm_enabled(SerialCommands* sender); // TODO
void cmd_set_pwm_enabled(SerialCommands* sender); // TODO
void cmd_set_pwm_duty_cycle(SerialCommands* sender); // TODO
void cmd_get_pwm_duty_cycle(SerialCommands* sender); // TODO


SerialCommand cmds[] { 
  SerialCommand("FREQ_LIMIT",  cmd_set_max_valve_frequency),
  SerialCommand("FREQ_LIMIT?", cmd_get_max_valve_frequency),
  SerialCommand("HELP", cmd_help),
  SerialCommand("HYSTERESIS",  cmd_set_hysteresis),
  SerialCommand("HYSTERESIS?", cmd_get_hysteresis),
  SerialCommand("KEEP_ALIVE_ENABLED", cmd_set_keep_alive_mode),
  SerialCommand("KEEP_ALIVE_ENABLED?", cmd_is_keep_alive_mode),
  SerialCommand("KEEP_ALIVE_PERIOD", cmd_set_keep_alive_period),
  SerialCommand("KEEP_ALIVE_PERIOD?", cmd_get_keep_alive_period),
  SerialCommand("P?", cmd_get_pressure),
  SerialCommand("P_SENSOR_INFO?", cmd_get_pressure_sensor_info),
  SerialCommand("PRS", cmd_enable),
  SerialCommand("PRS?", cmd_is_enabled),
  SerialCommand("PWM_DUTY_CYCLE", cmd_set_pwm_duty_cycle),
  SerialCommand("PWM_DUTY_CYCLE?", cmd_get_pwm_duty_cycle),
  SerialCommand("PWM_ENABLED", cmd_set_pwm_enabled),
  SerialCommand("PWM_ENABLED?", cmd_is_pwm_enabled),
  SerialCommand("P_SET",  cmd_set_p_setpoint),
  SerialCommand("P_SET?", cmd_get_p_setpoint),
  SerialCommand("RESET_SYSTEM", cmd_reset_system),
  SerialCommand("VALVE_MANUAL", cmd_valve_manual),
  SerialCommand("VALVE_OPEN?", cmd_is_valve_open),
  SerialCommand("VERSION?", cmd_get_version)
};





// #############################    CORE 0    ##################################################################

void setup() 
{
	Serial.begin(PC_SERIAL_BAUD_RATE);

  // setup serial commands
	serial_commands_.SetDefaultHandler(cmd_unrecognized);
  for (SerialCommand& cmd : cmds) {
    serial_commands_.AddCommand(&cmd);
  }

	Serial.println("Ready!");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}



/*
  This is the main loop of the first processor core. It handles the serial communication
  with the PC and the pressure regulator.
*/
void loop() 
{
  // wait for serial data and process it
	switch(serial_commands_.ReadSerial()) // non-blocking
  {
    case SERIAL_COMMANDS_NO_COMMAND:
      // no command received -> do nothing
      break;
    case SERIAL_COMMANDS_SUCCESS:
      // command received 
      pregulator.keepAliveTick(); // reset the keep alive timer
      break;
    case SERIAL_COMMANDS_ERROR_NO_SERIAL:
      // wait for serial
      // Serial.begin(115200); // TODO
      break;
    case SERIAL_COMMANDS_ERROR_BUFFER_FULL:
      // buffer full
      Serial.print("ERROR BUFFER FULL - CMD TOO LONG (>");
      Serial.print(CMD_BUFFER_SIZE);
      Serial.println(")");
      break;
  }

  
  // update the pressure regulator
  pregulator.update();
}









// #############################    CORE 1    ##################################################################
/*
  This is the second processor core, which is used to handle the RS485 communication via UART (HAT from Waveshare).
  It deals with the connection / reconnection of the pressure sensor and the communication with the first processor core.
  Data is sent to the first processor core via a software fifo.
*/

uint8_t faulty_sensor_counter = 0; // the counter for the faulty sensor data
void setup1() 
{
  // init the second processor core
  // init UART connection to the RS485 HAT (Waveshare, only one sensor / the UART0 is used)
  // UART0 -> Serial1
  Serial1.setPinout(RS485_TX0_PIN, RS485_RX0_PIN);
  Serial2.setPinout(RS485_TX1_PIN, RS485_RX1_PIN); // unused
  delay(2000);

  pressure_sensor = new IDCT531_PressureSensor(&Serial1);
  pressure_sensor->connect();
  delay(1000);
}

void loop1() 
{
  // TODO read the pressure sensor and handles the reconnecting of the sensor etc...
  // float data = (float) random(0, 16);
  float data = pressure_sensor->readPressure();

  // Serial.println(data);
  // check if the data is valid
  if (data < -1e6f) {
    // no valid data, sensor disconnected?
    faulty_sensor_counter++;
    if (faulty_sensor_counter > 10) {
      // try to reconnect the sensor
      faulty_sensor_counter = 0;
      pressure_sensor->reconnect();
    }

    

    delay(10);
    return; // try again
  }

  // valid data, reset the faulty sensor counter
  faulty_sensor_counter = 0;

  // send the data to the first processor core
  // convert the float to a binary representation (uint32_t)
  uint32_t data_bin = *(uint32_t*)&data;
  rp2040.fifo.push_nb(data_bin); // non-blocking - send the data to the first processor core
  // wait a bit for the sensor -> necessary or the communication will have many timeouts
  delay(100);
}











// #############################    SERIAL COMMANDS METHODS   ##################################################################
void return_OK(SerialCommands* sender) {
  sender->GetSerial()->println(CMD_SUCCESS_STR);
}

//This is the default handler, and gets called when no other command matches. 
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
    sender->GetSerial()->print("Unrecognized command [");
    sender->GetSerial()->print(cmd);
    sender->GetSerial()->println("]");
    sender->GetSerial()->println("Type HELP to see the available commands");
}

void cmd_set_p_setpoint(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (!pregulator.setSetpoint(atof(next))) {
    sender->GetSerial()->println("ERROR");
    return;
  }
  cmd_get_p_setpoint(sender);
}

void cmd_get_p_setpoint(SerialCommands* sender) {
  sender->GetSerial()->print("PSET=");
  sender->GetSerial()->println(pregulator.getSetpoint());
  return_OK(sender);
}

void cmd_help(SerialCommands* sender) {
  sender->GetSerial()->println("Available commands:");
  for (SerialCommand& cmd : cmds) {
    sender->GetSerial()->print("  ");
    sender->GetSerial()->println(cmd.command);
  }
  sender->GetSerial()->println();
  return_OK(sender);
}

void cmd_set_hysteresis(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (!pregulator.setHysteresis(atof(next))) {
    sender->GetSerial()->println("ERROR");
    return;
  }
  cmd_get_hysteresis(sender);
}

void cmd_get_hysteresis(SerialCommands* sender) {
  sender->GetSerial()->print("HYSTERESYS=");
  sender->GetSerial()->println(pregulator.getHysteresis());
  return_OK(sender);
}

void cmd_set_max_valve_frequency(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (!pregulator.setMaxValveFrequency(atof(next))) {
    sender->GetSerial()->println("ERROR");
    return;
  }
  cmd_get_max_valve_frequency(sender);
}

void cmd_get_max_valve_frequency(SerialCommands* sender) {
  sender->GetSerial()->print("FSET=");
  sender->GetSerial()->println(pregulator.getMaxValveFrequency());
  return_OK(sender);
}

void cmd_enable(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (atoi(next)==1) {
    pregulator.enable();
  } else {
    pregulator.disable();
  }

  cmd_is_enabled(sender);
}

void cmd_is_enabled(SerialCommands* sender) {
  sender->GetSerial()->print("PRS=");
  sender->GetSerial()->println(pregulator.isEnabled());
  return_OK(sender);
}

void cmd_get_pressure(SerialCommands* sender) {
  sender->GetSerial()->print("P=");
  sender->GetSerial()->println(pregulator.getPressure());
  return_OK(sender);
}

void cmd_set_keep_alive_mode(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (atoi(next)==1) {
    pregulator.setKeepAliveModeEnabled(true);
  } else {
    pregulator.setKeepAliveModeEnabled(false);
  }
  cmd_is_keep_alive_mode(sender);
}

void cmd_is_keep_alive_mode(SerialCommands* sender) {
  sender->GetSerial()->print("KEEP_ALIVE=");
  sender->GetSerial()->println(pregulator.isKeepAliveModeEnabled());
  sender->GetSerial()->println(CMD_SUCCESS_STR);
}

void cmd_set_keep_alive_period(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  pregulator.setKeepAlivePeriod(atoi(next));
  cmd_get_keep_alive_period(sender);
}

void cmd_get_keep_alive_period(SerialCommands* sender) {
  sender->GetSerial()->print("KEEP_ALIVE_PERIOD=");
  sender->GetSerial()->println(pregulator.getKeepAlivePeriod());
  return_OK(sender);
}

void cmd_reset_system(SerialCommands* sender) {
  sender->GetSerial()->println("ERROR NOT IMPLEMENTED");
}


/*
  Manually open or close the valve
  1 -> open
  0 -> close
  Only works if the pressure regulator mode (PRS) is disabled
*/
void cmd_valve_manual(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }
  if (atoi(next)==1) {
    if (pregulator.isEnabled()) {
      sender->GetSerial()->println("ERROR PRESSURE REGULATOR MODE IS ENABLED. CAN'T OPEN VALVE MANUALY. DISABLE PRESSURE REGULATOR MODE WITH PRS=0 FIRST.");
      return;
    }
    pregulator.openValveManual();
  } else {
    pregulator.closeValve();
  }
  cmd_is_valve_open(sender);
}

void cmd_is_valve_open(SerialCommands* sender) {
  sender->GetSerial()->print("VALVE_OPEN=");
  sender->GetSerial()->println(pregulator.isValveOpen());
  return_OK(sender);
}

void cmd_get_version(SerialCommands* sender) {
  sender->GetSerial()->print("VERSION=");
  sender->GetSerial()->println(PRESSURE_REGULATOR_SOFTWARE_VERSION);
  sender->GetSerial()->print("BUILD_DATE=");
  sender->GetSerial()->println(PRESSURE_REGULATOR_SOFTWARE_BUILD_DATE);
  sender->GetSerial()->print("AUTHOR=");
  sender->GetSerial()->println(PRESSURE_REGULATOR_SOFTWARE_AUTHOR);
  sender->GetSerial()->print("PICO_W=");
  sender->GetSerial()->println(rp2040.isPicoW());
  sender->GetSerial()->print("PICO_BOARD_ID=");
  sender->GetSerial()->println(rp2040.getChipID());
  sender->GetSerial()->println();
  return_OK(sender);
}


void cmd_get_pressure_sensor_info(SerialCommands* sender) {
  /*
    Warning! The pressure sensor instance is accessed here from the first processor core.
    However, the pressure sensor should only be accessed from the second processor core.
    It will lead to freezing the pico ! 
    The best way to transport the data from core 1 to core 0 is to use a software fifo.
    -> a protocol is needed (right now only raw pressure data is sent to the first processor core)
  
  sender->GetSerial()->println("[SENSOR_INFO]");
  sender->GetSerial()->print("CONNECTED=");
  sender->GetSerial()->println(pressure_sensor->isConnected());
  sender->GetSerial()->print("SERIAL_NUMBER=");
  sender->GetSerial()->println(pressure_sensor->getSerialNumber());
  sender->GetSerial()->print("UNIT=");
  sender->GetSerial()->println(pressure_sensor->getPressureUnit());
  sender->GetSerial()->print("PRESSURE_RANGE_MIN=");
  sender->GetSerial()->println(pressure_sensor->getPressureLowerRange());
  sender->GetSerial()->print("PRESSURE_RANGE_MAX=");
  sender->GetSerial()->println(pressure_sensor->getPressureUpperRange());
  sender->GetSerial()->print("LAST_CALIBRATION_DATE=");
  sender->GetSerial()->println(pressure_sensor->getLastCalibrationDate());
  sender->GetSerial()->println();
  return_OK(sender);
  */

  sender->GetSerial()->println("ERROR NOT IMPLEMENTED");
}


void cmd_is_pwm_enabled(SerialCommands* sender) {
  sender->GetSerial()->print("PWM_ENABLED=");
  sender->GetSerial()->println(pregulator.isPWMEnabled());
  return_OK(sender);
}

void cmd_set_pwm_enabled(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }

  pregulator.setPWMEnabled(atoi(next)==1);
  cmd_is_pwm_enabled(sender);
}

void cmd_set_pwm_duty_cycle(SerialCommands* sender) {
  char*next = sender->Next();
  if (next==NULL) {
    sender->GetSerial()->println(CMD_MISSING_ARGS_STR);
    return;
  }

  int duty_cycle = atoi(next);

  if (duty_cycle < 0 || duty_cycle > 255) {
    sender->GetSerial()->println("ERROR INVALID DUTY CYCLE (must be between 0 and 255)");
    return;
  }
  pregulator.setPWMDutyCycle(duty_cycle);
  cmd_get_pwm_duty_cycle(sender);
}

void cmd_get_pwm_duty_cycle(SerialCommands* sender) {
  sender->GetSerial()->print("PWM_DUTY_CYCLE=");
  sender->GetSerial()->println(pregulator.getPWMDutyCycle());
  return_OK(sender);
}


#define DEFAULT_BLINK_ZEIT 1000
#define MIN_BLINK_ZEIT 10
#define MAX_BLINK_ZEIT 10000



void losgehen(SerialCommands* sender){
    
    const char* text=sender->Next();

    // kein Parameter angegeben -> benutze default wert
    if(text == 0){
        zeit = DEFAULT_BLINK_ZEIT;
        Serial.println("LED flash");
        return;
    }


    if(number_verify(text)) {
        zeit = constrain(stoi(text), MIN_BLINK_ZEIT, MAX_BLINK_ZEIT);
        x = true;
        Serial.println("LED flash");
    }
    else {
        Serial.println("Please, give one integer number für the delay!");
        Serial.println("Die LED stop");
        x = false;
    } 



    
}