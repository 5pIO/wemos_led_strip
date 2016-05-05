#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

//Start Customization Variables

#define WifiLEDInterval 200
#define mDNSLEDInterval 1500

#define GREEN1PIN 5
#define RED1PIN 4
#define BLUE1PIN 0

#define GREEN2PIN 2
#define RED2PIN 14
#define BLUE2PIN 12

#define SWITCH1PIN 13
#define SWITCH2PIN 15

#define LEDPIN BUILTIN_LED
#define LEDON LOW

#define SWITCHON LOW
// End Customization Variables 


// System and Switch states
int switch1State;
int switch2State;

enum status {
  WifiWait,
  MDNSWait,
  Ready
};

status currentStatus = WifiWait;

bool serverInitialized = false;
bool wifiInitialized = false;

// End System and Switch states

// Status LED Setup
unsigned long previousMillis = 0;
int ledState = LEDON;
// End of Status LED Setup

// WiFi,mDNS,Webserver setup
const char* ssid = "FellowBros";
const char* password = "bhaijaan";
const char* mdns_hostname = "wemos";

ESP8266WebServer server(80);
// End of WiFi,mDNS,Webserver setup

// Color Variables for each strip
int red1 = 0;
int green1 = 0;
int blue1 = 0; 

int red2 = 0;
int green2 = 0;
int blue2 = 0; 
// End of Color Variables for each strip

void setup() {
  // Set Pin Modes
  pinMode(LEDPIN, OUTPUT);
  pinMode(RED1PIN, OUTPUT);
  pinMode(GREEN1PIN, OUTPUT);
  pinMode(BLUE1PIN, OUTPUT);
  pinMode(RED2PIN, OUTPUT);
  pinMode(GREEN2PIN, OUTPUT);
  pinMode(BLUE2PIN, OUTPUT);
  pinMode(SWITCH1PIN, INPUT);
  pinMode(SWITCH2PIN, INPUT);

  // Set Initial Switch states
  switch1State = digitalRead(SWITCH1PIN);
  switch2State = digitalRead(SWITCH2PIN);

  Serial.begin(115200);
}

void loop() {
  // Check if Wifi Disconnected
  if(wifiInitialized && serverInitialized) {
    if(WiFi.status() != WL_CONNECTED) {
      server.stop();
      serverInitialized = false;
      wifiInitialized = false;
      currentStatus = WifiWait;
    }
  }

  // WifiWait State, waiting for Wifi to initialize and connect
  if(currentStatus == WifiWait) {
    // If Wifi is not Initialized start the WiFi connection
    if(!wifiInitialized) {
      WiFi.begin(ssid, password);
      wifiInitialized = true;
    }
    // When the Wifi Connects move to the MDNSWait State
    if(WiFi.status() == WL_CONNECTED) {
      currentStatus = MDNSWait;
    } else {
      // Waiting for Wifi to connect, flash LED accordingly
      unsigned long currentMillis = millis();
      unsigned long elapsedTime = currentMillis - previousMillis;
      if(elapsedTime >= WifiLEDInterval) {
        previousMillis = currentMillis;
        toggleLed(); 
      }
    }
  } 

  // MDNSWait Sate, waiting for MDNS to initialize
  if(currentStatus == MDNSWait) {
    // When the MDNS initializes move to Ready State
    if (MDNS.begin(mdns_hostname)) {
      currentStatus = Ready;
    } else {
      // Waiting for MDNS to Init, flash LED accordingly
      unsigned long currentMillis = millis();
      unsigned long elapsedTime = currentMillis - previousMillis;
      if(elapsedTime >= MDNSWait) {
        previousMillis = currentMillis;
        toggleLed(); 
      }
    }
  }

  // Ready State, start webserver if not initialized and handle clients
  if(currentStatus == Ready) {
    if(serverInitialized == false) {
      //Initialize Webserver
      digitalWrite(LEDPIN, LEDON);
      ledState = LEDON;
      
      // Setup Server Routes
      server.on("/", handleRoot);
      server.on("/set", handleSet);
      server.on("/status", handleStatus);
      server.onNotFound(handleNotFound);
      // Start Server
      server.begin();
      
      // Update Color Values
      updateStrip1();
      updateStrip2();
      
      serverInitialized = true;
    } else {
      server.handleClient();
    }
  }

  // Check Switch 1 State and turn on or off based on it
  int new_switch1State = digitalRead(SWITCH1PIN);
  if(new_switch1State!= switch1State) {
    switch1State = new_switch1State;
    updateStrip1();
  }
  // Check Switch 2 State and turn on or off based on it
  int new_switch2State = digitalRead(SWITCH2PIN);
  if(new_switch2State!= switch2State) {
    switch2State = new_switch2State;
    updateStrip2();
  }
}

// Toggle Status LED
void toggleLed() {
   ledState = !ledState;
   digitalWrite(LEDPIN, ledState);
}


// Handle Root route
void handleRoot() {
  server.send(200, "text/plain", "Welcome to the WeMOS LED Strip Controller, Please POST to this URL to set colors or go to /status to see current color");  
}

// Handel Set route and set colors based on url params
void handleSet() {
  int n=0;
  int r_new=-1;
  int g_new=-1;
  int b_new=-1;
  bool r_changed=false;
  bool g_changed=false;
  bool b_changed=false;
  server.send(200, "text/plain", "Setting Colors");
  // Get url params
  Serial.println(server.args());
  for (uint8_t i=0; i<server.args(); i++) {
    Serial.println(server.argName(i));
    if(server.argName(i)=="r") {
      r_new = server.arg(i).toInt();
    } else if (server.argName(i)=="g") {
      g_new = server.arg(i).toInt();
    } else if (server.argName(i)=="b") {
      b_new = server.arg(i).toInt();
    } else if (server.argName(i)=="n") {
      n = server.arg(i).toInt();
    }
  }

  // See if params are present
  if(r_new > -1) {
    r_changed = true;
  }
  if(g_new > -1) {
    g_changed = true;
  }
  if(b_new > -1) {
    b_changed = true;
  }

  // Set colors for corresponding led strip(s) and update.
  if(n==1) {
    if(r_changed) {
      red1 = r_new;  
    }
    if(g_changed) {
      green1 = g_new;  
    }
    if(b_changed) {
      blue1 = b_new;
    }
    updateStrip1();
  } else if(n==2) {
    if(r_changed) {
      red2 = r_new;  
    }
    if(g_changed) {
      green2 = g_new;  
    }
    if(b_changed) {
      blue2 = b_new;
    }
    updateStrip2();
  } else {
    if(r_changed) {
      red1 = r_new;  
      red2 = r_new;  
    }
    if(g_changed) {
      green1 = g_new;  
      green2 = g_new;  
    }
    if(b_changed) {
      blue1 = b_new;
      blue2 = b_new;
    }
    updateStrip1();
    updateStrip2();
  }
}
// Handle status route, returns colors for each strip
void handleStatus() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonObject& strip1 = root.createNestedObject("strip1");
  JsonObject& strip2 = root.createNestedObject("strip2");
  strip1["red"] = red1;
  strip1["green"] = green1;
  strip1["blue"] = blue1;
  strip2["red"] = red2;
  strip2["green"] = green2;
  strip2["blue"] = blue2;
  char buffer[256];
  root.printTo(buffer, sizeof(buffer));
  server.send(200, "application/json", buffer);
}

// Handle not found route
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

// Set Strip 1 colors
void updateStrip1() {
  if(switch1State == SWITCHON) {
    Serial.println("Updating Switch 1");
    analogWrite(RED1PIN, red1);
    analogWrite(BLUE1PIN, blue1);
    analogWrite(GREEN1PIN, green1);  
  } else {
    Serial.println("Not Updating Switch 1");
    analogWrite(RED1PIN, 0);
    analogWrite(BLUE1PIN, 0);
    analogWrite(GREEN1PIN, 0);  
  }
}

// Set Strip 2 colors
void updateStrip2() {
  if(switch1State == SWITCHON) {
    analogWrite(RED2PIN, red2);
    analogWrite(BLUE2PIN, blue2);
    analogWrite(GREEN2PIN, green2);  
  } else {
    analogWrite(RED2PIN, 0);
    analogWrite(BLUE2PIN, 0);
    analogWrite(GREEN2PIN, 0);  
  } 
}

