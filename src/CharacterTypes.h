#ifndef CHARACTER_TYPES_H
#define CHARACTER_TYPES_H

#include <Arduino.h> // Use Arduino's String class

// Definition of the CharacterType enum
enum CharacterType
{
    UNKNOWN = 0,
    ELF = 1,
    DWARF,
    WIZARD,
    KNIGHT,
    WITCH,
    MERMAID,
    OGRE,
    CHARACTER_TYPE_COUNT // Automatically set to the number of character types
};

// Function declaration
String getCharacterTypeName(CharacterType type);

#endif // CHARACTER_TYPES_H