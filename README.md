
# Overview
This project is an automated irrigation system based around an ESP8266 microprocessor, built as a PlatformIO project running in Visual Studio Code. This code is released under the MIT license.

The design has the following capabilities:
* Up to eight analogue sensors, which can be any combination of water level sensors and moisture sensors
* Up to five pumps, this project uses submersible pumps
* Reconfigurable over the network via JSON POST
* Configurable "SensorGroups" which combine one or more moisture sensors, a water level sensor, and one or more pumps
* Configurable timers to check and report water levels and moisture levels, per sensor group
* Configurable pump times for each Sensor Group
* Persistent configuration, stored in LitteFS filesystem
* Integration with Loki and/or MQTT for logging. Visualisations thus possible with Graphana
* Use of WifiManager to support easy setup and safe configuration of Wifi access (this keeps wifi SIDs/passwords separate fron code)

# Hardware summary:
A [circuit schematic](docs/IrrigationSystem.pdf) can be found in the docs folder.
* For simplicity, we use a NodeMCU development board that provides the associated circuitry for 5v->3.3 volt step down power regulator for the ESP8266, and associated serial port circuitry for code flashing/monitoring.
* It uses an 74HC4051 multiplexer, switched using 3 digital out pins, to enable monitoring of 8 sensors using the single analogue input of the ESP8266.
* Separate 3v optocoupler relay boards are used to switch on/off the pumps (see link below). Other boards could be used, but careful attention to coil operation voltage, and switching current draw off the ESP8266 digital out pin must be considered.
* A LM317 is configured to provide 3.3v >1Amp current to drive the relay coils. Switching of these boards is performed directly off the ESP8266 pin out, with a measured draw of ~5ma (other relay boards will behave differently)
* Pumps are driven directly off the 5v power supply, with protection from flyback currents using a Schottkey diode. Consideration must be given to the possibility of all configured pumps running at the same time, and overall power supply current requirement.
* Current draw when not pumping is in the 100-120ma range. Power could be slightly optimised by only powering the sensors/multiplexer when reading values (power being switched by a digital out pin, at the loss of a pump output)

# Key Hardware Components
* ESP8266 NodeMCU v2 module ([Amazon link](https://www.amazon.co.uk/AZDelivery-NodeMCU-Development-Unsoldered-including/dp/B07V4M3SGT/))
* 3v Relay Switch with Octocoupler ([Amazon link](https://www.amazon.co.uk/dp/B09LS7S1H7?p))
* 74HC4051 8-Channel-Mux Analog Multiplexer Selector Module ([Amazon link](https://www.amazon.co.uk/dp/B09Z29W8XV))
* LM317 voltage regulator and associated resistors/capacitors
* Aideepen 3PCS Water Level Sensor Liquid Level ([Amazon link](https://www.amazon.co.uk/dp/B08D63B9PY?))
* 5v submersible water pump ([Amazon link](https://www.amazon.co.uk/dp/B0B24G8WP7))
* Water pipe, 6mm ([Amazon link](https://www.amazon.co.uk/dp/B08HT2C2MC?))
* Water pipe connectors ([Amazon link](https://www.amazon.co.uk/dp/B097K22Z5G?psc=1&ref=ppx_yo2ov_dt_b_product_details))
* Water moisture sensors ([Amazon link](https://www.amazon.co.uk/dp/B09V7HFHX3?))
* 5V 3A power supply ([Amazon link](https://www.amazon.co.uk/dp/B0BQBR4RZM?))
 
# Notes on building
* PlatformIO appears fussier than the Arduino IDE compiler, and errors on file b64.cpp in the HttpClient library with a function not providing a return value for a non-void function. See [this note](https://forum.arduino.cc/t/httpclient-library-example-with-nodemcu/1042659/10) on how to resolve this issue, which is essentially a small edit to provide a return value.

# Flashing
* The project is setup to flash using ElegantOTA. The first tine you flash the device, comment out the following two lines in the platformio.ini file to force it to flash via serial port
```
;extra_scripts = platformio_upload.py
;upload_protocol = custom
```

# Configuration
1. Once flashed, the first boot will result in the WifiManager library making the ESP available as a Wifi access point to facilitate Wifi configuration. Join the ESP prefixed SSID, and follow the instructions to join your home Wifi
2. On first boot, a default irrigation setup will be configured. You can retrieve this configuration using the following:
```
% curl http://<myESPipaddress>/config
{"instance": "MyIrrigationServer"}
```
If you can't work out what IP address DHCP has assigned, this will be printed to the ESP8266 serial port during boot. Once the service is configured with a logger, you will get the ip address reported in boot log events for easier identifiation.

To set configuration, you specify zero or more logging interfaces and zero or more SensorGroups. Here's an example which:
- Configures both a loki integration for logging, and an Mqtt integration. If you don't require logging, remove one or both of these
- A single sensor group called strawberries, with a water level sensor on channel 1
- Two moisture sensors on channels 1 and 2, set to that if any reach minimum, pumping occurs. Other valid values are "all", so all sensors must reach minimim before triggering
- A single pump, on D4 output pin. Valid values are D0, D1, D2, D3, D4
- Minimum moisture level of 250 (valid values are the 10 bit ADC range, 0-1023)
- Pump duration of 2 seconds
- Respective millisecond check periods for water level reporting, pump checks (during pumping), and moisture checks
- Water levels are continuously checked during pumping, and stops pumping if low water detected
```
{
    "instance": "testserver",
    "loggers": [
       {"type": "loki",
        "server": "192.168.x.x"
       },
       {"type": "mqtt",
        "server": "192.168.x.x",
        "topicPrefix": "home/irrigation/testserver/"
       }
    ],
    "groups": [
        {
            "name": "strawberries",
            "triggerType": "any",
            "waterSensorChannel": 0,
            "moistureSensorChannels": [
                1,2
            ],
            "pumpPinIds": [
                "D4"
            ],
            "minMoisture": 250,
            "pumpSecs": 2,
            "waterCheckPeriodMs": 1200000,
            "pumpCheckPeriodMs": 1000,
            "moistureCheckPeriodMs": 60000
        }
    ]
}
```
To save this configuration to the device, copy this json to a file myconfig.json (or other name you choose) and use this command:
```
%curl -X POST --data-binary @myconfig.json http://<myESPipaddress>:80/config
Config file writen%
```
If the posted configuration is invalid JSON, you will get an error message indicating a deserialisation issue. If the JSON is valid, but not expected structure, you will get an error indicating what is missing.
If validation passes, configuration is written to LittleFS permanent storage, and applied to the running system, thus allowing remote changes to configuration. This is particularly useful for tuning the minMoisture setting.

Additional groups can be specified. This example has two SensorGroups, sharing the same water sensor (i.e. pumping from the same bucket), and logging only to Loki.
```
{
    "instance": "testserver",
    "loggers": [
       {"type": "loki",
        "server": "192.168.86.5"
       }
    ],
    "groups": [
        {
            "name": "strawberries",
            "triggerType": "any",
            "waterSensorChannel": 0,
            "moistureSensorChannels": [
                0,1
            ],
            "pumpPinIds": [
                "D4"
            ],
            "minMoisture": 250,
            "pumpSecs": 2,
            "waterCheckPeriodMs": 1200000,
            "pumpCheckPeriodMs": 1000,
            "moistureCheckPeriodMs": 600000
        },
        {
            "name": "basil",
            "triggerType": "all",
            "waterSensorChannel": 0,
            "moistureSensorChannels": [
                2,3
            ],
            "pumpPinIds": [
                "D0"
            ],
            "minMoisture": 500,
            "pumpSecs": 5,
            "waterCheckPeriodMs": 1200000,
            "pumpCheckPeriodMs": 1000,
            "moistureCheckPeriodMs": 60000
        }
    ]
}
```
