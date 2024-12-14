#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TFT_eSPI.h>
#include <ESPAsyncWebServer.h>

// Pin definitions for the RFID module
#define SS_PIN 13  // Slave Select pin for RFID
#define RST_PIN 22 // Reset pin for RFID

// Define block addresses based on sector
#define SECTOR1_BLOCK1 1 // Block 1 in Sector 1

// External instances
extern TFT_eSPI tft;
extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;
extern AsyncWebServer server;

// Variables
extern String inputData;  // Variable to store input data from the web page
extern bool dataReceived; // Flag to indicate data has been received

#endif // GLOBALS_H