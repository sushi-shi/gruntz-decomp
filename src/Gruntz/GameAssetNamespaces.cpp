// GameAssetNamespaces.cpp - load the per-area GAME asset namespaces (RVA
// 0xf9ea0). Resolves AREA%i, then lazily scans the GAME image/sound/aniz trees
// into the worker registries, builds the tool/toy colour table and the two 64x64
// scratch pools. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <rva.h>

#include <stdio.h>
#include <Bute/SymParser.h>               // the shared CSymParser (ResolvePath 0x13c030)
#include <Gruntz/State.h>                 // CState: the real owner of the loader (all leaf states inherit it)
#include <Gruntz/SpriteRefTable.h>        // the shared CSpriteRefTable (g_gameReg->m_74)
#include <DDrawMgr/DDrawPtrCollections.h> // the ONE CDDrawPtrCollections shape (MakeAndAddB)
#include <DDrawMgr/DDrawAssetRegistryViews.h> // shared CDDrawWorkerRegistry/LeafScan/Ani namespace views
#include <Globals.h>

DATA(0x002bf37c) // VA-typo fix: 0x6bf37c -> 0x2bf37c (canonical, Play.cpp); 6->2 typo class
extern i32 g_resourceInstallActive;

// The build number the version string embeds (owner-TU def; .bss, VA 0x651614).
DATA(0x00251614)
i32 g_buildNumber; // 0x651614  sprintf("... Build %i ...", g_buildNumber)

// CSymParser (ResolvePath 0x13c030) is the shared <Bute/SymParser.h> shape.

// CDDrawWorkerRegistry / CDDrawSubMgrLeafScan / CDDrawSubMgrAni: shared views from
// <DDrawMgr/DDrawAssetRegistryViews.h> (mirror the per-object loader AssetNamespacePrefixes.cpp).

// CSpriteRefTable (BuildToolToyColorTable == 0xe2400, thunk 0x32d8) is the shared
// <Gruntz/SpriteRefTable.h> shape.

SIZE_UNKNOWN(WorkerHolder);
struct WorkerHolder {
    char m_pad00[0x10];
    CDDrawWorkerRegistry* m_imageReg; // +0x10
    char m_pad14[0x1c - 0x14];
    CDDrawPtrCollections* m_ptrCollections; // +0x1c
    char m_pad20[0x28 - 0x20];
    CDDrawSubMgrLeafScan* m_soundScan; // +0x28
    CDDrawSubMgrAni* m_aniScan;        // +0x2c
};

SIZE_UNKNOWN(AssetMgr);
struct AssetMgr {
    char m_pad00[0x30];
    WorkerHolder* m_workerHolder; // +0x30
    CSymParser* m_symParser;      // +0x34
    char m_pad38[0x40 - 0x38];
    i32 m_40; // +0x40
    char m_pad44[0x74 - 0x44];
    CSpriteRefTable* m_spriteRefTable; // +0x74
};

// CAssetLoader is the loader's field-view of the CState-derived state object the
// method runs on (the +0x04..+0x164 asset-loading scratch slots CState reuses; a
// full field reconciliation onto CState/CGruntzMgr is out of scope - those are
// frozen mega-folds). The method itself is CState::LoadGameAssetNamespaces so every
// leaf state binds cast-free; the body casts `this` to this view once for the names.
SIZE_UNKNOWN(CAssetLoader);
struct CAssetLoader {
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
// ~94.5% - /O2 register-allocation/scheduling entropy on a large loader: the branch
// structure, every extern call and the namespace strings match retail, but the
// optimizer distributes the descriptor/holder loads across eax/ecx/edx differently
// and schedules the a3 store later (with the version-string lea). Logic + externs
// match retail. Return type is int (retail sets eax=1 on success; every leaf state
// TESTs it - the int fix lifted 73% -> ~94.5%) - re-homed onto CState so the ~7 leaf
// callers bind cast-free. Final sweep.
RVA(0x000f9ea0, 0x21d)
i32 CState::LoadGameAssetNamespaces(i32 mgrArg, i32 areaArg, i32 a3) {
    CAssetLoader* self = (CAssetLoader*)this;
    AssetMgr* mgr = (AssetMgr*)mgrArg;
    self->m_mgr = mgr;
    self->m_symParser = mgr->m_symParser;
    self->m_workerHolder = mgr->m_workerHolder;
    self->m_10 = mgr->m_40;
    self->m_areaArg = areaArg;
    i32 t = (areaArg - 1) % 0x24;
    self->m_44 = -1;
    self->m_48 = -1;
    self->m_14c = 0;
    self->m_24 = a3;
    self->m_areaIndex = t / 4 + 1;
    sprintf(self->m_versionString, "Alpha Version, Build %i, Monolith Productions Inc.", g_buildNumber);
    char area[32];
    sprintf(area, "AREA%i", self->m_areaIndex);
    void* node = self->m_symParser->ResolvePath(area);
    self->m_areaNode = node;
    if (node == 0) {
        return 0;
    }
    if (self->m_workerHolder->m_imageReg->HasKeyEqual("GAME") == 0) {
        void* img = self->m_symParser->ResolvePath("GAME_IMAGEZ");
        if (img == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        self->m_workerHolder->m_imageReg->LoadTree(img, "GAME", "_");
        g_resourceInstallActive = 0;
    }
    if (self->m_workerHolder->m_soundScan->HasKeyEqual_1583c0("GAME") == 0) {
        void* snd = self->m_symParser->ResolvePath("GAME_SOUNDZ");
        if (snd == 0) {
            return 0;
        }
        self->m_workerHolder->m_soundScan->ScanTree_157ee0((DirNode*)snd, "GAME", "_");
    }
    if (self->m_workerHolder->m_aniScan->HasKeyPrefix("GAME") == 0) {
        void* aniz = self->m_symParser->ResolvePath("GAME_ANIZ");
        if (aniz == 0) {
            return 0;
        }
        self->m_workerHolder->m_aniScan->ScanTree(aniz, "GAME", "_");
    }
    // the shared CSpriteRefTable types the source resolver as i32 (a raw 4-byte
    // handle); the parser pointer is passed through unchanged (reloc-masked).
    if (self->m_mgr->m_spriteRefTable->BuildToolToyColorTable((i32)self->m_mgr->m_symParser) == 0) {
        return 0;
    }
    if (self->m_scratch0 == 0 && self->m_scratch1 == 0) {
        CDDrawPtrCollections* coll = self->m_workerHolder->m_ptrCollections;
        if (coll == 0) {
            return 0;
        }
        self->m_scratch0 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (self->m_scratch0 == 0) {
            return 0;
        }
        self->m_scratch1 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (self->m_scratch1 == 0) {
            return 0;
        }
    }
    self->m_loaded = 1;
    return 1;
}
