#include "Storage.h"

#include <stdlib.h>
#include <time.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "StdFileIO.h"

C4JStorage StorageManager;

static XMARKETPLACE_CONTENTOFFER_INFO s_dummyOffer = {};
static XCONTENT_DATA s_dummyContentData = {};

// def save state
// TODO: find a better way
static SAVE_DETAILS* s_SavesInfo = nullptr;
static std::wstring s_CurrentSaveTitle = L"New World";
static std::string s_CurrentSaveFilename = "";
static bool s_SaveDisabled = false;

// Console VFS Memory Blob
static std::uint8_t* s_SaveBuffer = nullptr;
static unsigned int s_SaveBufferSize = 0;

struct SubfileData {
    std::vector<std::uint8_t> data;
};
static std::map<int, SubfileData> s_Subfiles;
static int s_SubfileCounter = 0;

// helper functions
static StdFileIO s_FileIO;

static std::filesystem::path GetSavesRoot() {
    return s_FileIO.getUserDataPath() / "Saves";
}

static std::filesystem::path GetSaveDir(const std::string& saveFilename) {
    return GetSavesRoot() / saveFilename;
}

static std::filesystem::path GetSaveFile(const std::string& saveFilename,
                                         const std::string& filename) {
    return GetSaveDir(saveFilename) / filename;
}
C4JStorage::C4JStorage() : m_pStringTable(nullptr) {}

void C4JStorage::Tick(void) {}

C4JStorage::EMessageResult C4JStorage::RequestMessageBox(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int pad,
    std::function<int(int, const C4JStorage::EMessageResult)> callback,
    C4JStringTable* pStringTable, wchar_t* pwchFormatString,
    unsigned int focusButton) {
    if (callback) callback(pad, EMessage_ResultAccept);
    return EMessage_ResultAccept;
}

C4JStorage::EMessageResult C4JStorage::GetMessageBoxResult() {
    return EMessage_Undefined;
}

bool C4JStorage::SetSaveDevice(std::function<int(const bool)> callback,
                               bool bForceResetOfSaveDevice) {
    if (callback) callback(true);
    return true;
}

void C4JStorage::Init(unsigned int uiSaveVersion,
                      const wchar_t* pwchDefaultSaveName, char* pszSavePackName,
                      int iMinimumSaveSize,
                      std::function<int(const ESavingMessage, int)> callback,
                      const char* szGroupID) {
    std::error_code ec;
    std::filesystem::create_directories(GetSavesRoot(), ec);
}

void C4JStorage::ResetSaveData() {
    s_CurrentSaveTitle = L"New World";
    s_CurrentSaveFilename = "";
    s_Subfiles.clear();
    s_SubfileCounter = 0;
    if (s_SaveBuffer) {
        free(s_SaveBuffer);
        s_SaveBuffer = nullptr;
    }
    s_SaveBufferSize = 0;
}

void C4JStorage::SetDefaultSaveNameForKeyboardDisplay(
    const wchar_t* pwchDefaultSaveName) {
    if (pwchDefaultSaveName) s_CurrentSaveTitle = pwchDefaultSaveName;
}

void C4JStorage::SetSaveTitle(const wchar_t* pwchDefaultSaveName) {
    if (pwchDefaultSaveName) s_CurrentSaveTitle = pwchDefaultSaveName;

    if (s_CurrentSaveFilename.empty()) {
        char buf[16];
        snprintf(buf, sizeof(buf), "sv%06X",
                 (unsigned int)(time(NULL) & 0xFFFFFF));
        s_CurrentSaveFilename = buf;

        std::error_code ec;
        std::filesystem::create_directories(GetSaveDir(s_CurrentSaveFilename),
                                            ec);
    }
}

bool C4JStorage::GetSaveUniqueNumber(int* piVal) {
    if (piVal) *piVal = 1; // she turned a 0 to a 1 now the world explodes
    return true;
}

bool C4JStorage::GetSaveUniqueFilename(char* pszName) {
    if (pszName) {
        strncpy(pszName, s_CurrentSaveFilename.c_str(),
                MAX_SAVEFILENAME_LENGTH - 1);
        pszName[MAX_SAVEFILENAME_LENGTH - 1] = '\0';
    }
    return true;
}
void C4JStorage::SetSaveUniqueFilename(char* szFilename) {
    if (szFilename) s_CurrentSaveFilename = szFilename;
}
void C4JStorage::SetState(ESaveGameControlState eControlState,
                          std::function<int(const bool)> callback) {
    if (callback) callback(true);
}
void C4JStorage::SetSaveDisabled(bool bDisable) { s_SaveDisabled = bDisable; }
bool C4JStorage::GetSaveDisabled(void) { return s_SaveDisabled; }
void* C4JStorage::AllocateSaveData(unsigned int uiBytes) {
    if (s_SaveBuffer) free(s_SaveBuffer);
    s_SaveBuffer = (std::uint8_t*)malloc(uiBytes);
    s_SaveBufferSize = uiBytes;
    return s_SaveBuffer;
}
void C4JStorage::GetSaveData(void* pvData, unsigned int* puiBytes) {
    if (puiBytes) *puiBytes = s_SaveBufferSize;
    if (pvData && s_SaveBuffer) memcpy(pvData, s_SaveBuffer, s_SaveBufferSize);
}
unsigned int C4JStorage::GetSaveSize() { return s_SaveBufferSize; }
void C4JStorage::SetSaveImages(std::uint8_t* pbThumbnail,
                               unsigned int thumbnailBytes,
                               std::uint8_t* pbImage, unsigned int imageBytes,
                               std::uint8_t* pbTextData,
                               unsigned int textDataBytes) {
    if (pbThumbnail && thumbnailBytes > 0 && !s_CurrentSaveFilename.empty()) {
        auto dir = GetSaveDir(s_CurrentSaveFilename);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        s_FileIO.writeFile(dir / "thumbnail.png", pbThumbnail, thumbnailBytes);
    }
}

C4JStorage::ESaveGameState C4JStorage::SaveSaveData(
    std::function<int(const bool)> callback) {
    if (s_CurrentSaveFilename.empty()) {
        if (callback) callback(false);
        return ESaveGame_Idle;
    }

    auto dir = GetSaveDir(s_CurrentSaveFilename);
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    // vfs blob -> disk
    if (s_SaveBuffer && s_SaveBufferSize > 0)
        s_FileIO.writeFile(dir / "savegame.dat", s_SaveBuffer,
                           s_SaveBufferSize);

    // save... title.. in txt... they do that.. honestly
    // i would wrap world info in a json.
    // but i'm not rewriting the whole
    // save -> storage system sooooooo nope!
    std::string titleStr(s_CurrentSaveTitle.begin(), s_CurrentSaveTitle.end());
    s_FileIO.writeFile(dir / "title.txt", titleStr.data(), titleStr.size());

    if (callback) callback(true);
    return ESaveGame_Save;
}
void C4JStorage::CopySaveDataToNewSave(std::uint8_t* pbThumbnail,
                                       unsigned int cbThumbnail,
                                       wchar_t* wchNewName,
                                       std::function<int(bool)> callback) {
    if (callback) callback(false);
}

void C4JStorage::SetSaveDeviceSelected(unsigned int uiPad, bool bSelected) {}
bool C4JStorage::GetSaveDeviceSelected(unsigned int iPad) { return true; }
C4JStorage::ESaveGameState C4JStorage::DoesSaveExist(bool* pbExists) {
    if (pbExists)
        *pbExists = s_FileIO.exists(GetSaveDir(s_CurrentSaveFilename));
    return ESaveGame_Idle;
}
bool C4JStorage::EnoughSpaceForAMinSaveGame() { return true; }
void C4JStorage::SetSaveMessageVPosition(float fY) {}
C4JStorage::ESaveGameState C4JStorage::GetSavesInfo(
    int iPad,
    std::function<int(SAVE_DETAILS* pSaveDetails, const bool)> callback,
    char* pszSavePackName) {
    ClearSavesInfo();

    auto savesRoot = GetSavesRoot();
    std::error_code ec;
    std::filesystem::create_directories(savesRoot, ec);

    std::vector<std::string> saveDirs;

    for (const auto& entry :
         std::filesystem::directory_iterator(savesRoot, ec)) {
        if (!entry.is_directory()) continue;

        std::string name = entry.path().filename().string();
        bool hasSave = s_FileIO.exists(entry.path() / "savegame.dat") ||
                       s_FileIO.exists(entry.path() / "subfile_0.dat") ||
                       s_FileIO.exists(entry.path() / "title.txt");

        if (hasSave) saveDirs.push_back(name);
    }

    s_SavesInfo = new SAVE_DETAILS();
    s_SavesInfo->iSaveC = (int)saveDirs.size();

    if (s_SavesInfo->iSaveC > 0) {
        s_SavesInfo->SaveInfoA = new SAVE_INFO[s_SavesInfo->iSaveC];
        for (size_t i = 0; i < saveDirs.size(); ++i) {
            SAVE_INFO& info = s_SavesInfo->SaveInfoA[i];
            memset(&info, 0, sizeof(SAVE_INFO));

            strncpy(info.UTF8SaveFilename, saveDirs[i].c_str(),
                    MAX_SAVEFILENAME_LENGTH - 1);

            auto titleData =
                s_FileIO.readFileToVec(GetSaveFile(saveDirs[i], "title.txt"));
            if (!titleData.empty()) {
                std::string title(titleData.begin(), titleData.end());
                strncpy(info.UTF8SaveTitle, title.c_str(),
                        MAX_DISPLAYNAME_LENGTH - 1);
            } else {
                strncpy(info.UTF8SaveTitle, saveDirs[i].c_str(),
                        MAX_DISPLAYNAME_LENGTH - 1);
            }
        }
    } else {
        s_SavesInfo->SaveInfoA = nullptr;
    }

    if (callback) callback(s_SavesInfo, true);
    return ESaveGame_Idle;
}

PSAVE_DETAILS C4JStorage::ReturnSavesInfo() { return s_SavesInfo; }

void C4JStorage::ClearSavesInfo() {
    if (s_SavesInfo) {
        if (s_SavesInfo->SaveInfoA) {
            for (int i = 0; i < s_SavesInfo->iSaveC; ++i) {
                if (s_SavesInfo->SaveInfoA[i].thumbnailData)
                    delete[] s_SavesInfo->SaveInfoA[i].thumbnailData;
            }
            delete[] s_SavesInfo->SaveInfoA;
        }
        delete s_SavesInfo;
        s_SavesInfo = nullptr;
    }
}

C4JStorage::ESaveGameState C4JStorage::LoadSaveDataThumbnail(
    PSAVE_INFO pSaveInfo,
    std::function<int(std::uint8_t* thumbnailData, unsigned int thumbnailBytes)>
        callback) {
    if (pSaveInfo) {
        auto data = s_FileIO.readFileToVec(
            GetSaveFile(pSaveInfo->UTF8SaveFilename, "thumbnail.png"));
        if (!data.empty()) {
            if (callback) callback(data.data(), (unsigned int)data.size());
            return (C4JStorage::ESaveGameState)4;
        }
    }
    if (callback) callback(nullptr, 0);
    return (C4JStorage::ESaveGameState)4;
}
void C4JStorage::GetSaveCacheFileInfo(unsigned int fileIndex,
                                      XCONTENT_DATA& xContentData) {
    memset(&xContentData, 0, sizeof(xContentData));
}
void C4JStorage::GetSaveCacheFileInfo(unsigned int fileIndex,
                                      std::uint8_t** ppbImageData,
                                      unsigned int* pImageBytes) {
    if (ppbImageData) *ppbImageData = nullptr;
    if (pImageBytes) *pImageBytes = 0;
}
C4JStorage::ESaveGameState C4JStorage::LoadSaveData(
    PSAVE_INFO pSaveInfo, std::function<int(const bool, const bool)> callback) {
    if (pSaveInfo) {
        s_CurrentSaveFilename = pSaveInfo->UTF8SaveFilename;
        std::string title(pSaveInfo->UTF8SaveTitle);
        s_CurrentSaveTitle = std::wstring(title.begin(), title.end());

        auto blobData = s_FileIO.readFileToVec(
            GetSaveFile(s_CurrentSaveFilename, "savegame.dat"));
        if (!blobData.empty()) {
            if (s_SaveBuffer) free(s_SaveBuffer);
            s_SaveBufferSize = (unsigned int)blobData.size();
            s_SaveBuffer = (std::uint8_t*)malloc(s_SaveBufferSize);
            memcpy(s_SaveBuffer, blobData.data(), s_SaveBufferSize);
        }

        // put in mem
        s_Subfiles.clear();
        for (int i = 0; i < 50; i++) {
            auto subData = s_FileIO.readFileToVec(
                GetSaveFile(s_CurrentSaveFilename,
                            "subfile_" + std::to_string(i) + ".dat"));
            if (!subData.empty()) {
                SubfileData sd;
                sd.data = std::move(subData);
                s_Subfiles[i] = std::move(sd);
            }
        }
    }

    if (callback) callback(false, true);
    return (C4JStorage::ESaveGameState)1;
}
C4JStorage::ESaveGameState C4JStorage::DeleteSaveData(
    PSAVE_INFO pSaveInfo, std::function<int(const bool)> callback) {
    bool success = false;
    if (pSaveInfo) {
        std::error_code ec;
        std::filesystem::remove_all(GetSaveDir(pSaveInfo->UTF8SaveFilename),
                                    ec);
        success = !ec;
    }
    if (callback) callback(success);
    return (C4JStorage::ESaveGameState)3;
}
void C4JStorage::RegisterMarketplaceCountsCallback(
    std::function<int(C4JStorage::DLC_TMS_DETAILS*, int)> callback) {}
void C4JStorage::SetDLCPackageRoot(char* pszDLCRoot) {}
C4JStorage::EDLCStatus C4JStorage::GetDLCOffers(
    int iPad, std::function<int(int, std::uint32_t, int)> callback,
    std::uint32_t dwOfferTypesBitmask) {
    if (callback) callback(0, dwOfferTypesBitmask, iPad);
    return EDLC_NoOffers;
}
unsigned int C4JStorage::CancelGetDLCOffers() { return 0; }
void C4JStorage::ClearDLCOffers() {}
XMARKETPLACE_CONTENTOFFER_INFO& C4JStorage::GetOffer(unsigned int dw) {
    return s_dummyOffer;
}
int C4JStorage::GetOfferCount() { return 0; }
unsigned int C4JStorage::InstallOffer(int iOfferIDC, std::uint64_t* ullOfferIDA,
                                      std::function<int(int, int)> callback,
                                      bool bTrial) {
    return 0;
}
unsigned int C4JStorage::GetAvailableDLCCount(int iPad) { return 0; }
C4JStorage::EDLCStatus C4JStorage::GetInstalledDLC(
    int iPad, std::function<int(int, int)> callback) {
    if (callback) {
        callback(0, iPad);
    }
    return EDLC_NoInstalledDLC;
}
XCONTENT_DATA& C4JStorage::GetDLC(unsigned int dw) {
    return s_dummyContentData;
}
std::uint32_t C4JStorage::MountInstalledDLC(
    int iPad, std::uint32_t dwDLC,
    std::function<int(int, std::uint32_t, std::uint32_t)> callback,
    const char* szMountDrive) {
    return 0;
}
unsigned int C4JStorage::UnmountInstalledDLC(const char* szMountDrive) {
    return 0;
}
void C4JStorage::GetMountedDLCFileList(const char* szMountDrive,
                                       std::vector<std::string>& fileList) {
    fileList.clear();
}
std::string C4JStorage::GetMountedPath(std::string szMount) { return ""; }
C4JStorage::ETMSStatus C4JStorage::ReadTMSFile(
    int iQuadrant, eGlobalStorage eStorageFacility,
    C4JStorage::eTMS_FileType eFileType, wchar_t* pwchFilename,
    std::uint8_t** ppBuffer, unsigned int* pBufferSize,
    std::function<int(wchar_t*, int, bool, int)> callback, int iAction) {
    return ETMSStatus_Fail;
}
bool C4JStorage::WriteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                              wchar_t* pwchFilename, std::uint8_t* pBuffer,
                              unsigned int bufferSize) {
    return false;
}
bool C4JStorage::DeleteTMSFile(int iQuadrant, eGlobalStorage eStorageFacility,
                               wchar_t* pwchFilename) {
    return false;
}
void C4JStorage::StoreTMSPathName(wchar_t* pwchName) {}
C4JStorage::ETMSStatus C4JStorage::TMSPP_ReadFile(
    int iPad, C4JStorage::eGlobalStorage eStorageFacility,
    C4JStorage::eTMS_FILETYPEVAL eFileTypeVal, const char* szFilename,
    std::function<int(int, int, PTMSPP_FILEDATA, const char*)> callback,
    int iUserData) {
    return ETMSStatus_Fail;
}
unsigned int C4JStorage::CRC(unsigned char* buf, int len) {
    unsigned int crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
    }
    return ~crc;
}

// that fucking bird that i hate
int C4JStorage::AddSubfile(int regionIndex) {
    s_Subfiles[s_SubfileCounter] = SubfileData();
    return s_SubfileCounter++;
}

unsigned int C4JStorage::GetSubfileCount() { return s_Subfiles.size(); }

void C4JStorage::GetSubfileDetails(unsigned int i, int* regionIndex,
                                   void** data, unsigned int* size) {
    auto it = s_Subfiles.find(i);
    if (it != s_Subfiles.end()) {
        if (regionIndex) *regionIndex = 0;
        if (data) *data = it->second.data.data();
        if (size) *size = (unsigned int)it->second.data.size();
    } else {
        if (regionIndex) *regionIndex = 0;
        if (data) *data = nullptr;
        if (size) *size = 0;
    }
}

void C4JStorage::ResetSubfiles() {
    s_Subfiles.clear();
    s_SubfileCounter = 0;
}
void C4JStorage::UpdateSubfile(int index, void* data, unsigned int size) {
    SubfileData& sd = s_Subfiles[index];  // inserts if missing
    sd.data.resize(size);
    memcpy(sd.data.data(), data, size);
}
void C4JStorage::SaveSubfiles(std::function<int(const bool)> callback) {
    if (!s_CurrentSaveFilename.empty() && !s_Subfiles.empty()) {
        auto dir = GetSaveDir(s_CurrentSaveFilename);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        for (const auto& pair : s_Subfiles) {
            s_FileIO.writeFile(
                dir / ("subfile_" + std::to_string(pair.first) + ".dat"),
                pair.second.data.data(), pair.second.data.size());
        }
    }
    if (callback) callback(true);
}
C4JStorage::ESaveGameState C4JStorage::GetSaveState() { return ESaveGame_Idle; }
void C4JStorage::ContinueIncompleteOperation() {}
