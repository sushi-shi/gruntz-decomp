#include <rva.h>

#include <Gruntz/InGameIcon.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueInline.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpellId.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TileSnapMacros.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <string.h>

RVA_DYNINIT(0x000977e0, 0xa, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097800, 0x15, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097830, 0xe, CActRegPool<CInGameIcon>::s_table)
RVA_DYNINIT(0x00097850, 0x1f, CActRegPool<CInGameIcon>::s_table)
template<> DATA(0x002458b0)
CActReg CActRegPool<CInGameIcon>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x00097d40, 0xa, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097d60, 0x15, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097d90, 0xe, CActRegPool<CToyPeek>::s_table)
RVA_DYNINIT(0x00097db0, 0x1f, CActRegPool<CToyPeek>::s_table)
template<> DATA(0x00245928)
CActReg CActRegPool<CToyPeek>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000993c0, 0xa, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x000993e0, 0x15, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x00099410, 0xe, CActRegPool<CInGameText>::s_table)
RVA_DYNINIT(0x00099430, 0x1f, CActRegPool<CInGameText>::s_table)
template<> DATA(0x00245950)
CActReg CActRegPool<CInGameText>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00011c10, 0x1e, ??_GCToyPeek@@UAEPAXI@Z)

RVA_COMPGEN(0x00011c40, 0x44, ??1CToyPeek@@UAE@XZ)

RVA_COMPGEN(0x00011cd0, 0x1e, ??_GCInGameIcon@@UAEPAXI@Z)
RVA_COMPGEN(0x00011d00, 0x44, ??1CInGameIcon@@UAE@XZ)

RVA_COMPGEN(0x00011d90, 0x1e, ??_GCInGameText@@UAEPAXI@Z)
RVA_COMPGEN(0x00011dc0, 0x44, ??1CInGameText@@UAE@XZ)

static inline SoundCue* LookupCue(CMapStringToPtr& cues, LPCTSTR name) {
    SoundCue* found = NULL;
    MapLookup(cues, name, found);
    return found;
}

static inline CAniElement* LookupAni(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* found = NULL;
    MapLookup(map, name, found);
    return found;
}

static inline CWwdSpriteObject* LookupSerialRef(CMapPtrToPtr& byId, i32 id) {
    CGameObject* found = NULL;
    if (MapLookupById(byId, id, found) == 0) {
        return NULL;
    }
    if (found == NULL) {
        return NULL;
    }
    return found->GetClassId() == CLASSID_SERIALREF ? static_cast<CWwdSpriteObject*>(found) : NULL;
}

// @early-stop
RVA(0x00095b10, 0x15f0)
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    m_driftPos.m_lo = 0;
    m_driftThresh.m_lo = 0;
    m_driftPos.m_hi = 0;
    m_driftThresh.m_hi = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    SNAP_OBJECT_TO_TILE_CENTER_COPY(m_object, snapX, snapY)

    CWwdSpriteObject* snapped = m_object;
    SET_SORT_KEY_IF_CHANGED(snapped, SORTKEY_INGAME_INFO)

    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);

    SetObjectFlags(2);
    SetupSprite(NULL);

    m_glitterSprite = NULL;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    InGameIconGlitter glitter = ICON_GLITTER_NONE;
    CDDrawWorker* frameSet = m_wwdObject->m_imageSet;
    if (frameSet != NULL) {
        CString name;
        name = frameSet->m_name;

        if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOMBZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BOMB);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BOOMERANG);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BRICKZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BRICK);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_CLUBZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_CLUB);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GAUNTLETZ);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GLOVEZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GLOVEZ);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GOOBERZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GOOBER);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GRAVITYBOOTZ);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GUNHATZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GUNHAT);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_NERFGUN);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_ROCKZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_ROCK);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHIELDZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SHIELD);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHOVELZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SHOVEL);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPRINGZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SPRING);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPYZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SPY);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SWORDZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SWORD);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_TIMEBOMB);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TOOBZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_TOOB);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WANDZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_WAND);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ1") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_FIRST);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[0].m_x = anchorX;
            lvl->m_anchors[0].m_y = anchorY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_SECOND);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[1].m_x = anchorX;
            lvl->m_anchors[1].m_y = anchorY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_THIRD);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[2].m_x = anchorX;
            lvl->m_anchors[2].m_y = anchorY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_FOURTH);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            i32 anchorX = m_object->m_screenX;
            i32 anchorY = m_object->m_screenY;
            lvl->m_anchors[3].m_x = anchorX;
            lvl->m_anchors[3].m_y = anchorY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WELDERZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_WELDER);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WINGZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_WINGZ);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BABYWALKER);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BEACHBALL);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_BIGWHEEL);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_GOKARTZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_GOKART);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_JACKINTHEBOX);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_JUMPROPE);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_POGOSTICK);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SCROLLZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SCROLL);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_SQUEAKTOY);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_YOYOZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_YOYO);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_MEGAPHONE);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH1") == 0) {
            m_object->m_smarts = IDX(PICKUP_HEALTH1);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH2") == 0) {
            m_object->m_smarts = IDX(PICKUP_HEALTH2);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH3") == 0) {
            m_object->m_smarts = IDX(PICKUP_HEALTH3);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_CONVERSION") == 0) {
            m_object->m_smarts = IDX(PICKUP_CONVERSION);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH") == 0) {
            m_object->m_smarts = IDX(PICKUP_DEATHTOUCH);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_GHOST") == 0) {
            m_object->m_smarts = IDX(PICKUP_GHOST);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY") == 0) {
            m_object->m_smarts = IDX(PICKUP_INVULNERABILITY);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR") == 0) {
            m_object->m_smarts = IDX(PICKUP_REACTIVEARMOR);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_ROIDZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_ROIDZ);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED") == 0) {
            m_object->m_smarts = IDX(PICKUP_SUPERSPEED);
            SetupSprite("GAME_POWERUP");
            glitter = ICON_GLITTER_POWERUP_RED;
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETW") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                SetObjectFlags(0x10000);
                return;
            }
            m_object->m_smarts = IDX(PICKUP_W);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETA") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                SetObjectFlags(0x10000);
                return;
            }
            m_object->m_smarts = IDX(PICKUP_A);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETR") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                SetObjectFlags(0x10000);
                return;
            }
            m_object->m_smarts = IDX(PICKUP_R);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETP") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
                SetObjectFlags(0x10000);
                return;
            }
            m_object->m_smarts = IDX(PICKUP_P);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH") == 0) {
            m_object->m_smarts = IDX(PICKUP_STOPWATCH);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_COIN") == 0) {
            m_object->m_smarts = IDX(PICKUP_COIN);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_TOYBOX") == 0) {
            m_object->m_smarts = IDX(PICKUP_TOYBOX);
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MINICAM") == 0) {
            m_object->m_smarts = IDX(PICKUP_MINICAM);
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SCREENSHAKE") == 0) {
            m_object->m_smarts = IDX(PICKUP_SCREENSHAKE);
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_RANDOMCOLORZ") == 0) {
            m_object->m_smarts = IDX(PICKUP_RANDOMCOLORZ);
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_BLACKSCREEN") == 0) {
            m_object->m_smarts = IDX(PICKUP_BLACKSCREEN);
            glitter = ICON_GLITTER_CURSE_GREEN;
            SetupSprite("GAME_CURSE");
        }
    }

    PickupType pickup = static_cast<PickupType>(m_object->m_smarts);
    if (pickup == PICKUP_WARPSTONE && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
        CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
        CString levelStr;
        levelStr.Format("Level%i", lvl->m_levelIndex);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        warpName.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->SetImageSetByName(warpName);
        m_object->m_health = target;
    }

    if (glitter != ICON_GLITTER_NONE) {
        CWwdSpriteObject* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            m_object->m_screenX,
            m_object->m_screenY,
            SORTKEY_INGAME_INFO_FX,
            "SimpleAnimation",
            0x40003
        );
        m_glitterSprite = fx;
        if (glitter == ICON_GLITTER_POWERUP_RED) {
            fx->SetImageSetByName("GAME_GLITTERRED");
        }
        if (glitter == ICON_GLITTER_CURSE_GREEN) {
            m_glitterSprite->SetImageSetByName("GAME_GLITTERGREEN");
        }
        m_glitterSprite->SetAnimationByName("GAME_CYCLE100", 0);
    }

    if (HandleInput() == 0) {
        SetObjectFlags(0x10000);
        return;
    }

    i32 mv = m_object->m_objectId;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 col = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 row = m_object->m_screenY >> TILE_SHIFT_PX;
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        grid->m_rowInts[row][col * 7 + 2] = mv;
        if (mv != 0) {
            grid->m_rowInts[row][col * 7] |= 0x40000;
        } else {
            grid->m_rowInts[row][col * 7] &= ~0x40000;
        }
    }
    m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
}

RVA(0x00097680, 0x110)
i32 CInGameIcon::HandleInput() {
    CWwdSpriteObject* obj = m_object;
    PickupType cmd = static_cast<PickupType>(obj->m_smarts);
    CShadeTable* rec;
    if (cmd == PICKUP_TOYBOX) {
        i32 key = obj->m_score;
        PickupType sub = static_cast<PickupType>(obj->m_points);
        if (sub < PICKUP_TOYZ_FIRST || sub > PICKUP_TOYZ_LAST) {
            return 0;
        }
        i32 icon = IDX(g_gameReg->m_players[key].m_color);
        if (icon < 0 || icon >= TINT_COUNT) {
            icon = IDX(TINT_ORANGE);
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == NULL) {
            rec = g_gameReg->m_spriteFactory->GetSel(IDX(TINT_GREEN), 0);
        }
    } else if (cmd == PICKUP_SCROLL || cmd == PICKUP_WAND) {
        i32 icon;
        switch (static_cast<SpellId>(obj->m_faceDirection)) {
            case SPELL_FREEZE:
                icon = IDX(TINT_WHITE);
                break;
            case SPELL_HEALTH:
                icon = IDX(TINT_GREEN);
                break;
            case SPELL_RESURRECTION:
                icon = IDX(TINT_ORANGE);
                break;
            case SPELL_RANDOM_TOYZ:
                icon = IDX(TINT_PINK);
                break;
            case SPELL_TELEPORT:
                icon = IDX(TINT_BLUE);
                break;
            case SPELL_ROLLING_BALLZ:
                icon = IDX(TINT_RED);
                break;
            default:
                icon = IDX(TINT_BLACK);
                break;
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == NULL) {
            rec = g_gameReg->m_spriteFactory->GetSel(IDX(TINT_GREEN), 0);
        }
    } else {
        return 1;
    }
    CWwdSpriteObject* o = m_object;
    SET_DRAW_FILL(o, SHADE_PAL_16, rec);
    return 1;
}

RVA(0x00097880, 0x102)
void CInGameIcon::FireActivation(i32 id) {
    if (*CActRegPool<CInGameIcon>::s_table.ResolveEntry(id) != NULL) {
        (this->*(*CActRegPool<CInGameIcon>::s_table.ResolveEntry(id)))();
    }
}

RVA(0x000979e0, 0x2ac)
void RegisterIconActions() {
    ACT_NAME_ID_CALL_REPORT(idxA, "A")
    CActHandler* dslotA = CActRegPool<CInGameIcon>::s_table.ResolveEntryCallReport(idxA);
    *dslotA = static_cast<CActHandler>(&CInGameIcon::PeekCycle);

    ACT_NAME_ID(idxB, "B")
    CActHandler* dslotB = CActRegPool<CInGameIcon>::s_table.ResolveEntryCallReport(idxB);
    *dslotB = static_cast<CActHandler>(&CInGameIcon::Reposition);
}

RVA(0x00097de0, 0x102)
void CToyPeek::FireActivation(i32 id) {
    if (*CActRegPool<CToyPeek>::s_table.ResolveEntry(id) != NULL) {
        (this->*(*CActRegPool<CToyPeek>::s_table.ResolveEntry(id)))();
    }
}

RVA(0x00097f40, 0x18d)
void RegisterIconState() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = CActRegPool<CToyPeek>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameIcon::RefreshCell);
}

// @early-stop
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_startClock.m_v = 0;
    m_countdown.m_v = 0;
    m_object->m_screenY -= 0x18;
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    SetImageFrameByName("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", m_object->m_smarts);
    m_countdown.m_v = 0x1388;
    m_startClock.m_v = static_cast<u32>(g_frameTime);
    SET_ANIMATION_ACT("A");
}

RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CWwdSpriteObject* obj = m_object;
    i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = (obj->m_screenY + 0x18) >> TILE_SHIFT_PX;
    i64 delta = static_cast<i64>(g_frameTime) - m_driftPos.m_v;
    if (delta < m_driftThresh.m_v) {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 cell;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            BrickzCell* row = grid->m_rows[tileY];
            cell = row[tileX].m_objectId;
        } else {
            cell = 0;
        }
        if (cell != 0) {
            return 0;
        }
    }
    CWwdSpriteObject* r = m_wwdObject;
    r->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    return 0;
}

RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)

    SerBandPair(ar, mode, &m_startClock);
    return 1;
}

// @early-stop
RVA(0x000984b0, 0x186)
i32 CInGameIcon::PeekCycle() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* obj = m_object;
    PickupType cmd = static_cast<PickupType>(obj->m_smarts);
    if (cmd == PICKUP_TOYBOX) {
        i32 tileY = obj->m_screenY >> TILE_SHIFT_PX;
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
        i32 cell = grid->CellFlagsAt(tileX, tileY);
        if ((cell & BRICKZ_BLOCKED_MASK) != 0 || (cell & 2) != 0) {
            if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
                grid->m_rows[tileY][tileX].m_objectId = 0;
                grid->m_rows[tileY][tileX].m_flags &= ~0x40000;
            }
            SetObjectFlags(0x10000);
        }
        return 0;
    }
    if (cmd != PICKUP_WAND && cmd != PICKUP_SCROLL) {
        return 0;
    }
    if (obj->m_faceDirection != 0) {
        return 0;
    }
    if (static_cast<i64>(g_frameTime) - m_peekTimer.m_v >= m_peekWindow.m_v) {
        CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(GetRandomNumber() % 0x11, 0);
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL(o, SHADE_PAL_16, rec);
        m_peekWindow.m_lo = 0xfa;
        m_peekWindow.m_hi = 0;
        m_peekTimer.m_lo = g_frameTime;
        m_peekTimer.m_hi = 0;
    }
    return 0;
}

static inline void ClearTileBit(CGruntzMgr* reg, CGameObject* owner) {
    CMapMgr* grid = reg->m_tileGrid;
    i32 tileX = owner->m_screenX >> TILE_SHIFT_PX;
    i32 tileY = owner->m_screenY >> TILE_SHIFT_PX;
    if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {

        i32 cellInt = tileX * 8 - tileX;
        i32* cell0 = grid->m_rowInts[tileY];
        cell0[cellInt + 2] = 0;
        i32* cell1 = grid->m_rowInts[tileY];
        cell1[cellInt] &= ~0x40000;
    }
}

// @early-stop
RVA(0x000986b0, 0x30c)

i32 CInGameIcon::PlaceAt(i32 playerIndex, i32 unitIndex) {
    CWwdSpriteObject* obj;
    CWwdSpriteObject* o;
    CWwdSpriteObject* r;
    CWwdSpriteObject* owner;
    CWwdSpriteObject* rend;
    CGrunt* cell;
    CGrunt* placed;
    CLogicRecord* logicRecord;
    PickupType cmd;
    PickupType toyboxPickup;
    i32 matchActive;
    i32 flag;
    i32 sub;
    i32 idx;
    i32 ok;
    PickupType pickup;
    CGruntzMgr* reg = g_gameReg;
    if (reg->m_gameMode == GAMEMODE_QUESTZ && playerIndex != g_curPlayer
        && static_cast<PickupType>(m_object->m_smarts) != PICKUP_TOYBOX) {
        goto fail;
    }
    pickup = static_cast<PickupType>(m_object->m_smarts);
    obj = m_object;
    if (pickup == PICKUP_TOYBOX) {

        toyboxPickup = static_cast<PickupType>(obj->m_points);
        matchActive = 0;
        flag = 1;
        if (obj->m_score == playerIndex) {
            matchActive = 1;
            flag = 0;
        }
        sub = obj->m_faceDirection;
        idx = playerIndex * 15 + unitIndex;
        cell = reg->m_triggerMgr->m_units[idx];
        if (cell == NULL || cell->m_entranceCommitted == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = cell->LoadPickupSprites(toyboxPickup, flag, 0, sub, 0);
        } else {
            ok = cell->LoadGruntTypeTable(toyboxPickup, flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            goto fail;
        }
        if (m_cue != NULL) {
            o = m_object;
            if (CGameLevel::PointInRect(&reg->m_viewBounds, o->m_screenX, o->m_screenY)) {

                m_cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        r = m_wwdObject;
        r->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        return 1;
    }

    sub = obj->m_faceDirection;
    cmd = static_cast<PickupType>(obj->m_smarts);
    idx = playerIndex * 15 + unitIndex;
    cell = reg->m_triggerMgr->m_units[idx];
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok != 0) {
        if (cmd == PICKUP_WARPSTONE) {
            placed = reg->m_triggerMgr->m_units[idx];
            if (placed != NULL) {
                placed->m_warpstoneAnchorIndex = m_object->m_health;
                reg = g_gameReg;
            }
        }
        if (m_cue != NULL) {
            o = m_object;
            if (CGameLevel::PointInRect(&reg->m_viewBounds, o->m_screenX, o->m_screenY)) {

                m_cue->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        owner = m_wwdObject;
        if (owner->m_damage > 0) {
            owner->m_stateFlags |= SPRITE_STATE_HIDDEN;
            logicRecord = m_logicRecord;
            SET_ANIMATION_ACT("B");
            owner = m_wwdObject;
            m_driftPos.m_lo = g_frameTime;
            m_driftPos.m_hi = 0;
            m_driftThresh.m_lo = owner->m_damage;
            m_driftThresh.m_hi = 0;
            return 1;
        }
        rend = m_glitterSprite;
        if (rend != NULL) {
            rend->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            m_glitterSprite = NULL;
        }
        r = m_wwdObject;
        r->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        return 1;
    }
fail:
    return 0;
}

// @early-stop
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::Reposition() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    i64 delta = static_cast<i64>(g_frameTime) - m_driftPos.m_v;
    if (delta >= m_driftThresh.m_v) {
        CWwdSpriteObject* r = m_wwdObject;
        r->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        SET_ANIMATION_ACT("A");

        CGruntzMgr* reg = g_gameReg;
        CWwdSpriteObject* obj = m_object;
        i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
        i32 tileY = obj->m_screenY >> TILE_SHIFT_PX;
        CMapMgr* grid = reg->m_tileGrid;
        i32 cellVal;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            cellVal = grid->m_rowInts[tileY][tileX * 7 + 2];
        } else {
            cellVal = 0;
        }
        if (cellVal != 0) {

            CGameObject* found = NULL;
            if (MapLookupById(
                    reg->m_world->m_childGroup->m_registeredGameObjectsById,
                    cellVal,
                    found
                )
                && found != NULL) {
                found->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
            }
        }
        reg = g_gameReg;
        grid = reg->m_tileGrid;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[tileY][tileX * 7 + 2] = 0;
            grid->m_rowInts[tileY][tileX * 7] &= ~0x40000;
        }
        obj = m_object;
        grid = g_gameReg->m_tileGrid;
        i32 tileX2 = obj->m_screenX >> TILE_SHIFT_PX;
        i32 tileY2 = obj->m_screenY >> TILE_SHIFT_PX;
        i32 mv = obj->m_objectId;
        if (static_cast<u32>(tileX2) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY2) < static_cast<u32>(grid->m_height)) {
            grid->m_rowInts[tileY2][tileX2 * 7 + 2] = mv;
            if (mv != 0) {
                grid->m_rowInts[tileY2][tileX2 * 7] |= 0x40000;
            } else {
                grid->m_rowInts[tileY2][tileX2 * 7] &= ~0x40000;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x00098c90, 0x382)
i32 CInGameIcon::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* obj
) {

    char name[SERIAL_NAME_LEN];

    if (ar == NULL) {
        return 0;
    }
    SERIALIZE_USER_LOGIC_OR_RETURN(ar, mode, typeId, obj)

    switch (mode) {
        case SERIAL_LOAD: {
            char aniName[SERIAL_NAME_LEN];
            ar->Read(aniName, SERIAL_NAME_LEN);
            ar->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdSpriteObject*>(obj);
            m_ownerLogicRecord = obj->m_logicRecord;
            if (strlen(aniName) == 0) {
                m_value = NULL;
            } else {
                m_value = LookupAni(
                    m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations,
                    aniName
                );
            }
            break;
        }
        case SERIAL_SAVE: {
            memset(name, 0, sizeof(name));
            if (m_value != NULL) {
                strcpy(
                    name,
                    static_cast<const char*>(
                        m_ownerLogicRecord->m_ownerCtx->m_animRegistry->FindAnimationKey(m_value)
                    )
                );
            }
            ar->Write(name, SERIAL_NAME_LEN);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    Clock64* drift = &m_driftPos;
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(drift, sizeof(*drift));
            drift++;
            ar->Read(drift, sizeof(*drift));
            break;
        case SERIAL_SAVE:
            ar->Write(drift, sizeof(*drift));
            drift++;
            ar->Write(drift, sizeof(*drift));
            break;
    }
    Clock64* idle = &m_peekTimer;
    switch (mode) {
        case SERIAL_LOAD:
            ar->Read(idle, sizeof(*idle));
            idle++;
            ar->Read(idle, sizeof(*idle));
            break;
        case SERIAL_SAVE:
            ar->Write(idle, sizeof(*idle));
            idle++;
            ar->Write(idle, sizeof(*idle));
            break;
    }

    switch (mode) {
        case SERIAL_SAVE: {
            memset(name, 0, sizeof(name));
            if (m_cue != NULL) {
                strcpy(
                    name,
                    static_cast<const char*>(
                        m_ownerLogicRecord->m_ownerCtx->m_soundRegistry->FindCueKey(m_cue)
                    )
                );
            }
            ar->Write(name, SERIAL_NAME_LEN);
            g_serialCounter++;
            i32 id = 0;
            if (m_glitterSprite != NULL) {
                id = m_glitterSprite->m_objectId;
            }
            ar->Write(&id, sizeof(id));
            break;
        }
        case SERIAL_LOAD: {
            ar->Read(name, SERIAL_NAME_LEN);

            if (strlen(name) != 0) {
                m_cue = LookupCue(m_ownerLogicRecord->m_ownerCtx->m_soundRegistry->m_cues, name);
            } else {
                m_cue = NULL;
            }
            g_serialCounter++;
            i32 id;
            ar->Read(&id, sizeof(id));
            CWwdSpriteObject* sprite = LookupSerialRef(
                m_ownerLogicRecord->m_ownerCtx->m_childGroup->m_registeredGameObjectsById,
                id
            );
            m_glitterSprite = sprite;
            if (sprite != NULL) {
                break;
            }

            if (id != 0) {
                return 0;
            }
            break;
        }
        case SERIAL_POSTLOAD:
            if (HandleInput() == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// @early-stop
RVA(0x00099110, 0x215)
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    if (g_gameReg->m_gameMode == GAMEMODE_MULTIPLAYER) {
        SetObjectFlags(0x10000);
        return;
    }
    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SetImageSetByName("GAME_HELPBOX");
    SetObjectFlags(2);

    InGameTextVisibility vis = static_cast<InGameTextVisibility>(m_object->m_health);
    if (vis == INGAME_TEXT_EASY_ONLY) {

        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            SetObjectFlags(0x10000);
            return;
        }
    } else if (vis == INGAME_TEXT_NORMAL_ONLY) {
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_QUESTZ) {
            SetObjectFlags(0x10000);
            return;
        }
    }

    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_INGAME_INFO)
    m_cachedPlayerIndex = -1;
    m_cachedUnitIndex = -1;
}

RVA(0x00099460, 0x102)
void CInGameText::FireActivation(i32 idx) {
    if (*CActRegPool<CInGameText>::s_table.ResolveEntry(idx) != NULL) {
        CActHandler fn = *CActRegPool<CInGameText>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

RVA(0x000995c0, 0x18d)
void RegisterTextLogic() {
    ACT_NAME_ID(idx, "A")
    CActHandler* dslot = CActRegPool<CInGameText>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameText::Update);
}

// @early-stop
RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    m_wwdObject->m_animationCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    i32 playerIndex;
    i32 unitIndex;
    CGrunt* found =
        g_gameReg->m_triggerMgr
            ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &playerIndex, &unitIndex, 1);

    if (found != NULL) {
        if (playerIndex != g_curPlayer) {
            return 0;
        }
        if (m_cachedUnitIndex != -1 && playerIndex == m_cachedPlayerIndex
            && unitIndex == m_cachedUnitIndex) {
            return 0;
        }

        CString* node = g_typeColl.ScratchResolve(found->m_logicRecord->EventCode());

        CString* p = g_typeColl.Slots();
        i32 n = g_typeColl.m_grown;
        while (n-- != 0) {
            if (p != NULL) {
                p->CString::CString();
            }
            p++;
        }
        bool eq = (strcmp(*node, "K") == 0);
        if (eq) {
            return 0;
        }

        if (!found->LoadPickupSprites(PICKUP_HELPBOX, 0, m_object->m_smarts, 0, 1)) {
            return 0;
        }

        CWwdSpriteObject* o = m_object;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        CGruntzMgr* reg = g_gameReg;
        if (CGameLevel::PointInRect(&reg->m_viewBounds, x, y)) {
            SoundCueRegistry* set = reg->m_world->m_soundRegistry;
            if (set->m_silentMode == 0) {
                SoundCue* res = LookupCue(set->m_cues, "GAME_HELPBOOK");
                if (res != NULL) {
                    PlaySoundCueIfElapsed(res, g_soundVolumePercent, 0, 0, 0);
                }
            }
        }

        m_cachedPlayerIndex = playerIndex;
        m_cachedUnitIndex = unitIndex;
        Hide();
        return 0;
    }
    m_cachedUnitIndex = -1;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 0;
}

RVA(0x00099a30, 0xaa)
i32 CInGameText::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_cachedPlayerIndex, sizeof(m_cachedPlayerIndex));
            ar->Write(&m_cachedUnitIndex, sizeof(m_cachedUnitIndex));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_cachedPlayerIndex, sizeof(m_cachedPlayerIndex));
            ar->Read(&m_cachedUnitIndex, sizeof(m_cachedUnitIndex));
            break;
    }
    return 1;
}

RVA(0x00099b10, 0x36)
void CInGameIcon::SetupSprite(const char* category) {
    SoundCue* found = NULL;
    if (category != NULL) {
        found = NULL;
        MapLookup(g_gameReg->m_world->m_soundRegistry->m_cues, category, found);
    }
    m_cue = found;
}
