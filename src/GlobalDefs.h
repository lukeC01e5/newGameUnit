#ifndef GLOBALDEFS_H
#define GLOBALDEFS_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <MFRC522.h>
#include "RFIDData.h"

// Define current SPI pins
#define SS_PIN 13   // Slave Select
#define RST_PIN 22  // Reset
#define SCK_PIN 25  // SPI Clock
#define MISO_PIN 33 // Master In Slave Out
#define MOSI_PIN 26 // Master Out Slave In

extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;
extern RFIDData pendingData;
extern bool dataPending;
extern TFT_eSPI tft;

// extern MFRC522::Uid lastCardUid;

#endif // GLOBALDEFS_H