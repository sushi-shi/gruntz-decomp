#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntCreationPoint.h>

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
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TileSnapMacros.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

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

    i32 idx;
    if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
        if (g_gameReg->m_players[m_object->m_smarts].m_active != false) {
            idx = IDX(g_gameReg->m_players[m_object->m_smarts].m_color);
        } else {
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));

        }
    } else {
        idx = m_object->m_smarts;
    }
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);

    m_object->SetDrawFill(SHADE_PAL_16, sel);
    SNAP_OBJECT_TO_TILE_CENTER(m_object)
    SET_ANIMATION_ACT("A");
}

template<>
CActReg CActRegPool<CGruntCreationPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 CGruntCreationPoint::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode != SERIAL_SAVE && mode == SERIAL_POSTLOAD) {
        i32 idx;
        if (g_gameReg->GetGameMode() != GAMEMODE_QUESTZ) {
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
        obj->SetDrawFill(SHADE_PAL_16, sel);
    }
    return 1;
}

void CGruntCreationPoint::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CGruntCreationPoint::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CGruntCreationPoint>::s_table[id] =
        static_cast<i32 (CUserLogic::*)()>(&CGruntCreationPoint::AdvanceAnim);
}

i32 CGruntCreationPoint::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
