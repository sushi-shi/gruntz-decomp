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
#include <Gruntz/TileSnapMacros.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_COMPGEN(0x00010700, 0x1e, ??_GCGruntCreationPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010730, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

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
    SwitchGeometry("GAME_CYCLE100", 0);

    i32 idx = m_object->m_smarts;
    if (g_gameReg->m_gameMode != GAMEMODE_SINGLE) {
        if (g_gameReg->m_options[idx].m_liveGate != 0) {
            idx = IDX(g_gameReg->m_options[idx].m_colorIndex);
        } else {
            SetObjectFlags(0x10000);

            AddrWord<CGameObject> handle;
            handle.m_addr = obj;
            idx = handle.m_word;
        }
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);

    SET_DRAW_FILL(m_object, SHADE_PAL_16, sel);
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    SET_ANIMATION_ACT("A");
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
    SERIALIZE_USER_LOGIC_AND_CHAIN_OR_RETURN(ar, tag, c, d)
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
        SET_DRAW_FILL(obj, SHADE_PAL_16, sel);
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
