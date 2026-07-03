// GameAssetNamespaces.cpp - load the per-area GAME asset namespaces (RVA
// 0xf9ea0). Resolves AREA%i, then lazily scans the GAME image/sound/aniz trees
// into the worker registries, builds the tool/toy colour table and the two 64x64
// scratch pools. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <stdio.h>
#include <Globals.h>

DATA(0x006bf37c)
extern i32 g_resourceInstallActive;

class CSymParser {
public:
    void* ResolvePath(char* path); // 0x13c030
};

// The image-namespace worker registry: non-virtual key probe + a load slot at
// vtable +0x48.
class CDDrawWorkerRegistry {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual void s24();
    virtual void s28();
    virtual void s2c();
    virtual void s30();
    virtual void s34();
    virtual void s38();
    virtual void s3c();
    virtual void s40();
    virtual void s44();
    virtual void LoadTree(void* tree, char* prefix, char* sep); // +0x48
    i32 HasKeyEqual(char* key);                                 // 0x155550
};

class CDDrawSubMgrLeafScan {
public:
    i32 HasKeyEqual(char* key);                         // 0x1583c0
    void ScanTree(void* tree, char* prefix, char* sep); // 0x157ee0
};

class CDDrawSubMgrAni {
public:
    i32 HasKeyPrefix(char* key);                        // 0x152c50
    void ScanTree(void* tree, char* prefix, char* sep); // 0x152ad0
};

class CSpriteRefTable {
public:
    i32 BuildToolToyColorTable(void* arg); // 0x0e2400 (thunk 0x32d8)
};

class CDDrawPtrCollections {
public:
    void* MakeAndAddB(i32 w, i32 h, i32 a, i32 b, i32 c); // 0x142e60
};

struct WorkerHolder {
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_imageReg; // +0x10
    char m_pad14[0x1c - 0x14];
    CDDrawPtrCollections* m_ptrCollections; // +0x1c
    char m_pad20[0x28 - 0x20];
    CDDrawSubMgrLeafScan* m_soundScan; // +0x28
    CDDrawSubMgrAni* m_aniScan;        // +0x2c
};

struct AssetMgr {
    char m_pad00[0x30];
    WorkerHolder* m_workerHolder; // +0x30
    CSymParser* m_symParser;      // +0x34
    char m_pad38[0x40 - 0x38];
    i32 m_40; // +0x40
    char m_pad44[0x74 - 0x44];
    CSpriteRefTable* m_spriteRefTable; // +0x74
};

class CAssetLoader {
public:
    void LoadGameAssetNamespaces(AssetMgr* mgr, i32 areaArg, i32 a3);

    char m_pad00[0x4];
    AssetMgr* m_mgr;              // +0x04
    CSymParser* m_symParser;      // +0x08
    WorkerHolder* m_workerHolder; // +0x0c
    i32 m_10;                     // +0x10
    char m_pad14[0x1c - 0x14];
    i32 m_areaArg;    // +0x1c
    i32 m_areaIndex;  // +0x20 area index
    i32 m_24;         // +0x24
    void* m_areaNode; // +0x28 resolved area
    char m_pad2c[0x3c - 0x2c];
    i32 m_loaded; // +0x3c loaded flag
    char m_pad40[0x44 - 0x40];
    i32 m_44;                           // +0x44
    i32 m_48;                           // +0x48
    char m_versionString[0x14c - 0x4c]; // +0x4c version-string buffer
    i32 m_14c;                          // +0x14c
    char m_pad150[0x160 - 0x150];
    void* m_scratch0; // +0x160
    void* m_scratch1; // +0x164
};

// @early-stop
// 73% - /O2 register-allocation/scheduling entropy on a large loader: the branch
// structure, every extern call and the namespace strings match retail, but the
// optimizer distributes the descriptor/holder loads across eax/ecx/edx differently
// and schedules the a3 store later (with the version-string lea). Logic + externs
// match retail. Final sweep.
RVA(0x000f9ea0, 0x21d)
void CAssetLoader::LoadGameAssetNamespaces(AssetMgr* mgr, i32 areaArg, i32 a3) {
    m_mgr = mgr;
    m_symParser = mgr->m_symParser;
    m_workerHolder = mgr->m_workerHolder;
    m_10 = mgr->m_40;
    m_areaArg = areaArg;
    i32 t = (areaArg - 1) % 0x24;
    m_44 = -1;
    m_48 = -1;
    m_14c = 0;
    m_24 = a3;
    m_areaIndex = t / 4 + 1;
    sprintf(m_versionString, "Alpha Version, Build %i, Monolith Productions Inc.", g_buildNumber);
    char area[32];
    sprintf(area, "AREA%i", m_areaIndex);
    void* node = m_symParser->ResolvePath(area);
    m_areaNode = node;
    if (node == 0) {
        return;
    }
    if (m_workerHolder->m_imageReg->HasKeyEqual("GAME") == 0) {
        void* img = m_symParser->ResolvePath("GAME_IMAGEZ");
        if (img == 0) {
            return;
        }
        g_resourceInstallActive = 1;
        m_workerHolder->m_imageReg->LoadTree(img, "GAME", "_");
        g_resourceInstallActive = 0;
    }
    if (m_workerHolder->m_soundScan->HasKeyEqual("GAME") == 0) {
        void* snd = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (snd == 0) {
            return;
        }
        m_workerHolder->m_soundScan->ScanTree(snd, "GAME", "_");
    }
    if (m_workerHolder->m_aniScan->HasKeyPrefix("GAME") == 0) {
        void* aniz = m_symParser->ResolvePath("GAME_ANIZ");
        if (aniz == 0) {
            return;
        }
        m_workerHolder->m_aniScan->ScanTree(aniz, "GAME", "_");
    }
    if (m_mgr->m_spriteRefTable->BuildToolToyColorTable(m_mgr->m_symParser) == 0) {
        return;
    }
    if (m_scratch0 == 0 && m_scratch1 == 0) {
        CDDrawPtrCollections* coll = m_workerHolder->m_ptrCollections;
        if (coll == 0) {
            return;
        }
        m_scratch0 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_scratch0 == 0) {
            return;
        }
        m_scratch1 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_scratch1 == 0) {
            return;
        }
    }
    m_loaded = 1;
}
