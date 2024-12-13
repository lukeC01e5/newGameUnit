#include "CharacterTypes.h"

// Array of character type names indexed by CharacterType enum
const char* characterTypeNames[] = {
    "Unknown", // 0
    "Elf",     // 1
    "Dwarf",   // 2
    "Wizard",  // 3
    "Knight",  // 4
    "Witch",   // 5
    "Mermaid", // 6
    "Ogre"     // 7
};

// Ensure the array size matches the enum count
static_assert(sizeof(characterTypeNames) / sizeof(characterTypeNames[0]) == CHARACTER_TYPE_COUNT, "CharacterType names array size does not match enum count.");

/**
 * @brief Returns the string name of the CharacterType enum.
 *
 * @param type The CharacterType.
 * @return The name as an Arduino String.
 */
String getCharacterTypeName(CharacterType type) {
    if (type >= UNKNOWN && type < CHARACTER_TYPE_COUNT) {
        return String(characterTypeNames[type]);
    }
    return "Unknown";
}