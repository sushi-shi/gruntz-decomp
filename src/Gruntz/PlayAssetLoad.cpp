#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
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
#include <Gruntz/BattlezData.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/CheatMgr.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/DrawDebugStats.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
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
#include <Gruntz/LightFxRender.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/Multi.h>
#include <Gruntz/ParseSource.h>
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

#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        SoundCueRegistry* _s = (sndHost);                                                          \
        if (_s->m_silentMode == 0) {                                                               \
            SoundCue* found = NULL;                                                                \
            MapLookup(_s->m_cues, "GAME_TABHIGHLIGHT1", found);                                    \
            if (found != NULL)                                                                     \
                found->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);                               \
        }                                                                                          \
    } while (0)

RVA(0x000db600, 0x8f)
i32 CPlay::LoadActionTileSprites(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
               ->HasWithPrefix("ACTION")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveWithPrefix("ACTION", "");
    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveWithPrefix("BACK", "");
    g_resourceInstallActive = 0;

    CSymTab* tiles = (self->m_levelBank)->ResolvePath("TILEZ");
    if (!tiles) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(tiles, "", "_");
    return 1;
}

RVA(0x000db6c0, 0x70)
i32 CPlay::LoadLevelSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
               ->HasWithPrefix("LEVEL")) {
        return 1;
    }

    (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
        ->RemoveWithPrefix("LEVEL", "_");

    CSymTab* sounds = (self->m_levelBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
        ->LoadFromTree(static_cast<CSymTab*>(sounds), "LEVEL", "_");
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
    CSymTab* e = m_levelBank->ResolvePath("ANIZ");
    if (e == NULL) {
        return 0;
    }
    m_world->m_animRegistry->LoadFromTree(static_cast<CSymTab*>(e), "LEVEL", "_");
    return 1;
}

RVA(0x000db7e0, 0x84)
i32 CPlay::LoadLevelImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!force
        && (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
               ->HasWithPrefix("LEVEL")) {
        return 1;
    }

    (static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
        ->RemoveWithPrefix("LEVEL", "_");
    g_resourceInstallActive = 0;

    CSymTab* images = (self->m_levelBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(images, "LEVEL", "_");
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000db8a0, 0x67)
i32 CPlay::LoadGameImages(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
            ->HasWithPrefix("GAME")) {
        return 1;
    }

    g_resourceInstallActive = 1;
    CSymTab* images = (self->m_gameBank)->ResolvePath("IMAGEZ");
    if (!images) {
        return 0;
    }
    self->m_world->m_imageRegistry->InstallTree(images, "GAME", "_");
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000db930, 0x53)
i32 CPlay::LoadGameSounds(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if ((static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))->HasWithPrefix("GAME")) {
        return 1;
    }

    CSymTab* sounds = (self->m_gameBank)->ResolvePath("SOUNDZ");
    if (!sounds) {
        return 0;
    }
    (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
        ->LoadFromTree(static_cast<CSymTab*>(sounds), "GAME", "_");
    return 1;
}

RVA(0x000db9b0, 0x53)
i32 CPlay::LoadGameAnims(i32 force) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (self->m_world->m_animRegistry->HasWithPrefix("GAME")) {
        return 1;
    }

    CSymTab* anims = (self->m_gameBank)->ResolvePath("ANIZ");
    if (!anims) {
        return 0;
    }
    self->m_world->m_animRegistry->LoadFromTree(static_cast<CSymTab*>(anims), "GAME", "_");
    return 1;
}

RVA(0x000dba30, 0x1ca)
i32 CPlay::BuildMusicCategoryTable(i32) {
    m_mgr->m_midi->ClearSequences();

    CSymTab* levelSet = m_levelBank->ResolvePath("MIDIZ");
    if (levelSet) {
        CParseSource* e = levelSet->Insert("AMBIENT0", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "AMBIENT0");
            }
        }
        e = levelSet->Insert("AMBIENT1", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "AMBIENT1");
            }
        }
        e = levelSet->Insert("INTRO0", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "INTRO0");
            }
        }
        e = levelSet->Insert("INTRO1", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "INTRO1");
            }
        }
    }

    CSymTab* gameSet = m_gameBank->ResolvePath("MIDIZ");
    if (gameSet) {
        CParseSource* e = gameSet->Insert("POWERUP", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "POWERUP");
            }
        }
        e = gameSet->Insert("CURSE", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "CURSE");
            }
        }
        e = gameSet->Insert("MONOLITH", REZ_TAG_XMI);
        if (e) {
            char* res = e->BeginParse();
            if (res) {
                m_mgr->m_midi->LoadBuffer(res, e->m_length, "MONOLITH");
            }
        }
    }
    return 1;
}

RVA(0x000dbc80, 0x309)
i32 CPlay::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    if (m_mgr->m_strWorldFile.GetLength() != 0) {
        if (m_mgr->m_isBattlezLevel != 0) {
            CString key = "BATTLEZ_" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_mgr->m_isMultiLevel != 0) {
            CString key = "MULTI_" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, REZ_TAG_WWD);
            if (node == NULL) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else {
            if (m_world->m_level->LoadFromFile(m_mgr->GetWorldFileName()) == 0) {
                return 0;
            }
        }
    } else {
        CString key;
        i32 sel = m_levelIndex;
        if (g_levelBias100 != 0) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        CParseSource* node = m_levelBank->ResolveQualified(key, REZ_TAG_WWD);
        if (node == NULL) {
            return 0;
        }
        if (m_world->m_level->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4;
    g_backView = m_world->m_level->FindPlaneByName("BACK");
    return 1;
}

static inline void LookupCue(CMapStringToPtr& cues, const char* name, SoundCue*& out) {
    out = NULL;
    MapLookup(cues, name, out);
}

// @early-stop
// CFG, size, and all 64 relocations agree. The only residue is the scratch
// pair for (m_world load, &out lea): retail rotates ecx/eax -> edx/ecx ->
// eax/edx across consecutive LookupCue sites, ours pins eax/edx, so every
// third site already matches.
RVA(0x000dc060, 0x51b)
i32 CPlay::SetEffectSpriteDurations() {
    SoundCue* d;
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_PYRAMIDMOVE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTEROPEN", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTERCLOSE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_TELEPORTERALL", d);
    if (d != NULL) {
        d->m_replayDelayMs = 4000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_BRICKBREAK", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_DEATHBRIDGEMOVE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_WATERBRIDGEMOVE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_ROCKBREAK", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_LAVAGEYSER", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_TRAPDOORCLOSE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_TRAPDOOROPEN", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEIGNITE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEUP", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_CANDLEDOWN", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLAIR2", d);
    if (d != NULL) {
        d->m_replayDelayMs = 250;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLHOLE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 250;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_GOLFBALLSINK", d);
    if (d != NULL) {
        d->m_replayDelayMs = 250;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GAME_EXPLOSION1", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_OUTLETHAZARD", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZFREEZE2A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZUNFREEZE1A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_RESSURECT", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZSQUASH1A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 100;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_CLOUDHAZARDMOVE", d);
    if (d != NULL) {
        d->m_replayDelayMs = 10000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_CLOUDHAZARDKILL", d);
    if (d != NULL) {
        d->m_replayDelayMs = 3000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_DEATHZ_DEATHZELECTROCUTE1A", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_NERFGUNGRUNT_NERFGUNZGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_GUNHATGRUNT_GUNHATGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "GRUNTZ_WELDERGRUNT_WELDERZGRUNTP1AS1", d);
    if (d != NULL) {
        d->m_replayDelayMs = 1000;
    }
    LookupCue(m_world->m_soundRegistry->m_cues, "LEVEL_PLANEHAZARDFLY", d);
    if (d != NULL) {
        d->m_replayDelayMs = 5000;
    }
    return 1;
}

RVA(0x000dc6d0, 0x2e0)
i32 CPlay::BuildGruntTypeNameTable(
    PickupType typeIdx,
    i32 mode,
    i32 lightGate,
    CMulti* finishGate
) {
    CString name("NORMALGRUNT");
    switch (typeIdx) {
        case GRUNT_BOMB:
            name = "BOMBGRUNT";
            break;
        case GRUNT_BOOMERANG:
            name = "BOOMERANGGRUNT";
            break;
        case GRUNT_BRICK:
            name = "BRICKGRUNT";
            break;
        case GRUNT_CLUB:
            name = "CLUBGRUNT";
            break;
        case GRUNT_GAUNTLETZ:
            name = "GAUNTLETZGRUNT";
            break;
        case GRUNT_GLOVEZ:
            name = "GLOVEZGRUNT";
            break;
        case GRUNT_GOOBER:
            name = "GOOBERGRUNT";
            break;
        case GRUNT_GRAVITYBOOTZ:
            name = "GRAVITYBOOTZGRUNT";
            break;
        case GRUNT_GUNHAT:
            name = "GUNHATGRUNT";
            break;
        case GRUNT_NERFGUN:
            name = "NERFGUNGRUNT";
            break;
        case GRUNT_ROCK:
            name = "ROCKGRUNT";
            break;
        case GRUNT_SHIELD:
            name = "SHIELDGRUNT";
            break;
        case GRUNT_SHOVEL:
            name = "SHOVELGRUNT";
            break;
        case GRUNT_SPRING:
            name = "SPRINGGRUNT";
            break;
        case GRUNT_SPY:
            name = "SPYGRUNT";
            break;
        case GRUNT_SWORD:
            name = "SWORDGRUNT";
            break;
        case GRUNT_TIMEBOMB:
            name = "TIMEBOMBGRUNT";
            break;
        case GRUNT_TOOB:
            name = "TOOBGRUNT";
            if (BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate) == 0) {
                return 0;
            }
            name = "TOOBWATERGRUNT";
            return BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate);
        case GRUNT_WAND:
            name = "WANDGRUNT";
            break;
        case GRUNT_WARPSTONE:
            name = "WARPSTONEGRUNT";
            break;
        case GRUNT_WELDER:
            name = "WELDERGRUNT";
            break;
        case GRUNT_WINGZ:
            name = "WINGZGRUNT";
            break;
        case GRUNT_BABYWALKER:
            name = "BABYWALKERGRUNT";
            break;
        case GRUNT_BEACHBALL:
            name = "BEACHBALLGRUNT";
            break;
        case GRUNT_BIGWHEEL:
            name = "BIGWHEELGRUNT";
            break;
        case GRUNT_GOKART:
            name = "GOKARTGRUNT";
            break;
        case GRUNT_JACKINTHEBOX:
            name = "JACKINTHEBOXGRUNT";
            break;
        case GRUNT_JUMPROPE:
            name = "JUMPROPEGRUNT";
            break;
        case GRUNT_POGOSTICK:
            name = "POGOSTICKGRUNT";
            break;
        case GRUNT_SCROLL:
            name = "SCROLLGRUNT";
            break;
        case GRUNT_SQUEAKTOY:
            name = "SQUEAKTOYGRUNT";
            break;
        case GRUNT_YOYO:
            name = "YOYOGRUNT";
            break;
        case GRUNT_HAREKRISHNA:
            name = "HAREKRISHNAGRUNT";
            break;
        case GRUNT_REAPER:
            name = "REAPERGRUNT";
            break;
    }
    return BuildAssetNamespacePrefixes(name, mode, lightGate, finishGate);
}

RVA(0x000dca70, 0x4a4)
i32 CState::BuildAssetNamespacePrefixes(
    const CString& name,
    i32 mode,
    i32 lightGate,
    CMulti* finishGate
) {
    i32 result;
    if (mode != 0) {
        if (m_world->m_imageRegistry->HasWithPrefix("GRUNTZ_" + name) == 0) {
            g_gameReg->m_voiceManager->PauseAllVoices();
            (static_cast<CTriggerMgr*>(g_gameReg->m_cmdGrid))->DestroyAllAnims();
            if (lightGate != 0) {
                CString cs;
                cs.LoadString(IDS_LOADING);
                RECT r = *(&g_gameReg->m_world->m_level->m_planeCtx);
                RECT r2;
                CopyRect(&r2, &r);
                EngStr_DrawText(g_gameReg->m_world, &cs, &r2, 0x82, 1, 0xff, 0xff, 0, 1);
            }
            g_resourceInstallActive = 1;
            CSymTab* tree = m_gruntzBank->ResolvePath("IMAGEZ_" + name);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->m_imageRegistry->InstallTree(tree, "GRUNTZ_" + name, "_");
            g_resourceInstallActive = 0;
            if (finishGate != NULL) {
                finishGate->AckJoinFailure();
            }
        }
        if (m_world->m_soundRegistry->HasWithPrefix("GRUNTZ_" + name) == 0) {
            CSymTab* tree = m_gruntzBank->ResolvePath("SOUNDZ_" + name);
            if (tree != NULL) {

                m_world->m_soundRegistry
                    ->LoadFromTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
            }
        }
        if (m_world->m_animRegistry->HasWithPrefix("GRUNTZ_" + name) == 0) {
            CSymTab* tree = m_gruntzBank->ResolvePath("ANIZ_" + name);
            if (tree == NULL) {
                result = 0;
                goto done;
            }
            m_world->m_animRegistry
                ->LoadFromTree(static_cast<CSymTab*>(tree), "GRUNTZ_" + name, "_");
        }
        result = 1;
        goto done;
    }

    if (m_world->m_imageRegistry->HasWithPrefix("GRUNTZ_" + name)) {
        m_world->m_imageRegistry->RemoveWithPrefix("GRUNTZ_" + name, "_");
        if (finishGate != NULL) {
            finishGate->AckJoinFailure();
        }
    }
    if (m_world->m_soundRegistry->HasWithPrefix("GRUNTZ_" + name)) {
        m_world->m_soundRegistry->RemoveWithPrefix("GRUNTZ_" + name, "_");
    }
    if (m_world->m_animRegistry->HasWithPrefix("GRUNTZ_" + name)) {
        m_world->m_animRegistry->RemoveWithPrefix("GRUNTZ_" + name, "_");
    }
    result = 1;
done:
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000dd050, 0x24b)
i32 CPlay::BuildGruntNamespaceList(CMulti* finishGate) {
    CString s;
    s = "NORMALGRUNT";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "DEATHZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "ENTRANCEZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "EXITZ";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "GRUNTPUDDLE";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "PICKUPS";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    s = "BOMBGRUNT";
    if (!BuildAssetNamespacePrefixes(s, 1, 0, finishGate)) {
        return 0;
    }
    return 1;
}

RVA(0x000dd340, 0x189)
i32 CPlay::BuildWarlordNameTable(CMulti* finishGate) {
    for (i32 id = IDX(GRUNT_BOOMERANG); id <= IDX(GRUNT_YOYO); id++) {
        if (!BuildGruntTypeNameTable(static_cast<PickupType>(id), 0, 0, NULL)) {
            return 0;
        }
    }
    if (!BuildGruntTypeNameTable(GRUNT_HAREKRISHNA, 0, 0, finishGate)) {
        return 0;
    }
    if (!BuildGruntTypeNameTable(GRUNT_REAPER, 0, 0, finishGate)) {
        return 0;
    }
    CString s("WARLORDZ_NAPOLEAN");
    if (!BuildAssetNamespacePrefixes(s, 0, 0, finishGate)) {
        return 0;
    }
    s = "WARLORDZ_VIKING";
    if (!BuildAssetNamespacePrefixes(s, 0, 0, finishGate)) {
        return 0;
    }
    s = "WARLORDZ_PATTON";
    if (!BuildAssetNamespacePrefixes(s, 0, 0, finishGate)) {
        return 0;
    }
    return 1;
}

RVA(0x000dd540, 0x241)
i32 CPlay::BuildSpriteImageKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    g_resourceInstallActive = 1;
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_EXITZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<CDDrawWorkerRegistry*>(self->m_world->m_imageRegistry))
             ->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("IMAGEZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_imageRegistry->InstallTree(s, "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    g_resourceInstallActive = 0;
    return 1;
}

RVA(0x000dd830, 0x1e3)
i32 CPlay::LoadGruntSoundNamespaces(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }

    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_NORMALGRUNT");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_NORMALGRUNT", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_DEATHZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_DEATHZ", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_ENTRANCEZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_ENTRANCEZ", "_");
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_EXITZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_EXITZ");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_EXITZ", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_GRUNTPUDDLE");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_PICKUPS");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_PICKUPS", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!(static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
             ->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("SOUNDZ_BOMBGRUNT");
        if (s) {
            (static_cast<SoundCueRegistry*>(self->m_world->m_soundRegistry))
                ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_BOMBGRUNT", "_");
        }
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}

RVA(0x000ddaa0, 0x228)
i32 CPlay::BuildAnizKeyTable(CMulti* notify) {
    CPlay* self = this;
    if (!self->m_world) {
        return 0;
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_NORMALGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_NORMALGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_NORMALGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_DEATHZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_DEATHZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_DEATHZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_ENTRANCEZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_ENTRANCEZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_ENTRANCEZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_EXITZ")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_EXITZ");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_EXITZ", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_GRUNTPUDDLE")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_GRUNTPUDDLE");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_GRUNTPUDDLE", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_PICKUPS")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_PICKUPS");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_PICKUPS", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    if (!self->m_world->m_animRegistry->HasWithPrefix("GRUNTZ_BOMBGRUNT")) {
        CSymTab* s = (self->m_gruntzBank)->ResolvePath("ANIZ_BOMBGRUNT");
        if (!s) {
            return 0;
        }
        self->m_world->m_animRegistry
            ->LoadFromTree(static_cast<CSymTab*>(s), "GRUNTZ_BOMBGRUNT", "_");
        if (notify) {
            notify->AckJoinFailure();
        }
    }
    return 1;
}
