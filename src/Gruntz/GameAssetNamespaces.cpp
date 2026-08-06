#include <rva.h>

#include <Gruntz/GameAssetNamespaces.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Image/CImage.h>

#include <stdio.h>

DATA(0x00251614)
i32 g_buildNumber;

// @early-stop
RVA(0x000f9ea0, 0x21d)
i32 CState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    m_mgr = mgr;
    m_symParser = mgr->m_symParser;
    m_world = mgr->m_world;

    m_faderMgr = mgr->m_faderMgr;
    m_levelIndex = areaArg;
    i32 t = (areaArg - 1) % 0x24;
    m_reserved44 = -1;
    m_reserved48 = -1;
    m_reserved14c = 0;
    m_previousStateId = static_cast<GameStateId>(prevStateId);
    m_levelType = static_cast<LevelArea>(t / 4 + 1);
    sprintf(m_versionString, "Alpha Version, Build %i, Monolith Productions Inc.", g_buildNumber);
    char area[32];
    sprintf(area, "AREA%i", IDX(m_levelType));
    CSymTab* node = static_cast<CSymTab*>(m_symParser->ResolvePath(area));
    m_levelBank = node;
    if (node == NULL) {
        return 0;
    }
    if (m_world->m_imageRegistry->HasKeyEqual("GAME") == 0) {
        void* img = m_symParser->ResolvePath("GAME_IMAGEZ");
        if (img == NULL) {
            return 0;
        }
        g_resourceInstallActive = 1;
        m_world->m_imageRegistry->InstallTree(img, "GAME", "_");
        g_resourceInstallActive = 0;
    }
    if (m_world->m_soundRegistry->HasKeyEqual("GAME") == 0) {
        void* snd = m_symParser->ResolvePath("GAME_SOUNDZ");
        if (snd == NULL) {
            return 0;
        }
        m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(snd), "GAME", "_");
    }
    if (m_world->m_animRegistry->HasKeyPrefix("GAME") == 0) {
        void* aniz = m_symParser->ResolvePath("GAME_ANIZ");
        if (aniz == NULL) {
            return 0;
        }
        m_world->m_animRegistry->ScanTree(static_cast<CSymTab*>(aniz), "GAME", "_");
    }

    if (m_mgr->m_spriteFactory->BuildToolToyColorTable(m_mgr->m_symParser) == 0) {
        return 0;
    }
    if (m_scratchSurface0 == NULL && m_scratchSurface1 == NULL) {
        CDDrawPtrCollections* coll = m_world->m_ptrColl;
        if (coll == NULL) {
            return 0;
        }
        m_scratchSurface0 = coll->MakeAndAddB(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_scratchSurface0 == NULL) {
            return 0;
        }
        m_scratchSurface1 = coll->MakeAndAddB(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_scratchSurface1 == NULL) {
            return 0;
        }
    }
    m_ready = 1;
    return 1;
}
