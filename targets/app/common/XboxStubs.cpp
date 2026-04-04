#include "platform/PlatformTypes.h"
#include "platform/XboxStubs.h"

bool IsEqualXUID(PlayerUID a, PlayerUID b) { return a == b; }

uint32_t XUserGetSigninInfo(uint32_t dwUserIndex, uint32_t dwFlags,
                            PXUSER_SIGNIN_INFO pSigninInfo) {
    return 0;
}

const wchar_t* CXuiStringTable::Lookup(const wchar_t* szId) { return szId; }
const wchar_t* CXuiStringTable::Lookup(uint32_t nIndex) { return L"String"; }
void CXuiStringTable::Clear() {}
int32_t CXuiStringTable::Load(const wchar_t* szId) { return 0; }

uint32_t XGetLanguage() { return 1; }
uint32_t XGetLocale() { return 0; }
uint32_t XEnableGuestSignin(bool fEnable) { return 0; }
