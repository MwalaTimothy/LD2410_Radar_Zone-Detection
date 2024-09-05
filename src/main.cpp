#include <Arduino.h>
#include <EEPROM.h>   // Include EEPROM library for non-volatile storage
#define MONITOR_SERIAL Serial
#define RADAR_SERIAL Serial1
#define RADAR_RX_PIN 4
#define RADAR_TX_PIN 5

#include <ld2410.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>  // Include WiFiManager library

// Function prototypes
void connectToWiFi();
void controlWled(int brightness);
void saveZoneAndWledToEEPROM();
void loadZoneAndWledFromEEPROM();

ld2410 radar;

uint32_t lastReading = 0;
bool radarConnected = false;

WiFiClient client;

char wledIP[16] = "192.168.2.141";  // Default WLED IP address
char zoneNumStr[4] = "1";            // Default zone number as a string

#define SCREEN_WIDTH 128  // OLED width, in pixels
#define SCREEN_HEIGHT 64  // OLED height, in pixels

#define RESET_PIN 0  // GPIO pin connected to the reset button

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

double zones_radius = 233;
int zone_num = 1;

void setup(void) {
    pinMode(RESET_PIN, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up

    // EEPROM initialization to store and retrieve persistent settings
    EEPROM.begin(512);
    loadZoneAndWledFromEEPROM();  // Load saved zone number and WLED IP

    // Check if the button is pressed for more than 5 seconds
    if (digitalRead(RESET_PIN) == LOW) {
        unsigned long startTime = millis();
        while (digitalRead(RESET_PIN) == LOW) {
            if (millis() - startTime > 5000) {
                Serial.println("Reset button pressed for 5 seconds, starting WiFiManager configuration portal...");
                WiFiManager wifiManager;
                wifiManager.resetSettings();  // Clear saved WiFi credentials
                wifiManager.setConfigPortalTimeout(180);  // Set timeout to 3 minutes (180 seconds)
                wifiManager.startConfigPortal("ZoneDetector");  // Start the configuration portal
                delay(1000);  // Give time for the portal to start
                ESP.restart();  // Restart the ESP32
            }
        }
    }

    MONITOR_SERIAL.begin(115200);
    connectToWiFi();

    controlWled(255);  // Turn the WLED OFF

    #if defined(ESP32)
    RADAR_SERIAL.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);  // UART for monitoring the radar
    #elif defined(__AVR_ATmega32U4__)
    RADAR_SERIAL.begin(256000);  // UART for monitoring the radar
    #endif

    delay(500);
    MONITOR_SERIAL.print(F("\nConnect LD2410 radar TX to GPIO:"));
    MONITOR_SERIAL.println(RADAR_RX_PIN);
    MONITOR_SERIAL.print(F("Connect LD2410 radar RX to GPIO:"));
    MONITOR_SERIAL.println(RADAR_TX_PIN);
    MONITOR_SERIAL.print(F("LD2410 radar sensor initialising: "));
    if (radar.begin(RADAR_SERIAL)) {
        MONITOR_SERIAL.println(F("OK"));
        MONITOR_SERIAL.print(F("LD2410 firmware version: "));
        MONITOR_SERIAL.print(radar.firmware_major_version);
        MONITOR_SERIAL.print('.');
        MONITOR_SERIAL.print(radar.firmware_minor_version);
        MONITOR_SERIAL.print('.');
        MONITOR_SERIAL.println(radar.firmware_bugfix_version, HEX);
    } else {
        MONITOR_SERIAL.println(F("not connected"));
    }

    if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("failed to start SSD1306 OLED"));
        while (1);
    }

    delay(2000);  // wait two seconds for initializing
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.display();
}

void connectToWiFi() {
    WiFiManager wifiManager;

    // Set the timeout to 3 minutes (180 seconds)
    wifiManager.setConfigPortalTimeout(180);

    // Create parameters for the WLED IP address and Zone number
    WiFiManagerParameter custom_wled_ip("wled_ip", "WLED IP Address", wledIP, 16);
    WiFiManagerParameter custom_zone_num("zone_num", "Zone Number", zoneNumStr, 4);
    wifiManager.addParameter(&custom_wled_ip);
    wifiManager.addParameter(&custom_zone_num);

    // Attempt to auto-connect to the Wi-Fi
    if (!wifiManager.autoConnect("ZoneDetector")) {
        Serial.println("Failed to connect and hit timeout");
        ESP.restart();  // Restart the ESP32 if it fails to connect
        delay(1000);
    }

    Serial.println("Connected to WiFi");

    // Copy the IP address and zone number set in the portal to the variables
    strcpy(wledIP, custom_wled_ip.getValue());
    strcpy(zoneNumStr, custom_zone_num.getValue());
    zone_num = atoi(zoneNumStr);  // Convert the zone number string to an integer

    Serial.print("WLED IP Address set to: ");
    Serial.println(wledIP);

    Serial.print("Zone Number set to: ");
    Serial.println(zone_num);

    // Save the values to EEPROM for persistence
    saveZoneAndWledToEEPROM();
}

void saveZoneAndWledToEEPROM() {
    // Save the WLED IP and Zone number to EEPROM for persistence
    for (int i = 0; i < 16; i++) {
        EEPROM.write(i, wledIP[i]);
    }
    EEPROM.write(16, zone_num);  // Save the zone number
    EEPROM.commit();  // Commit the changes
}

void loadZoneAndWledFromEEPROM() {
    // Load the WLED IP and Zone number from EEPROM
    for (int i = 0; i < 16; i++) {
        wledIP[i] = EEPROM.read(i);
    }
    zone_num = EEPROM.read(16);  // Load the zone number
    sprintf(zoneNumStr, "%d", zone_num);  // Update the zone number string
}

void controlWled(int brightness) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "http://" + String(wledIP) + "/win&T=2&A=" + String(brightness);
        http.begin(client, url);
        int httpCode = http.GET();
        http.end();
    } else {
        Serial.println("WiFi not connected");
    }
}

void loop() {
    oled.clearDisplay();
    oled.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    oled.setTextSize(1.6);
    oled.setCursor(10, 8);
    oled.println("Presence in Zone " + String(zone_num));  // Display the dynamic zone number
    oled.drawLine(0, 24, SCREEN_WIDTH, 24, WHITE);

    radar.read();
    if (radar.isConnected() && millis() - lastReading > 1000) {  // Report every 1000ms
        lastReading = millis();
        int distance = 0;
        if (radar.presenceDetected()) {
            if (radar.stationaryTargetDetected() && radar.stationaryTargetDistance() < zones_radius) {
                controlWled(0);  // Turn ON the WLED
                distance = radar.stationaryTargetDistance();
                oled.setTextSize(2);
                oled.setCursor(50, 30);
                oled.println("Yes");
                oled.fillCircle(110, 50, 10, WHITE);
                for (int i = 0; i < 360; i += 45) {
                    int x1 = 110 + 10 * cos(i * DEG_TO_RAD);
                    int y1 = 50 + 10 * sin(i * DEG_TO_RAD);
                    int x2 = 110 + 15 * cos(i * DEG_TO_RAD);
                    int y2 = 50 + 15 * sin(i * DEG_TO_RAD);
                    oled.drawLine(x1, y1, x2, y2, WHITE);
                }
                oled.display();
            } else if (radar.movingTargetDetected() && radar.movingTargetDistance() < zones_radius) {
                controlWled(0);  // Turn ON the WLED
                distance = radar.movingTargetDistance();
                oled.setTextSize(2);
                oled.setCursor(50, 30);
                oled.println("Yes");
                oled.fillCircle(110, 50, 10, WHITE);
                for (int i = 0; i < 360; i += 45) {
                    int x1 = 110 + 10 * cos(i * DEG_TO_RAD);
                    int y1 = 50 + 10 * sin(i * DEG_TO_RAD);
                    int x2 = 110 + 15 * cos(i * DEG_TO_RAD);
                    int y2 = 50 + 15 * sin(i * DEG_TO_RAD);
                    oled.drawLine(x1, y1, x2, y2, WHITE);
                }
                 oled.display();
            } else {
                controlWled(255);  // Turn OFF the WLED
                oled.setTextSize(2);
                oled.setCursor(50, 30);
                oled.println("No");
                oled.drawCircle(110, 50, 10, WHITE);
                oled.drawLine(105, 45, 115, 55, WHITE);
                oled.drawLine(105, 55, 115, 45, WHITE);
                oled.display();
            }
            Serial.print(F("Distance: "));
            Serial.print(distance);
            Serial.println(F("cm"));

            // Display distance in the smallest font at the bottom left corner of the screen
            oled.setTextSize(1);  // Smallest font size
            oled.setCursor(0, SCREEN_HEIGHT - 8);  // Bottom left corner
            oled.print("Dist: ");
            oled.print(distance);
            oled.print(" cm");
            oled.display();
        } else {
            Serial.println(F("No target"));
            controlWled(255);  // Turn OFF the WLED
            oled.setTextSize(2);
            oled.setCursor(50, 30);
            oled.println("No");
            oled.drawCircle(110, 50, 10, WHITE);
            oled.drawLine(105, 45, 115, 55, WHITE);
            oled.drawLine(105, 55, 115, 45, WHITE);
            oled.display();
        }
    }
}

