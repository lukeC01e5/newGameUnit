/*

trying to re-figure
so that rfid code matches new requirements

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

// Forward declarations if they're in a different .cpp and not included by the header:
void displayRFIDData(const RFIDData &data);
void displayRFIDParsed(const RFIDParsed &data);
void displayCreature(const Creature &creature);

// Make sure you declare variables you want to display:
// String pendingData; // Remove or comment this out
RFIDParsed parsedData;
Creature myCreature;

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
void add_5_coin(const String &customName);
// void displayRFIDParsed(const RFIDParsed &data);

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

void waitForCard()
{
    while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
    {
        delay(200);
        Serial.println("Waiting for RFID tag...");
    }
}

void loop()
{
    static bool initialized = false;

    if (!initialized)
    {
        waitForCard(); // Loop until card is detected
        Serial.println("Card detected");
        // Once a card is detected, read raw data from a specific block

        // In setup(), after detecting a new card:
        int myIntPart = 0;
        String myStrPart;

        // Use the new readFromRFID signature
        String rawData = readFromRFID(mfrc522, key, 1, myIntPart, myStrPart);

        Serial.println("[setup] RFID Data read on reboot:");
        Serial.println("intPart: " + String(myIntPart));
        Serial.println("strPart: " + myStrPart);

        // Check the bool values
        bool A = (myIntPart & 0x01) != 0;
        bool B = (myIntPart & 0x02) != 0;
        bool C = (myIntPart & 0x04) != 0;
        bool D = (myIntPart & 0x08) != 0;

        Serial.println("Bool A: " + String(A));
        Serial.println("Bool B: " + String(B));
        Serial.println("Bool C: " + String(C));
        Serial.println("Bool D: " + String(D));

        if (A && B && C && D)
        {
            allChallBools = true;
            Serial.println("[setup] allChallBools set to TRUE");
        }
        else
        {
            allChallBools = false;
            Serial.println("[setup] allChallBools set to FALSE");
        }

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
                ESP.restart();       // This line resets the ESP32
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

        // Example usage after reading:
        Serial.print("YearLevel: ");
        Serial.println(myCreature.yearLevel);
        Serial.print("ChallengeCode: ");
        Serial.println(myCreature.challengeCode);
        Serial.print("WrongGuesses: ");
        Serial.println(myCreature.wrongGuesses);
        Serial.print("BoolVal: ");
        Serial.println(myCreature.boolVal);

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

        // Assuming userId is available as myCreature.userId
        if (allChallBools)
        {
            add_5_coin(myCreature.customName);
            tft.println("5 Coin added");
            ////////////////////////////////////////////////
            displayCreature(myCreature);

            //     /*
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
            {
                // Re-authenticate or finalize any pending operations if needed
                bool success = clearChallBools(mfrc522, key, myCreature);
                Serial.println(success ? "[loop] clearChallBools success" : "[loop] clearChallBools failed");
            }
            //     */
            ////////////////////////////////////////////////
        }
        else
        {
            // tft.fillScreen(TFT_BLACK);
            // tft.setCursor(0, 0);
            tft.println("Challenges to be completed");
        }

        // Example call to clearChallBools
        // bool success = clearChallBools(mfrc522, key, myCreature);

        initialized = true;

        Serial.println("[loop] finishes here when challbool is empty");
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
                ESP.restart();       // This line resets the ESP32
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
    payload += "\"age\":" + String(creature.yearLevel) + ",";
    payload += "\"coins\":" + String(creature.challengeCode) + ",";
    payload += "\"creatureType\":" + String(creature.wrongGuesses) + ",";
    payload += "\"customName\":\"" + creature.customName + "\",";
    payload += "\"intVal\":" + String(creature.boolVal);
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
// ...existing code...
void handleFormSubmit(AsyncWebServerRequest *request)
{
    // Indicate that a form has been submitted, used by other parts of the program
    Serial.print("should change form bool here");
    formSubmitted = true;

    // Only proceed if the request is an HTTP POST
    if (request->method() == HTTP_POST)
    {
        // Ensure the required parameters are present
        if (request->hasParam("age", true) &&
            request->hasParam("coins", true) &&
            request->hasParam("wrong", true) &&
            request->hasParam("name", true) &&
            request->hasParam("creatureType", true))
        {
            // Extract form data from request and populate pendingData
            pendingData.yearLevel = request->getParam("age", true)->value().toInt();
            pendingData.challengeCode = request->getParam("coins", true)->value().toInt();
            pendingData.wrongGuesses = request->getParam("wrong", true)->value().toInt();
            pendingData.name = request->getParam("name", true)->value();

            // Assign creatureType to the global Creature object only
            myCreature.creatureType = request->getParam("creatureType", true)->value().toInt();

            // Convert checkbox values into a single encoded integer (encodeBools is defined elsewhere)
            bool A = request->hasParam("A", true);
            bool B = request->hasParam("B", true);
            bool C = request->hasParam("C", true);
            bool D = request->hasParam("D", true);
            pendingData.bools = encodeBools(A, B, C, D);
            /*
                        // If all checkboxes are true, set this special boolean flag
                        if (A && B && C && D) {
                            allChallBools = true;
                        }
            */
            // Debug output to the serial monitor
            Serial.println("[handleFormSubmit] Received NEW form data:");
            Serial.print(" Year Level: ");
            Serial.println(pendingData.yearLevel);
            Serial.print(" Challenge Code: ");
            Serial.println(pendingData.challengeCode);
            Serial.print(" Wrong Guesses: ");
            Serial.println(pendingData.wrongGuesses);
            Serial.print(" Bools (bin): ");
            Serial.println(pendingData.bools, BIN);
            Serial.print(" Name: ");
            Serial.println(pendingData.name);

            // Notify other code that new form data is ready to be processed
            dataPending = true;
            Serial.println("[handleFormSubmit] dataPending set to TRUE.");

            // Send a response to the client indicating success
            request->send(200, "text/html", "Data received. Please present your RFID card to complete write.");
        }
        else
        {
            // Inform the client that not all expected parameters were provided
            request->send(400, "text/html", "Missing parameters!");
        }
    }
}
// ...existing code...

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

void add_5_coin(const String &customName)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[add_5_coin] WiFi not connected.");
        return;
    }

    Serial.println("[add_5_coin] WiFi connected. Starting HTTP POST request...");

    WiFiClient wifiClient;
    HttpClient http(wifiClient, "gameapi-2e9bb6e38339.herokuapp.com", 80);

    // Build JSON payload
    String payload = "{";
    payload += "\"customName\":\"" + customName + "\"";
    payload += "}";

    Serial.println("[add_5_coin] Payload: " + payload);

    // Connect to the server
    if (wifiClient.connect("gameapi-2e9bb6e38339.herokuapp.com", 80))
    {
        Serial.println("[add_5_coin] Connected to server.");

        // Send HTTP POST request
        http.beginRequest();
        http.post("/api/v1/add_5_coin");
        http.sendHeader("Content-Type", "application/json");
        http.sendHeader("Content-Length", payload.length());
        http.beginBody();
        http.print(payload);
        http.endRequest();

        Serial.println("[add_5_coin] HTTP POST request sent. Waiting for response...");

        // Get the response
        int statusCode = http.responseStatusCode();
        String response = http.responseBody();

        if (statusCode > 0)
        {
            Serial.println("[add_5_coin] Response code: " + String(statusCode));
            Serial.println("[add_5_coin] Response: " + response);
        }
        else
        {
            Serial.print("[add_5_coin] Error, status code: ");
            Serial.println(statusCode);
        }

        // Close the connection
        wifiClient.stop();
    }
    else
    {
        Serial.println("[add_5_coin] Connection failed.");
    }
}

void displayRFIDData(const RFIDData &data)
{
    Serial.println("RFIDData:");
    Serial.println(" Name: " + data.name);
    Serial.println(" Year Level: " + String(data.yearLevel));
    Serial.println(" Challenge Code: " + String(data.challengeCode));
    Serial.println(" Wrong Guesses: " + String(data.wrongGuesses));
    Serial.println(" Bools: " + String(data.bools, BIN));
}

void displayRFIDParsed(const RFIDParsed &data)
{
    Serial.println("RFIDParsed:");
    Serial.println(" Year Level: " + String(data.yearLevel));
    Serial.println(" Challenge Code: " + String(data.challengeCode));
    Serial.println(" Wrong Guesses: " + String(data.wrongGuesses));
    Serial.println(" BoolVal: " + String(data.boolVal, BIN));
    Serial.println(" Name: " + data.name);
}

void displayCreature(const Creature &creature)
{
    Serial.println("Creature:");
    Serial.println(" TrainerAge: " + String(creature.yearLevel));
    Serial.println(" Coins: " + String(creature.challengeCode));
    Serial.println(" CreatureType: " + String(creature.wrongGuesses));
    Serial.println(" CustomName: " + creature.customName);
    Serial.println(" IntVal: " + String(creature.boolVal));
}
