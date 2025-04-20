// RFIDData.h
#ifndef RFIDDATA_H
#define RFIDDATA_H

#include <Arduino.h>
#include <MFRC522.h>

// Struct to hold RFID data
struct RFIDData
{
    String name;       // 10 characters max
    //int yearLevel;     // 00-99
    int challengeCode; // 00-99
    int wrongGuesses;  // 00-34 (0 reserved for 'no creature')
    uint8_t bools;     // 0-15 representing 4 boolean values
    String creature;   // New field
};

// Add RFIDParsed struct BEFORE references to parseRawRFID
struct RFIDParsed
{
    uint8_t boolVal;
    //int yearLevel;     // 00-99
    int challengeCode; // 00-99
    int wrongGuesses;  // 00-34 (0 reserved for 'no creature')
    String name;
};

struct Creature
{
    //int yearLevel;       // 1 digit (1-9)
    int challengeCode;   // 3 digits (000-999)
    int wrongGuesses;    // 1 digit (0-9)
    int boolVal;         // unchanged
    int creatureType;    // Now 0-99
    int artifactValue;   // New field, 0-99
    /////////////String creatureName; // Increased to 10 chars max
    // int creatureType;    // 2 digit (0-9)
    String customName;
    int coins; // New field
};
// extern RFIDData pendingData; // Remove or comment out this line
extern bool dataPending;

uint8_t encodeBools(bool A, bool B, bool C, bool D);
void decodeBools(uint8_t bools, bool &A, bool &B, bool &C, bool &D);
String readFromRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, byte blockAddr);
String readFromRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, byte blockAddr, int &intPart, String &strPart);
bool writeToRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const String &data, byte blockAddr);
bool writeRFIDData(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const RFIDData &data);
void parseRFIDData(const String &data, RFIDData &rfidData);
RFIDParsed parseRawRFID(const String &raw);

// Add decode(...) prototype here:
Creature decode(int numericPart, const String &namePart);
bool clearChallBools(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const Creature &creature);

#endif // RFIDDATA_H