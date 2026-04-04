#pragma once

#include <cstdint>

namespace DebugSettings {

using BoolFn = bool (*)();
using MaskFn = unsigned int (*)(int, bool);

void init(BoolFn debugOn, BoolFn artToolsOn, MaskFn mask,
          BoolFn mobsDontAttack, BoolFn mobsDontTick, BoolFn freezePlayers);

[[nodiscard]] bool isOn();
[[nodiscard]] bool artToolsOn();
[[nodiscard]] unsigned int getMask(int iPad = -1, bool overridePlayer = false);
[[nodiscard]] bool mobsDontAttack();
[[nodiscard]] bool mobsDontTick();
[[nodiscard]] bool freezePlayers();

}  // namespace DebugSettings
