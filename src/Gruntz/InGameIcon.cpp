#include <Mfc.h>
#include <Wap32/zBitVec.h>
#include <Io/FileMem.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/Play.h>

#include <rva.h>

#include <string.h>
#include <Bute/ButeMgr.h>
#include <Wap32/ZVec.h>
#include <Gruntz/LogicFnTable.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/AniElement.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Random.h>
#include <Utils/MapTyped.h>

VTBL(CInGameText, 0x001e7cac);
VTBL(CInGameIcon, 0x001e7d04);
template<> DATA(0x002458b0)
CActReg CActRegPool<CInGameIcon>::s_table(2000, 2010);
template<> DATA(0x00245928)
CActReg CActRegPool<CToyPeek>::s_table(2000, 2010);
template<> DATA(0x00245950)
CActReg CActRegPool<CInGameText>::s_table(2000, 2010);

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(v, msg, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

static inline CString* ResolveNameSlotCallReport(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        v->Report(g_errOutOfMem, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

RVA_COMPGEN(0x00011cd0, 0x1e, ??_GCInGameIcon@@UAEPAXI@Z)
RVA_COMPGEN(0x00011d00, 0x44, ??1CInGameIcon@@UAE@XZ)

RVA_COMPGEN(0x00011d90, 0x1e, ??_GCInGameText@@UAEPAXI@Z)
RVA_COMPGEN(0x00011dc0, 0x44, ??1CInGameText@@UAE@XZ)

// @early-stop
RVA(0x00095b10, 0x15f0)
CInGameIcon::CInGameIcon(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {

    m_driftPos.m_lo = 0;
    m_driftThresh.m_lo = 0;
    m_driftPos.m_hi = 0;
    m_driftThresh.m_hi = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    obj->m_screenX = (obj->m_screenX & ~0x1f) + 0x10;
    obj->m_screenY = (obj->m_screenY & ~0x1f) + 0x10;

    if (obj->m_sortKey != 0x17318) {
        obj->m_sortKey = 0x17318;
        obj->m_flags |= 0x20000;
    }

    AnimWorkerObj* aux = m_objAux;
    m_prevAnimSetNode = aux->m_1c;
    aux->m_1c = ActFindId("A");
    m_value = m_wwdObject->m_1a0.m_14;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);

    m_wwdObject->m_flags |= 2;
    SetupSprite(0);

    m_glitterSprite = 0;
    m_peekTimer.m_lo = 0;
    m_peekWindow.m_lo = 0;
    m_peekTimer.m_hi = 0;
    m_peekWindow.m_hi = 0;

    i32 glitter = 0;
    char* rec = static_cast<CWwdGameObjectA*>(obj)->m_194;
    if (rec != 0) {
        CString name;
        name = rec + 0x24;

        if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOMBZ") == 0) {
            m_object->m_124 = 1;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BOOMERANGZ") == 0) {
            m_object->m_124 = 2;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_BRICKZ") == 0) {
            m_object->m_124 = 3;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_CLUBZ") == 0) {
            m_object->m_124 = 4;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GAUNTLETZ") == 0) {
            m_object->m_124 = 5;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GLOVEZ") == 0) {
            m_object->m_124 = 6;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GOOBERZ") == 0) {
            m_object->m_124 = 7;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GRAVITYBOOTZ") == 0) {
            m_object->m_124 = 8;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_GUNHATZ") == 0) {
            m_object->m_124 = 9;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_NERFGUNZ") == 0) {
            m_object->m_124 = 0xa;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_ROCKZ") == 0) {
            m_object->m_124 = 0xb;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHIELDZ") == 0) {
            m_object->m_124 = 0xc;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SHOVELZ") == 0) {
            m_object->m_124 = 0xd;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPRINGZ") == 0) {
            m_object->m_124 = 0xe;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SPYZ") == 0) {
            m_object->m_124 = 0xf;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_SWORDZ") == 0) {
            m_object->m_124 = 0x10;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TIMEBOMBZ") == 0) {
            m_object->m_124 = 0x11;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_TOOBZ") == 0) {
            m_object->m_124 = 0x12;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WANDZ") == 0) {
            m_object->m_124 = 0x13;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ1") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 1;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[0].m_x = m_object->m_screenX;
            lvl->m_anchors[0].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ2") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 2;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[1].m_x = m_object->m_screenX;
            lvl->m_anchors[1].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ3") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 3;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[2].m_x = m_object->m_screenX;
            lvl->m_anchors[2].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ4") == 0) {
            m_object->m_124 = 0x14;
            m_object->m_placeMode = 4;
            CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
            lvl->m_anchors[3].m_x = m_object->m_screenX;
            lvl->m_anchors[3].m_y = m_object->m_screenY;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WELDERZ") == 0) {
            m_object->m_124 = 0x15;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOOLZ_WINGZ") == 0) {
            m_object->m_124 = 0x16;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BABYWALKERZ") == 0) {
            m_object->m_124 = 0x17;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BEACHBALLZ") == 0) {
            m_object->m_124 = 0x18;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_BIGWHEELZ") == 0) {
            m_object->m_124 = 0x19;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_GOKARTZ") == 0) {
            m_object->m_124 = 0x1a;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JACKINTHEBOXZ") == 0) {
            m_object->m_124 = 0x1b;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_JUMPROPEZ") == 0) {
            m_object->m_124 = 0x1c;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_POGOSTICKZ") == 0) {
            m_object->m_124 = 0x1d;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SCROLLZ") == 0) {
            m_object->m_124 = 0x1e;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_SQUEAKTOYZ") == 0) {
            m_object->m_124 = 0x1f;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_TOYZ_YOYOZ") == 0) {
            m_object->m_124 = 0x20;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MEGAPHONEZ") == 0) {
            m_object->m_124 = 0x32;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH1") == 0) {
            m_object->m_124 = 0x33;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH2") == 0) {
            m_object->m_124 = 0x34;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_HEALTH3") == 0) {
            m_object->m_124 = 0x35;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_CONVERSION") == 0) {
            m_object->m_124 = 0x39;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_DEATHTOUCH") == 0) {
            m_object->m_124 = 0x3a;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_GHOST") == 0) {
            m_object->m_124 = 0x36;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_INVULNERABILITY") == 0) {
            m_object->m_124 = 0x38;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_REACTIVEARMOR") == 0) {
            m_object->m_124 = 0x3c;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_ROIDZ") == 0) {
            m_object->m_124 = 0x3b;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SUPERSPEED") == 0) {
            m_object->m_124 = 0x37;
            SetupSprite("GAME_POWERUP");
            glitter = 2;
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETW") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5a;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETA") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5b;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETR") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5c;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_SECRETP") == 0) {
            if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
                m_wwdObject->m_flags |= 0x10000;
                return;
            }
            m_object->m_124 = 0x5d;
            SetupSprite("GAME_POWERUP");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_STOPWATCH") == 0) {
            m_object->m_124 = 0x4b;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_COIN") == 0) {
            m_object->m_124 = 0x50;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_TOYBOX") == 0) {
            m_object->m_124 = 0x55;
            SetupSprite("GAME_TREASURE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_MINICAM") == 0) {
            m_object->m_124 = 0x40;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_SCREENSHAKE") == 0) {
            m_object->m_124 = 0x3e;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_RANDOMCOLORZ") == 0) {
            m_object->m_124 = 0x3d;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        } else if (strcmp(name, "GAME_INGAMEICONZ_POWERUPZ_BLACKSCREEN") == 0) {
            m_object->m_124 = 0x3f;
            glitter = 1;
            SetupSprite("GAME_CURSE");
        }
    }

    if (m_object->m_124 == 0x14 && g_gameReg->m_134 == 1) {
        CPlay* lvl = static_cast<CPlay*>(g_gameReg->m_curState);
        CString levelStr;
        levelStr.Format("Level%i", lvl->m_levelIndex);
        CString warpName;
        i32 target = g_buteMgr.GetInt("WarpStone", levelStr);
        warpName.Format("GAME_INGAMEICONZ_TOOLZ_WARPSTONEZ%i", target);
        m_object->ApplyName(warpName);
        m_object->m_placeMode = target;
    }

    if (glitter != 0) {
        CWwdGameObjectA* fx = g_gameReg->m_world->m_childGroup->CreateSprite(
            0,
            m_object->m_screenX,
            m_object->m_screenY,
            0x17319,
            "SimpleAnimation",
            0x40003
        );
        m_glitterSprite = fx;
        if (glitter == 2) {
            fx->ApplyName("GAME_GLITTERRED");
        }
        if (glitter == 1) {
            m_glitterSprite->ApplyName("GAME_GLITTERGREEN");
        }
        m_glitterSprite->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }

    if (HandleInput() == 0) {
        m_wwdObject->m_flags |= 0x10000;
        return;
    }

    i32 mv = m_object->m_188;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    i32 col = m_object->m_screenX >> 5;
    i32 row = m_object->m_screenY >> 5;
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
    m_object->m_stateFlags &= ~1;
}

RVA(0x00097680, 0x110)
i32 CInGameIcon::HandleInput() {
    CWwdGameObjectA* obj = m_object;
    i32 cmd = obj->m_124;
    CShadeTable* rec;
    if (cmd == 0x55) {
        i32 key = obj->m_114;
        i32 sub = obj->m_118;
        if (sub < 0x17 || sub > 0x20) {
            return 0;
        }
        i32 icon = g_gameReg->m_options[key].m_008;
        if (icon < 0 || icon >= 0x11) {
            icon = 0;
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
    } else if (cmd == 0x1e || cmd == 0x13) {
        i32 icon;
        switch (obj->m_130) {
            case 1:
                icon = 0x10;
                break;
            case 2:
                icon = 1;
                break;
            case 3:
                icon = 0;
                break;
            case 4:
                icon = 0xc;
                break;
            case 5:
                icon = 2;
                break;
            case 6:
                icon = 3;
                break;
            default:
                icon = 7;
                break;
        }
        rec = g_gameReg->m_spriteFactory->GetSel(icon, 0);
        if (rec == 0) {
            rec = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
    } else {
        return 1;
    }
    CWwdGameObjectA* o = m_object;
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0xa;
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
    i32 idxA = ActFindId("A");
    if (idxA == 0) {
        ActInsertId("A", g_typeCounter);
        idxA = g_typeCounter;
        CString* slot = ResolveNameSlotCallReport(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslotA = CActRegPool<CInGameIcon>::s_table.ResolveEntryCallReport(idxA);
    *dslotA = static_cast<CActHandler>(&CInGameIcon::PeekCycle);

    i32 idxB = ActFindId("B");
    if (idxB == 0) {
        ActInsertId("B", g_typeCounter);
        idxB = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "B";
        g_typeCounter++;
    }
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
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CToyPeek>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameIcon::RefreshCell);
}

RVA(0x00098340, 0x71)
i32 CInGameIcon::RefreshCell() {
    CWwdGameObjectA* obj = m_object;
    i32 tileY = obj->m_screenX >> 5;
    i32 tileX = (obj->m_screenY + 0x18) >> 5;
    i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_driftPos.m_v;
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

// @early-stop
RVA(0x000984b0, 0x186)
i32 CInGameIcon::PeekCycle() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* obj = m_object;
    i32 cmd = obj->m_124;
    if (cmd == 0x55) {
        CGruntzMgr* reg = g_gameReg;
        i32 tileY = obj->m_screenY >> 5;
        CMapMgr* grid = reg->m_tileGrid;
        i32 tileX = obj->m_screenX >> 5;
        i32 cell;
        if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
            && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
            cell = grid->m_rows[tileY][tileX].m_0;
        } else {
            cell = 1;
        }
        if ((cell & 0x939) != 0 || (cell & 2) != 0) {
            if (static_cast<u32>(tileX) < static_cast<u32>(grid->m_width)
                && static_cast<u32>(tileY) < static_cast<u32>(grid->m_height)) {
                BrickzCell* row0 = grid->m_rows[tileY];
                row0[tileX].m_objectId = 0;
                BrickzCell* row1 = grid->m_rows[tileY];
                row1[tileX].m_0 &= ~0x40000;
            }
            m_wwdObject->m_flags |= 0x10000;
        }
        return 0;
    }
    if (cmd != 0x13 && cmd != 0x1e) {
        return 0;
    }
    if (obj->m_130 != 0) {
        return 0;
    }
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_peekTimer.m_v >= m_peekWindow.m_v) {
        u32 x;
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            x = ::timeGetTime();
        } else {
            x = g_randSeed;
        }
        g_randSeed = x * 214013 + 2531011;
        CShadeTable* rec = g_gameReg->m_spriteFactory->GetSel(
            ((static_cast<i32>(g_randSeed) >> 16) & 0x7fff) % 0x11,
            0
        );
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 0xa;
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
    i32 tileX = owner->m_screenY >> 5;
    i32 tileY = owner->m_screenX >> 5;
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
    if (reg->m_134 == 1 && tileOwnerHi != g_curPlayer && m_object->m_124 != 0x55) {
        return 0;
    }
    CWwdGameObjectA* obj = m_object;
    if (obj->m_124 == 0x55) {

        i32 param = obj->m_118;
        i32 matchActive = 0;
        i32 flag = 1;
        if (obj->m_114 == tileOwnerHi) {
            matchActive = 1;
            flag = 0;
        }
        i32 sub = obj->m_130;
        i32 idx = tileOwnerHi * 15 + tileOwnerLo;
        CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
        i32 ok;
        if (cell == 0 || cell->m_entranceCommitted == 0) {
            ok = 0;
        } else if (matchActive) {
            ok = cell->LoadPickupSprites(param, flag, 0, sub, 0);
        } else {
            ok = cell->LoadGruntTypeTable(param, flag, sub, 0);
        }
        reg = g_gameReg;
        if (ok == 0) {
            return 0;
        }
        if (m_cue != 0) {
            CWwdGameObjectA* o = m_object;
            if (o->m_screenX < reg->m_viewBounds.right && o->m_screenX >= reg->m_viewBounds.left
                && o->m_screenY < reg->m_viewBounds.bottom
                && o->m_screenY >= reg->m_viewBounds.top) {

                m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                reg = g_gameReg;
            }
        }
        ClearTileBit(reg, m_object);
        CWwdGameObjectA* r = m_wwdObject;
        r->m_flags |= 0x10000;
        return 1;
    }

    i32 sub = obj->m_130;
    i32 cmd = obj->m_124;
    i32 idx = tileOwnerHi * 15 + tileOwnerLo;
    CGrunt* cell = reg->m_cmdGrid->m_grid[idx];
    i32 ok;
    if (cell == 0 || cell->m_entranceCommitted == 0) {
        ok = 0;
    } else {
        ok = cell->LoadPickupSprites(cmd, 0, 0, sub, 1);
    }
    reg = g_gameReg;
    if (ok == 0) {
        return 0;
    }
    if (cmd == 0x14) {
        CGrunt* placed = reg->m_cmdGrid->m_grid[idx];
        if (placed != 0) {
            placed->m_38c = m_object->m_placeMode;
            reg = g_gameReg;
        }
    }
    if (m_cue != 0) {
        CWwdGameObjectA* o = m_object;
        if (o->m_screenX < reg->m_viewBounds.right && o->m_screenX >= reg->m_viewBounds.left
            && o->m_screenY < reg->m_viewBounds.bottom && o->m_screenY >= reg->m_viewBounds.top) {

            m_cue->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            reg = g_gameReg;
        }
    }
    ClearTileBit(reg, m_object);
    CWwdGameObjectA* owner = m_wwdObject;
    if (owner->m_120 > 0) {
        owner->m_stateFlags |= 1;
        AnimWorkerObj* aux = m_objAux;
        m_prevAnimSetNode = aux->m_1c;
        aux->m_1c = ActFindId("B");
        owner = m_wwdObject;
        m_driftPos.m_lo = owner->m_120;
        m_driftPos.m_hi = 0;
        m_driftThresh.m_lo = g_frameTime;
        m_driftThresh.m_hi = 0;
        return 1;
    }
    CWwdGameObjectA* rend = m_glitterSprite;
    if (rend != 0) {
        rend->m_flags |= 0x10000;
        m_glitterSprite = 0;
    }
    CWwdGameObjectA* r = m_wwdObject;
    r->m_flags |= 0x10000;
    return 1;
}

// @early-stop
RVA(0x00098a90, 0x18d)
i32 CInGameIcon::Reposition() {
    m_wwdObject->m_1a0.Advance(g_engineFrameDelta);
    i64 delta = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_driftPos.m_v;
    if (delta >= m_driftThresh.m_v) {
        CWwdGameObjectA* r = m_wwdObject;
        r->m_stateFlags &= ~1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");

        CGruntzMgr* reg = g_gameReg;
        CWwdGameObjectA* obj = m_object;
        i32 tileX = obj->m_screenX >> 5;
        i32 tileY = obj->m_screenY >> 5;
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
            if (MapLookupById(reg->m_world->m_childGroup->m_map48, cellVal, found) && found != 0) {
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
        i32 tileX2 = obj->m_screenX >> 5;
        i32 tileY2 = obj->m_screenY >> 5;
        i32 mv = obj->m_188;
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
i32 CInGameIcon::SerializeMove(CFileMemBase* ar, i32 mode, i32 typeId, CGameObject* obj) {

    char chainName[0x80];

    if (ar == 0) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, mode, typeId, obj) == 0) {
        return 0;
    }

    switch (mode) {
        case 7: {
            ar->Read(chainName, 0x80);
            ar->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_animWorker = obj->m_animWorker;
            if (strlen(chainName) == 0) {
                m_value = 0;
            } else {
                void* val = 0;
                m_animWorker->m_ownerCtx->m_animRegistry->m_animations.Lookup(chainName, val);
                m_value = static_cast<CAniElement*>(val);
            }
            break;
        }
        case 4: {
            memset(chainName, 0, sizeof(chainName));
            if (m_value != 0) {
                CString nm = m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value);
                strcpy(chainName, static_cast<const char*>(nm));
            }
            ar->Write(chainName, 0x80);
            ar->Write(m_blob, 0x10);
            break;
        }
    }

    Clock64* drift = &m_driftPos;
    switch (mode) {
        case 7:
            ar->Read(drift, 8);
            drift++;
            ar->Read(drift, 8);
            break;
        case 4:
            ar->Write(drift, 8);
            drift++;
            ar->Write(drift, 8);
            break;
    }
    Clock64* idle = &m_peekTimer;
    switch (mode) {
        case 7:
            ar->Read(idle, 8);
            idle++;
            ar->Read(idle, 8);
            break;
        case 4:
            ar->Write(idle, 8);
            idle++;
            ar->Write(idle, 8);
            break;
    }

    char tailName[0x80];
    switch (mode) {
        case 4: {
            memset(tailName, 0, sizeof(tailName));
            if (m_cue != 0) {
                CString nm = m_animWorker->m_ownerCtx->m_soundRegistry->FindKeyOfValue(m_cue);
                strcpy(tailName, static_cast<const char*>(nm));
            }
            ar->Write(tailName, 0x80);
            g_serialCounter++;
            i32 id = 0;
            if (m_glitterSprite != 0) {
                id = m_glitterSprite->m_188;
            }
            ar->Write(&id, 4);
            break;
        }
        case 7: {
            ar->Read(tailName, 0x80);

            if (strlen(tailName) == 0) {
                m_cue = 0;
            } else {
                void* val = 0;
                m_animWorker->m_ownerCtx->m_soundRegistry->m_cues.Lookup(tailName, val);
                m_cue = static_cast<LeafCue*>(val);
            }
            g_serialCounter++;
            i32 id = 0;
            ar->Read(&id, 4);
            void* found = 0;
            CWwdGameObjectA* sprite = 0;
            if (MapLookupById(m_animWorker->m_ownerCtx->m_childGroup->m_map48, id, found) != 0
                && found != 0
                && static_cast<CGameObject*>(found)->GetClassId() == CLASSID_SERIALREF) {
                sprite = static_cast<CWwdGameObjectA*>(found);
            }
            m_glitterSprite = sprite;
            if (sprite != 0) {
                break;
            }

            if (id != 0) {
                return 0;
            }
            break;
        }
        case 8:
            if (HandleInput() == 0) {
                return 0;
            }
            break;
    }
    return 1;
}

// @early-stop
RVA(0x00099110, 0x215)
CInGameText::CInGameText(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    if (g_gameReg->m_134 == 2) {
        m_wwdObject->m_flags |= 0x10000;
        return;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_value = m_wwdObject->m_1a0.m_14;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_wwdObject->ApplyName("GAME_HELPBOX");
    m_wwdObject->m_flags |= 2;

    i32 vis = m_object->m_placeMode;
    if (vis == 1) {

        if (g_gameReg->m_isEasyMode == 0 || g_gameReg->m_134 != 1) {
            m_wwdObject->m_flags |= 0x10000;
            return;
        }
    } else if (vis == 2) {
        if (g_gameReg->m_isEasyMode != 0 && g_gameReg->m_134 == 1) {
            m_wwdObject->m_flags |= 0x10000;
            return;
        }
    }

    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    if (m_object->m_sortKey != 0x17318) {
        m_object->m_sortKey = 0x17318;
        m_object->m_flags |= 0x20000;
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
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CInGameText>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CInGameText::Update);
}

RVA(0x00099a30, 0xaa)
i32 CInGameText::SerializeMove(CFileMemBase* ar, i32 tag, i32 a, CGameObject* b) {
    if (ar == 0) {
        return 0;
    }
    if (CUserLogic::SerializeMove(ar, tag, a, b) == 0) {
        return 0;
    }
    if (Chain(ar, tag, a, b) == 0) {
        return 0;
    }
    switch (tag) {
        case 4:
            ar->Write(&m_cachedAreaId, 4);
            ar->Write(&m_cachedSubId, 4);
            break;
        case 7:
            ar->Read(&m_cachedAreaId, 4);
            ar->Read(&m_cachedSubId, 4);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x00099b10, 0x36)
void CInGameIcon::SetupSprite(const char* category) {
    LeafCue* cue = 0;
    if (category != 0) {

        CGruntzMgr* reg = g_gameReg;
        void* found = 0;
        reg->m_world->m_soundRegistry->m_cues.Lookup(category, found);
        cue = static_cast<LeafCue*>(found);
    }
    m_cue = cue;
}
