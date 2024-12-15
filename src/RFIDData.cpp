// RFIDData.cpp
#include "RFIDData.h"

void parseRFIDData(const String &data, RFIDData &rfidData)
{
    int startIndex, endIndex;

    // Extract name
    startIndex = data.indexOf("&||name.") + 8;
    endIndex = data.indexOf("||&", startIndex);
    if (startIndex > 7 && endIndex > startIndex)
    {
        rfidData.name = data.substring(startIndex, endIndex);
    }
    else
    {
        rfidData.name = "";
    }

    // Extract age
    startIndex = data.indexOf("&||age.") + 7;
    endIndex = data.indexOf("||&", startIndex);
    if (startIndex > 6 && endIndex > startIndex)
    {
        rfidData.age = data.substring(startIndex, endIndex).toInt();
    }
    else
    {
        rfidData.age = 0;
    }

    // Extract character type
    startIndex = data.indexOf("&||characterType.") + 17;
    endIndex = data.indexOf("||&", startIndex);
    if (startIndex > 16 && endIndex > startIndex)
    {
        rfidData.characterType = data.substring(startIndex, endIndex);
    }
    else
    {
        rfidData.characterType = "";
    }
}