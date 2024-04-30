//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <list>
#include "SensorGroup.h"


#ifndef __WATERINGSYSTEM_IRRIGATIONSERVICE_H__
#define __WATERINGSYSTEM_IRRIGATIONSERVICE_H__
  
class IrrigationService 
{
  private:
    IrrigationLogger* _logger;
    std::list<SensorGroup*> _sensorGroups{};
    IrrigationTimer _systemStatsTimer = IrrigationTimer("systemStats");

  public:
    IrrigationService();
    ~IrrigationService();
    // Configuration methods
    void registerSensorGroup(SensorGroup *group);
    void removeSensorGroups();
    void setInstanceName(String instanceName);
    IrrigationLogger *getLogger();
    
    // Operation methods
    bool isPumping();
    void performWateringCycle();
    void loop();
}; 
/****************************************/

IrrigationService::IrrigationService() {
  _logger = new IrrigationLogger();
  return;
}

IrrigationService::~IrrigationService() {
  removeSensorGroups();
  delete _logger;
  return;
}

IrrigationLogger *IrrigationService::getLogger() {
  return _logger;
}

// Register a sensor group with the service. IrrigationService takes
// responsibilty for destruction of a registered SensorGroup object
void IrrigationService::registerSensorGroup(SensorGroup *sensorGroup) {
  _sensorGroups.push_back(sensorGroup);
}

void IrrigationService::setInstanceName(String instanceName) {
//    _logger->setJobName(instanceName);
}

void IrrigationService::removeSensorGroups() {
  for (auto & group : _sensorGroups) {
    delete group;
  }
  _sensorGroups.clear();
}


// Loop through all the pumps, return true if any are pumping
bool IrrigationService::isPumping() {
  bool isPumping = false;
  for (auto & group : _sensorGroups) {
    isPumping = isPumping || group->isPumping();
  }
  return isPumping;
}

//
// Main logic function
//
void IrrigationService::loop() {
    _logger->loop();

    // Loop throughh each group, triggering any required actions
    for (auto & group : _sensorGroups) {
        group->loop();
    }

    // Report system stats to help monitor heap and available memory
    if (_systemStatsTimer.hasLapsed()) {
        _logger->logSystemStats();
        _systemStatsTimer.setTimer(600*1000); // Report stats every 10 minutes
    }

}

#endif
