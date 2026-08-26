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

RVA_COMPGEN(0x00010710, 0x1e, ??_GCGruntCreationPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010740, 0x44, ??1CGruntCreationPoint@@UAE@XZ)

RVA(0x0003e440, 0x1fd)
CGruntCreationPoint::CGruntCreationPoint(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    CWwdSpriteObject* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_CREATION) {
        o->m_sortKey = SORTKEY_GRUNT_CREATION;
        i32 f = o->m_flags;
        f |= 0x20000;
        o->m_flags = f;
    }
    SwitchAnimationByName("GAME_CYCLE100", 0);

    i32 idx = m_object->m_smarts;
    if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
        if (g_gameReg->m_players[idx].m_active != false) {
            idx = IDX(g_gameReg->m_players[idx].m_color);
        } else {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));

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

RVA_DYNINIT(0x0003e7e0, 0xa, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e800, 0x15, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e830, 0xe, CActRegPool<CGruntCreationPoint>::s_table)
RVA_DYNINIT(0x0003e850, 0x1f, CActRegPool<CGruntCreationPoint>::s_table)
template<> DATA(0x00245658)
CActReg CActRegPool<CGruntCreationPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003e6c0, 0xd7)
i32 CGruntCreationPoint::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode != SERIAL_SAVE && mode == SERIAL_POSTLOAD) {
        i32 idx;
        if (g_gameReg->m_gameMode != GAMEMODE_QUESTZ) {
            if (g_gameReg->m_players[m_object->m_smarts].m_active != false) {
                idx = IDX(g_gameReg->m_players[m_object->m_smarts].m_color);
            } else {
                idx = IDX(FindAvailablePlayerColor());
            }
        } else {
            idx = m_object->m_smarts;
        }
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        if (sel == NULL) {
            sel = g_gameReg->m_spriteFactory->GetSel(1, 0);
        }
        CWwdSpriteObject* obj = m_object;
        SET_DRAW_FILL(obj, SHADE_PAL_16, sel);
    }
    return 1;
}

RVA(0x0003e880, 0x102)
void CGruntCreationPoint::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
    if (*e != NULL) {
        CActHandler* e2 = (CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(coord));
        (this->*(*e2))();
    }
}

RVA(0x0003e9e0, 0x18d)
void CGruntCreationPoint::RegisterActs() {
    ACT_NAME_ID(id, "A")
    *(CActRegPool<CGruntCreationPoint>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

RVA(0x0003ebe0, 0x17)
i32 CGruntCreationPoint::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
