#include <rva.h>

#include <Gruntz/ActionArea.h>

#include <Mfc.h>

#include <Gruntz/ActionAreaOwner.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ObjTypeRegistrars.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

RVA_DYNINIT(0x00008040, 0xa, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x00008060, 0x15, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x00008090, 0xe, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x000080b0, 0x1f, CActRegPool<CActionArea>::s_table)
template<> DATA(0x00229388)
CActReg CActRegPool<CActionArea>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x00007c60, 0xf1)
i32 DispatchActionAreaLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CActionArea)
}

RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_ACTIONAREA_RED");
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTION_AREA)
    m_phase = 1;
    m_timing.m_intervalLo = 0;
    m_timing.m_intervalHi = 0;
    Hide();
}

RVA_COMPGEN(0x00007fa0, 0x1e, ??_GCActionArea@@UAEPAXI@Z)
RVA_COMPGEN(0x00007fd0, 0x44, ??1CActionArea@@UAE@XZ)

RVA(0x000080e0, 0x102)
void CActionArea::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x00008240, 0x18d)
void CProjActObj::RegisterType() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CActionArea>::s_table[id] = static_cast<CActHandler>(&CActionArea::Tick);
}

RVA(0x00008440, 0xfe)
i32 CActionArea::Tick() {
    ClockInterval* timing = &m_timing;
    i32* phase = &m_phase;
    if (timing->Expired()) {
        *phase = (*phase == 0);
        timing->Start(0x1f4);
    }
    if (*phase != 0) {
        double t = static_cast<double>(timing->Elapsed());
        m_wwdObject->m_imageSet->SetAllLightLevels(
            static_cast<i32>(((1.0 - t * 0.002) * 50.0 - (-155.0)))
        );
    } else {
        double t = static_cast<double>(timing->Elapsed());
        m_wwdObject->m_imageSet->SetAllLightLevels(static_cast<i32>((t * 0.1 - (-155.0))));
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00008580, 0x5e)
i32 CActionArea::ApplyColor(i32 owner) {
    switch (static_cast<ActionAreaOwner>(owner)) {
        case ACTION_AREA_BLUE_OWNER: {
            SetImageSetByName("GAME_ACTIONAREA_BLUE");

            CDDrawWorker* rec = m_wwdObject->m_imageSet;
            rec->SetAllTypes(SHADE_ALPHA_16);
            break;
        }
        case ACTION_AREA_RED_OWNER: {
            SetImageSetByName("GAME_ACTIONAREA_RED");

            CDDrawWorker* rec = m_wwdObject->m_imageSet;
            rec->SetAllTypes(SHADE_ALPHA_16);
            break;
        }
        default:
            return 0;
    }
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x00008600, 0xcd)
i32 CActionArea::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    SerializeClockPair(ar, mode, &m_timing);
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_phase, sizeof(m_phase));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_phase, sizeof(m_phase));
            break;
    }
    return 1;
}

RVA_COMPGEN(0x00008710, 0x2b, ??0?$zDArray@P8CUserLogic@@AEHXZ@@QAE@HH@Z)
RVA_COMPGEN(0x00008750, 0x15, ??1?$zDArray@P8CUserLogic@@AEHXZ@@UAE@XZ)
RVA_COMPGEN(0x00008780, 0x1e, ??_G?$zDArray@P8CUserLogic@@AEHXZ@@UAEPAXI@Z)

RVA_COMPGEN(0x000087b0, 0x7, ??1CUserBase@@UAE@XZ)
RVA_COMPGEN(0x00008810, 0x20, ??_GCUserBase@@UAEPAXI@Z)

RVA_COMPGEN(0x00008860, 0x44, ??1CUserLogic@@UAE@XZ)
RVA_COMPGEN(0x00008a10, 0x1e, ??_GCUserLogic@@UAEPAXI@Z)
