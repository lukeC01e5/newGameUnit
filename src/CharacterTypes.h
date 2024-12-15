#ifndef CHARACTERTYPES_H
#define CHARACTERTYPES_H

#include <Arduino.h> // Use Arduino's String class

// Definition of the CharacterType enum
enum CharacterType
{
    UNKNOWN = -1,
    ELF,
    DWARF,
    WIZARD,
    KNIGHT,
    WITCH,
    MERMAID,
    OGRE,
    CHARACTER_TYPE_COUNT  // Sentinel value
};

// Function declaration
CharacterType getCharacterTypeByName(const String &typeName);

// Optional: Function to convert enum to string
String getCharacterTypeName(CharacterType typeEnum);

#endif // CHARACTERTYPES_H