//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include "LoggerInterface.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#ifndef __WATERINGSYSTEM_LOGGERINTEFACEMQTT_H__
#define __WATERINGSYSTEM_LOGGERINTEFACEMQTT_H__

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Unused, as we won't be receiving MQTT messages
}

//
// Concrete interface class to support logging to Mqtt
//
class LoggerInterfaceMqtt : public LoggerInterface 
{
  private: 
      WiFiClient _wifiClient;
      PubSubClient*  _mqttClient;
      String        _topicPrefix;
      String        _instanceName;

      bool isConnected();
  public:
      virtual void logJsonMetric(String metric, JsonDocument valueJsonDoc);
      virtual void logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc);
      virtual void loop();

      LoggerInterfaceMqtt(String instanceName, const char* server, int port, String topicPrefix);
      ~LoggerInterfaceMqtt();
};

LoggerInterfaceMqtt::LoggerInterfaceMqtt(String instanceName, const char* server, int port, String topicPrefix) {
    _instanceName = instanceName;
    _mqttClient = new PubSubClient(_wifiClient);
    _mqttClient->setServer(server, port);
    
    if (!isConnected()) {
        Serial.println("Failed to connect toMQTT broker in constructor");
    }
    _topicPrefix = topicPrefix;

}

LoggerInterfaceMqtt::~LoggerInterfaceMqtt() {
    delete _mqttClient;
}


void LoggerInterfaceMqtt::logJsonMetric(String metric, JsonDocument valueJsonDoc) {
    String topic = _topicPrefix;
    topic += metric;
    String valueString;
    serializeJson(valueJsonDoc,valueString);

    if (isConnected()) {
        if (!_mqttClient->publish(topic.c_str(), valueString.c_str())) {
            Serial.println("Publish failed");
        }
    } else {
        Serial.println("MQTT not currently connected");
    }
}

void LoggerInterfaceMqtt::logJsonGroupMetric(String metric, String group, JsonDocument valueJsonDoc) {
    String topic = _topicPrefix;
    topic += group;
    topic += "/";
    topic += metric;
    String valueString;
    serializeJson(valueJsonDoc,valueString);

    if (isConnected()) {
        if (!_mqttClient->publish(topic.c_str(), valueString.c_str())) {
            Serial.println("Publish failed");
        }
    } else {
        Serial.println("MQTT not currently connected");
    }
}

bool LoggerInterfaceMqtt::isConnected() {
    // If we're not currently connected, try to reconnect
    if (!_mqttClient->connected()) {
        if (_mqttClient->connect((char*) _instanceName.c_str())) {
            Serial.println("Reconnected to MQTT broker");
        } else {
            Serial.println("MQTT reconnect failed");
        }
    }
    return _mqttClient->connected();
}

void LoggerInterfaceMqtt::loop() {
    _mqttClient->loop();
}

#endif
