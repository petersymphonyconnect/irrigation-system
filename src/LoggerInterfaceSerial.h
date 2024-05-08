//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <ArduinoJson.h>
#include "LoggerInterface.h"

#ifndef __WATERINGSYSTEM_LOGGERINTERFACESERIAL_H__
#define __WATERINGSYSTEM_LOGGERINTERFACESERIAL_H__


//
// Concrete interface class to support logging to Serial
//
class LoggerInterfaceSerial : public LoggerInterface 
{
  private: 
    String _instanceName;

  public:
    LoggerInterfaceSerial(String instanceName);
    ~LoggerInterfaceSerial();

    virtual void logJsonMetric(String metric, JsonDocument valueJsonDoc);
    virtual void logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc);
};
/****************************************/

LoggerInterfaceSerial::LoggerInterfaceSerial(String instanceName) {
    _instanceName = instanceName;
    return;
}

LoggerInterfaceSerial::~LoggerInterfaceSerial() {
    return;
}

void LoggerInterfaceSerial::logJsonMetric(String metric, JsonDocument valueJsonDoc) {
    JsonDocument logDoc;
    logDoc["instance"]   = _instanceName;
    logDoc["metric"]   = metric;
    logDoc["value"]   = valueJsonDoc;

    String logString;
    serializeJson(logDoc,logString);

    Serial.println(logString.c_str());
}

void LoggerInterfaceSerial::logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc) {
    JsonDocument logDoc;
    logDoc["instance"]  = _instanceName;
    logDoc["group"]     = group;
    logDoc["metric"]    = metric;
    logDoc["value"]     = valueJsonDoc;

    String logString;
    serializeJson(logDoc,logString);

    Serial.println(logString.c_str());
}

#endif
