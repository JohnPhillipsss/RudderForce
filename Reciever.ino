#include <SPI.h>
#include <LoRa.h>
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

void setup() {
    Serial.begin(115200);
    while (!Serial);
    
    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        while (1);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("LoRa Receiver...");
    display.display();
    
    // Initialize LoRa
    SPI.begin(5, 19, 27, SS);  // (SCK, MISO, MOSI, SS)
    LoRa.setPins(SS, RST, DIO0);
    
    if (!LoRa.begin(915E6)) {  // Set frequency to 833 MHz
        Serial.println("LoRa init failed. Check connections.");
        while (1);
    }

    // Optimize LoRa for faster data
    LoRa.setSpreadingFactor(7);  
    LoRa.setSignalBandwidth(250E3);
    LoRa.setCodingRate4(5);

    Serial.println("LoRa Receiver Ready");
    display.setCursor(0, 10);
    display.println("LoRa Ready!");
    display.display();
}

void loop() {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        String receivedData = "";
        while (LoRa.available()) {
            receivedData += (char)LoRa.read();
        }

        float receivedWeight = receivedData.toFloat(); // Convert to float

        Serial.print("Received Weight: ");
        Serial.print(receivedWeight);
        Serial.println(" g");

        // Display on OLED
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("Received Weight:");
        display.setTextSize(2);
        display.setCursor(10, 20);
        display.print(receivedWeight);
        display.println(" g");
        display.display();
    }
}
