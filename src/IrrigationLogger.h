//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include "LoggerInterface.h"

#ifndef __WATERINGSYSTEM_IRRIGATIONLOGGER_H__
#define __WATERINGSYSTEM_IRRIGATIONLOGGER_H__

const char buildDate[] = __DATE__ " " __TIME__;

class IrrigationLogger
{
  private: 
      std::list<LoggerInterface*> _interfaces{};

  public:
      IrrigationLogger();
      IrrigationLogger(int lokiPort, String lokiServer, String lokiPath);
      ~IrrigationLogger();

      // Config methods
      void addLoggerInterface(LoggerInterface* interface);
      void removeLoggerInterfaces();
      void loop();

      // Context specific log methods
      void logStartup(IPAddress ipAddress);
      void logSystemStats();
      void logConfigLoad();
      void logPumpStatus(String group, bool status);
      void logMoistureLevel(String group, int channelNumber, int level, int minLevel);
      void logWaterLevel(String group, int value);
      void logMoistureAlarmStatus(String group, bool status);
};
/****************************************/

IrrigationLogger::IrrigationLogger() {
    return;
}

IrrigationLogger::~IrrigationLogger() {
    removeLoggerInterfaces();
}

void IrrigationLogger::removeLoggerInterfaces() {
    for (auto & interface : _interfaces) {
        delete interface;
    }
    _interfaces.clear();
}

void IrrigationLogger::addLoggerInterface(LoggerInterface* interface) {
    _interfaces.push_back(interface);
}

void IrrigationLogger::logStartup(IPAddress ipAddress) {
      // Build value
      JsonDocument valuesDoc;
      String valueString;
      JsonDocument valueDoc = JsonObject();
      valueDoc["ipAddress"] = ipAddress.toString();
      valueDoc["buildDate"] = buildDate;
    //  serializeJson(valueDoc,valueString);

      for (auto & interface : _interfaces) {
          interface->logJsonMetric("boot",valueDoc);
      }
  //  logString(streamDoc, valueString);
  }

  void IrrigationLogger::logSystemStats() {
    // Build stream value
    // JsonDocument valuesDoc;
    // String valueString;
    JsonDocument valueDoc = JsonObject();
    valueDoc["getHeapFragmentation"] = ESP.getHeapFragmentation();
    valueDoc["getFreeHeap"] = ESP.getFreeHeap();
    valueDoc["getFreeSketchSpace"] = ESP.getFreeSketchSpace();
    valueDoc["getMaxFreeBlockSize"] = ESP.getMaxFreeBlockSize();

    for (auto & interface : _interfaces) {
        interface->logJsonMetric("system-stats",valueDoc);
    }
}

void IrrigationLogger::logConfigLoad() {
    JsonDocument valuesDoc;

    for (auto & interface : _interfaces) {
        interface->logJsonMetric("config-load",valuesDoc);
    }
}

void IrrigationLogger::logPumpStatus(String group, bool status) {
    JsonDocument valueDoc = JsonObject();
    valueDoc["status"] = status;

    for (auto & interface : _interfaces) {
        interface->logJsonGroupMetric("pump-status",group,valueDoc);
    }
}

void IrrigationLogger::logMoistureLevel(String group, int channelNumber, int level, int minLevel) {
    JsonDocument valueDoc = JsonObject();
    valueDoc["channel"] = channelNumber;
    valueDoc["level"] = level;
    valueDoc["minLevel"] = minLevel;

    for (auto & interface : _interfaces) {
        interface->logJsonGroupMetric("moisture",group,valueDoc);
    }
}

void IrrigationLogger::logWaterLevel(String group, int value) {
    JsonDocument valueDoc = JsonObject();
    valueDoc["level"] = value;

    for (auto & interface : _interfaces) {
        interface->logJsonGroupMetric("water",group,valueDoc);
    }
}

void IrrigationLogger::logMoistureAlarmStatus(String group, bool status) {

    JsonDocument valueDoc = JsonObject();
    valueDoc["status"] = status;

    for (auto & interface : _interfaces) {
        interface->logJsonGroupMetric("moisture-alarm-status",group,valueDoc);
    }
}

void IrrigationLogger::loop() {
    for (auto & interface : _interfaces) {
        interface->loop();
    }
}

#endif