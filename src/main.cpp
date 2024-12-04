#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <SPIFFS.h>
#include <MFRC522.h>

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

// Function to write data to RFID card
bool writeToRFID(String data)
{
  MFRC522::StatusCode status;

  // Authenticate with the card (using key A)
  byte sector = 1;       // Sector to write to (avoid manufacturer block 0)
  byte blockAddr = 4;    // Block address within the sector
  byte trailerBlock = 7; // Trailer block for the sector

  // Default key for MIFARE cards
  for (byte i = 0; i < 6; i++)
    key.keyByte[i] = 0xFF;

  // Authenticate
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("PCD_Authenticate() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Prepare data (must be 16 bytes)
  byte dataBlock[16];
  memset(dataBlock, 0, sizeof(dataBlock)); // Clear the array
  data.toCharArray((char *)dataBlock, 16);

  // Write data to the card
  status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    mfrc522.PCD_StopCrypto1();
    return false;
  }

  // Stop authentication
  mfrc522.PCD_StopCrypto1();

  return true;
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

  // Check for new RFID tag
  while (!tagDetected)
  {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      Serial.println("RFID tag detected");
      tagDetected = true;

      // Display "Weapon present" for 3 seconds
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("Weapon present");
      delay(3000);
    }
    else
    {
      // No RFID tag detected
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      tft.println("No weapon present");
      delay(500);
    }
  }

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
    if (request->hasParam("data", true)) {
      inputData = request->getParam("data", true)->value();
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
  if (dataReceived && writeToRFID(inputData))
  {
    Serial.println("Data written to RFID card");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Data written to");
    tft.println("RFID card");
  }
  else if (!dataReceived)
  {
    Serial.println("No data received within timeout period");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("No data received");
  }
  else
  {
    Serial.println("Failed to write data to RFID card");
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0);
    tft.println("Write failed");
  }

  // Reset flags
  dataReceived = false;
  tagDetected = false;

  mfrc522.PICC_HaltA();      // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD

  delay(500);
}