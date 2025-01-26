/*

have makrked sticking point with ////////////////////////////////////////////////////


*/

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MFRC522.h>
#include "RFIDData.h"
#include "arduino_secrets.h"
#include "GlobalDefs.h"
#include <HTTPClient.h>

// Create the AsyncWebServer on port 80
AsyncWebServer server(80);

// WiFi credentials
const char *ssid = SECRET_SSID;
const char *pass = SECRET_PASS;

bool tagDetected = false;
bool serverRunning = false;
bool newCreature = false;

RFIDData rfidData;
Creature creature;

// Creature list
const char *creatures[35] = {
    "No Creature", "Flamingo", "Flame-Kingo", "Kitten", "Flame-on", "Pup",
    "Dog", "Wolf", "Birdy", "Haast-eagle", "Squidy", "Giant-Squid",
    "Kraken", "BabyShark", "Shark", "Megalodon", "Tadpole",
    "Bush-Monster", "Baby-Dragon", "Dragon", "Dino-Egg", "T-Rex",
    "Baby-Ray", "Mega-Manta", "Orca", "Big-Bitey", "Flame-Lily",
    "Monster-Lily", "Bear-Cub", "Moss-Bear"};

// Track the last card UID
MFRC522::Uid lastCardUid;
// Track RFID card presence and profile status
bool cardPresent = false;
bool hasCreature = false;
bool lastCardPresent = false;
bool lastHasCreature = false;
bool cardProcessing = false;
// bool formSubmitted = false; // Have exactly one definition
//  ...existing code...

// Global WiFiClient
WiFiClient client;

// Function prototypes
void listSPIFFSFiles();
void handleFormSubmit(AsyncWebServerRequest *request);
void startWebServer();
void stopWebServer();
bool uidsMatch(MFRC522::Uid uid1, MFRC522::Uid uid2);
void copyUid(MFRC522::Uid &dest, MFRC522::Uid &src);
void clearUid(MFRC522::Uid &uid);
bool writeToRFID(const String &data, byte blockAddr);
void createOrUpdateUserOnServer();
void exampleReadAndDecode(const String &rawRFID);
void handleFormSubmit(AsyncWebServerRequest *request);
void startWebServer();
void clearUid(MFRC522::Uid &uid);

bool sendCreatureToDatabase(const Creature &creature);
bool checkForCreature(const Creature &creature);

// Setup
void setup()
{
    Serial.begin(115200);
    tft.init();
    tft.setRotation(1);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("TFT Initialized");

    // Initialize SPI and RFID
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

    startWebServer(); // Actually start the server from setup

    Serial.println("Setup complete. Waiting for RFID tag...");
    tft.println("Waiting for RFID...");

    Serial.println("[setup] Place an RFID card now to read...");
}

void loop()
{
    static bool initialized = false;

    if (!initialized)
    {
        // Wait until a card is presented (optional, but ensures a single read at startup)
        while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
        {
            delay(200);
        }
        Serial.println("card detected");
        // Once a card is detected, read raw data from a specific block

        // In setup(), after detecting a new card:
        int myIntPart = 0;
        String myStrPart;

        // Use the new readFromRFID signature
        String rawData = readFromRFID(mfrc522, key, 1, myIntPart, myStrPart);

        // If valid data came back, mark cardPresent true
        if (rawData.indexOf('%') != -1)
        {
            // Card appears to have valid data
            cardPresent = true;
            Serial.println("[loop] Found '%', continuing...");
        }
        else
        {
            cardPresent = true;
            tft.println("blank profile");

            while (!formSubmitted)
            {
                delay(100); // Small delay to prevent overwhelming the CPU
            }

            if (writeRFIDData(mfrc522, key, pendingData))
            {
                Serial.println("Write succeeded!");
                tft.println("Write Succeeded!");
                dataPending = false; // Clear pending state
                hasCreature = true;  // Profile now exists
                ESP.restart();
            }
            else
            {
                Serial.println("Write failed!");
                tft.println("Write Failed!");
                hasCreature = false;
            }

            // Reset the formSubmitted flag
            formSubmitted = false;

            // Halt the card and stop encryption
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            tft.println("end of writeing new card");
        }

        Serial.println("and here??");
        Serial.println(rawData);

        // Show the split parts
        Serial.print("Integer part: ");
        Serial.println(myIntPart);
        Serial.print("String part: ");
        Serial.println(myStrPart);

        // Decode it into a Creature
        Creature myCreature = decode(myIntPart, myStrPart);

        // if customName is not empty, hasCreature = true
        hasCreature = (myCreature.customName.length() > 0);

        checkForCreature(myCreature);

        if (newCreature == true)
        {
            sendCreatureToDatabase(myCreature);
        }
        else
        {
            Serial.println("Creature already exists." + myCreature.customName);
            // return; // Exit the function early
        }

        // Show on TFT display
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Hello " + myCreature.customName);

        // Halt card so it won’t continue reading
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        Serial.println("makes it to here");

        initialized = true;
    }

    // Handle Writing RFID Data
    if (dataPending)
    {
        if (!cardProcessing && mfrc522.PICC_ReadCardSerial())
        {
            cardProcessing = true; // Start processing the card
            cardPresent = true;    // Card is present

            // Write pending data to RFID
            if (writeRFIDData(mfrc522, key, pendingData))
            {
                Serial.println("Write succeeded!");
                tft.println("Write Succeeded!");
                dataPending = false; // Clear pending state
                hasCreature = true;  // Profile now exists
                ESP.restart();
            }
            else
            {
                Serial.println("Write failed!");
                tft.println("Write Failed!");
                hasCreature = false;
            }

            // Halt the card and stop encryption
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            cardProcessing = false;
            cardPresent = false;
        }
        else if (!mfrc522.PICC_IsNewCardPresent())
        {
            // Card removed while waiting to write
            cardProcessing = false;
            cardPresent = false;
            hasCreature = false;
        }

        delay(100);
        return; // Exit early since we're handling writing
    }
    delay(100);
}

// Example function to write raw data to block
bool writeToRFID(const String &data, byte blockAddr)
{
    MFRC522::StatusCode status;

    // Authenticate with the card (using key A)
    byte trailerBlock = (blockAddr / 4) * 4 + 3; // Trailer block for the sector

    // Default key for MIFARE cards (0xFF)
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    Serial.print("Authenticating with trailer block ");
    Serial.println(trailerBlock);

    // Select the card
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        Serial.println("No card selected or failed to read card serial.");
        return false;
    }

    // Authenticate
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    Serial.println("Authentication successful");

    // Prepare data (must be 16 bytes)
    byte dataBlock[16];
    memset(dataBlock, 0, sizeof(dataBlock)); // Clear the array
    data.toCharArray((char *)dataBlock, 16); // Convert string to char array

    // Write data to the card
    Serial.print("Writing to block ");
    Serial.println(blockAddr);
    status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("MIFARE_Write() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    Serial.println("Write successful");

    // Halt PICC and stop encryption on PCD
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return true;
}

// Function to send decoded creature data to your Flask API
bool sendCreatureToDatabase(const Creature &creature)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected.");
        return false;
    }

    Serial.println("WiFi connected. Starting HTTP POST request...");

    WiFiClient wifiClient;
    HttpClient http(wifiClient, "gameapi-2e9bb6e38339.herokuapp.com", 80);

    // Build JSON payload
    String payload = "{";
    payload += "\"age\":" + String(creature.trainerAge) + ",";
    payload += "\"coins\":" + String(creature.coins) + ",";
    payload += "\"creatureType\":" + String(creature.creatureType) + ",";
    payload += "\"customName\":\"" + creature.customName + "\",";
    payload += "\"intVal\":" + String(creature.intVal);
    payload += "}";

    Serial.println("Payload: " + payload);

    // Connect to the server
    if (wifiClient.connect("gameapi-2e9bb6e38339.herokuapp.com", 80))
    {
        Serial.println("Connected to server.");

        // Send HTTP POST request
        http.beginRequest();
        http.post("/api/v1/create_user_from_rfid");
        http.sendHeader("Content-Type", "application/json");
        http.sendHeader("Content-Length", payload.length());
        http.beginBody();
        http.print(payload);
        http.endRequest();

        Serial.println("HTTP POST request sent. Waiting for response...");

        // Get the response status code
        int statusCode = http.responseStatusCode();
        String response = http.responseBody();

        if (statusCode > 0)
        {
            Serial.println("POST request sent successfully.");
            Serial.println("Response code: " + String(statusCode));
            Serial.println("Response: " + response);
        }
        else
        {
            Serial.print("Error: ");
            Serial.println(statusCode);
        }

        // Close the connection
        wifiClient.stop();
        return (statusCode == 201);
    }
    else
    {
        Serial.println("Connection failed.");
        return false;
    }
}

// Function to check if a creature is already in the database
bool checkForCreature(const Creature &creature)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi not connected.");
        return false;
    }

    Serial.println("WiFi connected. Starting HTTP GET request...");

    WiFiClient wifiClient;
    HttpClient http(wifiClient, "gameapi-2e9bb6e38339.herokuapp.com", 80);

    // Send HTTP GET request
    http.beginRequest();
    http.get("/api/v1/get_custom_names");
    http.sendHeader("Content-Type", "application/json");
    http.endRequest();

    Serial.println("HTTP GET request sent. Waiting for response...");

    // Get the response status code
    int statusCode = http.responseStatusCode();
    String response = http.responseBody();

    if (statusCode > 0)
    {
        Serial.println("GET request sent successfully.");
        Serial.println("Response code: " + String(statusCode));
        Serial.println("Response: " + response);

        // Check if the customName is in the response
        if (response.indexOf("\"" + creature.customName + "\"") == -1)
        {
            newCreature = true;
            Serial.println("New creature detected: " + creature.customName);
        }
        else
        {
            newCreature = false;
            Serial.println("creature already exists: " + creature.customName);
        }
    }
    else
    {
        Serial.print("Error: ");
        Serial.println(statusCode);
    }

    // Close the connection
    wifiClient.stop();
    return (statusCode == 200);
}

void handleFormSubmit(AsyncWebServerRequest *request)
{
    Serial.print("should change form bool here");
    formSubmitted = true;
    if (request->method() == HTTP_POST)
    {
        // Check parameters
        if (request->hasParam("age", true) &&
            request->hasParam("coins", true) &&
            request->hasParam("creatureType", true) &&
            request->hasParam("name", true))
        {
            // Populate pendingData from POST params
            pendingData.age = request->getParam("age", true)->value().toInt();
            pendingData.coins = request->getParam("coins", true)->value().toInt();
            pendingData.creatureType = request->getParam("creatureType", true)->value().toInt();
            pendingData.name = request->getParam("name", true)->value();

            // Parse booleans
            bool A = request->hasParam("A", true);
            bool B = request->hasParam("B", true);
            bool C = request->hasParam("C", true);
            bool D = request->hasParam("D", true);
            pendingData.bools = encodeBools(A, B, C, D);

            // Debug prints
            Serial.println("[handleFormSubmit] Received NEW form data:");
            Serial.print(" Age: ");
            Serial.println(pendingData.age);
            Serial.print(" Coins: ");
            Serial.println(pendingData.coins);
            Serial.print(" CreatureType: ");
            Serial.println(pendingData.creatureType);
            Serial.print(" Bools (bin): ");
            Serial.println(pendingData.bools, BIN);
            Serial.print(" Name: ");
            Serial.println(pendingData.name);

            // Mark as pending
            dataPending = true;
            Serial.println("[handleFormSubmit] dataPending set to TRUE.");

            // Respond to client
            request->send(200, "text/html", "Data received. Please present your RFID card to complete write.");
        }
        else
        {
            request->send(400, "text/html", "Missing parameters!");
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
            // Build JSON with cardPresent and hasCreature
            String response = "{";
            response += "\"cardPresent\":" + String(cardPresent ? "true" : "false") + ",";
            response += "\"hasCreature\":" + String(hasCreature ? "true" : "false") + "}";
            request->send(200, "application/json", response); });

        // Start the server
        server.begin();
        serverRunning = true;
        Serial.println("Web server started.");
        tft.println("Web Server Running");
        tft.print("IP: ");
        tft.println(WiFi.localIP());
    }
}

// Clear a UID
void clearUid(MFRC522::Uid &uid)
{
    uid.size = 0;
    for (byte i = 0; i < sizeof(uid.uidByte); i++)
    {
        uid.uidByte[i] = 0;
    }
}