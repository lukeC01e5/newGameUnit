#ifndef RFIDFUNCTIONS_H
#define RFIDFUNCTIONS_H

#include <Arduino.h>
#include <MFRC522.h>
#include "globals.h"

// Struct definition for RFID data
struct RFIDData {
    String name;
    int age;
    String characterType;
};

// Declare the functions
bool writeToRFID(const String &data, byte blockAddr);
String readFromRFID(byte blockAddr);
void parseRFIDData(const String &data, RFIDData &rfidData);

#endif // RFIDFUNCTIONS_H