#pragma once
#include <string>

#include "AbstractTexturePack.h"
#include "java/InputOutputStream/InputStream.h"
#include "minecraft/locale/Strings.h"
#include "strings.h"

class DefaultTexturePack : public AbstractTexturePack {
public:
    DefaultTexturePack();
    DLCPack* getDLCPack() { return nullptr; }

protected:
    //@Override
    void loadIcon();
    void loadName();
    void loadDescription();

public:
    //@Override
    bool hasFile(const std::wstring& name);
    bool isTerrainUpdateCompatible();

    std::wstring getDesc1() { return Strings::get(IDS_DEFAULT_TEXTUREPACK); }

protected:
    //@Override
    InputStream* getResourceImplementation(
        const std::wstring& name);  // throws FileNotFoundException

public:
    virtual bool hasData() { return true; }
    virtual bool hasAudio() { return false; }
    virtual bool isLoadingData() { return false; }
    virtual void loadUI();
    virtual void unloadUI();
};