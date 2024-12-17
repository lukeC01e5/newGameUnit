// RFIDData.h
#ifndef RFIDDATA_H
#define RFIDDATA_H

#include <Arduino.h>

struct RFIDData {
  String name;             // 6 characters max
  int age;                 // 00-99
  int coins;               // 00-99
  int creatureType;        // 00-34 (0 reserved for 'no creature')
  uint8_t bools;           // 0-15 representing 4 boolean values
};

void parseRFIDData(const String& data, RFIDData& rfidData);

#endif // RFIDDATA_H