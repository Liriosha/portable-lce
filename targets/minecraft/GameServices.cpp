#include "minecraft/GameServices.h"

#include <cassert>

namespace GameServices {

// Level generation

static LevelGenOptsFn s_levelGenOpts = nullptr;
static GameRuleDefsFn s_gameRuleDefs = nullptr;

void initLevelGen(LevelGenOptsFn levelGenOpts, GameRuleDefsFn gameRuleDefs) {
    s_levelGenOpts = levelGenOpts;
    s_gameRuleDefs = gameRuleDefs;
}

LevelGenerationOptions* getLevelGenerationOptions() {
    assert(s_levelGenOpts);
    return s_levelGenOpts();
}

LevelRuleset* getGameRuleDefinitions() {
    assert(s_gameRuleDefs);
    return s_gameRuleDefs();
}

// Texture cache

static AddTexFn s_addTex = nullptr;
static RemoveTexFn s_removeTex = nullptr;
static GetTexDetailsFn s_getTexDetails = nullptr;
static HasTexFn s_hasTex = nullptr;

void initTextureCache(AddTexFn add, RemoveTexFn remove,
                      GetTexDetailsFn getDetails, HasTexFn has) {
    s_addTex = add;
    s_removeTex = remove;
    s_getTexDetails = getDetails;
    s_hasTex = has;
}

void addMemoryTextureFile(const std::wstring& name, std::uint8_t* data,
                          unsigned int size) {
    if (s_addTex) s_addTex(name, data, size);
}

void removeMemoryTextureFile(const std::wstring& name) {
    if (s_removeTex) s_removeTex(name);
}

void getMemFileDetails(const std::wstring& name, std::uint8_t** data,
                       unsigned int* size) {
    if (s_getTexDetails) s_getTexDetails(name, data, size);
}

bool isFileInMemoryTextures(const std::wstring& name) {
    return s_hasTex ? s_hasTex(name) : false;
}

// Per-player settings

static GetSettingsFn s_getSettings = nullptr;
static GetSettingsNoArgFn s_getSettingsNoArg = nullptr;

void initPlayerSettings(GetSettingsFn getSettings,
                        GetSettingsNoArgFn getSettingsNoArg) {
    s_getSettings = getSettings;
    s_getSettingsNoArg = getSettingsNoArg;
}

unsigned char getGameSettings(int iPad, int setting) {
    return s_getSettings ? s_getSettings(iPad, setting) : 0;
}

unsigned char getGameSettings(int setting) {
    return s_getSettingsNoArg ? s_getSettingsNoArg(setting) : 0;
}

// App time

static AppTimeFn s_appTime = nullptr;

void initAppTime(AppTimeFn fn) { s_appTime = fn; }

float getAppTime() { return s_appTime ? s_appTime() : 0.0f; }

// Game state

static bool (*s_getGameStarted)() = nullptr;
static void (*s_setGameStarted)(bool) = nullptr;
static bool (*s_getTutorialMode)() = nullptr;
static void (*s_setTutorialMode)(bool) = nullptr;
static bool (*s_isAppPaused)() = nullptr;
static int (*s_getLocalPlayerCount)() = nullptr;
static bool (*s_autosaveDue)() = nullptr;
static void (*s_setAutosaveTimerTime)() = nullptr;
static int64_t (*s_secondsToAutosave)() = nullptr;
static void (*s_setDisconnectReason)(DisconnectPacket::eDisconnectReason) = nullptr;
static void (*s_lockSaveNotify)() = nullptr;
static void (*s_unlockSaveNotify)() = nullptr;
static bool (*s_getResetNether)() = nullptr;
static bool (*s_getUseDPadForDebug)() = nullptr;
static bool (*s_getWriteSavesToFolderEnabled)() = nullptr;
static bool (*s_isLocalMultiplayerAvailable)() = nullptr;
static bool (*s_dlcInstallPending)() = nullptr;
static bool (*s_dlcInstallProcessCompleted)() = nullptr;
static bool (*s_canRecordStatsAndAchievements)() = nullptr;
static bool (*s_getTMSGlobalFileListRead)() = nullptr;
static void (*s_setRequiredTexturePackID)(std::uint32_t) = nullptr;
static void (*s_setSpecialTutorialCompletionFlag)(int, int) = nullptr;
static void (*s_setBanListCheck)(int, bool) = nullptr;
static bool (*s_getBanListCheck)(int) = nullptr;
static unsigned int (*s_getGameNewWorldSize)() = nullptr;
static unsigned int (*s_getGameNewWorldSizeUseMoat)() = nullptr;
static unsigned int (*s_getGameNewHellScale)() = nullptr;

void initGameState(
    bool (*getGameStartedFn)(),
    void (*setGameStartedFn)(bool),
    bool (*getTutorialModeFn)(),
    void (*setTutorialModeFn)(bool),
    bool (*isAppPausedFn)(),
    int (*getLocalPlayerCountFn)(),
    bool (*autosaveDueFn)(),
    void (*setAutosaveTimerTimeFn)(),
    int64_t (*secondsToAutosaveFn)(),
    void (*setDisconnectReasonFn)(DisconnectPacket::eDisconnectReason),
    void (*lockSaveNotifyFn)(),
    void (*unlockSaveNotifyFn)(),
    bool (*getResetNetherFn)(),
    bool (*getUseDPadForDebugFn)(),
    bool (*getWriteSavesToFolderEnabledFn)(),
    bool (*isLocalMultiplayerAvailableFn)(),
    bool (*dlcInstallPendingFn)(),
    bool (*dlcInstallProcessCompletedFn)(),
    bool (*canRecordStatsAndAchievementsFn)(),
    bool (*getTMSGlobalFileListReadFn)(),
    void (*setRequiredTexturePackIDFn)(std::uint32_t),
    void (*setSpecialTutorialCompletionFlagFn)(int, int),
    void (*setBanListCheckFn)(int, bool),
    bool (*getBanListCheckFn)(int),
    unsigned int (*getGameNewWorldSizeFn)(),
    unsigned int (*getGameNewWorldSizeUseMoatFn)(),
    unsigned int (*getGameNewHellScaleFn)()
) {
    s_getGameStarted = getGameStartedFn;
    s_setGameStarted = setGameStartedFn;
    s_getTutorialMode = getTutorialModeFn;
    s_setTutorialMode = setTutorialModeFn;
    s_isAppPaused = isAppPausedFn;
    s_getLocalPlayerCount = getLocalPlayerCountFn;
    s_autosaveDue = autosaveDueFn;
    s_setAutosaveTimerTime = setAutosaveTimerTimeFn;
    s_secondsToAutosave = secondsToAutosaveFn;
    s_setDisconnectReason = setDisconnectReasonFn;
    s_lockSaveNotify = lockSaveNotifyFn;
    s_unlockSaveNotify = unlockSaveNotifyFn;
    s_getResetNether = getResetNetherFn;
    s_getUseDPadForDebug = getUseDPadForDebugFn;
    s_getWriteSavesToFolderEnabled = getWriteSavesToFolderEnabledFn;
    s_isLocalMultiplayerAvailable = isLocalMultiplayerAvailableFn;
    s_dlcInstallPending = dlcInstallPendingFn;
    s_dlcInstallProcessCompleted = dlcInstallProcessCompletedFn;
    s_canRecordStatsAndAchievements = canRecordStatsAndAchievementsFn;
    s_getTMSGlobalFileListRead = getTMSGlobalFileListReadFn;
    s_setRequiredTexturePackID = setRequiredTexturePackIDFn;
    s_setSpecialTutorialCompletionFlag = setSpecialTutorialCompletionFlagFn;
    s_setBanListCheck = setBanListCheckFn;
    s_getBanListCheck = getBanListCheckFn;
    s_getGameNewWorldSize = getGameNewWorldSizeFn;
    s_getGameNewWorldSizeUseMoat = getGameNewWorldSizeUseMoatFn;
    s_getGameNewHellScale = getGameNewHellScaleFn;
}

bool getGameStarted() { return s_getGameStarted ? s_getGameStarted() : false; }
void setGameStarted(bool val) { if (s_setGameStarted) s_setGameStarted(val); }
bool getTutorialMode() { return s_getTutorialMode ? s_getTutorialMode() : false; }
void setTutorialMode(bool val) { if (s_setTutorialMode) s_setTutorialMode(val); }
bool isAppPaused() { return s_isAppPaused ? s_isAppPaused() : false; }
int getLocalPlayerCount() { return s_getLocalPlayerCount ? s_getLocalPlayerCount() : 0; }
bool autosaveDue() { return s_autosaveDue ? s_autosaveDue() : false; }
void setAutosaveTimerTime() { if (s_setAutosaveTimerTime) s_setAutosaveTimerTime(); }
int64_t secondsToAutosave() { return s_secondsToAutosave ? s_secondsToAutosave() : 0; }
void setDisconnectReason(DisconnectPacket::eDisconnectReason reason) {
    if (s_setDisconnectReason) s_setDisconnectReason(reason);
}
void lockSaveNotification() { if (s_lockSaveNotify) s_lockSaveNotify(); }
void unlockSaveNotification() { if (s_unlockSaveNotify) s_unlockSaveNotify(); }
bool getResetNether() { return s_getResetNether ? s_getResetNether() : false; }
bool getUseDPadForDebug() { return s_getUseDPadForDebug ? s_getUseDPadForDebug() : false; }
bool getWriteSavesToFolderEnabled() {
    return s_getWriteSavesToFolderEnabled ? s_getWriteSavesToFolderEnabled() : false;
}
bool isLocalMultiplayerAvailable() {
    return s_isLocalMultiplayerAvailable ? s_isLocalMultiplayerAvailable() : false;
}
bool dlcInstallPending() { return s_dlcInstallPending ? s_dlcInstallPending() : false; }
bool dlcInstallProcessCompleted() {
    return s_dlcInstallProcessCompleted ? s_dlcInstallProcessCompleted() : false;
}
bool canRecordStatsAndAchievements() {
    return s_canRecordStatsAndAchievements ? s_canRecordStatsAndAchievements() : false;
}
bool getTMSGlobalFileListRead() {
    return s_getTMSGlobalFileListRead ? s_getTMSGlobalFileListRead() : true;
}
void setRequiredTexturePackID(std::uint32_t id) {
    if (s_setRequiredTexturePackID) s_setRequiredTexturePackID(id);
}
void setSpecialTutorialCompletionFlag(int iPad, int index) {
    if (s_setSpecialTutorialCompletionFlag) s_setSpecialTutorialCompletionFlag(iPad, index);
}
void setBanListCheck(int iPad, bool val) {
    if (s_setBanListCheck) s_setBanListCheck(iPad, val);
}
bool getBanListCheck(int iPad) {
    return s_getBanListCheck ? s_getBanListCheck(iPad) : false;
}
unsigned int getGameNewWorldSize() {
    return s_getGameNewWorldSize ? s_getGameNewWorldSize() : 0;
}
unsigned int getGameNewWorldSizeUseMoat() {
    return s_getGameNewWorldSizeUseMoat ? s_getGameNewWorldSizeUseMoat() : 0;
}
unsigned int getGameNewHellScale() {
    return s_getGameNewHellScale ? s_getGameNewHellScale() : 0;
}

// UI dispatch

static void (*s_setAction)(int, eXuiAction, void*) = nullptr;
static void (*s_setXuiServerAction)(int, eXuiServerAction, void*) = nullptr;
static eXuiAction (*s_getXuiAction)(int) = nullptr;
static eXuiServerAction (*s_getXuiServerAction)(int) = nullptr;
static void* (*s_getXuiServerActionParam)(int) = nullptr;
static void (*s_setGlobalXuiAction)(eXuiAction) = nullptr;
static void (*s_handleButtonPresses)() = nullptr;
static void (*s_setTMSAction)(int, eTMSAction) = nullptr;

void initUIDispatch(
    void (*setActionFn)(int, eXuiAction, void*),
    void (*setXuiServerActionFn)(int, eXuiServerAction, void*),
    eXuiAction (*getXuiActionFn)(int),
    eXuiServerAction (*getXuiServerActionFn)(int),
    void* (*getXuiServerActionParamFn)(int),
    void (*setGlobalXuiActionFn)(eXuiAction),
    void (*handleButtonPressesFn)(),
    void (*setTMSActionFn)(int, eTMSAction)
) {
    s_setAction = setActionFn;
    s_setXuiServerAction = setXuiServerActionFn;
    s_getXuiAction = getXuiActionFn;
    s_getXuiServerAction = getXuiServerActionFn;
    s_getXuiServerActionParam = getXuiServerActionParamFn;
    s_setGlobalXuiAction = setGlobalXuiActionFn;
    s_handleButtonPresses = handleButtonPressesFn;
    s_setTMSAction = setTMSActionFn;
}

void setAction(int iPad, eXuiAction action, void* param) {
    if (s_setAction) s_setAction(iPad, action, param);
}
void setXuiServerAction(int iPad, eXuiServerAction action, void* param) {
    if (s_setXuiServerAction) s_setXuiServerAction(iPad, action, param);
}
eXuiAction getXuiAction(int iPad) {
    return s_getXuiAction ? s_getXuiAction(iPad) : eAppAction_Idle;
}
eXuiServerAction getXuiServerAction(int iPad) {
    return s_getXuiServerAction ? s_getXuiServerAction(iPad) : eXuiServerAction_Idle;
}
void* getXuiServerActionParam(int iPad) {
    return s_getXuiServerActionParam ? s_getXuiServerActionParam(iPad) : nullptr;
}
void setGlobalXuiAction(eXuiAction action) {
    if (s_setGlobalXuiAction) s_setGlobalXuiAction(action);
}
void handleButtonPresses() {
    if (s_handleButtonPresses) s_handleButtonPresses();
}
void setTMSAction(int iPad, eTMSAction action) {
    if (s_setTMSAction) s_setTMSAction(iPad, action);
}

// Skin / cape / animation

static std::wstring (*s_getPlayerSkinName)(int) = nullptr;
static std::uint32_t (*s_getPlayerSkinId)(int) = nullptr;
static std::wstring (*s_getPlayerCapeName)(int) = nullptr;
static std::uint32_t (*s_getPlayerCapeId)(int) = nullptr;
static std::uint32_t (*s_getAdditionalModelPartsForPad)(int) = nullptr;
static void (*s_setAdditionalSkinBoxes)(std::uint32_t, SKIN_BOX*, unsigned int) = nullptr;
static std::vector<SKIN_BOX*>* (*s_getAdditionalSkinBoxes)(std::uint32_t) = nullptr;
static std::vector<ModelPart*>* (*s_getAdditionalModelParts)(std::uint32_t) = nullptr;
static std::vector<ModelPart*>* (*s_setAdditionalSkinBoxesFromVec)(
    std::uint32_t, std::vector<SKIN_BOX*>*) = nullptr;
static void (*s_setAnimOverrideBitmask)(std::uint32_t, unsigned int) = nullptr;
static unsigned int (*s_getAnimOverrideBitmask)(std::uint32_t) = nullptr;
static std::uint32_t (*s_skinIdFromPath)(const std::wstring&) = nullptr;
static std::wstring (*s_skinPathFromId)(std::uint32_t) = nullptr;
static bool (*s_defaultCapeExists)() = nullptr;
static bool (*s_isXuidNotch)(PlayerUID) = nullptr;
static bool (*s_isXuidDeadmau5)(PlayerUID) = nullptr;

void initSkinCape(
    std::wstring (*getPlayerSkinNameFn)(int),
    std::uint32_t (*getPlayerSkinIdFn)(int),
    std::wstring (*getPlayerCapeNameFn)(int),
    std::uint32_t (*getPlayerCapeIdFn)(int),
    std::uint32_t (*getAdditionalModelPartsForPadFn)(int),
    void (*setAdditionalSkinBoxesFn)(std::uint32_t, SKIN_BOX*, unsigned int),
    std::vector<SKIN_BOX*>* (*getAdditionalSkinBoxesFn)(std::uint32_t),
    std::vector<ModelPart*>* (*getAdditionalModelPartsFn)(std::uint32_t),
    std::vector<ModelPart*>* (*setAdditionalSkinBoxesFromVecFn)(
        std::uint32_t, std::vector<SKIN_BOX*>*),
    void (*setAnimOverrideBitmaskFn)(std::uint32_t, unsigned int),
    unsigned int (*getAnimOverrideBitmaskFn)(std::uint32_t),
    std::uint32_t (*skinIdFromPathFn)(const std::wstring&),
    std::wstring (*skinPathFromIdFn)(std::uint32_t),
    bool (*defaultCapeExistsFn)(),
    bool (*isXuidNotchFn)(PlayerUID),
    bool (*isXuidDeadmau5Fn)(PlayerUID)
) {
    s_getPlayerSkinName = getPlayerSkinNameFn;
    s_getPlayerSkinId = getPlayerSkinIdFn;
    s_getPlayerCapeName = getPlayerCapeNameFn;
    s_getPlayerCapeId = getPlayerCapeIdFn;
    s_getAdditionalModelPartsForPad = getAdditionalModelPartsForPadFn;
    s_setAdditionalSkinBoxes = setAdditionalSkinBoxesFn;
    s_getAdditionalSkinBoxes = getAdditionalSkinBoxesFn;
    s_getAdditionalModelParts = getAdditionalModelPartsFn;
    s_setAdditionalSkinBoxesFromVec = setAdditionalSkinBoxesFromVecFn;
    s_setAnimOverrideBitmask = setAnimOverrideBitmaskFn;
    s_getAnimOverrideBitmask = getAnimOverrideBitmaskFn;
    s_skinIdFromPath = skinIdFromPathFn;
    s_skinPathFromId = skinPathFromIdFn;
    s_defaultCapeExists = defaultCapeExistsFn;
    s_isXuidNotch = isXuidNotchFn;
    s_isXuidDeadmau5 = isXuidDeadmau5Fn;
}

std::wstring getPlayerSkinName(int iPad) {
    return s_getPlayerSkinName ? s_getPlayerSkinName(iPad) : L"";
}
std::uint32_t getPlayerSkinId(int iPad) {
    return s_getPlayerSkinId ? s_getPlayerSkinId(iPad) : 0;
}
std::wstring getPlayerCapeName(int iPad) {
    return s_getPlayerCapeName ? s_getPlayerCapeName(iPad) : L"";
}
std::uint32_t getPlayerCapeId(int iPad) {
    return s_getPlayerCapeId ? s_getPlayerCapeId(iPad) : 0;
}
std::uint32_t getAdditionalModelPartsForPad(int iPad) {
    return s_getAdditionalModelPartsForPad ? s_getAdditionalModelPartsForPad(iPad) : 0;
}
void setAdditionalSkinBoxes(std::uint32_t dwSkinID, SKIN_BOX* boxA,
                            unsigned int boxC) {
    if (s_setAdditionalSkinBoxes) s_setAdditionalSkinBoxes(dwSkinID, boxA, boxC);
}
std::vector<SKIN_BOX*>* getAdditionalSkinBoxes(std::uint32_t dwSkinID) {
    return s_getAdditionalSkinBoxes ? s_getAdditionalSkinBoxes(dwSkinID) : nullptr;
}
std::vector<ModelPart*>* getAdditionalModelParts(std::uint32_t dwSkinID) {
    return s_getAdditionalModelParts ? s_getAdditionalModelParts(dwSkinID) : nullptr;
}
std::vector<ModelPart*>* setAdditionalSkinBoxesFromVec(
    std::uint32_t dwSkinID, std::vector<SKIN_BOX*>* pvSkinBoxA) {
    return s_setAdditionalSkinBoxesFromVec
               ? s_setAdditionalSkinBoxesFromVec(dwSkinID, pvSkinBoxA)
               : nullptr;
}
void setAnimOverrideBitmask(std::uint32_t dwSkinID, unsigned int bitmask) {
    if (s_setAnimOverrideBitmask) s_setAnimOverrideBitmask(dwSkinID, bitmask);
}
unsigned int getAnimOverrideBitmask(std::uint32_t dwSkinID) {
    return s_getAnimOverrideBitmask ? s_getAnimOverrideBitmask(dwSkinID) : 0;
}
std::uint32_t getSkinIdFromPath(const std::wstring& skin) {
    return s_skinIdFromPath ? s_skinIdFromPath(skin) : 0;
}
std::wstring getSkinPathFromId(std::uint32_t skinId) {
    return s_skinPathFromId ? s_skinPathFromId(skinId) : L"";
}
bool defaultCapeExists() {
    return s_defaultCapeExists ? s_defaultCapeExists() : false;
}
bool isXuidNotch(PlayerUID xuid) {
    return s_isXuidNotch ? s_isXuidNotch(xuid) : false;
}
bool isXuidDeadmau5(PlayerUID xuid) {
    return s_isXuidDeadmau5 ? s_isXuidDeadmau5(xuid) : false;
}

// Platform features

static void (*s_fatalLoadError)() = nullptr;
static void (*s_setRichPresenceContext)(int, int) = nullptr;
static void (*s_captureSaveThumbnail)() = nullptr;
static void (*s_getSaveThumbnail)(std::uint8_t**, unsigned int*) = nullptr;
static void (*s_readBannedList)(int, eTMSAction, bool) = nullptr;
static void (*s_updatePlayerInfo)(std::uint8_t, int16_t, unsigned int) = nullptr;
static unsigned int (*s_getPlayerPrivileges)(std::uint8_t) = nullptr;
static void (*s_setGameSettingsDebugMask)(int, unsigned int) = nullptr;

void initPlatformFeatures(
    void (*fatalLoadErrorFn)(),
    void (*setRichPresenceContextFn)(int, int),
    void (*captureSaveThumbnailFn)(),
    void (*getSaveThumbnailFn)(std::uint8_t**, unsigned int*),
    void (*readBannedListFn)(int, eTMSAction, bool),
    void (*updatePlayerInfoFn)(std::uint8_t, int16_t, unsigned int),
    unsigned int (*getPlayerPrivilegesFn)(std::uint8_t),
    void (*setGameSettingsDebugMaskFn)(int, unsigned int)
) {
    s_fatalLoadError = fatalLoadErrorFn;
    s_setRichPresenceContext = setRichPresenceContextFn;
    s_captureSaveThumbnail = captureSaveThumbnailFn;
    s_getSaveThumbnail = getSaveThumbnailFn;
    s_readBannedList = readBannedListFn;
    s_updatePlayerInfo = updatePlayerInfoFn;
    s_getPlayerPrivileges = getPlayerPrivilegesFn;
    s_setGameSettingsDebugMask = setGameSettingsDebugMaskFn;
}

void fatalLoadError() {
    if (s_fatalLoadError) s_fatalLoadError();
}
void setRichPresenceContext(int iPad, int contextId) {
    if (s_setRichPresenceContext) s_setRichPresenceContext(iPad, contextId);
}
void captureSaveThumbnail() {
    if (s_captureSaveThumbnail) s_captureSaveThumbnail();
}
void getSaveThumbnail(std::uint8_t** data, unsigned int* size) {
    if (s_getSaveThumbnail) s_getSaveThumbnail(data, size);
}
void readBannedList(int iPad, eTMSAction action, bool bCallback) {
    if (s_readBannedList) s_readBannedList(iPad, action, bCallback);
}
void updatePlayerInfo(std::uint8_t networkSmallId, int16_t playerColourIndex,
                      unsigned int playerPrivileges) {
    if (s_updatePlayerInfo) s_updatePlayerInfo(networkSmallId, playerColourIndex, playerPrivileges);
}
unsigned int getPlayerPrivileges(std::uint8_t networkSmallId) {
    return s_getPlayerPrivileges ? s_getPlayerPrivileges(networkSmallId) : 0;
}
void setGameSettingsDebugMask(int iPad, unsigned int uiVal) {
    if (s_setGameSettingsDebugMask) s_setGameSettingsDebugMask(iPad, uiVal);
}

// Schematics / terrain

static void (*s_processSchematics)(LevelChunk*) = nullptr;
static void (*s_processSchematicsLighting)(LevelChunk*) = nullptr;
static void (*s_addTerrainFeaturePosition)(_eTerrainFeatureType, int, int) = nullptr;
static bool (*s_getTerrainFeaturePosition)(_eTerrainFeatureType, int*, int*) = nullptr;
static void (*s_loadDefaultGameRules)() = nullptr;

void initSchematics(
    void (*processSchematicsFn)(LevelChunk*),
    void (*processSchematicsLightingFn)(LevelChunk*),
    void (*addTerrainFeaturePositionFn)(_eTerrainFeatureType, int, int),
    bool (*getTerrainFeaturePositionFn)(_eTerrainFeatureType, int*, int*),
    void (*loadDefaultGameRulesFn)()
) {
    s_processSchematics = processSchematicsFn;
    s_processSchematicsLighting = processSchematicsLightingFn;
    s_addTerrainFeaturePosition = addTerrainFeaturePositionFn;
    s_getTerrainFeaturePosition = getTerrainFeaturePositionFn;
    s_loadDefaultGameRules = loadDefaultGameRulesFn;
}

void processSchematics(LevelChunk* chunk) {
    if (s_processSchematics) s_processSchematics(chunk);
}
void processSchematicsLighting(LevelChunk* chunk) {
    if (s_processSchematicsLighting) s_processSchematicsLighting(chunk);
}
void addTerrainFeaturePosition(_eTerrainFeatureType type, int x, int z) {
    if (s_addTerrainFeaturePosition) s_addTerrainFeaturePosition(type, x, z);
}
bool getTerrainFeaturePosition(_eTerrainFeatureType type, int* pX, int* pZ) {
    return s_getTerrainFeaturePosition ? s_getTerrainFeaturePosition(type, pX, pZ) : false;
}
void loadDefaultGameRules() {
    if (s_loadDefaultGameRules) s_loadDefaultGameRules();
}

// Archive / resources

static bool (*s_hasArchiveFile)(const std::wstring&) = nullptr;
static std::vector<std::uint8_t> (*s_getArchiveFile)(const std::wstring&) = nullptr;

void initArchive(
    bool (*hasArchiveFileFn)(const std::wstring&),
    std::vector<std::uint8_t> (*getArchiveFileFn)(const std::wstring&)
) {
    s_hasArchiveFile = hasArchiveFileFn;
    s_getArchiveFile = getArchiveFileFn;
}

bool hasArchiveFile(const std::wstring& filename) {
    return s_hasArchiveFile ? s_hasArchiveFile(filename) : false;
}
std::vector<std::uint8_t> getArchiveFile(const std::wstring& filename) {
    return s_getArchiveFile ? s_getArchiveFile(filename) : std::vector<std::uint8_t>{};
}

// Strings / formatting / misc queries

static int (*s_getHTMLColour)(eMinecraftColour) = nullptr;
static std::wstring (*s_getEntityName)(EntityTypeId) = nullptr;
static const wchar_t* (*s_getGameRulesString)(const std::wstring&) = nullptr;
static unsigned int (*s_createImageTextData)(std::uint8_t*, int64_t, bool,
                                             unsigned int, unsigned int) = nullptr;
static std::wstring (*s_getFilePath)(std::uint32_t, std::wstring, bool,
                                     std::wstring) = nullptr;
static char* (*s_getUniqueMapName)() = nullptr;
static void (*s_setUniqueMapName)(char*) = nullptr;
static unsigned int (*s_getOpacityTimer)(int) = nullptr;
static void (*s_setOpacityTimer)(int) = nullptr;
static void (*s_tickOpacityTimer)(int) = nullptr;
static bool (*s_isInBannedLevelList)(int, PlayerUID, char*) = nullptr;
static MOJANG_DATA* (*s_getMojangDataForXuid)(PlayerUID) = nullptr;
static void (*s_debugPrintf)(const char*) = nullptr;

void initStringsAndMisc(
    int (*getHTMLColourFn)(eMinecraftColour),
    std::wstring (*getEntityNameFn)(EntityTypeId),
    const wchar_t* (*getGameRulesStringFn)(const std::wstring&),
    unsigned int (*createImageTextDataFn)(std::uint8_t*, int64_t, bool,
                                          unsigned int, unsigned int),
    std::wstring (*getFilePathFn)(std::uint32_t, std::wstring, bool,
                                  std::wstring),
    char* (*getUniqueMapNameFn)(),
    void (*setUniqueMapNameFn)(char*),
    unsigned int (*getOpacityTimerFn)(int),
    void (*setOpacityTimerFn)(int),
    void (*tickOpacityTimerFn)(int),
    bool (*isInBannedLevelListFn)(int, PlayerUID, char*),
    MOJANG_DATA* (*getMojangDataForXuidFn)(PlayerUID),
    void (*debugPrintfFn)(const char*)
) {
    s_getHTMLColour = getHTMLColourFn;
    s_getEntityName = getEntityNameFn;
    s_getGameRulesString = getGameRulesStringFn;
    s_createImageTextData = createImageTextDataFn;
    s_getFilePath = getFilePathFn;
    s_getUniqueMapName = getUniqueMapNameFn;
    s_setUniqueMapName = setUniqueMapNameFn;
    s_getOpacityTimer = getOpacityTimerFn;
    s_setOpacityTimer = setOpacityTimerFn;
    s_tickOpacityTimer = tickOpacityTimerFn;
    s_isInBannedLevelList = isInBannedLevelListFn;
    s_getMojangDataForXuid = getMojangDataForXuidFn;
    s_debugPrintf = debugPrintfFn;
}

int getHTMLColour(eMinecraftColour colour) {
    return s_getHTMLColour ? s_getHTMLColour(colour) : 0;
}
int getHTMLColor(eMinecraftColour colour) { return getHTMLColour(colour); }
std::wstring getEntityName(EntityTypeId type) {
    return s_getEntityName ? s_getEntityName(type) : L"";
}
const wchar_t* getGameRulesString(const std::wstring& key) {
    return s_getGameRulesString ? s_getGameRulesString(key) : L"";
}
unsigned int createImageTextData(std::uint8_t* textMetadata, int64_t seed,
                                 bool hasSeed, unsigned int uiHostOptions,
                                 unsigned int uiTexturePackId) {
    return s_createImageTextData
               ? s_createImageTextData(textMetadata, seed, hasSeed,
                                       uiHostOptions, uiTexturePackId)
               : 0;
}
std::wstring getFilePath(std::uint32_t packId, std::wstring filename,
                         bool bAddDataFolder, std::wstring mountPoint) {
    return s_getFilePath ? s_getFilePath(packId, filename, bAddDataFolder, mountPoint) : L"";
}
char* getUniqueMapName() {
    return s_getUniqueMapName ? s_getUniqueMapName() : nullptr;
}
void setUniqueMapName(char* name) {
    if (s_setUniqueMapName) s_setUniqueMapName(name);
}
unsigned int getOpacityTimer(int iPad) {
    return s_getOpacityTimer ? s_getOpacityTimer(iPad) : 0;
}
void setOpacityTimer(int iPad) {
    if (s_setOpacityTimer) s_setOpacityTimer(iPad);
}
void tickOpacityTimer(int iPad) {
    if (s_tickOpacityTimer) s_tickOpacityTimer(iPad);
}
bool isInBannedLevelList(int iPad, PlayerUID xuid, char* levelName) {
    return s_isInBannedLevelList ? s_isInBannedLevelList(iPad, xuid, levelName) : false;
}
MOJANG_DATA* getMojangDataForXuid(PlayerUID xuid) {
    return s_getMojangDataForXuid ? s_getMojangDataForXuid(xuid) : nullptr;
}
void debugPrintf(const char* msg) {
    if (s_debugPrintf) s_debugPrintf(msg);
}

// Member variable access

static DLCManager* s_dlcManager = nullptr;
static GameRuleManager* s_gameRules = nullptr;
static std::vector<std::wstring>* s_skinNames = nullptr;
static std::vector<FEATURE_DATA*>* s_terrainFeatures = nullptr;

void initMemberAccess(
    DLCManager* dlcManager,
    GameRuleManager* gameRules,
    std::vector<std::wstring>* skinNames,
    std::vector<FEATURE_DATA*>* terrainFeatures
) {
    s_dlcManager = dlcManager;
    s_gameRules = gameRules;
    s_skinNames = skinNames;
    s_terrainFeatures = terrainFeatures;
}

DLCManager& getDLCManager() {
    assert(s_dlcManager);
    return *s_dlcManager;
}
GameRuleManager& getGameRules() {
    assert(s_gameRules);
    return *s_gameRules;
}
std::vector<std::wstring>& getSkinNames() {
    assert(s_skinNames);
    return *s_skinNames;
}
std::vector<FEATURE_DATA*>& getTerrainFeatures() {
    assert(s_terrainFeatures);
    return *s_terrainFeatures;
}

// Menu service

static IMenuService* s_menuService = nullptr;

void initMenuService(IMenuService* service) { s_menuService = service; }

IMenuService& menus() {
    assert(s_menuService);
    return *s_menuService;
}

}  // namespace GameServices
