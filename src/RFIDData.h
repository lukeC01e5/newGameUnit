// RFIDData.h
#ifndef RFIDDATA_H
#define RFIDDATA_H

#include <Arduino.h>
#include <MFRC522.h>

// Struct to hold RFID data
struct RFIDData {
    String name;          // 6 characters max
    int age;              // 00-99
    int coins;            // 00-99
    int creatureType;     // 00-34 (0 reserved for 'no creature')
    uint8_t bools;        // 0-15 representing 4 boolean values
};

// External variable declarations
extern RFIDData pendingData;
extern bool dataPending;

// Function declarations
void parseRFIDData(const String &data, RFIDData &rfidData);
uint8_t encodeBools(bool A, bool B, bool C, bool D);
void decodeBools(uint8_t bools, bool &A, bool &B, bool &C, bool &D);
String readFromRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, byte blockAddr);
bool writeToRFID(MFRC522& mfrc522, MFRC522::MIFARE_Key& key, const String& data, byte blockAddr);
bool writeRFIDData(MFRC522& mfrc522, MFRC522::MIFARE_Key& key, const RFIDData& data);

// ... other declarations ...

#endif // RFIDDATA_H