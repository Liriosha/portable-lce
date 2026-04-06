#pragma once

#include <string>

#include "minecraft/GameEnums.h"

class Rarity {
public:
    static const Rarity* common;
    static const Rarity* uncommon;
    static const Rarity* rare;
    static const Rarity* epic;

    const eMinecraftColour color;
    const std::wstring name;

    Rarity(eMinecraftColour color, const std::wstring& name);
};