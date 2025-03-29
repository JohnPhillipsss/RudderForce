#include <SPI.h>
#include <LoRa.h>
#include <HX711_ADC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// LoRa Module Pins (Adjust for your board)
#define SS 18
#define RST 23
#define DIO0 26

// HX711 Pins
const int HX711_dout = 13;
const int HX711_sck = 14;
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Calibration factor for load cell
const float calibrationFactor = 64.15;

void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println("Starting Sender...");

    // Initialize OLED Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LoRa Sender...");
    display.display();

    // Initialize LoRa
    SPI.begin(5, 19, 27, SS);  // (SCK, MISO, MOSI, SS)
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(915E6)) {  // Set frequency to 833 MHz
        Serial.println("LoRa init failed. Check connections.");
        while (1);
    }

    // Optimize LoRa Settings
    LoRa.setSpreadingFactor(7);  
    LoRa.setSignalBandwidth(250E3);
    LoRa.setCodingRate4(5);

    Serial.println("LoRa Sender Ready");
    display.setCursor(0, 10);
    display.println("LoRa Ready!");
    display.display();

    // Initialize HX711
    LoadCell.begin();
    LoadCell.start(2000, true);  // Stabilizing time: 2s

    if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
        Serial.println("HX711 Timeout! Check wiring.");
        display.clearDisplay();
        display.setCursor(0, 20);
        display.println("HX711 Timeout!");
        display.display();
        while (1);
    } else {
        LoadCell.setCalFactor(calibrationFactor);
        Serial.println("HX711 initialized.");
    }
}

void loop() {
    static bool newDataReady = false;
    const int sendInterval = 100; // 100ms interval

    if (LoadCell.update()) {
        newDataReady = true;
    }

    if (newDataReady) {
        float weight = LoadCell.getData();

        // Send weight via LoRa
        LoRa.beginPacket();
        LoRa.print(weight);
        LoRa.endPacket();

        Serial.print("Sent Weight: ");
        Serial.print(weight);
        Serial.println(" g");

        // Display on OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.println("Sent Weight:");
        display.setTextSize(2);
        display.setCursor(10, 20);
        display.print(weight);
        display.println(" g");
        display.display();

        newDataReady = false;
        delay(sendInterval);  // Control transmission rate
    }
}
