#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MFRC522.h>
#include "RFIDData.h"

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
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

String inputData = "";     // Variable to store input data from the web page
bool dataReceived = false; // Flag to indicate data has been received
bool tagDetected = false;  // Flag to indicate RFID tag is detected

RFIDData rfidData; // Struct to hold parsed RFID data

// Function to write data to RFID card
bool writeToRFID(const String &data, byte blockAddr)
{
  MFRC522::StatusCode status;

  // Authenticate with the card (using key A)
  byte trailerBlock = (blockAddr / 4) * 4 + 3; // Trailer block for the sector

  // Default key for MIFARE cards
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

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
    return false;
  }

  Serial.println("Authentication successful");

  // Prepare data (must be 16 bytes)
  byte dataBlock[16];
  memset(dataBlock, 0, sizeof(dataBlock)); // Clear the array
  data.toCharArray((char *)dataBlock, 16);

  // Write data to the card
  Serial.print("Writing to block ");
  Serial.println(blockAddr);
  status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PCD_StopCrypto1();
    return false;
  }

  Serial.println("Write successful");

  // Stop authentication
  mfrc522.PCD_StopCrypto1();

  return true;
}

// Function to read data from RFID card
String readFromRFID(byte blockAddr)
{
  MFRC522::StatusCode status;

  // Authenticate with the card (using key A)
  byte trailerBlock = (blockAddr / 4) * 4 + 3; // Trailer block for the sector

  // Default key for MIFARE cards
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  Serial.print("Authenticating with trailer block ");
  Serial.println(trailerBlock);

  // Select the card
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial())
  {
    Serial.println("No card selected or failed to read card serial.");
    return "";
  }

  // Authenticate
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
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
    mfrc522.PCD_StopCrypto1();
    return "";
  }

  Serial.println("Read successful");

  // Stop authentication
  mfrc522.PCD_StopCrypto1();

  // Convert buffer to string
  String data = "";
  for (byte i = 0; i < 16; i++)
  {
    if (buffer[i] != 0) // Ignore null characters
    {
      data += (char)buffer[i];
    }
  }

  return data;
}

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
  tft.println("TFT OK");
  Serial.println("TFT initialized");

  // Initialize RFID module
  Serial.println("Initializing RFID...");
  SPI.begin(25, 33, 26); // SPI with SCK=25, MISO=33, MOSI=26
  mfrc522.PCD_Init();    // Initialize RFID reader
  Serial.println("RFID initialized");
  tft.println("RFID OK");

  // Prepare the key (used both as key A and as key B)
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  // Check for new RFID tag
  while (!tagDetected)
  {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      Serial.println("RFID tag detected");
      tagDetected = true;

      // Read and display RFID tag contents
      String name = readFromRFID(1); // Block 1
      Serial.print("Name: ");
      Serial.println(name);

      // Display name on TFT
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Name:");
      tft.println(name);
      delay(3000);
    }
    else
    {
      // No RFID tag detected
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("No RFID tag detected");
      delay(500);
    }
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
  tft.println("SPIFFS OK");

  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  tft.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  int wifi_attempts = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    tft.print(".");
    wifi_attempts++;
    if (wifi_attempts > 20)
    { // Timeout after 20 seconds
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
    if (request->hasParam("name", true)) {
      String name = request->getParam("name", true)->value();
      inputData = "name:" + name;
      dataReceived = true;
      Serial.println("Data received: " + inputData);
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Data received:");
      tft.println(inputData);
      request->send(200, "text/plain", "Data received and will be written to the RFID card.");
    } else {
      Serial.println("No data received");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("No data received");
      request->send(400, "text/plain", "No data received");
    } });

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  // Display "Web address:" and IP
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("Web address:");
  tft.println(WiFi.localIP());

  // Wait until data is received
  unsigned long startTime = millis();
  while (!dataReceived && millis() - startTime < 30000) // Timeout after 30 seconds
  {
    delay(100);
  }

  // Write data to RFID card
  if (dataReceived)
  {
    // Reinitialize RFID module before each write operation
    Serial.println("Reinitializing RFID module...");
    mfrc522.PCD_Init();

    // Write name to block 1
    String name = inputData.substring(inputData.indexOf("name:") + 5);
    Serial.print("Writing name to block 1: ");
    Serial.println(name);
    if (writeToRFID(name, 1))
    {
      Serial.println("Name written to RFID card");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Name written to");
      tft.println("RFID card");
    }
    else
    {
      Serial.println("Failed to write name to RFID card");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Write failed");
    }
  }
  else if (!dataReceived)
  {
    Serial.println("No data received within timeout period");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("No data received");
  }

  // Reset flags
  dataReceived = false;
  tagDetected = false;

  mfrc522.PICC_HaltA();      // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD

  delay(500);
}