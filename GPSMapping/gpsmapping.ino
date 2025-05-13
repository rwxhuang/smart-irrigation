#include "Particle.h"
#include <ParticleSoftSerial.h>
#include <TinyGPS++.h>
// #include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
// #include <Adafruit_SH110X_Z.h>
#include <Adafruit_BusIO_Register.h>

// System mode and logging
SYSTEM_MODE(AUTOMATIC); // Let Device OS manage the connection to the Particle Cloud
SerialLogHandler logHandler(LOG_LEVEL_INFO); // Show system, cloud connectivity, and application logs over USB; View logs with CLI using 'particle serial monitor --follow'

// Device constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
static const int RXPin = 4; // Connects to GT-U7 TXD Pin
static const int TXPin = 3; // No data sent to GT-U7 (connects to GT-U7 RXD pin)
static const uint32_t GPSBaud = 9600; // For GT-U7
static const int switchpinLeft = 5;
static const int switchpinRight = 6;
const int OLED_RESET_PIN = D6; // DIRECT TO E-SERIES
const int buttonPin = D7;


// Initialize Sensors and Display
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire, OLED_RESET); // or delete OLED_RESET


// Timing Variables
int interval = 2000; 
unsigned long previousMillis = 0;
unsigned long keySavedTimestamp = 0;
const unsigned long keySavedDisplayDuration = 2000;

// OLED Display Formatting
int disp_view = 0;
const int max_disp_view = 3;
int size = 1;
int msglen = 6;
uint16_t bkgnd_color;
uint16_t txt_color;

// Struct definitions
struct locationStruct {
    float lat;
    float lon;
    String locationStr;
};

// Data Variables and Flags
locationStruct locationData;
String current_location;
int sentID = 0;
String sentIDStr;
String recordedLocationsBuffer;
String recordedLocationsAll;
String keyLocation;
int mode = 0;
int indicator = 0;
bool debug = false;
bool switchtoggle;
bool showKeySaved = false;


// Button & debounce logic
bool button;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;




void setup() {
    Serial.begin(115200);
    ss.begin(GPSBaud);
    
    Particle.variable("current_location", current_location);
    Particle.variable("sentID", sentIDStr);
    Particle.variable("recordedLocationsBuffer", recordedLocationsBuffer);
    Particle.variable("recordedLocationsAll", recordedLocationsAll);
    Particle.variable("keyLocation", keyLocation);
    
    Particle.function("set_mode", set_mode);
    Particle.function("set_debug", set_debug);
    
    pinMode(switchpinLeft, INPUT_PULLUP);
    pinMode(switchpinRight, INPUT_PULLUP);
    pinMode(buttonPin, INPUT_PULLUP);
    
    setupDisplay(-4.0);
}

void loop() {
    while (ss.available() > 0) {
        if (debug) {
        Serial.write(ss.read());
        current_location = get_location();
        } else { 
        gps.encode(ss.read()); // debug mode
        }
    }
    
    getSwitchPosition();
    if (switchtoggle && mode == 0) {
        recordedLocationsBuffer = "";
        sentID += 1;
        mode = 1;
    } else if (!switchtoggle && mode == 1) {
        recordedLocationsAll = recordedLocationsBuffer;
        sentIDStr = String(sentID);
        mode = 0;
    }

    if (!debug && millis() > 5000 && gps.charsProcessed() < 10) {
      current_location = "No GPS Found";
      mode = 0;
    }
    
    switch (mode) {
        case 0: // idle
            break;
        
        case 1: // record
            if (millis() - previousMillis >= interval) {
                previousMillis = millis();
                recordedLocationsBuffer = recordedLocationsBuffer + current_location + ";";
            }
            break;
    }
    
    button = digitalRead(buttonPin);
    if (button != lastButtonState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (button == LOW && lastButtonState == HIGH) {
            keyLocation = current_location;
            showKeySaved = true;
            keySavedTimestamp = millis();
        }
    }
    lastButtonState = button;
    
    renderDisplay();
    
}


// Helper Functions

/*!
 ****************************************************************************
 *  @brief      Read Switch Position (false: OFF, true: ON)
 ***********************************************************************/
void getSwitchPosition() {
    switchtoggle = (digitalRead(switchpinLeft) == LOW) || (digitalRead(switchpinRight) == LOW);
}

/*!
 ****************************************************************************
 *  @brief      Get Location Data
 ***********************************************************************/
 String get_location() {
    if (gps.location.isValid()) {
        locationData.lat = gps.location.lat();
        locationData.lon = gps.location.lng();
        locationData.locationStr = String(locationData.lat, 6) + "," + String(locationData.lon, 6);
    } else {
        locationData.locationStr = "invalid";
    }
    return locationData.locationStr; // success
}

/*!
 ****************************************************************************
 *  @brief      Set Mode (0:idle, 1:active pipe)
 ***********************************************************************/
 int set_mode(String command) {
    if (command == "0") {
        mode = 0;
    } else if (command == "1") {
        mode = 1;
    } else {
        mode = 0;
    }
    return mode;
}

/*!
 ****************************************************************************
 *  @brief      Set Debug (0:false, 1:true)
 ***********************************************************************/
 bool set_debug(String command) {
    if (command == "0") {
        debug = false;
    } else if (command == "1") {
        debug = true;
    } else {
        debug = false;
    }
    return mode;
}
 
 /*!
 ****************************************************************************
 *  @brief      Original Print Location Function
 ***********************************************************************/
void displayInfo()
{
  Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid())
  {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  }
  else
  {
    Serial.print(F("INVALID"));
  }

  Serial.println();
}

 /*!
 ****************************************************************************
 *  @brief      Original Print Location Function
 ***********************************************************************/
void setupDisplay(float utcoffset) {
    // Wait for time sync
    waitUntil(Particle.connected);
    Time.zone(utcoffset);  // Set your local timezone, e.g., EDT = -4.0 for EST, +4.5 for Afghanistan
    
    pinMode(OLED_RESET_PIN, OUTPUT);
    digitalWrite(OLED_RESET_PIN, LOW);
    delay(10);
    digitalWrite(OLED_RESET_PIN, HIGH);
    delay(100);
    display.begin(SCREEN_ADDRESS, true); // Address 0x3C default
    display.display();
    delay(1000);
    display.begin(SCREEN_ADDRESS, true);
    // Clear the buffer.
    display.clearDisplay();
    display.setTextSize(1);
    display.setRotation(1);
}


/*!
 ****************************************************************************
 *  @brief      Update OLED Display
 ***********************************************************************/
void renderDisplay() {
    display.clearDisplay();
    
    display.fillRect(0,0, SCREEN_WIDTH, 8, SH110X_BLACK);
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);

    
    msglen = 6 * strlen("Location Mapper") * size;
    display.setCursor((SCREEN_WIDTH - msglen)/2, 0);
    display.print("Location Mapper");
    
    String currentTime = Time.format(Time.now(), "%Y-%m-%d %H:%M:%S");
    display.setCursor(5,12);
    display.println(currentTime);
    
    if (mode == 0) {
        display.fillRect(0,24, SCREEN_WIDTH, 40, SH110X_BLACK);
        display.setTextColor(SH110X_WHITE);
    } else if (mode == 1) {
        display.fillRect(0,24, SCREEN_WIDTH, 40, SH110X_WHITE);
        display.setTextColor(SH110X_BLACK);
    }
    
    msglen = 6 * strlen("Mode 0") * size;
    display.setCursor((SCREEN_WIDTH - msglen)/2, 24);
    display.print("Mode " + String(mode));
    
    display.setCursor(5, 32);
    display.print("Lat: " + String(locationData.lat, 6));
    display.setCursor(5, 40);
    display.print("Lng: " + String(locationData.lon, 6));   
    
    if (showKeySaved) {
        if (millis() - keySavedTimestamp < keySavedDisplayDuration) {
            display.fillRect(0, 48, SCREEN_WIDTH, 16, SH110X_WHITE);
            display.setTextColor(SH110X_BLACK);
            display.setCursor(5, 52);
            display.setTextSize(1);
            display.print("Key Location Saved!");
            display.display();
        } else {
            showKeySaved = false;
        }
    }
    
    display.display(); // show everything
}
