#include <StdAfx.h>

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <DinMgr2/InputMgrPtr.h>
#include <Dsndmgr/MidiManager.h>
#include <Enums.h>
#include <Gruntz/ActionOptionsMenuBar.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/AreaMgr.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GameStats.h>
#include <Gruntz/GameText.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/InputState.h>
#include <Gruntz/LevelArea.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Minimap.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/PlayStringId.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SbiMenuItemState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/String.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/View.h>
#include <Gruntz/VoiceManager.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Io/SaveGame.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/EngStr.h>
#include <Wap32/Object.h>
#include <Wap32/ScreenGeometry.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <ddraw.h>
#include <new>
#include <stdio.h>
#include <string.h>

class CImage;

RVA(0x000db600, 0x8f)
i32 CPlay::LoadActionTileSprites(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
               ->HasWithPrefix("ACTION")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
        ->RemoveWithPrefix("ACTION", "");
    (static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
        ->RemoveWithPrefix("BACK", "");
    g_resourceInstallActive = false;

    CRezDir* tiles = (self->m_levelResources)->GetDirFromPath("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_world->GetImageRegistry()->InstallTree(tiles, "", "_");
    return 1;
}

RVA(0x000db6c0, 0x70)
i32 CPlay::LoadLevelSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
               ->HasWithPrefix("LEVEL")) {
        return 1;
    }

    (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
        ->RemoveWithPrefix("LEVEL", "_");

    CRezDir* sounds = (self->m_levelResources)->GetDirFromPath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
        ->LoadFromTree(static_cast<CRezDir*>(sounds), "LEVEL", "_");
    return 1;
}

RVA(0x000db750, 0x70)
i32 CPlay::LoadLevelAnims(i32 force) {
    if (m_world == NULL) {
        return 0;
    }
    if (force == 0) {
        if (m_world->m_animRegistry->HasWithPrefix("LEVEL") != 0) {
            return 1;
        }
    }
    m_world->m_animRegistry->RemoveWithPrefix("LEVEL", "_");
    CRezDir* e = m_levelResources->GetDirFromPath("ANIZ");
    if (e == NULL) {
        return 0;
    }
    m_world->m_animRegistry->LoadFromTree(static_cast<CRezDir*>(e), "LEVEL", "_");
    return 1;
}

RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
               ->HasWithPrefix("LEVEL")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
        ->RemoveWithPrefix("LEVEL", "_");
    g_resourceInstallActive = false;

    CRezDir* images = (self->m_levelResources)->GetDirFromPath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->GetImageRegistry()->InstallTree(images, "LEVEL", "_");
    g_resourceInstallActive = false;
    return 1;
}

RVA(0x000db8a0, 0x67)
i32 CPlay::LoadGameImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
            ->HasWithPrefix("GAME")) {
        return 1;
    }

    g_resourceInstallActive = true;
    CRezDir* images = (self->m_gameResources)->GetDirFromPath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->GetImageRegistry()->InstallTree(images, "GAME", "_");
    g_resourceInstallActive = false;
    return 1;
}

RVA(0x000db930, 0x53)
i32 CPlay::LoadGameSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))->HasWithPrefix("GAME")) {
        return 1;
    }

    CRezDir* sounds = (self->m_gameResources)->GetDirFromPath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
        ->LoadFromTree(static_cast<CRezDir*>(sounds), "GAME", "_");
    return 1;
}

RVA(0x000db9b0, 0x53)
i32 CPlay::LoadGameAnims(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (self->m_world->GetAnimationRegistry()->HasWithPrefix("GAME")) {
        return 1;
    }

    CRezDir* anims = (self->m_gameResources)->GetDirFromPath("ANIZ");
    if (!anims) {
        return 0;
    }
    self->m_world->GetAnimationRegistry()->LoadFromTree(static_cast<CRezDir*>(anims), "GAME", "_");
    return 1;
}

RVA(0x000dba30, 0x1ca)
i32 CPlay::LoadMusicSequences(i32) {
    m_mgr->m_midi->ClearSequences();

    CRezDir* levelSet = m_levelResources->GetDirFromPath("MIDIZ");
    if (levelSet) {
        CRezItm* e = levelSet->GetRez("AMBIENT0", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "AMBIENT0");
            }
        }
        e = levelSet->GetRez("AMBIENT1", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "AMBIENT1");
            }
        }
        e = levelSet->GetRez("INTRO0", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "INTRO0");
            }
        }
        e = levelSet->GetRez("INTRO1", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "INTRO1");
            }
        }
    }

    CRezDir* gameSet = m_gameResources->GetDirFromPath("MIDIZ");
    if (gameSet) {
        CRezItm* e = gameSet->GetRez("POWERUP", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "POWERUP");
            }
        }
        e = gameSet->GetRez("CURSE", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "CURSE");
            }
        }
        e = gameSet->GetRez("MONOLITH", REZ_TAG_XMI);
        if (e) {
            u8* res = e->Load();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->GetSize(), "MONOLITH");
            }
        }
    }
    return 1;
}

RVA(0x000dbc80, 0x309)
i32 CPlay::LoadLevelWorld(i32 unused) {
    m_world->GetLevel()->ReleaseChildren();
    if (!m_mgr->m_strWorldFile.IsEmpty()) {
        if (m_mgr->m_isBuiltInBattlezLevel != false) {
            CString key = "BATTLEZ_" + m_mgr->GetWorldFileName();
            CRezItm* node = m_gameResources->GetRezFromPath(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->GetLevel()->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_mgr->m_isBuiltInMultiplayerLevel != false) {
            CString key = "MULTI_" + m_mgr->GetWorldFileName();
            CRezItm* node = m_gameResources->GetRezFromPath(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->GetLevel()->LoadFromSource(node) == 0) {
                return 0;
            }
        } else {
            if (m_world->GetLevel()->LoadFromFile(m_mgr->GetWorldFileName()) == 0) {
                return 0;
            }
        }
    } else {
        CString key;
        i32 sel = m_levelIndex;
        if (g_levelBias100 != false) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        CRezItm* node = m_levelResources->GetRezFromPath(key, REZ_TAG_WWD);
        if (node == NULL) {
            return 0;
        }
        if (m_world->GetLevel()->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_world->GetLevel()->NotifyAllPlanes();
    m_world->GetLevel()->AddFlags(4);
    g_backView = m_world->GetLevel()->FindPlaneByName("BACK");
    return 1;
}

RVA(0x000dc060, 0x51b)
i32 CPlay::ConfigureSoundReplayDelays() {
    SoundCue* cue;
    cue = m_world->SoundRegistry()->FindCue("GAME_PYRAMIDMOVE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GAME_TELEPORTEROPEN");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("GAME_TELEPORTERCLOSE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("GAME_TELEPORTERALL");
    if (cue != NULL) {
        cue->m_replayDelayMs = 4000;
    }
    cue = m_world->SoundRegistry()->FindCue("GAME_BRICKBREAK");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_DEATHBRIDGEMOVE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_WATERBRIDGEMOVE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_ROCKBREAK");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_LAVAGEYSER");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_TRAPDOORCLOSE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_TRAPDOOROPEN");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_CANDLEIGNITE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_CANDLEUP");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_CANDLEDOWN");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_GOLFBALLAIR2");
    if (cue != NULL) {
        cue->m_replayDelayMs = 250;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_GOLFBALLHOLE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 250;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_GOLFBALLSINK");
    if (cue != NULL) {
        cue->m_replayDelayMs = 250;
    }
    cue = m_world->SoundRegistry()->FindCue("GAME_EXPLOSION1");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_OUTLETHAZARD");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZFREEZE1A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZFREEZE2A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZUNFREEZE1A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_RESSURECT");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZSQUASH1A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 100;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_CLOUDHAZARDMOVE");
    if (cue != NULL) {
        cue->m_replayDelayMs = 10000;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_CLOUDHAZARDKILL");
    if (cue != NULL) {
        cue->m_replayDelayMs = 3000;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1");
    if (cue != NULL) {
        cue->m_replayDelayMs = 1000;
    }
    cue = m_world->SoundRegistry()->FindCue("LEVEL_PLANEHAZARDFLY");
    if (cue != NULL) {
        cue->m_replayDelayMs = 5000;
    }
    return 1;
}

RVA(0x000dc6d0, 0x2e0)
i32 CPlay::SetGruntTypeAssetsLoaded(
    PickupType gruntType,
    i32 loadAssets,
    i32 showLoadingText,
    CMulti* multiplayerSession
) {
    CString resourceGroup("NORMALGRUNT");
    switch (gruntType) {
        case GRUNT_BOMB:
            resourceGroup = "BOMBGRUNT";
            break;
        case GRUNT_BOOMERANG:
            resourceGroup = "BOOMERANGGRUNT";
            break;
        case GRUNT_BRICK:
            resourceGroup = "BRICKGRUNT";
            break;
        case GRUNT_CLUB:
            resourceGroup = "CLUBGRUNT";
            break;
        case GRUNT_GAUNTLETZ:
            resourceGroup = "GAUNTLETZGRUNT";
            break;
        case GRUNT_GLOVEZ:
            resourceGroup = "GLOVEZGRUNT";
            break;
        case GRUNT_GOOBER:
            resourceGroup = "GOOBERGRUNT";
            break;
        case GRUNT_GRAVITYBOOTZ:
            resourceGroup = "GRAVITYBOOTZGRUNT";
            break;
        case GRUNT_GUNHAT:
            resourceGroup = "GUNHATGRUNT";
            break;
        case GRUNT_NERFGUN:
            resourceGroup = "NERFGUNGRUNT";
            break;
        case GRUNT_ROCK:
            resourceGroup = "ROCKGRUNT";
            break;
        case GRUNT_SHIELD:
            resourceGroup = "SHIELDGRUNT";
            break;
        case GRUNT_SHOVEL:
            resourceGroup = "SHOVELGRUNT";
            break;
        case GRUNT_SPRING:
            resourceGroup = "SPRINGGRUNT";
            break;
        case GRUNT_SPY:
            resourceGroup = "SPYGRUNT";
            break;
        case GRUNT_SWORD:
            resourceGroup = "SWORDGRUNT";
            break;
        case GRUNT_TIMEBOMB:
            resourceGroup = "TIMEBOMBGRUNT";
            break;
        case GRUNT_TOOB:
            resourceGroup = "TOOBGRUNT";
            if (SetAssetGroupLoaded(resourceGroup, loadAssets, showLoadingText, multiplayerSession)
                == 0) {
                return 0;
            }
            resourceGroup = "TOOBWATERGRUNT";
            return SetAssetGroupLoaded(
                resourceGroup,
                loadAssets,
                showLoadingText,
                multiplayerSession
            );
        case GRUNT_WAND:
            resourceGroup = "WANDGRUNT";
            break;
        case GRUNT_WARPSTONE:
            resourceGroup = "WARPSTONEGRUNT";
            break;
        case GRUNT_WELDER:
            resourceGroup = "WELDERGRUNT";
            break;
        case GRUNT_WINGZ:
            resourceGroup = "WINGZGRUNT";
            break;
        case GRUNT_BABYWALKER:
            resourceGroup = "BABYWALKERGRUNT";
            break;
        case GRUNT_BEACHBALL:
            resourceGroup = "BEACHBALLGRUNT";
            break;
        case GRUNT_BIGWHEEL:
            resourceGroup = "BIGWHEELGRUNT";
            break;
        case GRUNT_GOKART:
            resourceGroup = "GOKARTGRUNT";
            break;
        case GRUNT_JACKINTHEBOX:
            resourceGroup = "JACKINTHEBOXGRUNT";
            break;
        case GRUNT_JUMPROPE:
            resourceGroup = "JUMPROPEGRUNT";
            break;
        case GRUNT_POGOSTICK:
            resourceGroup = "POGOSTICKGRUNT";
            break;
        case GRUNT_SCROLL:
            resourceGroup = "SCROLLGRUNT";
            break;
        case GRUNT_SQUEAKTOY:
            resourceGroup = "SQUEAKTOYGRUNT";
            break;
        case GRUNT_YOYO:
            resourceGroup = "YOYOGRUNT";
            break;
        case GRUNT_HAREKRISHNA:
            resourceGroup = "HAREKRISHNAGRUNT";
            break;
        case GRUNT_REAPER:
            resourceGroup = "REAPERGRUNT";
            break;
    }
    return SetAssetGroupLoaded(resourceGroup, loadAssets, showLoadingText, multiplayerSession);
}

RVA(0x000dca70, 0x4a4)
i32 CState::SetAssetGroupLoaded(
    const CString& resourceGroup,
    i32 loadAssets,
    i32 showLoadingText,
    CMulti* multiplayerSession
) {
    i32 result;
    if (loadAssets != 0) {
        if (m_world->GetImageRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup) == 0) {
            g_gameReg->VoiceMgr()->PauseAllVoices();
            (static_cast<CTriggerMgr*>(g_gameReg->GetTriggerMgr()))->DestroyAllAnims();
            if (showLoadingText != 0) {
                CString cs;
                cs.LoadString(IDS_LOADING);
                RECT r = g_gameReg->World()->GetLevel()->GetViewportRect();
                RECT r2;
                CopyRect(&r2, &r);
                DrawTextToFrontSurface(g_gameReg->World(), &cs, &r2, 0x82, 1, 0xff, 0xff, 0, 1);
            }
            g_resourceInstallActive = true;
            CRezDir* tree = m_gruntResources->GetDirFromPath("IMAGEZ_" + resourceGroup);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->GetImageRegistry()->InstallTree(tree, "GRUNTZ_" + resourceGroup, "_");
            g_resourceInstallActive = false;
            if (multiplayerSession != NULL) {
                multiplayerSession->SendLobbyKeepAlive();
            }
        }
        if (m_world->SoundRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup) == 0) {
            CRezDir* tree = m_gruntResources->GetDirFromPath("SOUNDZ_" + resourceGroup);
            if (tree != NULL) {

                m_world->SoundRegistry()
                    ->LoadFromTree(static_cast<CRezDir*>(tree), "GRUNTZ_" + resourceGroup, "_");
            }
        }
        if (m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup) == 0) {
            CRezDir* tree = m_gruntResources->GetDirFromPath("ANIZ_" + resourceGroup);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->GetAnimationRegistry()
                ->LoadFromTree(static_cast<CRezDir*>(tree), "GRUNTZ_" + resourceGroup, "_");
        }
        result = 1;
        goto done;
    }

    if (m_world->GetImageRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup)) {
        m_world->GetImageRegistry()->RemoveWithPrefix("GRUNTZ_" + resourceGroup, "_");
        if (multiplayerSession != NULL) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (m_world->SoundRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup)) {
        m_world->SoundRegistry()->RemoveWithPrefix("GRUNTZ_" + resourceGroup, "_");
    }
    if (m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_" + resourceGroup)) {
        m_world->GetAnimationRegistry()->RemoveWithPrefix("GRUNTZ_" + resourceGroup, "_");
    }
    result = 1;
done:
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000dd050, 0x24b)
i32 CPlay::LoadGruntAssetNamespaces(CMulti* multiplayerSession) {
    CString s;
    s = "NORMALGRUNT";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "DEATHZ";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "ENTRANCEZ";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "EXITZ";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "GRUNTPUDDLE";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "PICKUPS";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    s = "BOMBGRUNT";
    if (!SetAssetGroupLoaded(s, 1, 0, multiplayerSession)) {
        return 0;
    }
    return 1;
}

RVA(0x000dd340, 0x189)
i32 CPlay::UnloadGruntAndWarlordAssets(CMulti* multiplayerSession) {
    for (i32 id = IDX(GRUNT_BOOMERANG); id <= IDX(GRUNT_YOYO); id++) {
        if (!SetGruntTypeAssetsLoaded(static_cast<PickupType>(id), 0, 0, NULL)) {
            return 0;
        }
    }
    if (!SetGruntTypeAssetsLoaded(GRUNT_HAREKRISHNA, 0, 0, multiplayerSession)) {
        return 0;
    }
    if (!SetGruntTypeAssetsLoaded(GRUNT_REAPER, 0, 0, multiplayerSession)) {
        return 0;
    }
    CString s("WARLORDZ_NAPOLEAN");
    if (!SetAssetGroupLoaded(s, 0, 0, multiplayerSession)) {
        return 0;
    }
    s = "WARLORDZ_VIKING";
    if (!SetAssetGroupLoaded(s, 0, 0, multiplayerSession)) {
        return 0;
    }
    s = "WARLORDZ_PATTON";
    if (!SetAssetGroupLoaded(s, 0, 0, multiplayerSession)) {
        return 0;
    }
    return 1;
}

RVA(0x000dd540, 0x241)
i32 CPlay::LoadGruntImageNamespaces(CMulti* multiplayerSession) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    g_resourceInstallActive = true;
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_NORMALGRUNT", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_DEATHZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_ENTRANCEZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_EXITZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_EXITZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_PICKUPS", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->GetImageRegistry()))
             ->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->GetImageRegistry()->InstallTree(s, "GRUNTZ_BOMBGRUNT", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    g_resourceInstallActive = false;
    return 1;
}

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CMulti* multiplayerSession) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }

    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_NORMALGRUNT");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_DEATHZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_ENTRANCEZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_EXITZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_EXITZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_EXITZ", "_");
        }
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_PICKUPS");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_PICKUPS", "_");
        }
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
             ->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("SOUNDZ_BOMBGRUNT");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->SoundRegistry()))
                ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_BOMBGRUNT", "_");
        }
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    return 1;
}

RVA(0x000ddaa0, 0x228)
i32 CPlay::LoadGruntAnimationNamespaces(CMulti* multiplayerSession) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_NORMALGRUNT", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_DEATHZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_ENTRANCEZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_EXITZ")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_EXITZ", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_PICKUPS", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    if (!self->m_world->GetAnimationRegistry()->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CRezDir* s = (self->m_gruntResources)->GetDirFromPath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->GetAnimationRegistry()
            ->LoadFromTree(static_cast<CRezDir*>(s), "GRUNTZ_BOMBGRUNT", "_");
        if (multiplayerSession) {
            multiplayerSession->SendLobbyKeepAlive();
        }
    }
    return 1;
}
