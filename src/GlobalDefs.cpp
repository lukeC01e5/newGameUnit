#include "GlobalDefs.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
bool dataPending = false;
RFIDData pendingData;
bool formSubmitted = false;
bool allChallBools = false;
TFT_eSPI tft; // Define the TFT instance
