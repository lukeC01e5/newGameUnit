// RFIDData.cpp
#include "RFIDData.h"
#include "Creature.h"
// #include "Creature.h"
void parseRFIDData(const String &data, RFIDData &rfidData)
{
    // Ensure the data contains the '%' separator
    int separatorIndex = data.indexOf('%');
    if (separatorIndex == -1)
    {
        Serial.println("Invalid data format: Missing separator");
        return;
    }

    // Extract the main data and name
    String mainData = data.substring(0, separatorIndex);
    String nameData = data.substring(separatorIndex + 1);

    // Ensure mainData has exactly 8 characters (AA CC TT BB)
    if (mainData.length() != 8)
    {
        Serial.println("Invalid data format: Expected 8 characters in main data");
        return;
    }

    // Extract Age
    rfidData.age = mainData.substring(0, 2).toInt();

    // Extract Coins
    rfidData.coins = mainData.substring(2, 4).toInt();

    // Extract Creature Type
    rfidData.creatureType = mainData.substring(4, 6).toInt();

    // Extract Boolean Values
    rfidData.bools = mainData.substring(6, 8).toInt();

    // Extract Name (ensure it's trimmed to 6 characters)
    if (nameData.length() > 6)
    {
        rfidData.name = nameData.substring(0, 6);
    }
    else
    {
        rfidData.name = nameData;
    }

    // Debugging output
    Serial.println("Parsed RFID Data:");
    Serial.print("Age: ");
    Serial.println(rfidData.age);
    Serial.print("Coins: ");
    Serial.println(rfidData.coins);
    Serial.print("Creature Type: ");
    Serial.println(rfidData.creatureType);
    Serial.print("Bools: ");
    Serial.println(rfidData.bools, BIN); // Print as binary
    Serial.print("Name: ");
    Serial.println(rfidData.name);
}