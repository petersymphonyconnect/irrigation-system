//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <list>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TZ.h>
#include "IrrigationLogger.h"
#include "IrrigationTimer.h"
#include "AnalogueSensorHandler.h"


#ifndef __WATERINGSYSTEM_SENSORGROUP_H__
#define __WATERINGSYSTEM_SENSORGROUP_H__

#define MOISTURE_CONTROLLER_TRIGGER_ANY 0
#define MOISTURE_CONTROLLER_TRIGGER_ALL 1

#define IRRIGATION_MINIMUM_WATER_LEVEL 50

/* Configuration of time */
#define MY_NTP_SERVER "pool.ntp.org"  
#define MY_TZ "GMT0BST,M3.5.0/1,M10.5.0" 
time_t sgNow;
tm sgTm;

class SensorGroup 
{
  private:
      AnalogueSensorHandler* _analogueSensorHandler;
      String _groupName;
      std::list<uint8_t> _pumpPinIds;
      std::list<uint8_t> _moistureSensorChannelNumbers;
      bool _hasWaterLevelSensor = false;
      int _waterLevelChannelNumber;
      int _triggerMode;
//      int _minThreshold;
      std::list<std::vector<int>> _wateringTimes;
      int _pumpPeriodSeconds;
      unsigned long _pumpStopTime;
      IrrigationLogger* _logger;
      std::list<int> _sensorValues;
      bool _isPumping = false;
      unsigned long _waterCheckPeriodMs;
      unsigned long _pumpCheckPeriodMs;
      unsigned long _moistureCheckPeriodMs;
      IrrigationTimer waterLevelCheckTimer = IrrigationTimer("water");
      IrrigationTimer moistureCheckTimer = IrrigationTimer("moisture");
      IrrigationTimer pumpCheckTimer = IrrigationTimer("pump");

    public:
        // Constructure with water level sensor
        SensorGroup(IrrigationLogger* logger,
                  AnalogueSensorHandler* sensorHandler,
                  String groupName,
                          int triggerMode,
                          uint8_t waterLevelChannelNumber,
                          std::list<uint8_t> moistureSensorChannelNumbers,
                          std::list<uint8_t> pumpPinIds,
                          std::list<std::vector<int>> wateringTimes,
                          int pumpPeriodSeconds,
                        unsigned long waterCheckPeriodMs,
                        unsigned long pumpCheckPeriodMs,
                        unsigned long moistureCheckPeriodMs);

        // Constructor without a water level sensor
        SensorGroup(  IrrigationLogger* logger,
                    AnalogueSensorHandler* sensorHandler,
                    String groupName,
                    int triggerMode,
                    std::list<uint8_t> moistureSensorChannelNumbers,
                    std::list<uint8_t> pumpPinIds,
                    std::list<std::vector<int>> wateringTimes,
                    int pumpPeriodSeconds,
                    unsigned long pumpCheckPeriodMs,
                    unsigned long moistureCheckPeriodMs);
                                            
        ~SensorGroup();
        void checkMoistureLevelAndWaterAndWaterIfNeeded();
        std::list<int> getSensorValues();
        bool needsWatering();
        String getGroupName();
        std::list<uint8_t> getPumpPinIds();
        int getMinSensorThreshold();
        void startPumping();
        bool isPumping();
        int getWaterLevel();
        void logWaterLevel();
        bool hasWater();
        void loop();
};
SensorGroup::SensorGroup(IrrigationLogger* logger,
    AnalogueSensorHandler* sensorHandler,
    String groupName,
    int triggerMode,
    uint8_t waterLevelChannelNumber,
    std::list<uint8_t> moistureSensorChannelNumbers,
    std::list<uint8_t> pumpPinIds,
    std::list<std::vector<int>> wateringTimes,
    int pumpPeriodSeconds,
    unsigned long waterCheckPeriodMs,
    unsigned long pumpCheckPeriodMs,
    unsigned long moistureCheckPeriodMs) {
 
    _logger = logger;
    _analogueSensorHandler = sensorHandler;
    _groupName = groupName;
    _moistureSensorChannelNumbers = moistureSensorChannelNumbers;
    _pumpPinIds = pumpPinIds;
    _wateringTimes = wateringTimes;
    _triggerMode = triggerMode;
    _pumpPeriodSeconds = pumpPeriodSeconds;
    _pumpCheckPeriodMs = pumpCheckPeriodMs;
    _moistureCheckPeriodMs = moistureCheckPeriodMs;
         
    _waterLevelChannelNumber = waterLevelChannelNumber;
    _waterCheckPeriodMs = waterCheckPeriodMs;
    _hasWaterLevelSensor = true;

    for (auto & pumpPinId : _pumpPinIds) {    
        pinMode(pumpPinId, OUTPUT);
        digitalWrite(pumpPinId, false);
    }
    configTime(MY_TZ, MY_NTP_SERVER);
    return;
}

SensorGroup::SensorGroup(IrrigationLogger* logger,
                         AnalogueSensorHandler* sensorHandler,
                         String groupName,
                         int triggerMode,
                         std::list<uint8_t> moistureSensorChannelNumbers,
                         std::list<uint8_t> pumpPinIds,
                         std::list<std::vector<int>> wateringTimes,
                         int pumpPeriodSeconds,
                         unsigned long pumpCheckPeriodMs,
                         unsigned long moistureCheckPeriodMs) {
    _logger = logger;
    _analogueSensorHandler = sensorHandler;
    _groupName = groupName;
    _moistureSensorChannelNumbers = moistureSensorChannelNumbers;
    _pumpPinIds = pumpPinIds;
    _wateringTimes = wateringTimes;
    _triggerMode = triggerMode;
    _pumpPeriodSeconds = pumpPeriodSeconds;
    _pumpCheckPeriodMs = pumpCheckPeriodMs;
    _moistureCheckPeriodMs = moistureCheckPeriodMs;
    _hasWaterLevelSensor = false;
  
    for (auto & pumpPinId : _pumpPinIds) {    
        pinMode(pumpPinId, OUTPUT);
        digitalWrite(pumpPinId, false);
    }
    configTime(MY_TZ, MY_NTP_SERVER);
    return;
}

SensorGroup::~SensorGroup() {
    // Stop pumping upon destruction
    for (auto & pumpPinId : _pumpPinIds) {
        digitalWrite(pumpPinId, false);
    }
}

String SensorGroup::getGroupName() {
    return _groupName;
}

std::list<uint8_t> SensorGroup::getPumpPinIds() {
    return _pumpPinIds;
}

int SensorGroup::getMinSensorThreshold() {
    // Get the hour of day
    time(&sgNow);
    localtime_r(&sgNow, &sgTm);
    int currentHour = sgTm.tm_hour;

    // Search wateringTimes for a matching hour entry
    for (auto & wateringTime: _wateringTimes) {
        if (currentHour >= wateringTime[0] && // startHour
            currentHour <= wateringTime[1]) { // endHour
            return wateringTime[2];
        }
    }
    
    // No valid watering time
    return 0;
}

// If we're not already pumping, start the pump
void SensorGroup::startPumping() {
    if (!_isPumping) {
        _pumpStopTime = millis() + _pumpPeriodSeconds * 1000;
        for (auto & pumpPinId : _pumpPinIds) {
            Serial.println("  *** Starting pumping on : " + String(pumpPinId) + ", " + String(millis()) + "->" + String(_pumpStopTime));
            _logger->logPumpStatus(_groupName, true); // TODO This should probably move out of the loop
            digitalWrite(pumpPinId, true);
        }
        _isPumping = true;
    }
    return;
}

// Returns the pumping status, stopping the pump
// if we've exceeded the pump time.
bool SensorGroup::isPumping() {
  if (_isPumping) {
      // Have we run out of water or pumped long enough?
      if (!hasWater() || (_pumpStopTime < millis())) {
          for (auto & pumpPinId : _pumpPinIds) {
              Serial.println("  ** Stopping pumping on : " + String(pumpPinId) + ", " + String(millis()));
              digitalWrite(pumpPinId, false);
          }
          _logger->logPumpStatus(_groupName, false);
          _isPumping = false;
      }
  }
  return _isPumping;
}

std::list<int> SensorGroup::getSensorValues() {
    _sensorValues = std::list<int>{};
    int sensorValue;
    // Loop through the sensors in the group, logging the values, and storing
    for (auto const& channelNumber : _moistureSensorChannelNumbers) {
        sensorValue = _analogueSensorHandler->getSensorSimpleMovingAverageReading(channelNumber);
        _logger->logMoistureLevel(_groupName, channelNumber, sensorValue, getMinSensorThreshold());
        _sensorValues.push_back(sensorValue);
    }

    return _sensorValues;
}

bool SensorGroup::needsWatering() {
    int sensorsTriggeredCount = 0;
    _sensorValues = getSensorValues();

    if (_sensorValues.size() == 0) {
        return false;
    }
    // Loop through the sensors, and if we're "ANY" mode, and
    // any sensors are below threshold, return true. Maintain
    // a count of total breached sensors.
    for (auto const& sensorValue : _sensorValues) {
        if (sensorValue < getMinSensorThreshold()) {
            if (_triggerMode == MOISTURE_CONTROLLER_TRIGGER_ANY) {
                return true;
            } else {
                sensorsTriggeredCount++;
            }
        }
    }

    //
    // We reach here if we're in "ALL" sensors mode. If all sensors
    // are below threshold, return true, otherwise return false.
    //
    if (sensorsTriggeredCount >= (int)_sensorValues.size()) {
        return true;
    } else {
        return false;
    }
}

void SensorGroup::logWaterLevel() {
    if (_hasWaterLevelSensor) {
        int waterLevel = _analogueSensorHandler->getAbsoluteSensorReading(_waterLevelChannelNumber);
        _logger->logWaterLevel(_groupName, waterLevel);
    }
}

int SensorGroup::getWaterLevel() {
    int waterLevel = _analogueSensorHandler->getAbsoluteSensorReading(_waterLevelChannelNumber);
    return waterLevel;
}

bool SensorGroup::hasWater() {
    if (_hasWaterLevelSensor) {
        return getWaterLevel() > IRRIGATION_MINIMUM_WATER_LEVEL;
    } else {
        return true;
    }
}

void SensorGroup::checkMoistureLevelAndWaterAndWaterIfNeeded() {
    bool needsWateringResult = needsWatering();
    _logger->logMoistureAlarmStatus(_groupName, needsWateringResult);
          
    // If it needs watering, and isn't already pumping, and we have water, start pumping
    if (needsWateringResult && !isPumping() && hasWater()) {
        startPumping();
    }
}

//
// Main SensorGroup control logic, called from IrrigationSystem control cycle loop.
// When respective timers have lapsed, check:
// - Moisture levels, starting watering cycle if required
// - Water level, reporting this to the logger
// - Pumping status, stopping the pump(s) if we've pumped long enough or
//   run out of water.
//
void SensorGroup::loop() {
    if (moistureCheckTimer.hasLapsed()) {
        if (!isPumping()) {
          checkMoistureLevelAndWaterAndWaterIfNeeded();
        }
        moistureCheckTimer.setTimer(_moistureCheckPeriodMs);
    }
    if (_hasWaterLevelSensor && waterLevelCheckTimer.hasLapsed()) {
        logWaterLevel();
        waterLevelCheckTimer.setTimer(_waterCheckPeriodMs);
    }
    if (pumpCheckTimer.hasLapsed()) {
        bool isPumpingResult = isPumping();
        if (isPumpingResult) {
            _logger->logPumpStatus(_groupName, true);
        }

        pumpCheckTimer.setTimer(_pumpCheckPeriodMs);
    }

    return;
}
#endif
