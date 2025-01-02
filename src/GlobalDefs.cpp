#include "GlobalDefs.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
RFIDData pendingData;
bool dataPending = false;
TFT_eSPI tft; // Define the TFT instance

