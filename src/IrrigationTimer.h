//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

#ifndef __WATERINGSYSTEM_IRRIGATIONTIMER_H__
#define __WATERINGSYSTEM_IRRIGATIONTIMER_H__
    
class IrrigationTimer 
{
  private: 
    unsigned long  getMillis();
    unsigned long _stopTimeMillis = 0;
    String _name;
  public:
    IrrigationTimer(String name);
    void setTimer(unsigned long timeInFuture);
    bool hasLapsed();

}; 
/****************************************/

IrrigationTimer::IrrigationTimer(String name) {
  _name = name;
  return;
}

void IrrigationTimer::setTimer(unsigned long timeInFuture) {
  _stopTimeMillis = getMillis() + timeInFuture;
}

unsigned long IrrigationTimer::getMillis() {
  return millis();
}

bool IrrigationTimer::hasLapsed() {
//  Serial.println("Timer check for: " + _name + ", " + String(_stopTimeMillis));
  if (_stopTimeMillis == 0) {
    return true;
  } else if (this->getMillis() >= _stopTimeMillis) {
//    Serial.println("      * Timer ceased: " + _name);
    _stopTimeMillis = 0;
    return true;
  } else {
    return false;
  }
}

#endif
