// GameAssetNamespaces.cpp - load the per-area GAME asset namespaces (RVA
// 0xf9ea0). Resolves AREA%i, then lazily scans the GAME image/sound/aniz trees
// into the worker registries, builds the tool/toy colour table and the two 64x64
// scratch pools. Field names are placeholders; only offsets + code bytes are
// load-bearing.
//
// FOLDED (2026-07-15): the three .cpp-local views are GONE, dissolved onto the
// canonicals they always were:
//   CAssetLoader  == the CState BASE the method runs on (it IS a CState method;
//       m_mgr@+0x04 == m_4, m_symParser@+0x08 == m_8, m_workerHolder@+0x0c == m_c,
//       m_10 == m_faderMgr, m_areaArg@+0x1c == m_levelIndex, m_areaIndex@+0x20 ==
//       m_levelType (the AREA terrain class), m_areaNode@+0x28 == m_levelBank,
//       m_loaded@+0x3c == m_ready, m_versionString@+0x4c == m_versionString,
//       m_scratch0/1@+0x160/+0x164 == m_160/m_164 - the same two surfaces
//       CState::ReleaseResources releases back to the pool. The +0x10..+0x164 scratch fields
//       are CState members now (State.h).
//   AssetMgr      == the CGruntzMgr singleton (m_workerHolder@+0x30 == m_world,
//       m_symParser@+0x34 == m_symParser (ex m_recolorSurface), m_40 == m_40,
//       m_spriteRefTable@+0x74 == m_spriteFactory).
//   WorkerHolder  == CDDrawSurfaceMgr (m_imageReg@+0x10 == m_10 CImageRegistry,
//       m_ptrCollections@+0x1c == m_ptrColl, m_soundScan@+0x28 == m_28 (CSndHost ==
//       CDDrawSubMgrLeafScan, one typedef), m_aniScan@+0x2c == m_animRegistry -
//       the canonical CDDrawSubMgrLeaf ANI set (HasKeyPrefix_152c50/ScanTree_152ad0;
//       the ex CAnimRegistry/CDDrawSubMgrAni views are dissolved onto it).
#include <Mfc.h> // afx-first umbrella (GruntzMgr.h/ResMgr.h need the MFC classes)
#include <rva.h>
#include <Image/CImage.h> // g_resourceInstallActive

#include <stdio.h>
#include <Bute/SymParser.h>   // the shared CSymParser (ResolvePath 0x13c030)
#include <Gruntz/State.h>     // CState: the real owner of the loader (all leaf states inherit it)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr - the manager arg (m_world/m_symParser/m_40/...)
#include <Gruntz/GameRegistry.h>      // CDDrawSurfaceMgr (m_10/m_ptrColl/m_28/m_animRegistry)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // CDDrawSubMgrLeaf (HasKeyPrefix_152c50 / ScanTree_152ad0)
#include <Gruntz/SpriteRefTable.h>    // the shared CSpriteRefTable (g_gameReg->m_spriteFactory)
#include <DDrawMgr/DDrawWorkerRegistry.h> // CImageRegistry == CDDrawWorkerRegistry (InstallTree)
#include <DDrawMgr/DDrawPtrCollections.h> // the ONE CDDrawPtrCollections shape (MakeAndAddB)
#include <Gruntz/FaderMgr.h>              // CFaderMgr - CState::m_faderMgr's real class
#include <Globals.h>

// The build number the version string embeds (owner-TU def; .bss, VA 0x651614).
DATA(0x00251614)
i32 g_buildNumber; // 0x651614  sprintf("... Build %i ...", g_buildNumber)

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
    // the manager arrives as the slot-1 virtual's i32 arg; one cast at the seam.
    CGruntzMgr* mgr = reinterpret_cast<CGruntzMgr*>(mgrArg);
    m_4 = mgr;
    m_8 = mgr->m_symParser;
    m_c = mgr->m_world;
    // +0x40 SETTLED (2026-07-16): the mgr's m_faderMgr is the real CFaderMgr
    // (Run news it + SetConfig @0x17d980, the src/DDrawMgr/FaderMgr.cpp method).
    m_faderMgr = mgr->m_faderMgr;
    m_levelIndex = areaArg;
    i32 t = (areaArg - 1) % 0x24;
    m_44 = -1;
    m_48 = -1;
    m_14c = 0;
    m_24 = a3;
    m_levelType = t / 4 + 1;
    sprintf(m_versionString, "Alpha Version, Build %i, Monolith Productions Inc.", g_buildNumber);
    char area[32];
    sprintf(area, "AREA%i", m_levelType);
    CSymTab* node = static_cast<CSymTab*>(m_8->ResolvePath(area));
    m_levelBank = node;
    if (node == 0) {
        return 0;
    }
    if (m_c->m_imageRegistry->HasKeyEqual_155550("GAME") == 0) {
        void* img = m_8->ResolvePath("GAME_IMAGEZ");
        if (img == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_c->m_imageRegistry->InstallTree(img, "GAME", "_");
        g_resourceInstallActive = 0;
    }
    if (m_c->m_soundRegistry->HasKeyEqual_1583c0("GAME") == 0) {
        void* snd = m_8->ResolvePath("GAME_SOUNDZ");
        if (snd == 0) {
            return 0;
        }
        m_c->m_soundRegistry->ScanTree_157ee0(static_cast<CSymTab*>(snd), "GAME", "_");
    }
    if (m_c->m_animRegistry->HasKeyPrefix_152c50("GAME") == 0) {
        void* aniz = m_8->ResolvePath("GAME_ANIZ");
        if (aniz == 0) {
            return 0;
        }
        m_c->m_animRegistry->ScanTree_152ad0(static_cast<CSymTab*>(aniz), "GAME", "_");
    }
    // the shared CSpriteRefTable types the source resolver as i32 (a raw 4-byte
    // handle); the parser pointer is passed through unchanged (reloc-masked).
    // Retail re-reads both through this->m_4 (spilled `this`, not the cached arg).
    if (m_4->m_spriteFactory->BuildToolToyColorTable(reinterpret_cast<i32>(m_4->m_symParser)) == 0) {
        return 0;
    }
    if (m_160 == 0 && m_164 == 0) {
        CDDrawPtrCollections* coll = m_c->m_ptrColl;
        if (coll == 0) {
            return 0;
        }
        m_160 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_160 == 0) {
            return 0;
        }
        m_164 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_164 == 0) {
            return 0;
        }
    }
    m_ready = 1;
    return 1;
}
