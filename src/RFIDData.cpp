#include "RFIDData.h"
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

    // Ensure mainData has exactly 7 characters (Y CCC W BB)
    if (mainData.length() != 7)
    {
        Serial.println("Invalid data format: Expected 7 characters in main data");
        return;
    }

    // Extract Year Level
    rfidData.yearLevel = mainData.substring(0, 1).toInt();

    // Extract Challenge Code
    rfidData.challengeCode = mainData.substring(1, 4).toInt();

    // Extract Wrong Guesses
    rfidData.wrongGuesses = mainData.substring(4, 5).toInt();

    // Extract Boolean Values
    rfidData.bools = mainData.substring(5, 7).toInt();

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
    Serial.print("Year Level: ");
    Serial.println(rfidData.yearLevel);
    Serial.print("Challenge Code: ");
    Serial.println(rfidData.challengeCode);
    Serial.print("Wrong Guesses: ");
    Serial.println(rfidData.wrongGuesses);
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

    if (mainData.length() < 7)
    {
        Serial.println("Invalid data format: Expected 7 chars in main data");
        return result;
    }

    result.yearLevel = mainData.substring(0, 1).toInt();
    result.challengeCode = mainData.substring(1, 4).toInt();
    result.wrongGuesses = mainData.substring(4, 5).toInt();
    result.boolVal = mainData.substring(5, 7).toInt();
    result.name = nameData;

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

// ...existing code...
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

    // Remove only trailing null characters
    while (result.length() > 0 && result[result.length() - 1] == '\0')
    {
        result.remove(result.length() - 1, 1);
    }

    Serial.print("[readFromRFID] Raw block content: ");
    Serial.println(result);

    // Separate the raw data into intPart (left of '%') and strPart (right of '%')
    int sepIndex = result.indexOf('%');
    if (sepIndex != -1)
    {
        String intPartStr = result.substring(0, sepIndex);
        strPart = result.substring(sepIndex + 1);
        intPart = intPartStr.toInt();

        // Check if all booleans are true (assuming they are stored in the lower bits of intPart)
        bool A = (intPart & 0x01) != 0;
        bool B = (intPart & 0x02) != 0;
        bool C = (intPart & 0x04) != 0;
        bool D = (intPart & 0x08) != 0;

        Serial.println("[readFromRFID] Bool values:");
        Serial.println("A: " + String(A));
        Serial.println("B: " + String(B));
        Serial.println("C: " + String(C));
        Serial.println("D: " + String(D));

        if (A && B && C && D)
        {
            allChallBools = true;
            Serial.println("[readFromRFID] allChallBools set to TRUE");
        }
        else
        {
            allChallBools = false;
            Serial.println("[readFromRFID] allChallBools set to FALSE");
        }
    }
    else
    {
        return "blank profile";
    }

    return result;
}
// ...existing code...
bool writeRFIDData(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const RFIDData &data)
{
    // Debug prints before constructing payload:
    Serial.println("[writeRFIDData] Preparing to write data:");
    Serial.print(" Year Level: ");
    Serial.println(data.yearLevel);
    Serial.print(" Challenge Code: ");
    Serial.println(data.challengeCode);
    Serial.print(" Wrong Guesses: ");
    Serial.println(data.wrongGuesses);
    Serial.print(" Bools: ");
    Serial.println(data.bools, BIN);
    Serial.print(" Name: ");
    Serial.println(data.name);

    // Create payload: "Y CCC W BB %NAME"
    char buffer[16]; // Increased from 12 to 16
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%01d%03d%01d%02d%%%s",
             data.yearLevel,
             data.challengeCode,
             data.wrongGuesses,
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

Creature decode(int numericPart, const String &namePart)
{
    // The Creature struct, per your new system, should look similar to:
    // struct Creature {
    //     int yearLevel;
    //     int challengeCode;
    //     int wrongGuesses;
    //     String customName;
    //     int intVal; // was boolVal
    // };

    Creature c;

    // Convert numericPart to a 7-digit string (leading zeros included)
    char buffer[8];
    snprintf(buffer, sizeof(buffer), "%07d", numericPart);
    String mainData = buffer;

    // We expect exactly 7 chars: [0]=yearLevel, [1..3]=challengeCode, [4]=wrongGuesses, [5..6]=intVal
    if (mainData.length() < 7)
    {
        Serial.println("[decode] Not enough digits in numericPart");
        return c; // return empty if invalid
    }

    // Parse fields
    c.yearLevel = mainData.substring(0, 1).toInt();
    c.challengeCode = mainData.substring(1, 4).toInt();
    c.wrongGuesses = mainData.substring(4, 5).toInt();
    c.boolVal = mainData.substring(5, 7).toInt();

    // Use namePart directly for customName (up to 6 chars if you want to limit it)
    // For now, let's just store the full string:
    c.customName = namePart;

    // Debug output
    Serial.println("[decode] Created Creature from numericPart & namePart:");
    Serial.print("  Year Level: ");
    Serial.println(c.yearLevel);
    Serial.print("  Challenge Code: ");
    Serial.println(c.challengeCode);
    Serial.print("  Wrong Guesses: ");
    Serial.println(c.wrongGuesses);
    Serial.print("  customName: ");
    Serial.println(c.customName);
    Serial.print("  intVal: ");
    Serial.println(c.boolVal);

    return c;
}

bool clearChallBools(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const Creature &creature)
{
    Serial.println(creature.customName);

    // 1) Set intVal to 0
    Creature updatedCreature = creature;
    updatedCreature.boolVal = 0;

    // 2) Build payload: "Y CCC W BB%NAME" (each field is 2 digits)
    char buffer[12];
    snprintf(buffer, sizeof(buffer), "%01d%03d%01d%02d%%%s",
             updatedCreature.yearLevel,
             updatedCreature.challengeCode,
             updatedCreature.wrongGuesses,
             updatedCreature.boolVal,
             updatedCreature.customName.substring(0, 6).c_str());
    String payload = String(buffer);

    Serial.print("[clearChallBools] Final payload ready...... ");

    // 3) Write to RFID
    return writeToRFID(mfrc522, key, payload, 1);
}
