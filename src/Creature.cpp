#include "Creature.h"

/*
  Expected format: "AA CC TT BB%NAME"
   - AA: age (2 chars)
   - CC: coins (2 chars)
   - TT: creature type (2 chars)
   - BB: boolean bits (2 chars; optional if you just skip it)
   - %NAME: up to 6 chars
*/
Creature decode(const String &rawData)
{
    Creature c;

    // Find separator for custom name
    int separatorIndex = rawData.indexOf('%');
    if (separatorIndex == -1)
    {
        Serial.println("decode: Missing '%' separator");
        return c; // empty Creature
    }
    // Split main data vs. name
    String mainData = rawData.substring(0, separatorIndex);
    String nameData = rawData.substring(separatorIndex + 1);

    // mainData should be 8 chars => 2 for age, 2 for coins, 2 for creatureType, 2 for bits
    if (mainData.length() < 6)
    {
        Serial.println("decode: mainData too short");
        return c;
    }

    // Extract fields
    String ageStr = mainData.substring(0, 2);
    String coinStr = mainData.substring(2, 4);
    String typeStr = mainData.substring(4, 6);
    // If you need bits => mainData.substring(6, 8);

    c.trainerAge = ageStr.toInt();
    c.coins = coinStr.toInt();
    c.creatureType = typeStr.toInt();
    c.customName = (nameData.length() > 6) ? nameData.substring(0, 6) : nameData;

    // Debug print
    Serial.println("[decode] Created Creature object from raw data:");
    Serial.print(" trainerAge: ");
    Serial.println(c.trainerAge);
    Serial.print(" coins: ");
    Serial.println(c.coins);
    Serial.print(" creatureType: ");
    Serial.println(c.creatureType);
    Serial.print(" customName: ");
    Serial.println(c.customName);

    return c;
}