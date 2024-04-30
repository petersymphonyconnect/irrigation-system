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

//get a bit from a variable
#define GETBIT(var, bit)  (((var) >> (bit)) & 1)

class AnalogueSensorHandler 
{
  private: 
    std::array<int,3> _selectorPins;
    const int _analogInPin = A0;   // ESP8266 Analog Pin ADC0 = A0
    void setActiveChannel(int channelNumber);

//    bool* _pCmdReceived;
    
  public: 
    AnalogueSensorHandler(std::array<int,3> selectorPins); 
    int getSensorReading(int channelNumber);

}; 
/****************************************/


AnalogueSensorHandler::AnalogueSensorHandler(std::array<int,3> selectorPins)
{
  _selectorPins = selectorPins;
  pinMode(_selectorPins.at(0), OUTPUT);
  pinMode(_selectorPins.at(1), OUTPUT);
  pinMode(_selectorPins.at(2), OUTPUT);
  return;
}

void AnalogueSensorHandler::setActiveChannel(int channelNumber) {
  digitalWrite(_selectorPins.at(0), GETBIT(channelNumber,0));
  digitalWrite(_selectorPins.at(1), GETBIT(channelNumber,1));
  digitalWrite(_selectorPins.at(2), GETBIT(channelNumber,2));
  return;
  
}

int AnalogueSensorHandler::getSensorReading(int channelNumber)
{
  int sensorValue;
  setActiveChannel(channelNumber);
  sensorValue = 1023 - analogRead(_analogInPin);
  return sensorValue;
}

#endif
