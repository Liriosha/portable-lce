#pragma once

#include <cstdint>

#include "app/common/App_Defines.h"
#include "app/common/App_enums.h"

namespace GameHostOptions {

unsigned int get(unsigned int settings, eGameHostOption option);
void set(unsigned int& settings, eGameHostOption option, unsigned int value);

// Global game settings - initialized by app layer at startup
void init(unsigned int* settingsPtr);
unsigned int get(eGameHostOption option);
void set(eGameHostOption option, unsigned int value);

}  // namespace GameHostOptions
