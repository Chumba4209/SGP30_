
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// I2C Configuration

#define SDA_PIN 8
#define SCL_PIN 10

#define SGP30_ADDR 0x58
#define OLED_ADDR  0x3C


// OLED Configuration

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);


// SGP30 Configuration

Adafruit_SGP30 sgp;

bool oledReady = false;
bool sensorReady = false;

unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 1000;


// I2C Scanner

void scanI2C() {

    Serial.println("\nScanning I2C bus...");

    int devices = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {

        Wire.beginTransmission(addr);

        if (Wire.endTransmission() == 0) {

            Serial.printf(
                "Device found at 0x%02X\n",
                addr
            );

            devices++;
        }
    }

    Serial.printf("Total devices: %d\n", devices);
}


// OLED Message

void showMessage(const char* message) {

    if (!oledReady) return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("SGP30 AIR QUALITY");

    display.drawLine(
        0, 12, 127, 12, SSD1306_WHITE
    );

    display.setCursor(0, 25);
    display.println(message);

    display.display();
}


// Display Sensor Values

void showReadings(uint16_t eco2, uint16_t tvoc) {

    if (!oledReady) return;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Header
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SGP30 AIR QUALITY");

    display.drawLine(
        0, 12, 127, 12, SSD1306_WHITE
    );

    // eCO2
    display.setTextSize(1);
    display.setCursor(0, 17);
    display.println("eCO2:");

    display.setTextSize(2);
    display.setCursor(0, 27);
    display.print(eco2);

    display.setTextSize(1);
    display.print(" ppm");

    // TVOC
    display.setTextSize(1);
    display.setCursor(0, 46);
    display.println("TVOC:");

    display.setTextSize(1);
    display.setCursor(40, 46);
    display.print(tvoc);
    display.print(" ppb");

    display.display();
}


// Setup

void setup() {

    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.println("");
    Serial.println(" ESP32-C3 + SGP30 + OLED");
    Serial.println("");

    // Initialize I2C
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(100000);
    Wire.setTimeOut(100);

    Serial.println("[I2C] Initialized");
    Serial.println("SDA: GPIO8");
    Serial.println("SCL: GPIO10");

    // Scan devices
    scanI2C();

    // Initialize OLED
    oledReady = display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDR,
        true,
        false
    );

    if (oledReady) {

        Serial.println("[OLED] Ready");
        showMessage("Initializing...");

    } else {

        Serial.println("[OLED] Not detected");
    }

    // Initialize SGP30
    sensorReady = sgp.begin(&Wire);

    if (sensorReady) {

        Serial.println("[SGP30] Sensor detected");
        Serial.println("[SGP30] Initialized");

        showMessage("Sensor Ready!");

    } else {

        Serial.println("[SGP30] Sensor not detected");

        showMessage("SGP30 NOT FOUND!");
    }

    delay(1000);
}


// Main Loop

void loop() {

    // Keep approximately 1-second
    // measurement interval
    unsigned long now = millis();

    if (now - lastRead < READ_INTERVAL) {
        return;
    }

    lastRead = now;

    if (!sensorReady) {

        static unsigned long lastError = 0;

        if (now - lastError >= 3000) {
            lastError = now;
            Serial.println("[SGP30] Sensor unavailable");
        }

        return;
    }

    // Measure air quality
    if (!sgp.IAQmeasure()) {

        Serial.println("[SGP30] Measurement failed");
        showMessage("Reading error!");

        return;
    }

    uint16_t eco2 = sgp.eCO2;
    uint16_t tvoc = sgp.TVOC;

    
    // Serial Monitor
    
    Serial.println();
    Serial.println("------ SGP30 DATA ------");

    Serial.printf(
        "eCO2 : %u ppm\n",
        eco2
    );

    Serial.printf(
        "TVOC : %u ppb\n",
        tvoc
    );

    Serial.println("------------------------");

    
    // OLED Display
    
    showReadings(eco2, tvoc);
}
