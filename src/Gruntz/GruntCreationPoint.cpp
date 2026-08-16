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
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_COMPGEN(0x00010700, 0x1e, ??_GCGruntCreationPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010730, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

// @early-stop
RVA(0x0003e520, 0x1fd)
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(2);
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_CREATION) {
        o->m_sortKey = SORTKEY_GRUNT_CREATION;
        i32 f = o->m_flags;
        f |= 0x20000;
        o->m_flags = f;
    }
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_CYCLE100", 0);

    i32 key = m_object->m_smarts;
    i32 idx;
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        if (g_gameReg->m_options[key].m_liveGate != 0) {
            idx = IDX(g_gameReg->m_options[key].m_colorIndex);
        } else {
            m_wwdObject->m_flags |= 0x10000;

            AddrWord<CGameObject> sel;
            sel.m_addr = obj;
            idx = sel.m_word;
        }
    } else {
        idx = key;
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

RVA_DYNINIT(0x0003e8c0, 0xa, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e8e0, 0x15, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e910, 0xe, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e930, 0x1f, CActRegPool<CGruntCreationPoint>::s_table)
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
                idx = IDX(g_gameReg->m_options[m_object->m_smarts].m_colorIndex);
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
    ACT_NAME_ID(id, "A")
    *(CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

RVA(0x0003ecc0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
