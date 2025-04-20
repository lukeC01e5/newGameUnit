#include "RFIDData.h"
#include <stdio.h> // For snprintf
#include "GlobalDefs.h"

void parseRFIDData(const String &data, RFIDData &rfidData)
{
    int separatorIndex = data.indexOf('%');
    if (separatorIndex == -1)
    {
        Serial.println("Invalid data format: Missing '%'");
        return;
    }

    // "mainData" = "1112?33"
    String mainData = data.substring(0, separatorIndex);
    // "nameData"  = "Name"
    String nameData = data.substring(separatorIndex + 1);

    if (mainData.length() != 7) // Must be exactly 7: [0..2]=CCC, [3]=W, [4]='?', [5..6]=BB
    {
        Serial.println("Invalid format: Expected 7 chars in main data ('CCCW?BB')");
        return;
    }

    // Parse out the fields
    rfidData.challengeCode = mainData.substring(0, 3).toInt(); // e.g. "111" -> 111
    rfidData.wrongGuesses = mainData.substring(3, 4).toInt();  // e.g. "2"
    // skip mainData[4] because it's '?'
    rfidData.bools = mainData.substring(5, 7).toInt(); // e.g. "33" -> 33 decimal

    // Name up to 8 chars
    rfidData.name = nameData.substring(0, 8);
}

RFIDParsed parseRawRFID(const String &raw)
{
    Serial.println("parsed called");

    // Fix: only four fields for { boolVal, challengeCode, wrongGuesses, name }
    RFIDParsed result{0, 0, 0, ""};

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
    Serial.println("[writeRFIDData] Preparing to write data:");
    Serial.print(" Challenge Code: ");
    Serial.println(data.challengeCode);
    Serial.print(" Wrong Guesses: ");
    Serial.println(data.wrongGuesses);
    Serial.print(" Bools: ");
    Serial.println(data.bools, BIN);
    Serial.print(" Name: ");
    Serial.println(data.name);

    // Now use 7 chars for main data:
    //   0–2 => challengeCode (3 digits),
    //   3   => wrongGuesses (1 digit),
    //   4   => '?' marker,
    //   5–6 => boolVal (2 digits),
    // then '%NAME' for the remainder
    char buffer[16];
    memset(buffer, 0, sizeof(buffer));

    // Example: "CCCW?BB%Name"
    snprintf(buffer, sizeof(buffer), "%03d%01d?%02d%%%s",
             data.challengeCode,
             data.wrongGuesses,
             data.bools,
             data.name.substring(0, 8).c_str());

    String payload = String(buffer);
    Serial.print("[writeRFIDData] Final payload: ");
    Serial.println(payload);

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
    Creature c;

    // Convert numericPart to a 6-digit string
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%06d", numericPart);
    String mainData = buffer;

    // Expect 6 chars: [0..2]=challengeCode, [3]=wrongGuesses, [4..5]=boolVal
    if (mainData.length() < 6)
    {
        Serial.println("[decode] Not enough digits in numericPart");
        return c; // return empty if invalid
    }

    c.challengeCode = mainData.substring(0, 3).toInt();
    c.wrongGuesses = mainData.substring(3, 4).toInt();
    c.boolVal = mainData.substring(4, 6).toInt();
    c.customName = namePart;

    // Debug output
    Serial.println("[decode] Created Creature from numericPart & namePart:");
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
             // updatedCreature.yearLevel,
             updatedCreature.challengeCode,
             updatedCreature.wrongGuesses,
             updatedCreature.boolVal,
             updatedCreature.customName.substring(0, 6).c_str());
    String payload = String(buffer);

    Serial.print("[clearChallBools] Final payload ready...... ");

    // 3) Write to RFID
    return writeToRFID(mfrc522, key, payload, 1);
}

// ...existing code...

bool writeMultiBlock(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, const String &largeData, byte startBlock, byte blockCount)
{
    // Example: write largeData across blockCount blocks starting at startBlock
    // ...existing code logic to authenticate...
    // For each block, take up to 16 bytes from largeData
    // ...existing code...
    for (byte offset = 0; offset < blockCount; offset++)
    {
        byte blockAddr = startBlock + offset;
        String chunk = largeData.substring(offset * 16, (offset + 1) * 16);
        // ...existing code to authenticate and write 'chunk' to blockAddr...
    }
    // ...existing code to finalize...
    return true;
}

// Similar function for reading multiple blocks and reassembling
String readMultiBlock(MFRC522 &mfrc522, MFRC522::MIFARE_Key &key, byte startBlock, byte blockCount)
{
    String result;
    // ...existing code logic to authenticate...
    for (byte offset = 0; offset < blockCount; offset++)
    {
        byte blockAddr = startBlock + offset;
        // ...existing code to read 16 bytes from blockAddr...
        // Append chunk to result
    }
    // Remove trailing null chars if any
    result.trim();
    return result;
}
