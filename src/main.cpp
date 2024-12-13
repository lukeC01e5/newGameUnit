#include "globals.h"
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "RFIDFunctions.h"

// Include the file with the WiFi credentials
#include "arduino_secrets.h"

// Pin definitions for the RFID module
#define SS_PIN 13  // Slave Select pin for RFID
#define RST_PIN 22 // Reset pin for RFID

// Global instances
TFT_eSPI tft = TFT_eSPI();
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// WiFi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// AsyncWebServer instance
AsyncWebServer server(80);

// Global variables for storing form data
String name = "";
String ageStr = "";
String type = "";
bool dataReceived = false;

void setup()
{
    // Initialize Serial Monitor
    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Initialize TFT display
    tft.init();
    tft.setRotation(1); // Adjust rotation as needed
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Initializing...");

    // Initialize RFID reader
    SPI.begin(25, 33, 26); // Initialize SPI with SCK=25, MISO=33, MOSI=26
    mfrc522.PCD_Init();
    Serial.println("RFID reader initialized.");
    tft.println("RFID Initialized");

    // Prepare the MIFARE key (default key)
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        tft.println("SPIFFS Mount Failed");
        return;
    }

    // Connect to WiFi
    WiFi.begin(ssid, pass);
    int wifi_attempts = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
        tft.print(".");
        wifi_attempts++;
        if (wifi_attempts > 20)
        {
            Serial.println("Failed to connect to WiFi");
            tft.println("No connection");
            return;
        }
    }
    Serial.println("Connected to WiFi");
    tft.println("WiFi OK");

    // Display IP address
    String ipAddress = WiFi.localIP().toString();
    Serial.print("IP Address: ");
    Serial.println(ipAddress);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("IP Address:");
    tft.println(ipAddress);

    // Setup server routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        Serial.println("Serving /index.html");
        request->send(SPIFFS, "/index.html", "text/html"); });

    server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        // Initialize variables for all expected fields
        bool hasName = false, hasAge = false, hasType = false;

        // Extract name
        if (request->hasParam("name", true)) {
            name = request->getParam("name", true)->value();
            hasName = true;
        }

        // Extract age
        if (request->hasParam("age", true)) {
            ageStr = request->getParam("age", true)->value();
            hasAge = true;
        }

        // Extract character type
        if (request->hasParam("type", true)) {
            type = request->getParam("type", true)->value();
            hasType = true;
        }

        if (hasName && hasAge && hasType) {
            dataReceived = true;
            Serial.println("Data received from web form:");
            Serial.println("Name: " + name);
            Serial.println("Age: " + ageStr);
            Serial.println("Type: " + type);
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Data received:");
            tft.println("Name: " + name);
            tft.println("Age: " + ageStr);
            tft.println("Type: " + type);
            request->send(200, "text/plain", "Data received. Please present your RFID card.");
        } else {
            Serial.println("Incomplete data received");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Incomplete data");
            request->send(400, "text/plain", "Incomplete data received");
        } });

    server.begin();
    Serial.println("HTTP server started");

    // Initial RFID card detection
    Serial.println("Waiting for RFID card...");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Place RFID card");
    tft.println("to start");

    // Wait for an RFID card to be present
    while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        delay(50);
    }

    // Read data from the RFID card
    String block1Data = readFromRFID(1);
    String block2Data = readFromRFID(2);
    String readData = block1Data + block2Data;

    if (readData != "")
    {
        Serial.print("Data read from RFID card: ");
        Serial.println(readData);
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Read Data:");
        tft.println(readData);

        // Parse the data
        RFIDData rfidData;
        parseRFIDData(readData, rfidData);

        // Display parsed data
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("RFID Data:");
        tft.println("Name: " + rfidData.name);
        tft.println("Age: " + String(rfidData.age));
        tft.println("Type: " + rfidData.characterType);
    }
    else
    {
        Serial.println("No data read from RFID card");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("No data read");
        tft.println("from RFID card");
    }

    // Halt PICC and stop encryption
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void loop()
{
    // Handle any data received from the web interface
    if (dataReceived)
    {
        // Reinitialize RFID module before each write operation
        Serial.println("Reinitializing RFID module...");
        mfrc522.PCD_Init();

        // Wait for an RFID card
        Serial.println("Ready to write to RFID card. Place the card now.");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Place RFID card");
        tft.println("to write data");

        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {
            delay(50);
        }

        // Prepare data
        String block1Data = "*&" + name + "." + ageStr + "&*";
        String block2Data = "*&" + type + "&*";

        // Ensure data fits into 16 bytes
        if (block1Data.length() > 16)
        {
            block1Data = block1Data.substring(0, 16);
        }
        if (block2Data.length() > 16)
        {
            block2Data = block2Data.substring(0, 16);
        }

        // Write to block 1
        if (writeToRFID(block1Data, 1))
        {
            Serial.println("Data written to block 1");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Data written to");
            tft.println("block 1");
        }
        else
        {
            Serial.println("Failed to write to block 1");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Write to block 1 failed");
        }

        // Write to block 2
        if (writeToRFID(block2Data, 2))
        {
            Serial.println("Data written to block 2");
            tft.println("Data written to");
            tft.println("block 2");
        }
        else
        {
            Serial.println("Failed to write to block 2");
            tft.println("Write to block 2 failed");
        }

        // Reset flags
        dataReceived = false;
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }
    else
    {
        // Check for RFID card to read
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            Serial.println("RFID tag detected");

            // Read data from block 1
            String block1Data = readFromRFID(1);

            // Read data from block 2
            String block2Data = readFromRFID(2);

            // Combine data
            String readData = block1Data + block2Data;

            // Process the combined data
            if (readData != "")
            {
                Serial.print("Data read from RFID card: ");
                Serial.println(readData);

                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("Read Data:");
                tft.println(readData);

                // Parse the data
                RFIDData rfidData;
                parseRFIDData(readData, rfidData);

                // Display parsed data
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("RFID Data:");
                tft.println("Name: " + rfidData.name);
                tft.println("Age: " + String(rfidData.age));
                tft.println("Type: " + rfidData.characterType);
            }
            else
            {
                Serial.println("No data read from RFID card");
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("No data read");
                tft.println("from RFID card");
            }

            // Halt the PICC and stop encryption
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();

            // Wait for card removal
            while (mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial())
            {
                delay(50);
            }
        }
    }

    // Small delay to prevent overwhelming the CPU
    delay(50);
}