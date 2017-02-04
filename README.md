# LifxButton
IoT Button for Lifx Bulbs - Using ESP8266 and Arduino Sketches

--

This Sketch uses a push button switch to toggle the state of a Lifx Bulb. By default the pushbutton is connected from pin D1 directly to ground. It uses an internal pullup resistor to keep the circuit simple. When the ESP has established a connection to the WiFi AP the internal LED will switch on and the IoT Button will be ready for use.

Refer to the diagram on this page to see the ESP8266 12E Board pinouts:
https://github.com/nodemcu/nodemcu-devkit
