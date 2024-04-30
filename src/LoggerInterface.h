//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef __WATERINGSYSTEM_LOGGERINTERFACE_H__
#define __WATERINGSYSTEM_LOGGERINTERFACE_H__

//
// Pure virtual base class representing an interface to an external logging capability.
// The ConfigManager is responsible for creating derived versions of this class (Loki/Mqtt
// currently supported), and adding them to the IrrigationLogger to use.
//
class LoggerInterface
{
    private: 

    public:
        virtual void logJsonMetric(String metric, JsonDocument valueJsonDoc) = 0;
        virtual void logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc) = 0;
        virtual void loop();
        virtual ~LoggerInterface() {};
};
/****************************************/

void LoggerInterface::loop() {
  return;
}

#endif
