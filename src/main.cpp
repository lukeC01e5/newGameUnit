#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <MFRC522.h> // Include the MFRC522 library for the RC522 RFID module

// Pin definitions for the RFID module
#define SS_PIN 13  // Slave Select pin for RFID
#define RST_PIN 22 // Reset pin for RFID

TFT_eSPI tft = TFT_eSPI();        // Create TFT instance
MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance
MFRC522::MIFARE_Key key;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  // Initialize the TFT display
  Serial.println("Initializing TFT...");
  tft.init();
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("TFT initialized");
  Serial.println("TFT initialized");

  // Initialize SPI
  Serial.println("Initializing SPI...");
  SPI.begin(25, 33, 26); // SPI with SCK=25, MISO=33, MOSI=26
  Serial.println("SPI initialized");
  tft.println("SPI OK");

  // Initialize RFID module
  Serial.println("Initializing RFID...");
  tft.println("RFID init...");
  mfrc522.PCD_Init();
  Serial.println("RFID initialized");
  tft.println("RFID OK");

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
}

void loop()
{
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    Serial.println("No new card present.");
    delay(50);
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial())
  {
    Serial.println("Failed to read card serial.");
    delay(50);
    return;
  }

  // Show UID on Serial Monitor
  Serial.print("RFID Tag ID: ");
  String rfidUID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    rfidUID += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Display RFID UID on TFT
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("RFID UID:");
  tft.println(rfidUID);

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  delay(2000); // Wait for 2 seconds before scanning again
}