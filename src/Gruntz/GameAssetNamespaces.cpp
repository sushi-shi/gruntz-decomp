#include <rva.h>

#include <Gruntz/GameAssetNamespaces.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/FaderMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/State.h>
#include <Image/CImage.h>
#include <Rez/RezArchive.h>

#include <stdio.h>

DATA(0x0025256c)
i32 g_buildNumber;

// @early-stop
RVA(0x000f9fd0, 0x21d)
i32 CState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    m_mgr = mgr;
    m_resourceArchive = mgr->m_resourceArchive;
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
    CRezArchiveDir* node = m_resourceArchive->FindDirectoryByPath(area);
    m_levelResources = node;
    if (node == NULL) {
        return 0;
    }
    if (m_world->m_imageRegistry->HasWithPrefix("GAME") == 0) {
        CRezArchiveDir* img = m_resourceArchive->FindDirectoryByPath("GAME_IMAGEZ");
        if (img == NULL) {
            return 0;
        }
        g_resourceInstallActive = true;
        m_world->m_imageRegistry->InstallTree(img, "GAME", "_");
        g_resourceInstallActive = false;
    }
    if (m_world->m_soundRegistry->HasWithPrefix("GAME") == 0) {
        CRezArchiveDir* snd = m_resourceArchive->FindDirectoryByPath("GAME_SOUNDZ");
        if (snd == NULL) {
            return 0;
        }
        m_world->m_soundRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(snd), "GAME", "_");
    }
    if (m_world->m_animRegistry->HasWithPrefix("GAME") == 0) {
        CRezArchiveDir* aniz = m_resourceArchive->FindDirectoryByPath("GAME_ANIZ");
        if (aniz == NULL) {
            return 0;
        }
        m_world->m_animRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(aniz), "GAME", "_");
    }

    if (m_mgr->m_spriteFactory->BuildToolToyColorTable(m_mgr->m_resourceArchive) == 0) {
        return 0;
    }
    if (m_cursorSavedSurfaces[0] == NULL && m_cursorSavedSurfaces[1] == NULL) {
        CDDrawDeviceManager* manager = m_world->m_deviceManager;
        if (manager == NULL) {
            return 0;
        }
        m_cursorSavedSurfaces[0] = manager->CreateOffscreenSurface(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_cursorSavedSurfaces[0] == NULL) {
            return 0;
        }
        m_cursorSavedSurfaces[1] = manager->CreateOffscreenSurface(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_cursorSavedSurfaces[1] == NULL) {
            return 0;
        }
    }
    m_ready = true;
    return 1;
}
