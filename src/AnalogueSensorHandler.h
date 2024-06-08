//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <array>
#include <Arduino.h>

//
// Code to read from sensors, handling multiplex
//

#ifndef __WATERINGSYSTEM_ANALOGUESENSORHANDLER_H__
#define __WATERINGSYSTEM_ANALOGUESENSORHANDLER_H__

#define WATERINGSYSTEM_MAXSAMPLESLOTS 10
#define WATERINGSYSTEM_NUMBEROFSENSORS 8

//get a bit from a variable
#define GETBIT(var, bit)  (((var) >> (bit)) & 1)

class AnalogueSensorHandler 
{
  private: 
    short int _sensorReadings[WATERINGSYSTEM_NUMBEROFSENSORS][WATERINGSYSTEM_MAXSAMPLESLOTS] ; // Current sensor readings
    short int _filledSensorSlots[WATERINGSYSTEM_NUMBEROFSENSORS] ; // Count of the number of readings
    short int _currentSensorSlot[WATERINGSYSTEM_NUMBEROFSENSORS] ; // Count of the number of readings
    std::array<int,3> _selectorPins;
    const int _analogInPin = A0;   // ESP8266 Analog Pin ADC0 = A0
    void setActiveChannel(int channelNumber);

//    bool* _pCmdReceived;
    
  public: 
    AnalogueSensorHandler(std::array<int,3> selectorPins); 
    int getAbsoluteSensorReading(int channelNumber);
    int getSensorSimpleMovingAverageReading(int channelNumber);
    void pollSensors();
}; 
/****************************************/


AnalogueSensorHandler::AnalogueSensorHandler(std::array<int,3> selectorPins)
{
  _selectorPins = selectorPins;
  pinMode(_selectorPins.at(0), OUTPUT);
  pinMode(_selectorPins.at(1), OUTPUT);
  pinMode(_selectorPins.at(2), OUTPUT);
  memset(_filledSensorSlots, 0, WATERINGSYSTEM_NUMBEROFSENSORS*sizeof(_filledSensorSlots[0]));
  memset(_currentSensorSlot, 0, WATERINGSYSTEM_NUMBEROFSENSORS*sizeof(_currentSensorSlot[0]));
  
  return;
}

void AnalogueSensorHandler::setActiveChannel(int channelNumber) {
  digitalWrite(_selectorPins.at(0), GETBIT(channelNumber,0));
  digitalWrite(_selectorPins.at(1), GETBIT(channelNumber,1));
  digitalWrite(_selectorPins.at(2), GETBIT(channelNumber,2));
  return;
  
}

int AnalogueSensorHandler::getAbsoluteSensorReading(int channelNumber)
{
  int sensorValue;
  setActiveChannel(channelNumber);
  
  unsigned long loop_time = millis();
  // Yielding for 100ms
  while((millis()-loop_time)< 50){
    yield();
  }
  sensorValue = 1023 - analogRead(_analogInPin);
  return sensorValue;
}

int AnalogueSensorHandler::getSensorSimpleMovingAverageReading(int channelNumber) {
  int sumOfSensorsReadings = 0;
  for (short int slot = 0; slot < _filledSensorSlots[channelNumber]; slot ++) {
    sumOfSensorsReadings += _sensorReadings[channelNumber][slot];
  }
  return sumOfSensorsReadings / _filledSensorSlots[channelNumber];    
}

void AnalogueSensorHandler::pollSensors() {
  // Loop through each sensor, reading a value into the next sensor reading slot
  // Keep track of how many readings we have, and which slot is next
  for (short int sensorChannel = 0; sensorChannel < WATERINGSYSTEM_NUMBEROFSENSORS; sensorChannel++) {
    short int filledSlots = _filledSensorSlots[sensorChannel];
    short int currentSlot = _currentSensorSlot[sensorChannel];
    int sensorReading = getAbsoluteSensorReading(sensorChannel);
    _sensorReadings[sensorChannel][currentSlot] = sensorReading;
    _filledSensorSlots[sensorChannel] =
                           (filledSlots < WATERINGSYSTEM_MAXSAMPLESLOTS)?(filledSlots+1):(WATERINGSYSTEM_MAXSAMPLESLOTS);
    _currentSensorSlot[sensorChannel] = (currentSlot + 1) % WATERINGSYSTEM_MAXSAMPLESLOTS;
  }
}

#endif
