// RFIDData.cpp
#include "RFIDData.h"
#include "Creature.h"
#include <stdio.h> // For snprintf
#include "GlobalDefs.h"

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

    // Extract Custom Name (ensure it's trimmed to 6 characters)
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
    // Serial.print("Creature Type: ");
    // Serial.println(rfidData.creatureType);
    Serial.print("Bools: ");
    Serial.println(rfidData.bools, BIN); // Print as binary

    // Strip any '\0' in rfidData.name:
    int nullPos;
    while ((nullPos = rfidData.name.indexOf('\0')) != -1)
    {
        rfidData.name.remove(nullPos, 1);
    }

    // Now prints the trimmed name only:
    Serial.print("Custom Name: ");
    Serial.println(rfidData.name);
}

RFIDParsed parseRawRFID(const String &raw)
{

    Serial.println("parsed called");

    RFIDParsed result{0, 0, 0, 0, ""};

    int sepIndex = raw.indexOf('%');
    if (sepIndex == -1)
    {
        Serial.println("Invalid data format: Missing '%'");
        return result;
    }
    String mainData = raw.substring(0, sepIndex);
    String nameData = raw.substring(sepIndex + 1);

    if (mainData.length() < 8)
    {
        Serial.println("Invalid data format: Expected 8 chars in main data");
        return result;
    }

    result.age = mainData.substring(0, 2).toInt();
    result.coins = mainData.substring(2, 4).toInt();
    result.creatureType = mainData.substring(4, 6).toInt();
    result.boolVal = mainData.substring(6, 8).toInt();
    result.name = nameData.substring(0, 6);

    // Remove embedded '\0'
    int nullPos;
    while ((nullPos = result.name.indexOf('\0')) != -1)
    {
        result.name.remove(nullPos, 1);
    }
    result.name.trim();

    return result;
}

uint8_t encodeBools(bool A, bool B, bool C, bool D)
{
    uint8_t result = 0;
    result |= (A << 0);
    result |= (B << 1);
    result |= (C << 2);
    result |= (D << 3);
    return result;
}

// Helper function to pad numbers with leading zeros
String padNumber(int number, int width)
{
    char buffer[10];
    snprintf(buffer, sizeof(buffer), "%0*d", width, number);
    return String(buffer);
}

// Helper function to pad a string with spaces to a fixed length
String padString(String str, int targetLength, char padChar = ' ')
{
    while (str.length() < targetLength)
    {
        str += padChar;
    }
    if (str.length() > targetLength)
    {
        str = str.substring(0, targetLength);
    }
    return str;
}

String readFromRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, byte blockAddr, int &intPart, String &strPart)
{
    Serial.println("[readFromRFID] Attempting to read block " + String(blockAddr));

    MFRC522::StatusCode status;

    // Authenticate with the card
    byte trailerBlock = (blockAddr / 4) * 4 + 3; // Trailer block for the sector

    // Default key for MIFARE cards (0xFF)
    for (byte i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    Serial.print("Authenticating with trailer block ");
    Serial.println(trailerBlock);

    // Authenticate
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailerBlock,
        &key,
        &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
        return "blank profile";
    }

    // Read data from the block
    byte buffer[18];
    byte size = sizeof(buffer);
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);

    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Read failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
        return "blank profile";
    }

    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

    // Convert buffer to String (16 bytes + possible null terminators)
    String result;
    for (byte i = 0; i < 16; i++)
    {
        result += (char)buffer[i];
    }

    // Remove only trailing null (0x00) characters, keeping leading zeros
    while (result.length() > 0 && result[result.length() - 1] == '\0')
    {
        result.remove(result.length() - 1, 1);
    }

    Serial.print("[readFromRFID] Raw block content: ");
    Serial.println(result);

    // Separate the raw data into intPart and strPart
    int sepIndex = result.indexOf('%');
    if (sepIndex != -1)
    {
        // Keep the left side exactly as is (including leading zeros)
        String intPartStr = result.substring(0, sepIndex);
        strPart = result.substring(sepIndex + 1);

        // Convert to int if needed, but that loses leading zeros numerically
        // If you need to preserve them for display, use intPartStr
        intPart = intPartStr.toInt();
    }
    else
    {
        // If no '%' is present, return "blank profile"
        return "blank profile";
    }

    return result;
}

bool writeRFIDData(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const RFIDData &data)
{
    // Debug prints before constructing payload:
    Serial.println("[writeRFIDData] Preparing to write data:");
    Serial.print(" Age: ");
    Serial.println(data.age);
    Serial.print(" Coins: ");
    Serial.println(data.coins);
    // Serial.print(" CreatureType: ");
    // Serial.println(data.creatureType);
    Serial.print(" Bools: ");
    Serial.println(data.bools, BIN);
    Serial.print(" Name: ");
    Serial.println(data.name);

    // Create payload: "AA CC TT BB %NAME"
    char buffer[15];
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%02d%02d%02d%02d%%%s",
             data.age,
             data.coins,
             data.creatureType,
             data.bools,
             data.name.substring(0, 6).c_str());
    String payload = String(buffer);

    // Debug
    Serial.print("[writeRFIDData] Final payload: ");
    Serial.println(payload);

    // Use helper
    bool result = writeToRFID(mfrc522, key, payload, 1);
    if (result)
    {
        Serial.println("[writeRFIDData] Write SUCCESS!");
    }
    else
    {
        Serial.println("[writeRFIDData] Write FAILED!");
    }
    return result;
}

bool writeToRFID(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const String &data, byte blockAddr)
{
    MFRC522::StatusCode status;
    byte trailerBlock = (blockAddr / 4) * 4 + 3;

    // Initialize Key A to defaults
    for (int i = 0; i < 6; i++)
    {
        key.keyByte[i] = 0xFF;
    }

    // Authenticate
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                      trailerBlock,
                                      &key,
                                      &mfrc522.uid);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    // Prepare 16-byte data buffer
    byte dataBlock[16];
    memset(dataBlock, 0, 16); // Zero out
    for (int i = 0; i < 16 && i < (int)data.length(); i++)
    {
        dataBlock[i] = data.charAt(i);
    }

    // Write block
    status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Write failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    // Stop encryption
    mfrc522.PCD_StopCrypto1();
    return true;
}

// Implementation of decode(int, String)
Creature decode(int numericPart, const String &namePart)
{
    Creature c;

    // Convert numericPart to an 8-digit string (leading zeros included)
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%08d", numericPart);
    String mainData = buffer;
    Serial.print("yes here?? ");
    // Expect mainData to have 8 chars => [0..1] age, [2..3] coins, [4..5] creatureType, [6..7] boolVal
    int lengthNeeded = 8;
    if (mainData.length() < lengthNeeded)
    {
        Serial.println("decode: Not enough digits in numericPart");
        return c; // return an empty Creature
    }
    Serial.print("yes here?? ");
    // Parse fields
    c.trainerAge = mainData.substring(0, 2).toInt();
    c.coins = mainData.substring(2, 4).toInt();
    c.creatureType = mainData.substring(4, 6).toInt();
    c.boolVal = mainData.substring(6, 8).toInt();
    // If your Creature struct has a boolVal, parse it here, e.g.:
    // int boolVal    = mainData.substring(6, 8).toInt();

    // Assign the name, up to 6 chars
    if (namePart.length() > 6)
    {
        c.customName = namePart.substring(0, 6);
    }
    else
    {
        c.customName = namePart;
    }

    Serial.print("def not here?? ");

    // Example creature lookup:
    if (c.creatureType >= 0 && c.creatureType < 35)
    {
        c.creatureName = "CreatureLookup"; // or creatures[c.creatureType];
    }
    else
    {
        c.creatureName = "Unknown Creature";
    }
    Serial.print("gotchya ");
    // Remove any embedded nulls
    /*
    int nullPos;
    while ((nullPos = c.customName.indexOf('\0')) != -1)
    {
        c.customName.remove(nullPos, 1);
    }
*/
    Serial.print("so here then?? ");

    // Debug output
    Serial.println("[decode] Created Creature from numericPart & namePart:");
    Serial.print("  Age: ");
    Serial.println(c.trainerAge);
    Serial.print("  Coins: ");
    Serial.println(c.coins);
    Serial.print("  creatureType: ");
    Serial.println(c.creatureType);
    Serial.print("  customName: ");
    Serial.println(c.customName);
    Serial.print("  intVal: ");
    Serial.println(c.boolVal);

    return c;
}

// If you need an older decode(const String &), keep it. Otherwise remove it.
// Creature decode(const String &rawData) { /* old version */ }