#ifndef GLOBALS_H
#define GLOBALS_H

#include <TFT_eSPI.h>
#include <MFRC522.h>

// Declare extern variables
extern TFT_eSPI tft;
extern MFRC522 mfrc522;
extern MFRC522::MIFARE_Key key;

#endif // GLOBALS_H