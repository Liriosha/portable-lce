#pragma once

#include <cstdint>

namespace Strings {

using LookupFn = const wchar_t* (*)(int);

void init(LookupFn fn);
[[nodiscard]] const wchar_t* get(int id);

}  // namespace Strings
