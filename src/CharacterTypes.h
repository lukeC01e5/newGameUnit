#ifndef CHARACTERTYPES_H
#define CHARACTERTYPES_H

#include <Arduino.h> // Use Arduino's String class

// Definition of the CharacterType enum
enum CharacterType
{
    ELF = 1,
    DWARF,
    WIZARD,
    KNIGHT,
    WITCH,
    MERMAID,
    OGRE,
    UNKNOWN,
    CHARACTER_TYPE_COUNT // Automatically set to the number of character types
};

// Function declaration
String getCharacterTypeName(CharacterType type);

#endif // CHARACTERTYPES_H