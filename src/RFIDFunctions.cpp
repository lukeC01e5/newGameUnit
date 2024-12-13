#include "RFIDFunctions.h"

String readFromRFID(byte blockAddr) {
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);
    String data = "";

    // Authenticate with the key
    byte trailerBlock = (blockAddr / 4) * 4 + 3;
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailerBlock,
        &key,
        &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return "";
    }

    // Read data from the block
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Read failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return "";
    }

    // Convert buffer to string
    for (byte i = 0; i < 16; i++) {
        if (buffer[i] != 0) {
            data += (char)buffer[i];
        }
    }

    // Stop encryption
    mfrc522.PCD_StopCrypto1();

    return data;
}

bool writeToRFID(const String &data, byte blockAddr) {
    MFRC522::StatusCode status;

    // Authenticate with the key
    byte trailerBlock = (blockAddr / 4) * 4 + 3;
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A,
        trailerBlock,
        &key,
        &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Authentication failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Prepare data for writing
    byte buffer[16] = {0};
    data.getBytes(buffer, sizeof(buffer));

    // Write data to block
    status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("Write failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
        return false;
    }

    // Stop encryption
    mfrc522.PCD_StopCrypto1();

    return true;
}

void parseRFIDData(const String &data, RFIDData &rfidData) {
    // Reset all fields
    rfidData.name = "";
    rfidData.age = 0;
    rfidData.characterType = "";

    // Extract data between delimiters *&
    int startIdx = data.indexOf("*&");
    int endIdx = data.indexOf("&*", startIdx + 2);
    if (startIdx != -1 && endIdx != -1) {
        String content = data.substring(startIdx + 2, endIdx);
        int delimiterIdx = content.indexOf(".");
        if (delimiterIdx != -1) {
            rfidData.name = content.substring(0, delimiterIdx);
            rfidData.age = content.substring(delimiterIdx + 1).toInt();
        }
    }

    // Extract characterType from the second set of delimiters
    startIdx = data.indexOf("*&", endIdx);
    endIdx = data.indexOf("&*", startIdx + 2);
    if (startIdx != -1 && endIdx != -1) {
        rfidData.characterType = data.substring(startIdx + 2, endIdx);
    }
}