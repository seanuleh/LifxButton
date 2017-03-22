/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <Ticker.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson


#define USE_SSL 1
#if USE_SSL
#   include <WiFiClientSecure.h>
    WiFiClientSecure client;
#   define PORT 443
#else
    WiFiClient client;
#   define PORT 80
#endif
 
const char* host = "api.lifx.com";

//define your default values here
char bulbId[16] = ""; // Lifx bulbId here, or you can configure from the Configure Web Page
char privateKey[65] = ""; // Lifx App Token here, or you can configure from the Configure Web Page
char resetFlag[6] = "false";

//flag for saving data
bool shouldSaveConfig = true;

// constants won't change. They're used here to
// set pin numbers:
const int buttonPin = 0;    // the number of the pushbutton pin

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// the following variables are unsigned long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;   // the last time the output pin was toggled
unsigned long debounceDelay = 50;     // the debounce time; increase if the output flickers
unsigned long resetTime = 0;      // the debounce time to reset the device
unsigned long resetDelay = 5000;      // the debounce time to reset the device
unsigned long ledOnTime = 0;          // the last time the led was switched on
unsigned long timerVal = 0;          // the last time the led was switched on
unsigned long ledOnDuration = 1000;   // the amount of time to leave an LED on after an ack'd request
bool bootFix = false;

//for LED status
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void setup() {
  // Set the LED and button input/output 
  // Turn the LED on
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);  
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Start LED Ticker
  ticker.attach(0.6, tick);
  
  // Connect serial port with 115,200 baud rate
  Serial.begin(115200);

  // Paramaters are saved to a config file - load that config file up
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          Serial.printf("\nbulb_id is: %s", json["bulb_id"]);
          Serial.printf("\nprivate_key is: %s\n", json["private_key"]);
          Serial.printf("\resetFlag is: %s\n", json["reset"]);
          
          strcpy(bulbId, json["bulb_id"]);
          strcpy(privateKey, json["private_key"]);
          strcpy(resetFlag, json["reset"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  Serial.printf("The Reset Flag is %s\n", resetFlag);
  if (strcmp(resetFlag,"true") == 0) {
    wifiManager.resetSettings();
  }
  strcpy(resetFlag,"false");

  WiFiManagerParameter custom_bulbId("bulbid", "Lifx Bulb ID", bulbId, 16);
  WiFiManagerParameter custom_privateKey("privatekey", "Lifx Private Key", privateKey, 65);

  wifiManager.addParameter(&custom_bulbId);
  wifiManager.addParameter(&custom_privateKey);

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("esp-wifi","password")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  strcpy(bulbId, custom_bulbId.getValue());
  strcpy(privateKey, custom_privateKey.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["bulb_id"] = bulbId;
    json["private_key"] = privateKey;
    json["reset"] = resetFlag;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  ticker.detach();
  //keep LED on for a seconds
  digitalWrite(LED_BUILTIN, LOW);
  ledOnTime = millis();
  lastDebounceTime = millis();
  resetTime = millis();
}

void loop() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  if ((millis() - ledOnTime > ledOnDuration)) {
    // turn the LED off if it's been on for too long
    digitalWrite(LED_BUILTIN, HIGH);
    ticker.detach();
  }

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  timerVal = millis();          
  if ((timerVal - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      Serial.println("\n\n\n------- 50u seconds ------");
      Serial.printf("button State is: %d\n", buttonState);
      Serial.printf("timerVal is:%lu\n", timerVal);
      Serial.printf("lastDebounceTime is:%lu\n", lastDebounceTime);
      Serial.printf("timeDif is:%lu\n", (timerVal - lastDebounceTime));
      Serial.printf("timeDif (reset) is:%lu\n", (timerVal - resetTime));
      Serial.printf("debounceDelay is:%lu\n", debounceDelay);
      Serial.printf("resetDelay is:%lu\n", resetDelay);
      Serial.println("\n\n\n");

      if (buttonState == LOW){
        resetTime = millis();
      }
      
      // only toggle the LED if the new button state is HIGH
      if (buttonState == HIGH) {
        // The toggle is triggered every time on boot, this makes
        // sure that the first time it is triggered does not create
        // a request
        if (bootFix) {
          // If the button was held for >resetDelay reset the device
          if ((timerVal - resetTime) > resetDelay){
            setResetFlag(1);
            ticker.attach(0.1, tick);
            ledOnTime = millis();
            Serial.println("Setting Reset Flag");
          } else {
            doToggle();
          }
        }
        bootFix = true;
      }
    }
  }


  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  
  
}

void doToggle(){
  ledOnTime = millis();
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClientSecure client;
  const int httpPort = 443; 
  
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/v1/lights/id:";
  url += bulbId;
  url += "/toggle";

  // turn it on
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("POST ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Authorization: Bearer " + privateKey + " \r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");

  // Turn the LED on
  digitalWrite(LED_BUILTIN, HIGH);
}

void setResetFlag(int val){
    if (val == 0) {
      strcpy(resetFlag,"false");
    } else {
      strcpy(resetFlag,"true");
    }
    
    Serial.printf("Setting Reset Flag to %s\n", resetFlag);
    
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["reset"] = resetFlag;
    json["bulb_id"] = bulbId;
    json["private_key"] = privateKey;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
}

