//
// Distributed under MIT license. See https://raw.githubusercontent.com/petersymphonyconnect/irrigation-system/main/LICENSE
//

#include <Arduino.h>
#include <fauxmoESP.h>
#include <ESP8266WiFi.h>
#include "IrrigationService.h"
#include "IrrigationLogger.h"
#include "LoggerInterface.h"
#include "AnalogueSensorHandler.h"
#include "ConfigManager.h"
#include <list>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ElegantOTA.h>

fauxmoESP fauxmo;
WiFiManager wifiManager;

// Irrigation service setup
std::array<int,3> analogueSelectorPinIds = {D5,D6,D7};
AnalogueSensorHandler analogueSensorHandler(analogueSelectorPinIds);
ESP8266WebServer server(80);
IrrigationService irrigationService = IrrigationService();
ConfigManager configManager(&server, &irrigationService, &analogueSensorHandler);

#define SERIAL_BAUD_RATE    115200

// Network connection status LED
int NETWORK_STATUS_LED = D8; 
bool serviceActive = false;

/*---------------------------------------*/
//Runs once, when device is powered on or code has just been flashed 
void setup()
{
    // Start with all digital pins off
    std::list<int> allPinIds{D0,D1,D2,D3,D4,D5,D6,D7,D8};
    for (auto & pinId : allPinIds) {    
      pinMode(pinId, OUTPUT);
      digitalWrite(pinId, false);
    }
    
    Serial.begin(SERIAL_BAUD_RATE);
  
    wifiManager.autoConnect();

    // fauxmo.addDevice("irigation system");
    // fauxmo.setPort(80); // required for gen3 devices
    // fauxmo.enable(true);   
    // fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    //   Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    //   digitalWrite(NETWORK_STATUS_LED, state); // turn the LED off
    //   serviceActive = state;   
    // });
    ElegantOTA.begin(&server);
    ElegantOTA.setAutoReboot(true);

    configManager.loadConfiguration();
    Serial.println(WiFi.localIP().toString());
    irrigationService.getLogger()->logStartup(WiFi.localIP());
}

/*---------------------------------------*/
//Runs constantly 
void loop()
{
//    fauxmo.handle();
    configManager.handleClient();
    irrigationService.loop();
    ElegantOTA.loop();
}
