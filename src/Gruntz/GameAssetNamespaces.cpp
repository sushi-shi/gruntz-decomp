#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/GameAssetNamespaces.h>

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

DATA(0x00251614)
i32 g_buildNumber;

RVA(0x000f9ea0, 0x21d)
i32 CState::LoadGameAssetNamespaces(CGruntzMgr* gameManager, i32 levelIndex, i32 previousStateId) {
    m_mgr = gameManager;
    m_resourceArchive = gameManager->m_resourceArchive;
    m_world = gameManager->World();

    m_faderMgr = gameManager->m_faderMgr;
    m_levelIndex = levelIndex;
    m_reserved44 = -1;
    m_reserved48 = -1;
    m_reserved14c = 0;
    m_levelArea = LevelAreaForLevel(levelIndex);
    m_previousStateId = static_cast<GameStateId>(previousStateId);
    sprintf(m_versionString, "Alpha Version, Build %i, Monolith Productions Inc.", g_buildNumber);
    char areaKey[32];
    sprintf(areaKey, "AREA%i", IDX(m_levelArea));
    CRezDir* levelResources = m_resourceArchive->GetDirFromPath(areaKey);
    m_levelResources = levelResources;
    if (levelResources == NULL) {
        return 0;
    }
    if (m_world->GetImageRegistry()->HasWithPrefix("GAME") == 0) {
        CRezDir* imageResources = m_resourceArchive->GetDirFromPath("GAME_IMAGEZ");
        if (imageResources == NULL) {
            return 0;
        }
        g_resourceInstallActive = true;
        m_world->GetImageRegistry()->LoadImageSetsFromTree(imageResources, "GAME", "_");
        g_resourceInstallActive = false;
    }
    if (m_world->SoundRegistry()->HasWithPrefix("GAME") == 0) {
        CRezDir* soundResources = m_resourceArchive->GetDirFromPath("GAME_SOUNDZ");
        if (soundResources == NULL) {
            return 0;
        }
        m_world->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(soundResources), "GAME", "_");
    }
    if (m_world->GetAnimationRegistry()->HasWithPrefix("GAME") == 0) {
        CRezDir* animationResources = m_resourceArchive->GetDirFromPath("GAME_ANIZ");
        if (animationResources == NULL) {
            return 0;
        }
        m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(animationResources), "GAME", "_");
    }

    if (m_mgr->GruntPalettes()->BuildToolToyColorTable(m_mgr->m_resourceArchive) == 0) {
        return 0;
    }
    if (m_cursorSavedSurfaces[0] == NULL && m_cursorSavedSurfaces[1] == NULL) {
        CDDrawDeviceManager* deviceManager = m_world->GetDeviceManager();
        if (deviceManager == NULL) {
            return 0;
        }
        m_cursorSavedSurfaces[0] =
            deviceManager->CreateOffscreenSurface(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_cursorSavedSurfaces[0] == NULL) {
            return 0;
        }
        m_cursorSavedSurfaces[1] =
            deviceManager->CreateOffscreenSurface(0x40, 0x40, BPP_RGB_16, 0, -1);
        if (m_cursorSavedSurfaces[1] == NULL) {
            return 0;
        }
    }
    m_ready = true;
    return 1;
}
