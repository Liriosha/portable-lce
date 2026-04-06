#pragma once

#include <cstdint>

#include "minecraft/GameEnums.h"

#ifndef MAX_CAPENAME_SIZE
#define MAX_CAPENAME_SIZE 32
#endif

struct MOJANG_DATA {
    eXUID eXuid;
    wchar_t wchCape[MAX_CAPENAME_SIZE];
    wchar_t wchSkin[MAX_CAPENAME_SIZE];
};

struct FEATURE_DATA {
    int x, z;
    _eTerrainFeatureType eTerrainFeature;
};
