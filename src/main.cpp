#include "globals.h"
#include "CharacterTypes.h" // Include the CharacterTypes library
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include "RFIDFunctions.h"

// Include the file with the WiFi credentials
#include "arduino_secrets.h"

// Initialize external instances
TFT_eSPI tft = TFT_eSPI();
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
AsyncWebServer server(80); // Create AsyncWebServer object on port 80

// WiFi credentials
const char *ssid = SECRET_SSID;
const char *pass = SECRET_PASS;

// Variables
String inputData = "";     // Variable to store input data from the web page
bool dataReceived = false; // Flag to indicate data has been received

/**
 * @brief Parses the inputData string into the RFIDData struct.
 *
 * @param input The raw input data string from the web interface.
 * @param rfidData The RFIDData struct to populate.
 */
void parseInputData(const String &input, RFIDData &rfidData)
{
    // Expected format: "name,age,typeIndex,gender"
    int firstComma = input.indexOf(',');
    int secondComma = input.indexOf(',', firstComma + 1);
    int thirdComma = input.indexOf(',', secondComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1)
    {
        Serial.println("Invalid input format");
        return;
    }

    rfidData.name = input.substring(0, firstComma);
    rfidData.age = input.substring(firstComma + 1, secondComma).toInt();
    int typeIndex = input.substring(secondComma + 1, thirdComma).toInt();
    if (typeIndex >= ELF && typeIndex < CHARACTER_TYPE_COUNT)
    {
        rfidData.characterType = static_cast<CharacterType>(typeIndex);
    }
    else
    {
        rfidData.characterType = UNKNOWN;
    }
    rfidData.gender = input.charAt(thirdComma + 1);
}

enum SystemState
{
    IDLE,
    SCANNING_RF,
    INITIALIZING_WIFI,
    WRITING_DATA
};

SystemState currentState = IDLE;
unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 500; // Interval in milliseconds to check for card presence

/**
 * @brief Handles form submission and processes the data.
 *
 * @param request The incoming web request.
 */
void handleFormSubmit(AsyncWebServerRequest *request)
{
    // Initialize variables for all expected fields
    String name = "";
    String ageStr = "";
    String typeStr = "";
    char gender = 'N'; // Default value

    bool hasName = false, hasAge = false, hasType = false, hasGender = false;

    // Extract name
    if (request->hasParam("name", true))
    {
        name = request->getParam("name", true)->value();
        hasName = true;
    }

    // Extract age
    if (request->hasParam("age", true))
    {
        ageStr = request->getParam("age", true)->value();
        hasAge = true;
    }

    // Extract character type
    if (request->hasParam("type", true))
    {
        typeStr = request->getParam("type", true)->value();
        hasType = true;
    }

    // Extract gender
    if (request->hasParam("gender", true))
    {
        String genderStr = request->getParam("gender", true)->value();
        if (genderStr.length() > 0)
        {
            gender = genderStr.charAt(0);
            hasGender = true;
        }
    }

    if (hasName && hasAge && hasType && hasGender)
    {
        // Convert character type to index using enum
        CharacterType typeEnum;
        if (typeStr.equalsIgnoreCase("Elf"))
        {
            typeEnum = ELF;
        }
        else if (typeStr.equalsIgnoreCase("Dwarf"))
        {
            typeEnum = DWARF;
        }
        else if (typeStr.equalsIgnoreCase("Wizard"))
        {
            typeEnum = WIZARD;
        }
        else if (typeStr.equalsIgnoreCase("Knight"))
        {
            typeEnum = KNIGHT;
        }
        else if (typeStr.equalsIgnoreCase("Witch"))
        {
            typeEnum = WITCH;
        }
        else if (typeStr.equalsIgnoreCase("Mermaid"))
        {
            typeEnum = MERMAID;
        }
        else if (typeStr.equalsIgnoreCase("Ogre"))
        {
            typeEnum = OGRE;
        }
        else
        {
            typeEnum = UNKNOWN;
        }

        int typeIndex = static_cast<int>(typeEnum);
        if (typeEnum == UNKNOWN)
        {
            Serial.println("Invalid character type received");
            request->send(400, "text/plain", "Invalid character type received");
            return;
        }

        // Format the data string as "Name,Age,TypeIndex,Gender"
        String formattedData = name + "," + ageStr + "," + String(typeIndex) + "," + String(gender);

        // Ensure the formattedData is exactly 16 characters by padding or trimming
        while (formattedData.length() < 16)
        {
            formattedData += ' ';
        }
        if (formattedData.length() > 16)
        {
            formattedData = formattedData.substring(0, 16);
        }

        inputData = formattedData;
        dataReceived = true;
        Serial.println("Data received: " + inputData);
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Data received:");
        tft.println(inputData);
        request->send(200, "text/plain", "Data received and will be written to the RFID card.");
    }
    else
    {
        Serial.println("Incomplete data received");
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.println("Incomplete data received");
        request->send(400, "text/plain", "Incomplete data received");
    }
}

void setup()
{
    // Initialize Serial Monitor
    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Initialize TFT display
    tft.init();
    tft.setRotation(3); // Rotate the screen 180 degrees
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Initializing...");
    Serial.println("TFT initialized.");

    // Initialize SPI with specified pins
    Serial.println("Initializing SPI...");
    SPI.begin(25, 33, 26); // Initialize SPI with SCK=25, MISO=33, MOSI=26
    Serial.println("SPI Initialized");

    // Initialize RFID reader
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
    Serial.println("SPIFFS mounted successfully.");
    tft.println("SPIFFS Mounted");
    /*
        // **Call Test Function**
        if (testWriteAndRead())
        {
            Serial.println("Test Write and Read Passed.");
            tft.println("Test Passed.");
        }
        else
        {
            Serial.println("Test Write and Read Failed.");
            tft.println("Test Failed.");
        }
    */
    // Connect to WiFi
    WiFi.begin(ssid, pass);
    int wifi_attempts = 0;
    Serial.print("Connecting to WiFi");
    tft.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
        tft.print(".");
        wifi_attempts++;
        if (wifi_attempts > 20) // 20 seconds timeout
        {
            Serial.println("\nFailed to connect to WiFi");
            tft.println("\nNo connection");
            return;
        }
    }
    Serial.println("\nConnected to WiFi");
    tft.println("\nWiFi OK");

    // Display IP address
    String ipAddress = WiFi.localIP().toString();
    Serial.print("IP Address: ");
    Serial.println(ipAddress);
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("IP Address:");
    tft.println(ipAddress);

    // Setup server routes
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Handle form submission
    server.on("/submit", HTTP_POST, handleFormSubmit);

    server.begin();
    Serial.println("HTTP server started");
}

/**
 * @brief Handles state transitions and RFID operations.
 */
void loop()
{
    switch (currentState)
    {
    case IDLE:
        // Continuously scan for RFID cards
        if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
        {
            Serial.println("RFID card detected.");
            tft.println("RFID card detected.");

            // Read data from RFID
            String readData = readFromRFID(SECTOR1_BLOCK1);
            Serial.print("Data read from Block1: ");
            Serial.println(readData);
            tft.println("Data read from RFID.");

            // Parse the read data
            RFIDData readRFIDData;
            parseRFIDData(readData, readRFIDData);

            if (readRFIDData.name.length() == 0)
            {
                Serial.println("Invalid block data format.");
                tft.println("Invalid data format.");
            }
            else
            {
                // Display parsed data
                Serial.println("Parsed RFID Data:");
                Serial.print("Name: ");
                Serial.println(readRFIDData.name);
                Serial.print("Age: ");
                Serial.println(readRFIDData.age);
                Serial.print("Type: ");
                Serial.println(getCharacterTypeName(readRFIDData.characterType));
                Serial.print("Gender: ");
                Serial.println(readRFIDData.gender);

                tft.fillScreen(TFT_BLACK);
                tft.setCursor(0, 0);
                tft.println("RFID Data:");
                tft.println("Name: " + readRFIDData.name);
                tft.println("Age: " + String(readRFIDData.age));
                tft.println("Type: " + getCharacterTypeName(readRFIDData.characterType));
                tft.println("Gender: " + String(readRFIDData.gender));
            }

            // Halt and stop encryption
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
        }

        // Check if data has been received from the web server
        if (dataReceived)
        {
            Serial.println("Data received. Preparing to write to RFID.");
            tft.println("Data received. Ready to write.");

            // Reinitialize RFID module
            mfrc522.PCD_Init();
            Serial.println("RFID module reinitialized.");

            // Transition to writing state
            currentState = WRITING_DATA;
        }
        break;

    case WRITING_DATA:
    {
        Serial.println("Writing data to RFID card.");
        tft.println("Writing data to RFID...");

        // Parse inputData into RFIDData struct
        RFIDData rfidData;
        parseInputData(inputData, rfidData);

        // Prepare data string: "Name,Age,TypeIndex,Gender"
        String dataString = rfidData.name + "," + String(rfidData.age) + "," + String(static_cast<int>(rfidData.characterType)) + "," + String(rfidData.gender);

        // Ensure the dataString is exactly 16 characters
        while (dataString.length() < 16)
        {
            dataString += ' ';
        }
        if (dataString.length() > 16)
        {
            dataString = dataString.substring(0, 16);
        }

        Serial.print("Formatted Data String: '");
        Serial.print(dataString);
        Serial.println("'");
        tft.println("Formatted Data:");

        // Write to RFID block
        if (writeToRFID(dataString, SECTOR1_BLOCK1))
        {
            Serial.println("Data successfully written to RFID.");
            tft.println("Data written to Block 5.");
            currentState = IDLE; // Return to IDLE after writing
        }
        else
        {
            Serial.println("Failed to write data to RFID.");
            tft.println("Write failed.");
            currentState = IDLE;
        }

        break;
    }

    default:
        currentState = IDLE;
        break;
    }

    // Optional: Add a small delay to prevent overwhelming the CPU
    delay(10);
}