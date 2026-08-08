#include <rva.h>

#include <Gruntz/InGameIcon.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
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
#include <Gruntz/LeafCue.h>
#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/SpellId.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SpriteStateFlags.h>
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

template<> DATA(0x002458b0)
CActReg CActRegPool<CInGameIcon>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x00245928)
CActReg CActRegPool<CToyPeek>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
template<> DATA(0x00245950)
CActReg CActRegPool<CInGameText>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00011c10, 0x1e, ??_GCToyPeek@@UAEPAXI@Z)

RVA_COMPGEN(0x00011c40, 0x44, ??1CToyPeek@@UAE@XZ)

RVA_COMPGEN(0x00011cd0, 0x1e, ??_GCInGameIcon@@UAEPAXI@Z)
RVA_COMPGEN(0x00011d00, 0x44, ??1CInGameIcon@@UAE@XZ)

RVA_COMPGEN(0x00011d90, 0x1e, ??_GCInGameText@@UAEPAXI@Z)
RVA_COMPGEN(0x00011dc0, 0x44, ??1CInGameText@@UAE@XZ)

// @early-stop
// Frame is 4 B larger than retail's and one callee-saved register short, so every
// inlined strcmp takes the 3-register form and cl cross-jumps the SetupSprite tails
// (docs/patterns/inline-strcmp-form-reads-out-a-spilled-register.md).
RVA(0x00095b10, 0x1720)
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {

    m_driftPos.m_lo = 0;
    m_driftThresh.m_lo = 0;
    m_driftPos.m_hi = 0;
    m_driftThresh.m_hi = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    obj->m_screenX = (obj->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    obj->m_screenY = (obj->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

    if (obj->m_sortKey != SORTKEY_INGAME_INFO) {
        obj->m_sortKey = SORTKEY_INGAME_INFO;
        obj->m_flags |= 0x20000;
    }

    AnimWorkerObj* aux = m_objAux;
    m_prevAnimSetNode = aux->m_actKey;
    aux->m_actKey = ActFindId("A");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);

    m_wwdObject->m_flags |= 2;
    SetupSprite(0);

    m_glitterSprite = NULL;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    InGameIconGlitter glitter = ICON_GLITTER_NONE;
    CDDrawWorker* frameSet = static_cast<CWwdGameObjectA*>(obj)->m_frameSet;
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
            lvl->m_anchors[0].m_x = m_object->m_screenX;
            lvl->m_anchors[0].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_SECOND);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[1].m_x = m_object->m_screenX;
            lvl->m_anchors[1].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_THIRD);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[2].m_x = m_object->m_screenX;
            lvl->m_anchors[2].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_smarts = IDX(PICKUP_WARPSTONE);
            m_object->m_health = IDX(WARPSTONE_FRAGMENT_FOURTH);
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[3].m_x = m_object->m_screenX;
            lvl->m_anchors[3].m_y = m_object->m_screenY;
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
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_smarts = IDX(PICKUP_W);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETA") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_smarts = IDX(PICKUP_A);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETR") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_smarts = IDX(PICKUP_R);
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETP") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
                m_wwdObject->m_flags |= 0x10000;
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
    if (pickup == PICKUP_WARPSTONE && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
        CString levelStr;
        levelStr.Format("Level%i", lvl->m_levelIndex);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        warpName.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->ApplyName(warpName);
        m_object->m_health = target;
    }

    if (glitter != ICON_GLITTER_NONE) {
        CWwdGameObjectA* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            m_object->m_screenX,
            m_object->m_screenY,
            SORTKEY_INGAME_INFO_FX,
            "SimpleAnimation",
            0x40003
        );
        m_glitterSprite = fx;
        if (glitter == ICON_GLITTER_POWERUP_RED) {
            fx->ApplyName("GAME_GLITTERRED");
        }
        if (glitter == ICON_GLITTER_CURSE_GREEN) {
            m_glitterSprite->ApplyName("GAME_GLITTERGREEN");
        }
        m_glitterSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }

    if (HandleInput() == 0) {
        m_wwdObject->m_flags |= 0x10000;
        return;
    }

    i32 mv = m_object->m_objectId;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 col = m_object->m_screenX >> TILE_SHIFT_PX;
    i32 row = m_object->m_screenY >> TILE_SHIFT_PX;
    if (static_cast<u32>(col) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(row) < static_cast<u32>(grid->m_height)) {
        i32* cell = &grid->m_rowInts[row][col * 7];
        cell[2] = mv;
        i32* cell0 = &grid->m_rowInts[row][col * 7];
        if (mv != 0) {
            cell0[0] |= 0x40000;
        } else {
            cell0[0] &= ~0x40000;
        }
    }
    m_object->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
}

RVA(0x00097680, 0x110)
i32 CInGameIcon::HandleInput() {
    CWwdGameObjectA* obj = m_object;
    PickupType cmd = static_cast<PickupType>(obj->m_smarts);
    CShadeTable* rec;
    if (cmd == PICKUP_TOYBOX) {
        i32 key = obj->m_score;
        PickupType sub = static_cast<PickupType>(obj->m_points);
        if (sub < PICKUP_TOYZ_FIRST || sub > PICKUP_TOYZ_LAST) {
            return 0;
        }
        i32 icon = IDX(g_gameReg->m_options[key].m_colorIndex);
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
    CWwdGameObjectA* o = m_object;
    o->m_drawActive = 1;
    o->m_drawFillCmd = SHADE_PAL_16;
    o->m_drawFillArg = rec;
    return 1;
}

RVA(0x00097880, 0x102)
void CInGameIcon::FireActivation(i32 id) {
    if (*CActRegPool<CInGameIcon>::s_table.ResolveEntry(id) != 0) {
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
    if (*CActRegPool<CToyPeek>::s_table.ResolveEntry(id) != 0) {
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
// Scheduling: cl issues the m_object load and the final vptr stamp ahead of the
// m_startClock/m_countdown zero stores; retail issues them after.
RVA(0x00098140, 0x18e)
CToyPeek::CToyPeek(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_startClock.m_v = 0;
    m_countdown.m_v = 0;
    m_object->m_screenY -= 0x18;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_HUD) {
        o->m_sortKey = SORTKEY_GRUNT_HUD;
        o->m_flags |= 0x20000;
    }
    m_wwdObject->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", m_object->m_smarts);
    m_countdown.m_v = 0x1388;
    m_startClock.m_v = static_cast<u32>(g_frameTime);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
}

// @interleaver SerializeMove - 152 B lone body at 0x983e0, between RefreshCell
// (ingameicon) and PeekCycle (ingameicon): a first-use placement.

RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CWwdGameObjectA* obj = m_object;
    i32 tileY = obj->m_screenX >> TILE_SHIFT_PX;
    i32 tileX = (obj->m_screenY + 0x18) >> TILE_SHIFT_PX;
    i64 delta = static_cast<i64>(g_frameTime) - m_driftPos.m_v;
    if (delta < m_driftThresh.m_v) {
        CMapMgr* grid = g_gameReg->m_tileGrid;
        i32 cell;
        if (static_cast<u32>(tileY) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileX) < static_cast<u32>(grid->m_height)) {
            BrickzCell* row = grid->m_rows[tileX];
            cell = row[tileY].m_objectId;
        } else {
            cell = 0;
        }
        if (cell != 0) {
            return 0;
        }
    }
    CWwdGameObjectA* r = m_wwdObject;
    r->m_flags |= 0x10000;
    return 0;
}

RVA(0x000983e0, 0x98)
i32 CToyPeek::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }

    SerBandPair(ar, mode, &m_startClock);
    return 1;
}

RVA(0x000984b0, 0x186)
i32 CInGameIcon::PeekCycle() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CWwdGameObjectA* obj = m_object;
    PickupType cmd = static_cast<PickupType>(obj->m_smarts);
    if (cmd == PICKUP_TOYBOX) {
        CGruntzMgr* reg = g_gameReg;
        i32 tileY = obj->m_screenY >> TILE_SHIFT_PX;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tileX = obj->m_screenX >> TILE_SHIFT_PX;
        i32 cell;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            cell = grid->m_rows[tileY][tileX].m_flags;
        } else {
            cell = 1;
        }
        if ((cell & BRICKZ_BLOCKED_MASK) != 0 || (cell & 2) != 0) {
            if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
                BrickzCell* row0 = grid->m_rows[tileY];
                row0[tileX].m_objectId = 0;
                BrickzCell* row1 = grid->m_rows[tileY];
                row1[tileX].m_flags &= ~0x40000;
            }
            m_wwdObject->m_flags |= 0x10000;
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
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = SHADE_PAL_16;
        o->m_drawFillArg = rec;
        m_peekWindow.m_lo = 0xfa;
        m_peekWindow.m_hi = 0;
        m_peekTimer.m_lo = g_frameTime;
        m_peekTimer.m_hi = 0;
    }
    return 0;
}

static inline void ClearTileBit(CGruntzMgr* reg, CGameObject* owner) {
    CMapMgr* grid = reg->m_tileGrid;
    i32 tileX = owner->m_screenY >> TILE_SHIFT_PX;
    i32 tileY = owner->m_screenX >> TILE_SHIFT_PX;
    if (static_cast<u32>(tileY) < static_cast<u32>(grid->m_width)
        && static_cast<u32>(tileX) < static_cast<u32>(grid->m_height)) {

        i32 cellInt = tileY * 8 - tileY;
        i32* cell0 = grid->m_rowInts[tileX];
        cell0[cellInt + 2] = 0;
        i32* cell1 = grid->m_rowInts[tileX];
        cell1[cellInt] &= ~0x40000;
    }
}

// @early-stop
RVA(0x000986b0, 0x30c)

i32 CInGameIcon::PlaceAt(i32 tileOwnerHi, i32 tileOwnerLo) {
    CGruntzMgr* reg = g_gameReg;
    PickupType pickup = static_cast<PickupType>(m_object->m_smarts);
    if (reg->m_gameMode == GAMEMODE_SINGLE && tileOwnerHi != g_curPlayer
        && pickup != PICKUP_TOYBOX) {
        return 0;
    }
    CWwdGameObjectA* obj = m_object;
    if (pickup == PICKUP_TOYBOX) {

        i32 param = obj->m_points;
        i32 matchActive = 0;
        i32 flag = 1;
        if (obj->m_score == tileOwnerHi) {
            matchActive = 1;
            flag = 0;
        }
        i32 sub = obj->m_faceDirection;
        i32 idx = tileOwnerHi * 15 + tileOwnerLo;
        CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
        i32 ok;
        if (cell == NULL || cell->m_entranceCommitted == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = cell->LoadPickupSprites(static_cast<PickupType>(param), flag, 0, sub, 0);
        } else {
            ok = cell->LoadGruntTypeTable(static_cast<PickupType>(param), flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            return 0;
        }
        if (m_cue != NULL) {
            CWwdGameObjectA* o = m_object;
            if (CGameLevel::PointInRect(&reg->m_viewBounds, o->m_screenX, o->m_screenY)) {

                m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        CWwdGameObjectA* r = m_wwdObject;
        r->m_flags |= 0x10000;
        return 1;
    }

    i32 sub = obj->m_faceDirection;
    PickupType cmd = static_cast<PickupType>(obj->m_smarts);
    i32 idx = tileOwnerHi * 15 + tileOwnerLo;
    CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
    i32 ok;
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == PICKUP_WARPSTONE) {
        CGrunt* placed = reg->m_cmdGrid->m_grid[idx];
        if (placed != NULL) {
            placed->m_warpstoneAnchorIndex = m_object->m_health;
            reg = g_gameReg;
        }
    }
    if (m_cue != NULL) {
        CWwdGameObjectA* o = m_object;
        if (CGameLevel::PointInRect(&reg->m_viewBounds, o->m_screenX, o->m_screenY)) {

            m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_object);
    CWwdGameObjectA* owner = m_wwdObject;
    if (owner->m_damage > 0) {
        owner->m_stateFlags |= SPRITE_STATE_HIDDEN;
        AnimWorkerObj* aux = m_objAux;
        m_prevAnimSetNode = aux->m_actKey;
        aux->m_actKey = ActFindId("B");
        owner = m_wwdObject;
        m_driftPos.m_lo = owner->m_damage;
        m_driftPos.m_hi = 0;
        m_driftThresh.m_lo = g_frameTime;
        m_driftThresh.m_hi = 0;
        return 1;
    }
    CWwdGameObjectA* rend = m_glitterSprite;
    if (rend != NULL) {
        rend->m_flags |= 0x10000;
        m_glitterSprite = NULL;
    }
    CWwdGameObjectA* r = m_wwdObject;
    r->m_flags |= 0x10000;
    return 1;
}

// @early-stop
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::Reposition() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    i64 delta = static_cast<i64>(g_frameTime) - m_driftPos.m_v;
    if (delta >= m_driftThresh.m_v) {
        CWwdGameObjectA* r = m_wwdObject;
        r->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("A");

        CGruntzMgr* reg = g_gameReg;
        CWwdGameObjectA* obj = m_object;
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

            void* found = 0;
            if (MapLookupById(reg->m_world->m_childGroup->m_map48, cellVal, found)
                && found != NULL) {
                (static_cast<CGameObject*>(found))->m_flags |= 0x10000;
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
i32 CInGameIcon::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* obj
) {

    char chainName[SERIAL_NAME_LEN];

    if (ar == NULL) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, mode, typeId, obj) == 0) {
        return 0;
    }

    switch (mode) {
        case SERIAL_LOAD: {
            ar->Read(chainName, SERIAL_NAME_LEN);
            ar->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_animWorker = obj->m_animWorker;
            if (strlen(chainName) == 0) {
                m_value = NULL;
            } else {
                void* val = 0;
                m_animWorker->m_ownerCtx->m_animRegistry->m_animations.Lookup(chainName, val);
                m_value = static_cast<CAniElement*>(val);
            }
            break;
        }
        case SERIAL_SAVE: {
            memset(chainName, 0, sizeof(chainName));
            if (m_value != NULL) {
                strcpy(
                    chainName,
                    static_cast<const char*>(
                        m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value)
                    )
                );
            }
            ar->Write(chainName, SERIAL_NAME_LEN);
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

    char tailName[SERIAL_NAME_LEN];
    switch (mode) {
        case SERIAL_SAVE: {
            memset(tailName, 0, sizeof(tailName));
            if (m_cue != NULL) {
                strcpy(
                    tailName,
                    static_cast<const char*>(
                        m_animWorker->m_ownerCtx->m_soundRegistry->FindKeyOfValue(m_cue)
                    )
                );
            }
            ar->Write(tailName, SERIAL_NAME_LEN);
            g_serialCounter++;
            i32 id = 0;
            if (m_glitterSprite != NULL) {
                id = m_glitterSprite->m_objectId;
            }
            ar->Write(&id, sizeof(id));
            break;
        }
        case SERIAL_LOAD: {
            ar->Read(tailName, SERIAL_NAME_LEN);

            if (strlen(tailName) == 0) {
                m_cue = NULL;
            } else {
                void* val = 0;
                m_animWorker->m_ownerCtx->m_soundRegistry->m_cues.Lookup(tailName, val);
                m_cue = static_cast<LeafCue*>(val);
            }
            g_serialCounter++;
            i32 id = 0;
            ar->Read(&id, sizeof(id));
            void* found = 0;
            CWwdGameObjectA* sprite = 0;
            if (MapLookupById(m_animWorker->m_ownerCtx->m_childGroup->m_map48, id, found) != 0
                && found != NULL
                && static_cast<CGameObject*>(found)->GetClassId() == CLASSID_SERIALREF) {
                sprite = static_cast<CWwdGameObjectA*>(found);
            }
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
        m_wwdObject->m_flags |= 0x10000;
        return;
    }
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_wwdObject->ApplyName("GAME_HELPBOX");
    m_wwdObject->m_flags |= 2;

    InGameTextVisibility vis = static_cast<InGameTextVisibility>(m_object->m_health);
    if (vis == INGAME_TEXT_EASY_ONLY) {

        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
            m_wwdObject->m_flags |= 0x10000;
            return;
        }
    } else if (vis == INGAME_TEXT_NORMAL_ONLY) {
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
            m_wwdObject->m_flags |= 0x10000;
            return;
        }
    }

    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_INGAME_INFO) {
        o->m_sortKey = SORTKEY_INGAME_INFO;
        o->m_flags |= 0x20000;
    }
    m_cachedAreaId = -1;
    m_cachedSubId = -1;
}

RVA(0x00099460, 0x102)
void CInGameText::FireActivation(i32 idx) {
    if (*CActRegPool<CInGameText>::s_table.ResolveEntry(idx) != 0) {
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

RVA(0x000997c0, 0x1e7)
i32 CInGameText::Update() {
    m_wwdObject->m_animCursor.Advance(static_cast<i32>(g_engineFrameDelta));

    i32 areaId;
    i32 subId;
    CGrunt* found = g_gameReg->m_cmdGrid
                        ->HitTestCell(m_object->m_screenX, m_object->m_screenY, &areaId, &subId, 1);

    if (found != NULL) {
        if (areaId != g_curPlayer) {
            return 0;
        }
        if (m_cachedSubId != -1 && areaId == m_cachedAreaId && subId == m_cachedSubId) {
            return 0;
        }

        CString* node = g_typeColl.ScratchResolve(found->m_objAux->ActKey());

        CString* p = g_typeColl.Slots();
        i32 n = g_typeColl.m_grown;
        while (n-- != 0) {
            if (p != NULL) {
                p->CString::CString();
            }
            p++;
        }
        bool eq = (strcmp(*node, s_codeK) == 0);
        if (eq) {
            return 0;
        }

        if (!found->LoadPickupSprites(PICKUP_HELPBOX, 0, m_object->m_smarts, 0, 1)) {
            return 0;
        }

        CWwdGameObjectA* o = m_object;
        i32 y = o->m_screenY;
        i32 x = o->m_screenX;
        CGruntzMgr* reg = g_gameReg;
        if (CGameLevel::PointInRect(&reg->m_viewBounds, x, y)) {
            CDDrawSubMgrLeafScan* set = reg->m_world->m_soundRegistry;
            if (set->m_emitGate == 0) {
                void* res_ob = 0;
                set->m_cues.Lookup("GAME_HELPBOOK", res_ob);
                LeafCue* res = static_cast<LeafCue*>(res_ob);
                if (res != NULL) {
                    i32 enable = g_sndEnabled;
                    i32 token = g_sndCueTag;
                    if (enable != 0) {
                        u32 now = g_killCueClock;
                        if (static_cast<u32>((now - res->m_lastPlayTime))
                            >= static_cast<u32>(res->m_replayDelay)) {
                            res->m_lastPlayTime = now;
                            res->m_sound->ConfigureItem(token, 0, 0, 0);
                        }
                    }
                }
            }
        }

        m_cachedAreaId = areaId;
        m_cachedSubId = subId;
        m_wwdObject->m_stateFlags |= SPRITE_STATE_HIDDEN;
        return 0;
    }
    m_cachedSubId = -1;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 0;
}

RVA(0x00099a30, 0xaa)
i32 CInGameText::SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId a, CGameObject* b) {
    if (ar == NULL) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, tag, a, b) == 0) {
        return 0;
    }
    if (Chain(ar, tag, a, b) == 0) {
        return 0;
    }
    switch (tag) {
        case SERIAL_SAVE:
            ar->Write(&m_cachedAreaId, sizeof(m_cachedAreaId));
            ar->Write(&m_cachedSubId, sizeof(m_cachedSubId));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_cachedAreaId, sizeof(m_cachedAreaId));
            ar->Read(&m_cachedSubId, sizeof(m_cachedSubId));
            break;
    }
    return 1;
}

RVA(0x00099b10, 0x36)
void CInGameIcon::SetupSprite(const char* category) {
    void* found = 0;
    if (category != NULL) {
        found = NULL;
        g_gameReg->m_world->m_soundRegistry->m_cues.Lookup(category, found);
    }
    m_cue = static_cast<LeafCue*>(found);
}
DATA(0x0020d7f8)
char s_codeK[] = "K";

// @early-stop
