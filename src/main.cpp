#include <Arduino.h> // !!!!install board from https://github.com/earlephilhower/arduino-pico
#include "SerialCommands.h"
#include <string>


#define LEDPIN 25
#define CMD_LENGTH 40
#define CARRIGE_RETURN_LINE_FEED "\r\n"
#define STRING_SPACE " "
#define DEFAULT_FLASH_TIME 1000
#define DEFAULT_FLASH_TIME_MIN 10
#define DEFAULT_FLASH_TIME_MAX 10000
#define BAUD_RATE 9600
#define MESSAGE_LENGTH 50

 
char buffer[CMD_LENGTH];
bool flash_cmd=false;
uint16_t waiting_period;
using namespace std;

  
SerialCommands serial_commands(&Serial, buffer, sizeof(buffer), CARRIGE_RETURN_LINE_FEED, STRING_SPACE);

// function 
void defaulHandler(SerialCommands* sender, const char* cmd);
void go_off(SerialCommands* sender);
void stop_flash(SerialCommands* sender);
bool number_verify(const char * txt);

// function code

bool number_verify(const char * txt){
  for(uint8_t i = 0;txt[i] != '\0';i++){ 
    if(!isDigit(txt[i])){
      return false;
    }
  }
  return true;
}

void defaulHandler(SerialCommands* sender, const char* cmd)
{
  char message[MESSAGE_LENGTH]; //  the messsage will be longer than 50 character
  snprintf(message, sizeof(message), "Unrecognized command: %s", cmd);
  Serial.println(message);
}

void go_off(SerialCommands* sender){
    
  const char* text = sender->Next();
  if(text == 0){
    waiting_period=DEFAULT_FLASH_TIME;
    Serial.println("LED flash");
    return;
  }
    
  if(number_verify(text)){
    waiting_period=constrain(stoi(text), DEFAULT_FLASH_TIME_MIN, DEFAULT_FLASH_TIME_MAX);
    flash_cmd = true;
    Serial.println("LED flash");
  }
  else {
    Serial.println("Please, give one integer number für the delay!");
    Serial.println("Die LED stop");
    flash_cmd = false;
  }   
}

void stop_flash(SerialCommands* sender){
  Serial.println("Die LED stop");
  flash_cmd = false;
}

SerialCommand startCommand("Start", go_off);
SerialCommand stopCommand("Stop", stop_flash);
void setup(){
  Serial.begin(BAUD_RATE);

  pinMode(LEDPIN,OUTPUT);
  digitalWrite(LEDPIN,HIGH);
 
  serial_commands.AddCommand(&startCommand);
  serial_commands.AddCommand(&stopCommand);
  
  serial_commands.SetDefaultHandler(defaulHandler);
}
void loop(){
  serial_commands.ReadSerial();

  if(flash_cmd){
    
    digitalWrite(LEDPIN,HIGH);
    delay(waiting_period);
    digitalWrite(LEDPIN,LOW);
    delay(waiting_period);
  }
  else {
    digitalWrite(LEDPIN,LOW);
  }
}
