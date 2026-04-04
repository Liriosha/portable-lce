#include "minecraft/locale/Strings.h"

#include <cassert>

namespace Strings {

static LookupFn s_lookup = nullptr;

void init(LookupFn fn) { s_lookup = fn; }

const wchar_t* get(int id) {
    assert(s_lookup && "Strings::init() must be called before Strings::get()");
    return s_lookup(id);
}

}  // namespace Strings
