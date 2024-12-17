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

RFIDData rfidData; // Struct to hold parsed RFID data
Creature creature; // Create a global Creature object

// Creature List for Reference
const char *creatures[35] = {
    "No Creature", "Flamingo", "Flame-Kingo", "Kitten", "Flame-on", "Pup",
    "Dog", "Wolf", "Birdy", "Haast-eagle", "Squidy", "Giant-Squid",
    "Kraken", "BabyShark", "Shark", "Megalodon", "Tadpole",
    "Poison-dart-frog", "Unicorn", "Master-unicorn", "Sprouty", "Tree-Folk",
    "Bush-Monster", "Baby-Dragon", "Dragon", "Dino-Egg", "T-Rex",
    "Baby-Ray", "Mega-Manta", "Orca", "Big-Bitey", "Flame-Lily",
    "Monster-Lily", "Bear-Cub", "Moss-Bear"};

// Function Declarations
void listSPIFFSFiles();
String readFromRFID(byte blockAddr);
bool writeRFIDData(const RFIDData &data);
bool writeToRFID(const String &data, byte blockAddr);
uint8_t encodeBools(bool A, bool B, bool C, bool D);
void handleFormSubmit(AsyncWebServerRequest *request);
void startWebServer();
void stopWebServer();

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
bool writeRFIDData(const RFIDData &data) {
    // Buffer to hold formatted data (maximum 16 bytes for MIFARE Classic block)
    char buffer[17]; // 16 characters + null terminator

    // Format data as AA CC TT BB%NAME, ensuring two-digit numbers with leading zeros
    snprintf(buffer, sizeof(buffer), "%02d%02d%02d%02d%%%s",
             data.age, data.coins, data.creatureType, data.bools, data.name.c_str());

    String formattedData = String(buffer);

    // Ensure the formattedData does not exceed 16 characters (including null terminator)
    if (formattedData.length() > 16) {
        formattedData = formattedData.substring(0, 16);
    }

    Serial.print("Formatted Data to Write: ");
    Serial.println(formattedData);

    // Write to RFID card (Block 1)
    return writeToRFID(formattedData, 1);
}

// Function to handle form submission
void handleFormSubmit(AsyncWebServerRequest *request)
{
    Serial.println("Handling form submission");

    // Ensure all required parameters are present
    if (request->hasParam("age", true) &&
        request->hasParam("coins", true) &&
        request->hasParam("creatureType", true) &&
        request->hasParam("A", true) &&
        request->hasParam("B", true) &&
        request->hasParam("C", true) &&
        request->hasParam("D", true) &&
        request->hasParam("name", true))
    {

        // Retrieve parameters
        int age = request->getParam("age", true)->value().toInt();
        int coins = request->getParam("coins", true)->value().toInt();
        int creatureType = request->getParam("creatureType", true)->value().toInt();

        bool A = request->getParam("A", true)->value() == "on" ? true : false;
        bool B = request->getParam("B", true)->value() == "on" ? true : false;
        bool C = request->getParam("C", true)->value() == "on" ? true : false;
        bool D = request->getParam("D", true)->value() == "on" ? true : false;

        String name = request->getParam("name", true)->value();
        if (name.length() > 6)
        {
            name = name.substring(0, 6);
        }

        // Encode bools
        uint8_t boolsEncoded = encodeBools(A, B, C, D);

        // Populate RFIDData structure
        RFIDData data;
        data.age = age;
        data.coins = coins;
        data.creatureType = creatureType;
        data.bools = boolsEncoded;
        data.name = name;

        Serial.println("Form Data Received:");
        Serial.print("Age: ");
        Serial.println(data.age);
        Serial.print("Coins: ");
        Serial.println(data.coins);
        Serial.print("Creature Type: ");
        Serial.println(data.creatureType);
        Serial.print("Bools (Binary): ");
        Serial.println(data.bools, BIN);
        Serial.print("Name: ");
        Serial.println(data.name);

        // Write data to RFID
        if (writeRFIDData(data))
        {
            Serial.println("Data written to RFID successfully.");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Data Written to RFID");
            request->send(200, "text/plain", "Data received and written to the RFID card.");
        }
        else
        {
            Serial.println("Failed to write data to RFID.");
            tft.fillScreen(TFT_BLACK);
            tft.setCursor(0, 0);
            tft.println("Write Failed");
            request->send(500, "text/plain", "Failed to write data to RFID card.");
        }

        // Stop the server after handling the submission
        stopWebServer();
    }
    else
    {
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
        // Serve index.html
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
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

        // Start the server
        server.begin();
        serverRunning = true;
        Serial.println("Web server started.");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Web Server Running");
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
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi. IP address: ");
    Serial.println(WiFi.localIP());

    // Display IP address on TFT
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("IP Address:");
    tft.println(WiFi.localIP());

    Serial.println("Setup complete. Waiting for RFID tag...");
    tft.println("Waiting for RFID...");
}

// Main loop function
void loop()
{
    if (!tagDetected)
    {
        // Check for RFID tag
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            tagDetected = true;
            Serial.println("RFID tag detected");

            // Read data from block 1
            String rawData = readFromRFID(1);
            if (rawData.length() > 0)
            {
                // Create a Creature object
                Creature creature;

                // Parse RFID data
                parseRFIDData(rawData, rfidData);

                // Map data to Creature object
                creature.trainerAge = rfidData.age;
                creature.coins = rfidData.coins;
                creature.creatureType = rfidData.creatureType;
                creature.creatureName = rfidData.name;
                creature.trainerName = ""; // Blank for now

                // Display data on TFT
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("Creature Data:");
                tft.print("Trainer Age: ");
                tft.println(creature.trainerAge);
                tft.print("Coins: ");
                tft.println(creature.coins);
                tft.print("Creature Type: ");
                if (creature.creatureType >= 0 && creature.creatureType <= 34)
                {
                    tft.println(creatures[creature.creatureType]);
                }
                else
                {
                    tft.println("Unknown");
                }
                tft.print("Creature Name: ");
                tft.println(creature.creatureName);

                // Serial Monitor Output
                Serial.println("Creature Object Data:");
                Serial.print("Trainer Age: ");
                Serial.println(creature.trainerAge);
                Serial.print("Coins: ");
                Serial.println(creature.coins);
                Serial.print("Creature Type: ");
                Serial.println(creature.creatureType);
                Serial.print("Creature Name: ");
                Serial.println(creature.creatureName);
                Serial.print("Trainer Name: ");
                Serial.println(creature.trainerName);
            }
            else
            {
                Serial.println("Failed to read data from RFID");
                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("Read Failed");
            }

            // Start the web server
            startWebServer();
        }
    }

    // Add a small delay to allow background tasks
    delay(100);
}