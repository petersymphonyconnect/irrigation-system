//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "LoggerInterface.h"

#ifndef __WATERINGSYSTEM_LOGGERINTERFACELOKI_H__
#define __WATERINGSYSTEM_LOGGERINTERFACELOKI_H__

// NTP client to get Epoch
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//
// Concrete interface class to support logging to Loki
//
class LoggerInterfaceLoki : public LoggerInterface 
{
  private: 
    String _job;
    int    _lokiPort;
    String _lokiServer;
    String _lokiPath;
    time_t getEpoch();

  public:
    LoggerInterfaceLoki(String instanceName, int lokiPort, String lokiServer, String lokiPath);
    ~LoggerInterfaceLoki();

    virtual void logJsonMetric(String metric, JsonDocument valueJsonDoc);
    virtual void logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc);
    void logString(JsonDocument streamDoc, String value);
};
/****************************************/

LoggerInterfaceLoki::LoggerInterfaceLoki(String instanceName, int lokiPort, String lokiServer, String lokiPath) {
  _job = instanceName;
  _lokiPort = lokiPort;
  _lokiServer  = lokiServer;
  _lokiPath = lokiPath;
  timeClient.begin();
  return;
}

LoggerInterfaceLoki::~LoggerInterfaceLoki() {
    return;
}

time_t LoggerInterfaceLoki::getEpoch() {
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  return epochTime;
}

// Function to log a Json structure
void LoggerInterfaceLoki::logJsonMetric(String metric, JsonDocument valueJsonDoc) {
  JsonDocument streamDoc;
  streamDoc["stream"]["job"]   = _job;
  streamDoc["stream"]["metric"]   = metric;
  // Build stream value
  String valueString;
  serializeJson(valueJsonDoc,valueString);

  logString(streamDoc, valueString);
}

void LoggerInterfaceLoki::logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc) {
  JsonDocument streamDoc;
  streamDoc["stream"]["job"]     = _job;
  streamDoc["stream"]["metric"]  = metric;
  streamDoc["stream"]["group"]   = group;
  // Build stream value
  String valueString;
  serializeJson(valueJsonDoc,valueString);

  logString(streamDoc, valueString);
}

//
// General use function to log an arbirary string
//
void LoggerInterfaceLoki::logString(JsonDocument streamDoc, String value) {
    streamDoc["stream"]["job"]   = _job;
    JsonDocument valuesDoc;
    JsonDocument valuesContainerDoc;
    JsonArray valuesContainerArray = valuesContainerDoc.to<JsonArray>();
    JsonArray valuesArray = valuesDoc.to<JsonArray>();
    // create an object
    
    valuesArray.add(String(getEpoch())+"000000000");
    valuesArray.add(value);  // Sensor value

    valuesContainerArray.add(valuesArray);
    streamDoc["values"] = valuesContainerArray;

    // Build the overall Json document
    JsonDocument doc;
    JsonDocument streamsListDoc;
    JsonArray streamsListArray = streamsListDoc.to<JsonArray>();
    streamsListArray.add(streamDoc);
    doc["streams"] = streamsListArray;
    String json;
    serializeJson(doc, json);
    
    // Send HTTP POST request to Loki
    WiFiClient client;
    HTTPClient http;
    String serverPath = "http://" + _lokiServer + ":" + _lokiPort + _lokiPath;
    http.begin(client, serverPath);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(json);
    if (httpResponseCode < 200 || httpResponseCode > 299) {
        Serial.printf("Payload: %s",json.c_str());
        Serial.printf("Loki returned unexpected return code %d (%s)",httpResponseCode,http.getString().c_str());
    }
    http.end();
}

#endif
