#include "RFIDFunctions.h"
#include "globals.h" // Ensure this header includes necessary globals like mfrc522 and key
#include "CharacterTypes.h"

/**
 * @brief Writes data to a specific RFID block.
 *
 * @param data The data string to write (must be exactly 16 characters).
 * @param blockAddr The block address to write to.
 * @return true if successful, false otherwise.
 */
bool writeToRFID(const String &data, byte blockAddr)
{
    Serial.print("Attempting to write to block ");
    Serial.println(blockAddr);
    Serial.print("Data to write: '");
    Serial.print(data);
    Serial.println("'");

    if (data.length() != 16)
    {
        Serial.println("Data length must be exactly 16 characters.");
        return false;
    }

    MFRC522::StatusCode status;

    // Authenticate using key A
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Convert String to byte array
    byte buffer[16];
    data.getBytes(buffer, 17); // 16 bytes + null terminator

    // Write data to the block
    status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Writing failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return false;
    }

    Serial.println("Write operation successful.");

    // Stop authentication
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return true;
}

/**
 * @brief Reads data from a specific RFID block.
 *
 * @param blockAddr The block address to read from.
 * @return The data string read from the block.
 */
String readFromRFID(byte blockAddr)
{
    String data = "";

    Serial.print("Attempting to read from block ");
    Serial.println(blockAddr);

    MFRC522::StatusCode status;

    // Authenticate using key A
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return "Invalid block data format";
    }

    // Read data from the block
    byte buffer[18];
    byte size = sizeof(buffer);
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK)
    {
        Serial.print("Reading failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        return "Invalid block data format";
    }

    // Convert byte array to String
    for (int i = 0; i < 16; i++)
    {
        if (buffer[i] != 0)
            data += (char)buffer[i];
    }

    // Stop authentication
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    // Trim any trailing spaces
    data.trim();

    Serial.print("Data read: '");
    Serial.print(data);
    Serial.println("'");

    return data;
}

/**
 * @brief Parses the read RFID data string into the RFIDData struct.
 *
 * @param blockData The raw data string read from the RFID block.
 * @param rfidData The RFIDData struct to populate.
 */
void parseRFIDData(const String &blockData, RFIDData &rfidData)
{
    // Expected format: "name,age,typeIndex,g"
    int firstComma = blockData.indexOf(',');
    int secondComma = blockData.indexOf(',', firstComma + 1);
    int thirdComma = blockData.indexOf(',', secondComma + 1);

    if (firstComma == -1 || secondComma == -1 || thirdComma == -1)
    {
        Serial.println("Invalid block data format");
        return;
    }

    rfidData.name = blockData.substring(0, firstComma);
    rfidData.age = blockData.substring(firstComma + 1, secondComma).toInt();
    int typeIndex = blockData.substring(secondComma + 1, thirdComma).toInt();
    if (typeIndex >= ELF && typeIndex < CHARACTER_TYPE_COUNT)
    {
        rfidData.characterType = static_cast<CharacterType>(typeIndex);
    }
    else
    {
        rfidData.characterType = UNKNOWN;
    }
    rfidData.gender = blockData.charAt(thirdComma + 1);
}