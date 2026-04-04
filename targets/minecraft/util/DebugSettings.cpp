#include "minecraft/util/DebugSettings.h"

#include <cassert>

namespace DebugSettings {

static BoolFn s_debugOn = nullptr;
static BoolFn s_artToolsOn = nullptr;
static MaskFn s_mask = nullptr;
static BoolFn s_mobsDontAttack = nullptr;
static BoolFn s_mobsDontTick = nullptr;
static BoolFn s_freezePlayers = nullptr;

void init(BoolFn debugOn, BoolFn artToolsOn, MaskFn mask,
          BoolFn mobsDontAttack, BoolFn mobsDontTick, BoolFn freezePlayers) {
    s_debugOn = debugOn;
    s_artToolsOn = artToolsOn;
    s_mask = mask;
    s_mobsDontAttack = mobsDontAttack;
    s_mobsDontTick = mobsDontTick;
    s_freezePlayers = freezePlayers;
}

bool isOn() { return s_debugOn && s_debugOn(); }
bool artToolsOn() { return s_artToolsOn && s_artToolsOn(); }
unsigned int getMask(int iPad, bool overridePlayer) {
    return s_mask ? s_mask(iPad, overridePlayer) : 0;
}
bool mobsDontAttack() { return s_mobsDontAttack && s_mobsDontAttack(); }
bool mobsDontTick() { return s_mobsDontTick && s_mobsDontTick(); }
bool freezePlayers() { return s_freezePlayers && s_freezePlayers(); }

}  // namespace DebugSettings
