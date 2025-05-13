#include "Particle.h"
#include <Wire.h>
#include <math.h>
#include <SPI.h>
#include <ThingSpeak.h>
#include <CellularHelper.h>
#include "Adafruit_seesaw.h"
#include <adafruit-sht31.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>



// System mode and logging
SYSTEM_MODE(AUTOMATIC); // Let Device OS manage the connection to the Particle Cloud
SerialLogHandler logHandler(LOG_LEVEL_INFO); // Show system, cloud connectivity, and application logs over USB; View logs with CLI using 'particle serial monitor --follow'

// Device constants
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define SOIL_SENSOR_ADDR 0x36
#define SHT31_SENSOR_ADDR 0x44
#define ADC_ADDR 0x48
#define TCA_ADDR 0x70
const int FLOW1_PIN = D2; // SIG1
const int FLOW2_PIN = D3; // SIG2
const int RELAY_PIN = D4; // SIG3
const int EXTRA_PIN = D5; // SIG4
const int OLED_RESET_PIN = D6; // DIRECT TO E-SERIES
const float calibrationFactor = 7.5;  // Flow Rate Frequency: F=7.5 * Q (L / Min) for YF-S201

// Initialize Sensors and Display
TCPClient client;
Adafruit_seesaw ss;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_HEIGHT, SCREEN_WIDTH, &Wire, OLED_RESET); // or delete OLED_RESET
Adafruit_BME280 bme; // I2C
Adafruit_ADS1115 ads1115(ADC_ADDR);

// ThingSpeak settings
bool useThingSpeak = false;
unsigned long myChannelNumber = 2902911;    // change this to your channel number
const char * myWriteAPIKey = "YZKX20WCFZ21RCOU"; // change this to your channels write API key

// Timing Variables
int interval = 5 * 60 * 1000; // 5 minutes in milliseconds
unsigned long previousMillis = 0;
int interval_disp = 5*1000; // rotate display every 5 seconds
unsigned long previousMillis_disp = 0;

// OLED Display Formatting
int disp_view = 0;
const int max_disp_view = 3;
int size = 1;
int msglen = 6;
uint16_t bkgnd_color;
uint16_t txt_color;

// Struct definitions
struct SoilSensorData {
    uint16_t raw;
    float percent;
    String rawStr;
    String percentStr;
};

struct FlowSensorData {
    volatile uint32_t pulseCount;
    float rateLPM;
    float volume_mL;
    String rateStr;
};

struct TempHumidityData {
    float tempC;
    float tempF;
    float humidity;
    String tempCStr;
    String tempFStr;
    String RHStr;
};

// Data Variables and Flags
SoilSensorData soilSensors[4];
FlowSensorData flowSensors[2];
TempHumidityData externalEnv, internalEnv;
bool checkSoilSensors[4] = {false, false, false, false};
bool checkSHT = false;
bool checkBME = false;
bool checkOLED = false;
bool analogbool = false; // For Sensor 2, Channel 1
String locationStr = "unknown";
String allDataAPI;
int soillow = 525;
int soilhigh = 860;
float soilLOWthreshold = 40.0;
float soilHIGHthreshold = 80.0;
float flowLOWthreshold = 0.10;
float flowHIGHthreshold = 5.00;


void setup() {
    Serial.begin(9600);
    delay(250);
    Wire.begin();
    
    if (useThingSpeak){
        ThingSpeak.begin(client);
    }
    
    setupParticleVariables();
    setupPinsAndInterrupts(); 
    setupDisplay(-4.0); // -4.0 for EST, +4.5 for Kabul
    setupSensors();
}


void loop() {
    if (millis() - previousMillis >= interval) {
        previousMillis = millis();
        readSoilSensors();
        readExternalTempHumidity();
        readInternalTempHumidity();
        readFlowSensors();
        updateThingSpeak();
    }
    
    allDataAPI = soilSensors[0].percentStr + "," + soilSensors[1].percentStr + "," + soilSensors[2].percentStr + "," + soilSensors[3].percentStr;
    allDataAPI = allDataAPI + "," + externalEnv.tempCStr + "," + externalEnv.RHStr + "," + externalEnv.tempFStr + "," + internalEnv.tempCStr + "," + internalEnv.RHStr + "," + internalEnv.tempFStr;
    allDataAPI = allDataAPI + "," + flowSensors[0].rateStr + "," + flowSensors[1].rateStr;
    
    
    if (millis() - previousMillis_disp >= interval_disp) {
        previousMillis_disp = millis();
        renderDisplay();
    }
}


// Helper Functions

/*!
 ****************************************************************************
 *  @brief      Setting Up Particle Variables and Functions
 ***********************************************************************/
void setupParticleVariables() {
    Particle.variable("allDataAPI", allDataAPI);
    Particle.variable("locationStr", locationStr);
    Particle.variable("rawRead_1", soilSensors[0].rawStr);
    Particle.variable("rawRead_2", soilSensors[1].rawStr);
    Particle.variable("rawRead_3", soilSensors[2].rawStr);
    Particle.variable("rawRead_4", soilSensors[3].rawStr);
    Particle.variable("soilmoisturePercent_1", soilSensors[0].percentStr);
    Particle.variable("soilmoisturePercent_2", soilSensors[1].percentStr);
    Particle.variable("soilmoisturePercent_3", soilSensors[2].percentStr);
    Particle.variable("soilmoisturePercent_4", soilSensors[3].percentStr);
    Particle.variable("degC_ext", externalEnv.tempCStr);
    Particle.variable("RH_ext", externalEnv.RHStr);
    Particle.variable("degF_ext", externalEnv.tempFStr);
    Particle.variable("degC_int", internalEnv.tempCStr);
    Particle.variable("RH_int", internalEnv.RHStr);
    Particle.variable("degF_int", internalEnv.tempFStr);
    Particle.variable("flowRate_1", flowSensors[0].rateStr);
    Particle.variable("flowRate_2", flowSensors[1].rateStr);
    
    Particle.function("set_interval", set_interval); 
    Particle.function("activate_relay", activate_relay);
    Particle.function("get_location", get_location);
    Particle.function("set_analogbool", set_analogbool);
}

/*!
 ****************************************************************************
 *  @brief      Setting Up Pins and Interrupts
 ***********************************************************************/
void setupPinsAndInterrupts() {
    pinMode(FLOW1_PIN, INPUT_PULLUP); // Enable internal pull-up resistor
    attachInterrupt(FLOW1_PIN, pulseCounter1, RISING); // Count rising edges
    
    pinMode(FLOW2_PIN, INPUT_PULLUP); // Enable internal pull-up resistor
    attachInterrupt(FLOW2_PIN, pulseCounter2, RISING); // Count rising edges
    
    pinMode(RELAY_PIN, OUTPUT); // Set the relay pin as an output
    digitalWrite(RELAY_PIN, LOW); // Ensure the relay is off initially   
}

/*!
 ****************************************************************************
 *  @brief      Update Time Zone and Setup Display
 *
 *  @param      utcoffset: offset from UTC in hours
 ***********************************************************************/
void setupDisplay(float utcoffset) {
    // Wait for time sync
    waitUntil(Particle.connected);
    Time.zone(utcoffset);  // Set your local timezone, e.g., EDT = -4.0 for EST, +4.5 for Afghanistan
    
    tca_select(7);
    delay(50);
    pinMode(OLED_RESET_PIN, OUTPUT);
    digitalWrite(OLED_RESET_PIN, LOW);
    delay(10);
    digitalWrite(OLED_RESET_PIN, HIGH);
    delay(100);
    display.begin(SCREEN_ADDRESS, true); // Address 0x3C default
    display.display();
    delay(1000);
    checkOLED = display.begin(SCREEN_ADDRESS, true);
    // Clear the buffer.
    display.clearDisplay();
    display.setTextSize(1);
    display.setRotation(1);
}

/*!
 ****************************************************************************
 *  @brief      Setup Sensors and Check Connections
 ***********************************************************************/
void setupSensors() {
    for (int i=0; i < 4; i++) {
        tca_select(i);
        delay(50);
        checkSoilSensors[i] = ss.begin(SOIL_SENSOR_ADDR); // Initialize the sensor at I²C address 0x36.
        if (!checkSoilSensors[i] && Particle.connected()) {
            Particle.publish("SensorError", "ERROR! Seesaw " + String(i+1) + " not found", PRIVATE);
        } 
    }
    tca_select(4);
    delay(50);
    checkSHT = sht31.begin(SHT31_SENSOR_ADDR);
    if (!checkSHT && Particle.connected()) {   
        Particle.publish("SensorError", "ERROR! SHT31 not found", PRIVATE);
    } 
    tca_select(5);
    delay(50);
    checkBME = bme.begin();
    if (!checkBME && Particle.connected()) { 
        Particle.publish("SensorError", "ERROR! BME280 not found", PRIVATE);
    }
    if (checkSoilSensors[0] && checkSoilSensors[0] && checkSoilSensors[0] && checkSoilSensors[0] && checkSHT && checkBME && Particle.connected()) {
        Particle.publish("SensorsVerified", "All Sensors Found!", PRIVATE);
    }
}

/*!
 ****************************************************************************
 *  @brief      Read All Soil Sensors (Check Connection) and Update Particle Variables
 ***********************************************************************/
void readSoilSensors() {
    for (int ch = 0; ch < 4; ch++) {
        tca_select(ch);
        delay(50);
        if (ch == 1 && analogbool) {
            ads1115.begin();
            delay(50);
            soilSensors[ch].raw = ads1115.readADC_SingleEnded(0);
            soilSensors[ch].rawStr = String(soilSensors[ch].raw);
        } else {
            checkSoilSensors[ch] = ss.begin(SOIL_SENSOR_ADDR);
            if (checkSoilSensors[ch]) {
                soilSensors[ch].raw = ss.touchRead(0);
                if (soilSensors[ch].raw > 1000) {
                    checkSoilSensors[ch] = false;
                }
                soilSensors[ch].percent = constrain(mapf(soilSensors[ch].raw, soillow, soilhigh, 0, 100), 0.0, 100.0);
                soilSensors[ch].rawStr = String(soilSensors[ch].raw);
                soilSensors[ch].percentStr = String(soilSensors[ch].percent, 2);
            }
        }
    }
}

/*!
 ****************************************************************************
 *  @brief      Read SHT External Sensor (Check Connection) and Update Particle Variables
 ***********************************************************************/
void readExternalTempHumidity() {
    tca_select(4);
    delay(50);
    checkSHT = sht31.begin(SHT31_SENSOR_ADDR);
    if (checkSHT) {
        externalEnv.tempC = sht31.readTemperature();
        externalEnv.humidity = sht31.readHumidity();
        externalEnv.tempF = (externalEnv.tempC * 9) / 5 + 32;
        externalEnv.tempCStr = String(externalEnv.tempC, 2);
        externalEnv.RHStr = String(externalEnv.humidity, 2);
        externalEnv.tempFStr = String(externalEnv.tempF, 2);
    }
}

/*!
 ****************************************************************************
 *  @brief      Read BME Internal Sensor (Check Connection) and Update Particle Variables
 ***********************************************************************/
void readInternalTempHumidity() {
    tca_select(5);
    delay(50);
    checkBME = bme.begin();
    if (checkBME) {
        internalEnv.tempC = bme.readTemperature();
        internalEnv.humidity = bme.readHumidity();
        internalEnv.tempF = (internalEnv.tempC * 9) / 5 + 32;
        internalEnv.tempCStr = String(internalEnv.tempC, 2);
        internalEnv.RHStr = String(internalEnv.humidity, 2);
        internalEnv.tempFStr = String(internalEnv.tempF, 2);
    }
}

/*!
 ****************************************************************************
 *  @brief      Read All FLow Rate Sensors and Update Particle Variables
 ***********************************************************************/
void readFlowSensors() {
    detachInterrupt(FLOW1_PIN);
    detachInterrupt(FLOW2_PIN);
    float elapsedTimeSec = interval / 1000.0; // Calculate frequency
    for (int i = 0; i < 2; i++) {
        flowSensors[i].rateLPM = (flowSensors[i].pulseCount / elapsedTimeSec) / calibrationFactor;
        flowSensors[i].volume_mL = (flowSensors[i].rateLPM / 60.0) * 1000.0;
        flowSensors[i].rateStr = String(flowSensors[i].rateLPM, 2);
        flowSensors[i].pulseCount = 0;
    }
    attachInterrupt(FLOW1_PIN, pulseCounter1, RISING);
    attachInterrupt(FLOW2_PIN, pulseCounter2, RISING);
}

/*!
 ****************************************************************************
 *  @brief      Update ThingSpeak (if toggled on)
 ***********************************************************************/
void updateThingSpeak() {
    if (!useThingSpeak) return;
    for (int i = 0; i < 4; i++) {
        ThingSpeak.setField(i + 1, soilSensors[i].percent);
    }
    ThingSpeak.setField(5, flowSensors[0].rateLPM);
    ThingSpeak.setField(6, flowSensors[1].rateLPM);
    ThingSpeak.setField(7, externalEnv.tempC);
    ThingSpeak.setField(8, externalEnv.humidity);
    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

/*!
 ****************************************************************************
 *  @brief      Update OLED Display
 ***********************************************************************/
void renderDisplay() {
    tca_select(7);
    delay(50);
    disp_view = (disp_view + 1) % (max_disp_view);
    display.clearDisplay();
    
    // Always Display DateTime and Temp + RH (if available)
    display.fillRect(0,0, SCREEN_WIDTH, 8, SH110X_BLACK);
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    String currentTime = Time.format(Time.now(), "%Y-%m-%d %H:%M:%S");
    display.setCursor(5,0);
    display.println(currentTime);
    if (checkSHT) {
        display.setCursor(5,8);
        display.print(externalEnv.tempCStr + " C | " + externalEnv.RHStr + "% RH");
    } else {
        display.print("SHT31 ERROR");
    }

    
    switch (disp_view) {
        case 0: // Temperatures and RHs
            display.fillRect(0,18, SCREEN_WIDTH, 8, SH110X_BLACK);
            display.setTextColor(SH110X_WHITE);
            size = 1;
            display.setTextSize(size);
            // Centering Text (size 1 is 6x8 pixels per char * text size)
            msglen = 6 * strlen("External (SHT31)") * size;
            display.setCursor((SCREEN_WIDTH - msglen)/2, 18);
            display.print("External (SHT31)");
            
            display.setCursor(5, 26);
            display.print(externalEnv.tempCStr + " C | " + externalEnv.RHStr + "% RH");
            
            // Centering Text (size 1 is 6x8 pixels per char * text size)
            msglen = 6 * strlen("Internal (BME280)") * size;
            display.setCursor((SCREEN_WIDTH - msglen)/2, 40);
            display.print("Internal (BME280)");
            
            display.setCursor(5, 48);
            display.print(internalEnv.tempCStr + " C | " + internalEnv.RHStr + "% RH");
            break;
            
        case 1: // Soil Moisture
            display.fillRect(0,18, SCREEN_WIDTH, 8, SH110X_BLACK);
            display.setTextColor(SH110X_WHITE);
            size = 1;
            display.setTextSize(size);
            // Centering Text (size 1 is 6x8 pixels per char * text size)
            msglen = 6 * strlen("Soil Moisture") * size;
            display.setCursor((SCREEN_WIDTH - msglen)/2, 18);
            display.print("Soil Moisture");

            for (int i=0; i<4; i++) {
                if (soilSensors[i].percent < soilLOWthreshold) {
                    bkgnd_color = SH110X_WHITE;
                    txt_color = SH110X_BLACK;
                } else if (soilSensors[i].percent > soilHIGHthreshold) {
                    bkgnd_color = SH110X_WHITE;
                    txt_color = SH110X_BLACK;
                } else {
                    bkgnd_color = SH110X_BLACK;
                    txt_color = SH110X_WHITE;
                }
                
                display.fillRect(0, 26+i*8, SCREEN_WIDTH, 8, bkgnd_color);
                display.setTextColor(txt_color);
                display.setTextSize(1);
                display.setCursor(5, 26+i*8);
                if (checkSoilSensors[i]) {
                    display.print("Sensor " + String(i+1) + ": " + soilSensors[i].percentStr + "%");
                } else {
                    display.print("Sensor " + String(i+1) + ": ERROR");
                }
            }
            break;
            
        case 2: // Flow Rate
            display.fillRect(0,18, SCREEN_WIDTH, 8, SH110X_BLACK);
            display.setTextColor(SH110X_WHITE);
            size = 1;
            display.setTextSize(size);
            // Centering Text (size 1 is 6x8 pixels per char * text size)
            msglen = 6 * strlen("Flow Rates") * size;
            display.setCursor((SCREEN_WIDTH - msglen)/2, 18);
            display.print("Flow Rates");

            for (int i=0; i<2; i++) {
                if (flowSensors[i].rateLPM < flowLOWthreshold) {
                    bkgnd_color = SH110X_WHITE;
                    txt_color = SH110X_BLACK;
                } else if (flowSensors[i].rateLPM > flowHIGHthreshold) {
                    bkgnd_color = SH110X_WHITE;
                    txt_color = SH110X_BLACK;
                } else {
                    bkgnd_color = SH110X_BLACK;
                    txt_color = SH110X_WHITE;
                }
                
                display.fillRect(0, 26+i*8, SCREEN_WIDTH, 8, bkgnd_color);
                display.setTextColor(txt_color);
                display.setTextSize(1);
                display.setCursor(5, 26+i*8);
                display.print("Sensor " + String(i+1) + ": " + flowSensors[i].rateStr + " L/min");
            }
            break;
    }
    display.display(); // show everything
}

/*!
 ****************************************************************************
 *  @brief      Set the loop interval rate
 *
 *  @param      command: input interval length (in ms)
 *
 *  @return     interval length (in ms)
 ***********************************************************************/
int set_interval(String command) {
    if (isAllDigits(command)) {
        interval = command.toInt();  // Convert input string to integer
    
        if (interval < 1000*2) { // lower threshold of 15 seconds
            interval = 1000*2;
        } 
        Particle.publish("intervalUpdate", "Loop Interval Updated to: " + String(interval) + "ms", PRIVATE);
        return interval;
    } else {
        interval = 5 * 60 * 10000; // default 5 minute interval
        Particle.publish("intervalUpdate", "Loop Interval Updated to: " + String(interval) + "ms", PRIVATE);
        return interval;
    }
}


/*!
 ****************************************************************************
 *  @brief      Activate (or inactivate - default) the relay for the water pump
 *
 *  @param      command: "HIGH" or "LOW"
 ***********************************************************************/
int activate_relay(String command) {
    if (command == "HIGH") {
        digitalWrite(RELAY_PIN, HIGH); // Activate the relay
        return 1;
    } else {
        digitalWrite(RELAY_PIN, LOW); // Deactivate the relay
        return 0;
    }
}

/*!
 ****************************************************************************
 *  @brief      Update Location
 *
 *  @param      command: any string
 ***********************************************************************/
int get_location(String command) {
    CellularHelperLocationResponse loc = CellularHelper.getLocation(20000);  // 10-second timeout
    if (loc.valid) {
        locationStr = "found";
        return 1;
        // locationStr = String::format("Lat: %.6f, Lon: %.6f", loc.lat, loc.lon);
    } else {
        return 0;
    }
}

/*!
 ****************************************************************************
 *  @brief      Update Location
 *
 *  @param      command: any string
 ***********************************************************************/
int set_analogbool(String command) {
    if (command == "true") {
        analogbool = true;
        return 1;
    } else {
        analogbool = false;
        return 0;
    }
}

/*!
 ****************************************************************************
 *  @brief      Check if input command is all digits
 *
 *  @param      command string
 *
 *  @return     boolean
 ***********************************************************************/
bool isAllDigits(const String &input) {
    for (int i = 0; i < input.length(); i++) {
        char c = input.charAt(i);
        // Alternatively, you can use: if (!isdigit(c)) { ... }
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

/*!
 ****************************************************************************
 *  @brief      Select the TCA9548A channel (0–7)
 *
 *  @param      channel number
 ***********************************************************************/
void tca_select(uint8_t channel) {
    if (channel > 7) return;
    if (channel == 1 && analogbool) {
        Wire.beginTransmission(ADC_ADDR);
    } else {
        Wire.beginTransmission(TCA_ADDR);
    }
    Wire.write(1 << channel);  // Enable only that channel
    Wire.endTransmission();
}

/*!
 ****************************************************************************
 *  @brief      map but for floats
 *
 *  @params     as specified
 ***********************************************************************/
float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*!
 ****************************************************************************
 *  @brief      helper function for attachInterupt (flow rate)
 *
 *  @params     as specified
 ***********************************************************************/
void pulseCounter1() { flowSensors[0].pulseCount++; }
void pulseCounter2() { flowSensors[1].pulseCount++; }

