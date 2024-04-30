
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
* Basic level of discoverability by Alexa using the FauxESP library

# Hardware summary:
* For simplicity, it uses a NodeMCU development board that provides the associated circuitry for 5v->3.3 volt step down power regulator for the ESP8266, and associated serial port circuitry for code flashing/monitoring.
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
 
