#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "GlobalDefs.h"
#include "RFIDData.h"
#include "Creature.h"
#include "arduino_secrets.h"

// Globals
AsyncWebServer server(80);
bool serverRunning = false;
RFIDData rfidData;
Creature creature;
WiFiClient client;

bool dataPending = false;
bool cardProcessing = false;

// Initialize RFID reader with defined SS_PIN and RST_PIN
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Initialize TFT display (ensure it's declared globally)
TFT_eSPI tft = TFT_eSPI(); // Make sure this matches your GlobalDefs.h declaration

// Function Prototypes
void listSPIFFSFiles();
void handleFormSubmit(AsyncWebServerRequest *request);
void startWebServer();
bool uidsMatch(MFRC522::Uid uid1, MFRC522::Uid uid2);
void copyUid(MFRC522::Uid &dest, MFRC522::Uid &src);
void clearUid(MFRC522::Uid &uid);
bool writeToRFID(const String &data, byte blockAddr);
bool writeRFIDData(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const RFIDData &data);

// Handle form submission
void handleFormSubmit(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_POST)
    {
        // Ensure all required parameters are present
        if (request->hasParam("age", true) &&
            request->hasParam("coins", true) &&
            request->hasParam("creatureType", true) &&
            request->hasParam("name", true))
        {

            // Parse form parameters
            rfidData.age = request->getParam("age", true)->value().toInt();
            rfidData.coins = request->getParam("coins", true)->value().toInt();
            rfidData.creatureType = request->getParam("creatureType", true)->value().toInt();
            rfidData.name = request->getParam("name", true)->value();

            // Parse checkbox values
            bool A = request->hasParam("A", true) ? true : false;
            bool B = request->hasParam("B", true) ? true : false;
            bool C = request->hasParam("C", true) ? true : false;
            bool D = request->hasParam("D", true) ? true : false;
            rfidData.bools = encodeBools(A, B, C, D);

            // Set flag to indicate data should be written to RFID
            dataPending = true;

            // Respond to the client immediately
            String htmlResponse = "<!DOCTYPE html><html><head><title>Submission Successful</title>";
            htmlResponse += "<meta http-equiv=\"refresh\" content=\"5; url=/\" />";
            htmlResponse += "</head><body>";
            htmlResponse += "<h2>Data Received</h2>";
            htmlResponse += "<p>Your data has been received. Please present your RFID card to write the data.</p>";
            htmlResponse += "<p>You will be redirected to the landing page in 5 seconds.</p>";
            htmlResponse += "<button onclick=\"window.location.href='/'\">Back to Landing Page</button>";
            htmlResponse += "</body></html>";

            request->send(200, "text/html", htmlResponse);
        }
        else
        {
            request->send(400, "text/plain", "Bad Request: Missing parameters");
        }
    }
}

// Start the web server
void startWebServer()
{
    if (!serverRunning)
    {
        // Register your routes
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            if (SPIFFS.exists("/landing.html")) {
                request->send(SPIFFS, "/landing.html", "text/html");
            } else {
                request->send(404, "text/plain", "File Not Found");
            } });

        server.on("/editProfile", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            if (SPIFFS.exists("/index.html")) {
                request->send(SPIFFS, "/index.html", "text/html");
            } else {
                request->send(404, "text/plain", "File Not Found");
            } });

        // Handle form submission
        server.on("/submit", HTTP_POST, handleFormSubmit);

        // Endpoint to check creature status
        server.on("/creatureStatus", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            bool hasCreature = !creature.creatureName.isEmpty();
            String jsonResponse = "{\"hasCreature\": " + String(hasCreature ? "true" : "false") + "}";
            request->send(200, "application/json", jsonResponse); });

        // Start the server
        server.begin();
        serverRunning = true;
        Serial.println("Web server started.");
        tft.println("Web Server Running");
        tft.print("IP: ");
        tft.println(WiFi.localIP());
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000); // Allow time for serial monitor to initialize

    // Initialize TFT display
    tft.init();
    tft.setRotation(1);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("TFT Initialized");

    // Initialize SPI and RFID reader
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    mfrc522.PCD_Init();
    Serial.println("RFID Initialized");
    tft.println("RFID Initialized");

    // Set default key for RFID
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Initialize SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS mount failed");
        tft.println("SPIFFS Mount Failed");
        return;
    }
    Serial.println("SPIFFS mounted successfully");
    tft.println("SPIFFS Initialized");
    listSPIFFSFiles();

    // Connect to Wi-Fi
    WiFi.begin(ssid, pass);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    tft.setCursor(0, 20);
    tft.print("IP: ");
    tft.println(WiFi.localIP());

    Serial.println("Setup complete. Waiting for RFID tag...");
    tft.println("Waiting for RFID...");

    // Start the web server
    startWebServer();
}

void loop()
{
    // Handle writing data to RFID if pending
    if (dataPending)
    {
        // Check if a card is present and not already being processed
        if (!cardProcessing && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            cardProcessing = true; // Start processing the card

            // Attempt to write data
            if (writeRFIDData(mfrc522, key, rfidData))
            {
                Serial.println("Write succeeded!");
                dataPending = false;
                tft.println("Write Succeeded!");
            }
            else
            {
                Serial.println("Write failed!");
                tft.println("Write Failed!");
            }

            // Halt and stop encryption to allow new operations
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }
        // If no card is present, reset the processing flag
        else if (!mfrc522.PICC_IsNewCardPresent())
        {
            cardProcessing = false;
        }

        // Small delay to prevent CPU hogging
        delay(100);
        return; // Exit loop to prevent interfering with server
    }

    // Normal RFID reading operation
    if (!cardProcessing && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
        cardProcessing = true;
        Serial.println("New RFID card detected!");

        String rawData = readFromRFID(mfrc522, key, 1);
        Serial.print("Raw Data: ");
        Serial.println(rawData);

        if (rawData.length() > 0)
        {
            parseRFIDData(rawData, rfidData);
            // Optionally, handle creature-related logic here
            // Avoid starting/stopping the server again
        }
        else
        {
            Serial.println("Failed to read data from RFID.");
        }

        // Halt and stop encryption after read
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }
    else
    {
        // If a card was previously present but now removed
        if (cardProcessing && !mfrc522.PICC_IsNewCardPresent())
        {
            cardProcessing = false;
            Serial.println("RFID card removed.");
            clearUid(lastCardUid);
        }
    }

    // Brief delay to free up CPU cycles
    delay(100);
}

// Example implementation of writeRFIDData
bool writeRFIDData(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const RFIDData &data)
{
    // Create payload
    char buffer[15] = {0}; // 14 chars + null terminator
    snprintf(buffer, sizeof(buffer), "%02d%02d%02d%02d%%%s",
             data.age,
             data.coins,
             data.creatureType,
             data.bools,
             data.name.substring(0, 6).c_str());
    String payload = String(buffer);

    Serial.print("writeRFIDData - Payload: ");
    Serial.println(payload);

    // Attempt to write to block 1
    MFRC522::StatusCode status;
    byte trailerBlock = (1 / 4) * 4 + 3;

    // Authenticate using Key A
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                      trailerBlock,
                                      &key,
                                      &mfrc522.uid);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    // Prepare data block
    byte dataBlock[16];
    memset(dataBlock, 0, 16);
    for (int i = 0; i < 16 && i < (int)payload.length(); i++)
    {
        dataBlock[i] = payload.charAt(i);
    }

    // Write to RFID
    status = mfrc522.MIFARE_Write(1, dataBlock, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Write failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    // Stop encryption
    mfrc522.PCD_StopCrypto1();
    return true;
}

// Implement other required functions (listSPIFFSFiles, uidsMatch, copyUid, clearUid, parseRFIDData)
// Ensure these functions are efficient and do not block the main loop