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
    unsigned long _stopTimeMillis = 0;
    String _name;
  public:
    IrrigationTimer(String name);
    void setTimer(unsigned long timeInFutureMs);
    bool hasLapsed();

}; 
/****************************************/

IrrigationTimer::IrrigationTimer(String name) {
    _name = name;
    return;
}

void IrrigationTimer::setTimer(unsigned long timeInFutureMs) {
    _stopTimeMillis = millis() + timeInFutureMs;
}

bool IrrigationTimer::hasLapsed() {
    if (_stopTimeMillis == 0) {
        return true;
    } else if (millis() >= _stopTimeMillis) {
        _stopTimeMillis = 0;
        return true;
    } else {
        return false;
    }
}

#endif
