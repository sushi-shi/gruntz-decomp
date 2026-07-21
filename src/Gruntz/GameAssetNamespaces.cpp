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
    m_mgr = mgr;
    m_symParser = mgr->m_symParser;
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
    CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath(area));
    m_levelBank = node;
    if (node == 0) {
        return 0;
    }
    if (m_c->m_imageRegistry->HasKeyEqual_155550("GAME") == 0) {
        void* img = m_symParser->ResolvePath("GAME_IMAGEZ");
        if (img == 0) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_c->m_imageRegistry->InstallTree(img, "GAME", "_");
        g_resourceInstallActive = 0;
    }
    if (m_c->m_soundRegistry->HasKeyEqual_1583c0("GAME") == 0) {
        void* snd = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (snd == 0) {
            return 0;
        }
        m_c->m_soundRegistry->ScanTree_157ee0(static_cast<CSymTab*>(snd), "GAME", "_");
    }
    if (m_c->m_animRegistry->HasKeyPrefix_152c50("GAME") == 0) {
        void* aniz = m_symParser->ResolvePath("GAME_ANIZ");
        if (aniz == 0) {
            return 0;
        }
        m_c->m_animRegistry->ScanTree_152ad0(static_cast<CSymTab*>(aniz), "GAME", "_");
    }
    // the shared CSpriteRefTable types the source resolver as i32 (a raw 4-byte
    // handle); the parser pointer is passed through unchanged (reloc-masked).
    // Retail re-reads both through this->m_4 (spilled `this`, not the cached arg).
    if (m_mgr->m_spriteFactory->BuildToolToyColorTable(reinterpret_cast<i32>(m_mgr->m_symParser)) == 0) {
        return 0;
    }
    if (m_scratchSurface0 == 0 && m_scratchSurface1 == 0) {
        CDDrawPtrCollections* coll = m_c->m_ptrColl;
        if (coll == 0) {
            return 0;
        }
        m_scratchSurface0 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_scratchSurface0 == 0) {
            return 0;
        }
        m_scratchSurface1 = coll->MakeAndAddB(0x40, 0x40, 0x10, 0, -1);
        if (m_scratchSurface1 == 0) {
            return 0;
        }
    }
    m_ready = 1;
    return 1;
}
