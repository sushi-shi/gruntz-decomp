#include <rva.h>

#include <Gruntz/GruntCreationPoint.h>

#include <AddrWord.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/GameModeId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteRefTable.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

VTBL(CGruntCreationPoint, 0x001e81d4);

RVA_COMPGEN(0x00010700, 0x1e, ??_GCGruntCreationPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010730, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

// @early-stop
RVA(0x0003e520, 0x1fd)
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
    if (m_object->m_sortKey != 5) {
        m_object->m_sortKey = 5;
        m_object->m_flags |= 0x20000;
    }
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);

    i32 key = m_object->m_smarts;
    i32 idx;
    if (g_gameReg->m_gameMode == GAMEMODE_SINGLE) {
        idx = key;
    } else if (g_gameReg->m_options[key].m_liveGate != 0) {
        idx = g_gameReg->m_options[key].m_colorIndex;
    } else {
        m_wwdObject->m_flags |= 0x10000;

        AddrWord<CGameObject> sel;
        sel.m_addr = obj;
        idx = sel.m_word;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);

    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_PAL_16;
    m_object->m_drawFillArg = sel;
    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
}

template<> DATA(0x00244700)
CActReg CActRegPool<CGruntCreationPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003e7a0, 0xd7)
i32 CGruntCreationPoint::SerializeMove(
    CFileMemBase* ar,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, d)) {
        return 0;
    }
    if (tag != SERIAL_SAVE && tag == SERIAL_POSTLOAD) {
        i32 idx;
        if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
            if (g_gameReg->m_options[m_object->m_smarts].m_liveGate != 0) {
                idx = g_gameReg->m_options[m_object->m_smarts].m_colorIndex;
            } else {
                idx = ChannelSlots_FindFree();
            }
        } else {
            idx = m_object->m_smarts;
        }
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        if (sel == NULL) {
            sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
        CWwdGameObjectA* obj = m_object;
        obj->m_drawActive = 1;
        obj->m_drawFillCmd = SHADE_PAL_16;
        obj->m_drawFillArg = sel;
    }
    return 1;
}

RVA(0x0003e960, 0x102)
void CGruntCreationPoint::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
    if (*e != 0) {
        CActHandler* e2 = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

RVA(0x0003eac0, 0x18d)
void CGruntCreationPoint::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != NULL) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    *(CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
