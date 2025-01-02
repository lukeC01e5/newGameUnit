#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MFRC522.h>
#include "RFIDData.h"
#include "Creature.h"

// Include the file with the WiFi credentials
#include "arduino_secrets.h"

// Pin definitions for the RFID module
#define SS_PIN 13  // Slave Select pin for RFID
#define RST_PIN 22 // Reset pin for RFID

TFT_eSPI tft = TFT_eSPI();        // Create TFT instance
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::MIFARE_Key key;          // Create MIFARE_Key instance

AsyncWebServer server(80); // Create AsyncWebServer object on port 80

// WiFi credentials
const char *ssid = SECRET_SSID;
const char *pass = SECRET_PASS;

bool tagDetected = false;   // Flag to indicate RFID tag is detected
bool serverRunning = false; // Flag to track server state
bool dataPending = false;   // Flag to indicate pending data to write to RFID

RFIDData rfidData;    // Struct to hold parsed RFID data
RFIDData pendingData; // Struct to hold pending RFID data
Creature creature;    // Create a global Creature object

// Creature List for Reference
const char *creatures[35] = {
    "No Creature", "Flamingo", "Flame-Kingo", "Kitten", "Flame-on", "Pup",
    "Dog", "Wolf", "Birdy", "Haast-eagle", "Squidy", "Giant-Squid",
    "Kraken", "BabyShark", "Shark", "Megalodon", "Tadpole",
    "Poison-dart-frog", "Unicorn", "Master-unicorn", "Sprouty", "Tree-Folk",
    "Bush-Monster", "Baby-Dragon", "Dragon", "Dino-Egg", "T-Rex",
    "Baby-Ray", "Mega-Manta", "Orca", "Big-Bitey", "Flame-Lily",
    "Monster-Lily", "Bear-Cub", "Moss-Bear"};

// Variables to track RFID card state
MFRC522::Uid lastCardUid;       // To store the UID of the last detected card
bool cardPresent = false;       // Flag to indicate if a card is currently present

// Function Declarations
void listSPIFFSFiles();
String readFromRFID(byte blockAddr);
bool writeRFIDData(const RFIDData &data);
bool writeToRFID(const String &data, byte blockAddr);
uint8_t encodeBools(bool A, bool B, bool C, bool D);
void handleFormSubmit(AsyncWebServerRequest *request);
void startWebServer();
void stopWebServer();
bool uidsMatch(MFRC522::Uid uid1, MFRC522::Uid uid2);
void copyUid(MFRC522::Uid &dest, MFRC522::Uid &src);
void clearUid(MFRC522::Uid &uid);

// Function Definitions

// Function to encode 4 bools into a single byte (0-15)
uint8_t encodeBools(bool A, bool B, bool C, bool D)
{
    uint8_t encoded = 0;
    encoded |= (A << 3); // A is the highest bit
    encoded |= (B << 2);
    encoded |= (C << 1);
    encoded |= (D << 0);
    return encoded;
}

// Function to write RFIDData to the RFID card
bool writeRFIDData(const RFIDData &data)
{
    // Buffer to hold formatted data (maximum 16 bytes for MIFARE Classic block)
    char buffer[17]; // 16 characters + null terminator

    // Format data as AA CC TT BB%NAME, ensuring two-digit numbers with leading zeros
    snprintf(buffer, sizeof(buffer), "%02d%02d%02d%02d%%%s",
             data.age, data.coins, data.creatureType, data.bools, data.name.c_str());

    String formattedData = String(buffer);

    // Ensure the formattedData does not exceed 16 characters (including null terminator)
    if (formattedData.length() > 16)
    {
        formattedData = formattedData.substring(0, 16);
    }

    Serial.print("Formatted Data to Write: ");
    Serial.println(formattedData);

    // Write to RFID card (Block 1)
    return writeToRFID(formattedData, 1);
}

// Function to handle form submission
void handleFormSubmit(AsyncWebServerRequest *request) {
    Serial.println("Handling form submission");

    // Ensure all required parameters are present
    if (request->hasParam("age", true) &&
        request->hasParam("coins", true) &&
        request->hasParam("creatureType", true) &&
        request->hasParam("name", true)) {

        // Retrieve parameters
        int age = request->getParam("age", true)->value().toInt();
        int coins = request->getParam("coins", true)->value().toInt();
        int creatureType = request->getParam("creatureType", true)->value().toInt();

        bool A = request->hasParam("A", true);
        bool B = request->hasParam("B", true);
        bool C = request->hasParam("C", true);
        bool D = request->hasParam("D", true);

        String name = request->getParam("name", true)->value();
        if (name.length() > 6) {
            name = name.substring(0, 6);
        }

        // Encode bools
        uint8_t boolsEncoded = encodeBools(A, B, C, D);

        // Populate RFIDData structure
        pendingData.age = age;
        pendingData.coins = coins;
        pendingData.creatureType = creatureType;
        pendingData.bools = boolsEncoded;
        pendingData.name = name;

        Serial.println("Form Data Received and stored:");
        Serial.print("Age: ");
        Serial.println(pendingData.age);
        Serial.print("Coins: ");
        Serial.println(pendingData.coins);
        Serial.print("Creature Type: ");
        Serial.println(pendingData.creatureType);
        Serial.print("Bools (Binary): ");
        Serial.println(pendingData.bools, BIN);
        Serial.print("Name: ");
        Serial.println(pendingData.name);

        // Set dataPending flag to true
        dataPending = true;

        // Update TFT display
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Waiting for RFID");
        tft.println("to write data...");

        // Send HTML response with a button to redirect back to the landing page
        String htmlResponse = "<!DOCTYPE html><html><head><title>Submission Successful</title></head><body>";
        htmlResponse += "<h2>Data Received</h2>";
        htmlResponse += "<p>Your data has been received. Please present your RFID card to write the data.</p>";
        htmlResponse += "<button onclick=\"window.location.href='/'\">Back to Landing Page</button>";
        htmlResponse += "</body></html>";

        request->send(200, "text/html", htmlResponse);

    } else {
        Serial.println("Incomplete form data");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Incomplete Data");
        request->send(400, "text/plain", "Incomplete data received.");
    }
}

// Function to write data to RFID card
bool writeToRFID(const String &data, byte blockAddr)
{
    MFRC522::StatusCode status;

    // Authenticate using key A
    byte sector = blockAddr / 4;
    byte trailerBlock = sector * 4 + 3;

    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("PCD_Authenticate() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Prepare data block
    byte dataBlock[16];
    int len = data.length();
    for (int i = 0; i < 16; i++)
    {
        if (i < len)
        {
            dataBlock[i] = data.charAt(i);
        }
        else
        {
            dataBlock[i] = 0x00; // Pad with zeros
        }
    }

    // Write data to the card
    status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("MIFARE_Write() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Success
    return true;
}

// Function to read data from RFID card
String readFromRFID(byte blockAddr)
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

    // Authenticate
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailerBlock,
        &key,
        &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
        return "";
    }

    Serial.println("Authentication successful");

    // Read data from the card
    byte buffer[18];
    byte size = sizeof(buffer);

    Serial.print("Reading from block ");
    Serial.println(blockAddr);

    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("MIFARE_Read() failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
        return "";
    }

    Serial.println("Read successful");

    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

    // Halt PICC
    mfrc522.PICC_HaltA();

    // Convert buffer to string
    String data = "";
    for (byte i = 0; i < 16; i++)
    {
        if (buffer[i] != 0)
            data += (char)buffer[i];
    }

    Serial.print("Raw Data Read: ");
    Serial.println(data);

    return data;
}

// Function to list all files in SPIFFS
void listSPIFFSFiles()
{
    Serial.println("Listing SPIFFS files:");
    fs::File root = SPIFFS.open("/");
    if (!root)
    {
        Serial.println("Failed to open root directory");
        return;
    }

    fs::File file = root.openNextFile();
    while (file)
    {
        Serial.print("FILE: ");
        Serial.print(file.name());
        Serial.print("\tSIZE: ");
        Serial.println(file.size());
        file = root.openNextFile();
    }
}

// Function to start the web server
void startWebServer()
{
    if (!serverRunning)
    {
        // Configure server routes and handlers
        // Serve landing.html at root URL
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            Serial.println("Serving /landing.html");
            if (SPIFFS.exists("/landing.html")) {
                request->send(SPIFFS, "/landing.html", "text/html");
            } else {
                Serial.println("landing.html not found");
                request->send(404, "text/plain", "File Not Found");
            } });

        // Serve index.html at /editProfile
        server.on("/editProfile", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            Serial.println("Serving /index.html");
            if (SPIFFS.exists("/index.html")) {
                request->send(SPIFFS, "/index.html", "text/html");
            } else {
                Serial.println("index.html not found");
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

        // Display IP address on TFT
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Web Server Running");
        tft.print("IP: ");
        tft.println(WiFi.localIP());
    }
}

// Function to stop the web server
void stopWebServer()
{
    if (serverRunning)
    {
        server.end();
        serverRunning = false;
        Serial.println("Web server stopped.");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Web Server Stopped");
    }
}

// Function to compare two UIDs
bool uidsMatch(MFRC522::Uid uid1, MFRC522::Uid uid2) {
    if (uid1.size != uid2.size) {
        return false;
    }
    for (byte i = 0; i < uid1.size; i++) {
        if (uid1.uidByte[i] != uid2.uidByte[i]) {
            return false;
        }
    }
    return true;
}

// Function to copy a UID
void copyUid(MFRC522::Uid &dest, MFRC522::Uid &src) {
    dest.size = src.size;
    for (byte i = 0; i < src.size; i++) {
        dest.uidByte[i] = src.uidByte[i];
    }
}

// Function to clear a UID
void clearUid(MFRC522::Uid &uid) {
    uid.size = 0;
    for (byte i = 0; i < sizeof(uid.uidByte); i++) {
        uid.uidByte[i] = 0;
    }
}

// Setup function
void setup()
{
    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Initialize the TFT display
    Serial.println("Initializing TFT...");
    tft.init();
    tft.setRotation(1);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("TFT Initialized");
    Serial.println("TFT initialized");

    // Initialize RFID module
    Serial.println("Initializing RFID...");
    SPI.begin(25, 33, 26); // SPI with SCK=25, MISO=33, MOSI=26
    mfrc522.PCD_Init();    // Initialize RFID reader
    Serial.println("RFID initialized");
    tft.println("RFID Initialized");

    // Prepare the key (used both as key A and as key B)
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Initialize SPIFFS
    Serial.println("Initializing SPIFFS...");
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        tft.println("SPIFFS Mount Failed");
        return;
    }
    Serial.println("SPIFFS mounted successfully");
    tft.println("SPIFFS Initialized");

    // List SPIFFS files for debugging
    listSPIFFSFiles();

    // Initialize WiFi
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

    // Optionally display IP on TFT
    tft.setCursor(0, 20);
    tft.print("IP: ");
    tft.println(WiFi.localIP());

    Serial.println("Setup complete. Waiting for RFID tag...");
    tft.println("Waiting for RFID...");
}

// Main loop function
void loop() {
    // If there's data pending to be written
    if (dataPending) {
        // Existing code to handle writing to RFID card
        // ...
    } else {
        // Check for a new card
        if (mfrc522.PICC_IsNewCardPresent()) {
            if (mfrc522.PICC_ReadCardSerial()) {
                // Check if this is a new card or a different card
                if (!cardPresent || !uidsMatch(lastCardUid, mfrc522.uid)) {
                    // Copy the UID of the new card
                    copyUid(lastCardUid, mfrc522.uid);
                    cardPresent = true;

                    Serial.println("New RFID card detected. Reading data...");

                    String rawData = readFromRFID(1); // Read block 1
                    Serial.print("Raw Data Read: ");
                    Serial.println(rawData);

                    if (rawData.length() > 0) {
                        // Parse RFID data
                        parseRFIDData(rawData, rfidData);

                        // Map data to the Creature object
                        // ... existing code to populate the creature object ...

                        // Display data on TFT
                        // ... existing code to display data ...

                        // Optionally, start the web server
                        startWebServer();
                    } else {
                        Serial.println("Failed to read data from RFID.");
                    }
                } else {
                    // The same card is still present; do nothing
                }

                // Halt the PICC
                mfrc522.PICC_HaltA();
                mfrc522.PCD_StopCrypto1();
            }
        } else {
            // No card is present
            if (cardPresent) {
                // Card was removed
                Serial.println("RFID card removed.");
                cardPresent = false;
                clearUid(lastCardUid);
            }
        }

        // Add a small delay to avoid high CPU usage
        delay(100);
    }
}