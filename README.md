# LifxButton
IoT Button for Lifx Bulbs - Using ESP8266 and Arduino Sketches

--

This Sketch uses the onboard push button (labelled 'FLASH') to toggle the state of a Lifx Bulb. Wifi Connection is managed via the WifiManager library, details here: https://github.com/tzapu/WiFiManager

On boot the ESP will try to connect to a previously configured WiFi AP. If no AP is available, it will host it's own AP to which a user can connect and configure connection details through. To configure the ESP, connect to the created AP with the SSID "esp-wifi" and navigate to 192.168.4.1. At this location you will be able to provide SSID and password credentials for the ESP to connect to as well as a Bulb ID and App Token required to communicate with the Lifx API's.

When the ESP is connecting to WiFi the onboard LED will blink, if the LED does not stop blinking, you will need to connect to the ESP's AP and configure the connection paramaters. When the IoT button is triggered, the onboard LED will light up for one second. If you hold the button for longer than 5 seconds, the ESP will enter a reset configuration state and the next time the board is reset, WifiManager will not attempt to automatically connect to any AP's, instead the ESP will go into configuration mode and you will be required to connect to the ESP's AP and configure some settings. (This will be required if you want to change the bulbId and access token).

Refer to the diagram on this page to see the ESP8266 12E Board pinouts:
https://github.com/nodemcu/nodemcu-devkit
