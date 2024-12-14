#include "CharacterTypes.h"

/**
 * @brief Returns the name of the CharacterType enum.
 *
 * @param type The CharacterType enum.
 * @return The corresponding name as a String.
 */
String getCharacterTypeName(CharacterType type)
{
    switch (type)
    {
    case ELF:
        return "Elf";
    case DWARF:
        return "Dwarf";
    case WIZARD:
        return "Wizard";
    case KNIGHT:
        return "Knight";
    case WITCH:
        return "Witch";
    case MERMAID:
        return "Mermaid";
    case OGRE:
        return "Ogre";
    default:
        return "Unknown";
    }
}