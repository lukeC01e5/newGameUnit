#include "Creature.h"

// Forward-declare your creatures array, or include a header that declares it.
extern const char *creatures[35]; // Must match the array in main.cpp

/*
  Expected format: "AA CC TT BB%NAME"
   - AA: age (2 chars)
   - CC: coins (2 chars)
   - TT: creature type (2 chars)
   - BB: bits (2 chars, optional)
   - %NAME: up to 6 chars
*/
Creature decode(const String &raw)
{
    Creature c;

    int separatorIndex = raw.indexOf('%');
    if (separatorIndex == -1)
    {
        Serial.println("decode: Missing '%' separator");
        return c;
    }
    String mainData = raw.substring(0, separatorIndex);
    String nameData = raw.substring(separatorIndex + 1);

    // mainData needs at least 6 chars => 2 (age) + 2 (coins) + 2 (creatureType)
    if (mainData.length() < 6)
    {
        Serial.println("decode: mainData too short");
        return c;
    }

    // Parse fields
    String ageStr = mainData.substring(0, 2);
    String coinStr = mainData.substring(2, 4);
    String typeStr = mainData.substring(4, 6);

    c.trainerAge = ageStr.toInt();
    c.coins = coinStr.toInt();
    c.creatureType = typeStr.toInt();
    c.customName = (nameData.length() > 6) ? nameData.substring(0, 6) : nameData;

    // Convert numeric creatureType to text name
    if (c.creatureType >= 0 && c.creatureType < 35)
        c.creatureName = creatures[c.creatureType];
    else
        c.creatureName = "Unknown Creature";

    // Strip possible '\0' from anywhere in customName
    int nullPos;
    while ((nullPos = c.customName.indexOf('\0')) != -1)
    {
        c.customName.remove(nullPos, 1);
    }
    // Optionally also trim whitespace:
    c.customName.trim();

    // Print out the results
    Serial.println("[decode] Created Creature object from raw data:");
    Serial.print("  Age: ");
    Serial.println(c.trainerAge);
    Serial.print("  Coins: ");
    Serial.println(c.coins);
    // Remove or comment out the next two lines if you do not want the numeric index:
    // Serial.print("  creatureType: ");
    // Serial.println(c.creatureType);

    // Display creature name instead of the numeric type
    Serial.print("  creatureName: ");
    Serial.println(c.creatureName);
    Serial.print("  customName: ");
    Serial.println(c.customName);

    return c;
}