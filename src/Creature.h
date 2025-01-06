#ifndef CREATURE_H
#define CREATURE_H

#include <Arduino.h>

struct Creature
{
  String creatureName; // Name from the creatures array based on creatureType
  String customName;   // Custom name from RFID data after '%'
  String trainerName;  // Can be set later
  int trainerAge;      // First two digits from RFID data
  int coins;           // Next two digits
  int creatureType;    // Next two digits, corresponds to creatures array
};

// Add this function prototype
Creature decode(const String &rawData);

#endif // CREATURE_H