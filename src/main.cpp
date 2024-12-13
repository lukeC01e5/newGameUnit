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

AsyncWebServer server(80); // Create AsyncWebServer object on port 80

// WiFi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

String inputData = "";     // Variable to store input data from the web page
bool dataReceived = false; // Flag to indicate data has been received

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

    // Prepare the MIFARE Key (assuming default key A)
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
        String name = "";
        String ageStr = "";
        String type = "";
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
            // Format the data string as expected by parseRFIDData
            String formattedData = "name:" + name + "||&age:" + ageStr + "||&characterType:" + type + "||&";
            inputData = formattedData;
            dataReceived = true;
            Serial.println("Data received: " + inputData);
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Data received:");
            tft.println(inputData);
            request->send(200, "text/plain", "Data received and will be written to the RFID card.");
        } else {
            Serial.println("Incomplete data received");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Incomplete data received");
            request->send(400, "text/plain", "Incomplete data received");
        } });

    server.begin();
    Serial.println("HTTP server started");
}

void loop()
{
    // Wait until data is received
    if (dataReceived)
    {
        // Reinitialize RFID module before each write operation
        Serial.println("Reinitializing RFID module...");
        mfrc522.PCD_Init();

        // Wait for an RFID card
        Serial.println("Waiting for RFID card...");
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {
            delay(50);
        }

        // Write data to block 1
        Serial.print("Writing to block 1: ");
        Serial.println(inputData);

        if (writeToRFID(inputData, 1))
        {
            Serial.println("Data written to RFID card");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Data written to");
            tft.println("RFID card");
        }
        else
        {
            Serial.println("Failed to write data to RFID card");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Write failed");
        }

        // Read back the data to verify
        String readData = readFromRFID(1);
        if (readData != "")
        {
            Serial.print("Data read from RFID card: ");
            Serial.println(readData);
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Read Data:");
            tft.println(readData);

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
        }

        // Reset flags
        dataReceived = false;
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }
}