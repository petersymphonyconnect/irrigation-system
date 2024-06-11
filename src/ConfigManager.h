//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <ESP8266WebServer.h>
#include <uri/UriRegex.h>
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "SensorGroup.h"
#include "IrrigationTimer.h"
#include "IrrigationService.h"
#include "LoggerInterface.h"
#include "LoggerInterfaceMqtt.h"
#include "LoggerInterfaceLoki.h"
#include "LoggerInterfaceSerial.h"

#ifndef __WATERINGSYSTEM_CONFIGMANAGER_H__
#define __WATERINGSYSTEM_CONFIGMANAGER_H__

#define LOKI_DEFAULT_PORT 3100
#define MQTT_DEFAULT_PORT 1883
#define LOKI_PATH "/loki/api/v1/push"

#define CHECK_FOUND(obj, key, friendly) {if (!obj.containsKey(key)) {return String("Failed to find field ") + String(friendly) + String(" in config");}}
#define CHECK_ARRAY_FOUND(array, friendly) {if (!array) {return String("Failed to find field ") + String(friendly) + String(" in config");}}

//
// Provides a web service to get/post Json configuration, stored in
// persistent LittleFS storage, and updates applied to the running IrrigationService
//
class ConfigManager
{
    private:
        const char* irrigationConfigFile = "/irrigationconfig.json";
        const char* defaultJsonStr = "{\"instance\": \"MyIrrigationServer\", \"loggers\": [{\"type\": \"serial\"}]}";
        ESP8266WebServer* _configServer;
        IrrigationService* _irrigationService; // The service we'll configure, set in constructor
        AnalogueSensorHandler* _analogueSensorHandler;

    public:
        ConfigManager(ESP8266WebServer *server, IrrigationService *irrigationService, AnalogueSensorHandler* analogueSensorHandler);
        void handleClient();
        void handleGet();
        void handlePost();
        void handleDelete();
        void handleSensorGroupTrigger();
        void loadConfiguration();
        void writeDefaultConfiguration();
        String processJsonConfig(JsonDocument configDoc, bool applyConfig);
}; 


ConfigManager::ConfigManager(ESP8266WebServer *server,
                             IrrigationService *irrigationService,
                             AnalogueSensorHandler* analogueSensorHandler) {
    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
    }

    _irrigationService = irrigationService;
    _analogueSensorHandler = analogueSensorHandler;
    _configServer = server;
    _configServer->on("/config",HTTP_GET,[this]() {
        this->handleGet();
    });
    _configServer->on("/config",HTTP_POST,[this]() {
        this->handlePost();
    });
    _configServer->on("/config",HTTP_DELETE,[this]() {
        this->handleDelete();
    });
    _configServer->on(UriRegex("/sensorgroup/(.+)/pump"),HTTP_POST,[this]() {
        this->handleSensorGroupTrigger();
    });
    _configServer->begin();
}

//
// Loads the configuration stored on LitteFS storage, configuring the IrrigationService.
// If no configuration exists, it creates a default without any sensor groups setup.
//
void ConfigManager::loadConfiguration() {
    JsonDocument jsonData;
    File file = LittleFS.open(irrigationConfigFile,"r");

    if (!file){
        Serial.println("Failed to open config file for reading");
        writeDefaultConfiguration();
        file = LittleFS.open(irrigationConfigFile,"r");
    }
    // Check we have a file handle.
    if (!file) {
        Serial.println("Failed to read or prepare default configuration file");
        return;
    }
    String configContents("");
    while (file.available()) {
        configContents.concat(file.readString());
    }
    DeserializationError error = deserializeJson(jsonData, configContents);
    if (error) {
        Serial.print("deserializeJson() failed during configuration load: ");
        Serial.println(error.f_str());
    } else {
        processJsonConfig(jsonData, true);
    }
}

//
// Writes a default configuration to LittleFS storage
//
void ConfigManager::writeDefaultConfiguration() {
    File file = LittleFS.open(irrigationConfigFile,"w");
    if (!file) {
        Serial.println("Failed to open config file for writing");
    } else {
        if (!file.write(defaultJsonStr,strlen(defaultJsonStr))) {
            Serial.println("Failed to write default configuration file");
            return;         
        }
        file.close();
    }
    return;
}

void ConfigManager::handleClient() {
    _configServer->handleClient();
}

//
// Callback handler for retrieval of the configuration via the web service
//
void ConfigManager::handleGet() {
    File file = LittleFS.open(irrigationConfigFile,"r");

    if (!file || file.isDirectory()){
//        Serial.println("Failed to open config file for reading");
        _configServer->send(500,"application/json","Failed to open config file for reading");
    } else {
        String configContents("");
        while (file.available()) {
            configContents.concat(file.readString());
        }
        _configServer->send(200,"application/json",configContents.c_str());
    }
}

//
// Callback handler for receiving new Json configuration via a web server POST message.
// Validates, persists to LittleFS, and then reconfigures the running IrrigationService.
//
void ConfigManager::handlePost() {
    JsonDocument jsonData;
    // Deserialize the JSON document
    String jsonString = _configServer->arg("plain");
    DeserializationError error = deserializeJson(jsonData, jsonString);

    // Test if parsing succeeds.
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        _configServer->send(500,"application/json",String("Error parsing JSON configuration: ") + String(jsonString));
    } else {
        // Try and parse the config without applying it
        String error = processJsonConfig(jsonData,false);
        if (!error.isEmpty()) {
            _configServer->send(500,"application/json","Invalid configuration JSON document: " + error);
        } else {
            // Parse successful, so write to persistent storage
            File file = LittleFS.open(irrigationConfigFile,"w");
            if (!file) {
                Serial.println("Failed to open config file for writing");
                _configServer->send(500,"application/json","Failed to open config file for writing");
            } else {
                if (file.write(jsonString.c_str(),jsonString.length())) {
                    _configServer->send(200,"application/json","Config file writen");
                } else {
                    _configServer->send(500,"application/json","Config file write failed");            
                }
                file.close();
            }
            // Now apply the config to the running application
            String error = processJsonConfig(jsonData, true);
        }
    }
    return;
}

void ConfigManager::handleDelete() {
    if (LittleFS.remove(irrigationConfigFile)) {
        _configServer->send(200,"application/json","Config file removed. Default configuration now used.");
    } else {
        _configServer->send(500,"application/json","Failed to delete configuration file");   
    }
    loadConfiguration();
} 

//
// Code to parse out configuration from a Json document.
// Optionally writes this configuration to runtime service.
//
// TODO: This function is becoming far too large. This should be split into smaller handler
// functions, possibly introducing a ConfigError class containing parse error info and an
// error code.
//
String ConfigManager::processJsonConfig(JsonDocument configDoc, bool applyConfig) {
    JsonString instanceName = configDoc["instance"];
    JsonArray groupsJson = configDoc["groups"];
    JsonArray loggersJson = configDoc["loggers"];
    CHECK_FOUND(configDoc,"instance","instance");
//    CHECK_ARRAY_FOUND(groupsJson,"groups");
    if (applyConfig) {
//        _irrigationService->setInstanceName(String(instanceName.c_str()));
        _irrigationService->removeSensorGroups();
    }

    if (applyConfig) {
        _irrigationService->getLogger()->removeLoggerInterfaces();
    }
    if (configDoc.containsKey("loggers")) {
        for (JsonVariant loggerJson : loggersJson) {
            CHECK_FOUND(loggerJson,"type","loggers.type");
            String typeStr = loggerJson["type"].as<String>();
            if (typeStr.equals("loki")) {
                // Process loki config
                int lokiPort = LOKI_DEFAULT_PORT;
                if (loggerJson.containsKey("port")) {
                    lokiPort = loggerJson["port"];
                }
                CHECK_FOUND(loggerJson,"server","loggers.server");
                String lokiServer = loggerJson["server"];
                if (applyConfig) {
//                    _irrigationService->getLogger()->setLokiConfig(lokiPort, lokiServer, LOKI_PATH);
                    LoggerInterface* interface = new LoggerInterfaceLoki(instanceName.c_str(),
                                                                         lokiPort,
                                                                         lokiServer.c_str(),
                                                                         LOKI_PATH);
                    _irrigationService->getLogger()->addLoggerInterface(interface);
                }
            } else if (typeStr.equals("mqtt")) {
                // Process mqtt config
                int mqttPort = MQTT_DEFAULT_PORT;
                if (loggerJson.containsKey("port")) {
                    mqttPort = loggerJson["port"];
                }
                CHECK_FOUND(loggerJson,"server","loggers.server");
                CHECK_FOUND(loggerJson,"topicPrefix","loggers.topicPrefix");
                String mqttServer = loggerJson["server"];
                String topicPrefix = loggerJson["topicPrefix"];
                if (applyConfig) {
                    LoggerInterface* interface = new LoggerInterfaceMqtt(instanceName.c_str(),
                                                                         mqttServer.c_str(),
                                                                         mqttPort,
                                                                         topicPrefix);
                    _irrigationService->getLogger()->addLoggerInterface(interface);
                }
            } else if (typeStr.equals("serial")) {
                if (applyConfig) {
                    LoggerInterface* interface = new LoggerInterfaceSerial(instanceName.c_str());
                    _irrigationService->getLogger()->addLoggerInterface(interface);
                }
            } else {
                String error("Invalid logger type ");
                error += typeStr;
                return error;
            }
        }
    }

    if (configDoc.containsKey("groups")) {
        for (JsonVariant groupJson : groupsJson) {
            CHECK_FOUND(groupJson,"name","groups.name");
            CHECK_FOUND(groupJson,"triggerType","groups.triggerType");
            CHECK_FOUND(groupJson,"waterSensorChannel","groups.waterSensorChannel");
            CHECK_FOUND(groupJson,"moistureSensorChannels","groups.waterSensomoistureSensorChannels");
            CHECK_FOUND(groupJson,"pumpPinIds","groups.pumpPinIds");
            CHECK_FOUND(groupJson,"minMoisture","groups.minMoisture");
            CHECK_FOUND(groupJson,"pumpSecs","groups.pumpSecs");
            CHECK_FOUND(groupJson,"waterCheckPeriodMs","groups.waterCheckPeriodMs");
            CHECK_FOUND(groupJson,"pumpCheckPeriodMs","groups.pumpCheckPeriodMs");
            CHECK_FOUND(groupJson,"moistureCheckPeriodMs","groups.moistureCheckPeriodMs");
            String name = groupJson["name"].as<String>();
            String typeStr = groupJson["triggerType"].as<String>();
            int type;
            if      (typeStr.equals("any")) {type = MOISTURE_CONTROLLER_TRIGGER_ANY;}
            else if (typeStr.equals("all")) {type = MOISTURE_CONTROLLER_TRIGGER_ALL;}
            else {
                return String("Invalid trigger type ") + typeStr;
            }
            
            uint8_t waterSensorChannel = groupJson["waterSensorChannel"].as<uint8_t>();
            if (waterSensorChannel < 0 || waterSensorChannel > 7) {
                return String("Invalid water sensor channel identifier ") + String(waterSensorChannel);
            }
            int minMoisture = groupJson["minMoisture"].as<int>();
            int pumpSecs = groupJson["pumpSecs"].as<int>();
            unsigned long waterCheckPeriodMs = groupJson["waterCheckPeriodMs"].as<unsigned long>();
            unsigned long pumpCheckPeriodMs = groupJson["pumpCheckPeriodMs"].as<unsigned long>();
            unsigned long moistureCheckPeriodMs = groupJson["moistureCheckPeriodMs"].as<unsigned long>();
            
            JsonArray pumpPinIdsArray = groupJson["pumpPinIds"];
            std::list<uint8_t> pumpPinIds;
            for (JsonVariant v : pumpPinIdsArray) {
                String pin = v.as<String>();
                if      (pin.equals("D0")) {pumpPinIds.push_back(D0);}
                else if (pin.equals("D1")) {pumpPinIds.push_back(D1);}
                else if (pin.equals("D2")) {pumpPinIds.push_back(D2);}
                else if (pin.equals("D3")) {pumpPinIds.push_back(D3);}
                else if (pin.equals("D4")) {pumpPinIds.push_back(D4);}
                else {
                    return String("Invalid pump pin identifier ") + v.as<String>();
                }
            }

            JsonArray moistureSensorChannelsArray = groupJson["moistureSensorChannels"];
            std::list<uint8_t> moistureSensorChannels;
            for (JsonVariant v : moistureSensorChannelsArray) {
                uint8_t channel = v.as<uint8_t>();
                if (channel < 0 || channel > 7) {
                    return String("Invalid moisture sensor channel identifier ") + v.as<String>();
                } else {
                    moistureSensorChannels.push_back(channel);
                }
            }
            if (applyConfig) {
                SensorGroup* group = new SensorGroup(_irrigationService->getLogger(),
                                                    _analogueSensorHandler,
                                                    name,
                                                    type,
                                                    waterSensorChannel,
                                                    moistureSensorChannels,
                                                    pumpPinIds,
                                                    minMoisture,
                                                    pumpSecs,
                                                    waterCheckPeriodMs,
                                                    pumpCheckPeriodMs,
                                                    moistureCheckPeriodMs
                                                    );
                _irrigationService->registerSensorGroup(group);
            }
        }
    }
    return "";
}

void ConfigManager::handleSensorGroupTrigger() {
  SensorGroup* sensorGroup = _irrigationService->getSensorGroupByName(_configServer->pathArg(0));
  if (sensorGroup) {
    if (sensorGroup->hasWater()) {
      _configServer->send(200, "text/plain", "Sensor group " + _configServer->pathArg(0) + " pumping triggered");
      sensorGroup->startPumping();
    } else {
      _configServer->send(503, "text/plain", "Sensor group " + _configServer->pathArg(0) + " has no water");
    }  
  } else {
    _configServer->send(404, "text/plain", "Sensor group " + _configServer->pathArg(0) + " not found");    
  }
}

#endif