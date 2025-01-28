#include "GlobalDefs.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
RFIDData pendingData;
bool dataPending = false;
bool formSubmitted = false;
bool allChallBools = false;
TFT_eSPI tft; // Define the TFT instance
