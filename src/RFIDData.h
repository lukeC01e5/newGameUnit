// RFIDData.h
#ifndef RFIDDATA_H
#define RFIDDATA_H

#include <Arduino.h>

struct RFIDData {
  String name;
  int age;
  String characterType;
};

void parseRFIDData(const String& data, RFIDData& rfidData);

#endif // RFIDDATA_H