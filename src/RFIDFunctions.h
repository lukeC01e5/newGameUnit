#ifndef RFID_FUNCTIONS_H
#define RFID_FUNCTIONS_H

#include <Arduino.h>        // Include Arduino definitions
#include "CharacterTypes.h" // Include CharacterType enum

// Structure to hold RFID data
struct RFIDData
{
    String name;
    int age;
    CharacterType characterType;
    char gender;
};

// Function declarations
bool writeToRFID(const String &data, byte blockAddr);
String readFromRFID(byte blockAddr);
void parseRFIDData(const String &blockData, RFIDData &rfidData);
String getCharacterTypeName(CharacterType type);

#endif // RFID_FUNCTIONS_H