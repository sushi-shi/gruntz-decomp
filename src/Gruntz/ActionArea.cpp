#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ActionArea.h>

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

template<>
CActReg CActRegPool<CActionArea>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 DispatchActionAreaLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CActionArea)
}

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

void CActionArea::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CProjActObj::RegisterType() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CActionArea>::s_table[id] = static_cast<CActHandler>(&CActionArea::Tick);
}

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
