// RFIDData.h
#ifndef RFIDDATA_H
#define RFIDDATA_H

#include <Arduino.h>
#include <MFRC522.h>

// Struct to hold RFID data
struct RFIDData
{
    String name;      // 6 characters max
    int age;          // 00-99
    int coins;        // 00-99
    int creatureType; // 00-34 (0 reserved for 'no creature')
    uint8_t bools;    // 0-15 representing 4 boolean values
};

// Add RFIDParsed struct BEFORE references to parseRawRFID
struct RFIDParsed
{
    int age;
    int coins;
    int creatureType;
    int boolVal;
    String name;
};

struct Creature
{
    int trainerAge;
    int coins;
    int creatureType;
    String customName;
    int intVal; // Ensure this member exists if needed
    // String userId;
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