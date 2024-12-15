#include "CharacterTypes.h"

// Converts a character type name to the corresponding CharacterType enum value.
CharacterType getCharacterTypeByName(const String &typeName) {
    if (typeName.equalsIgnoreCase("Elf")) {
        return ELF;
    } else if (typeName.equalsIgnoreCase("Dwarf")) {
        return DWARF;
    } else if (typeName.equalsIgnoreCase("Wizard")) {
        return WIZARD;
    } else if (typeName.equalsIgnoreCase("Knight")) {
        return KNIGHT;
    } else if (typeName.equalsIgnoreCase("Witch")) {
        return WITCH;
    } else if (typeName.equalsIgnoreCase("Mermaid")) {
        return MERMAID;
    } else if (typeName.equalsIgnoreCase("Ogre")) {
        return OGRE;
    } else {
        return UNKNOWN;
    }
}

// Converts a CharacterType enum value to its corresponding string representation.
String getCharacterTypeName(CharacterType typeEnum) {
    switch (typeEnum) {
        case ELF: return "Elf";
        case DWARF: return "Dwarf";
        case WIZARD: return "Wizard";
        case KNIGHT: return "Knight";
        case WITCH: return "Witch";
        case MERMAID: return "Mermaid";
        case OGRE: return "Ogre";
        default: return "Unknown";
    }
}