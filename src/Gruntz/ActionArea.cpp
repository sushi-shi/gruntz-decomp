#include <rva.h>

#include <Gruntz/ActionArea.h>

#include <Mfc.h>

#include <Bute/ButeTree.h>
#include <Gruntz/ActionAreaOwner.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
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
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

RVA_DYNINIT(0x00008040, 0xa, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x00008060, 0x15, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x00008090, 0xe, CActRegPool<CActionArea>::s_table)
RVA_DYNINIT(0x000080b0, 0x1f, CActRegPool<CActionArea>::s_table)
template<> DATA(0x00229388)
CActReg CActRegPool<CActionArea>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

static inline CActHandler* R3Lookup(i32 coord) {
    return (CActRegPool<CActionArea>::s_table.ResolveEntry(coord));
}

RVA(0x00007c60, 0xf1)
i32 DispatchActionAreaLogic(CGameObject* owner){LOGIC_RECORD_DISPATCH(CActionArea)}

// @early-stop
RVA(0x00007da0, 0x17e)
CActionArea::CActionArea(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_timestamp = 0;
    m_duration = 0;
    SetImageSetByName("GAME_ACTIONAREA_RED");
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTION_AREA)
    m_phase = 1;
    m_duration = 0;
    Hide();
}

RVA_COMPGEN(0x00007fa0, 0x1e, ??_GCActionArea@@UAEPAXI@Z)
RVA_COMPGEN(0x00007fd0, 0x44, ??1CActionArea@@UAE@XZ)

RVA(0x000080e0, 0x102)
void CActionArea::FireActivation(i32 coord) {
    CActHandler* e = R3Lookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = R3Lookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00008240, 0x18d)
void CProjActObj::RegisterType() {
    ACT_NAME_ID(id, "A")

    *R3Lookup(id) = static_cast<CActHandler>(&CActionArea::Tick);
}

RVA(0x00008440, 0xfe)
i32 CActionArea::Tick() {
    i64* ts = &m_timestamp;
    i32* phase = &m_phase;
    if (static_cast<i64>(g_frameTime) - *ts >= m_duration) {
        *phase = (*phase == 0);
        m_duration = 0x1f4;
        *ts = static_cast<u32>(g_frameTime);
    }
    if (*phase != 0) {
        i64 d2 = static_cast<i64>(g_frameTime) - *ts;
        double t = static_cast<double>((d2 < 0 ? 0 : static_cast<u32>(d2)));
        m_wwdObject->m_imageSet->SetAllLightLevels(
            static_cast<i32>(((1.0 - t * 0.002) * 50.0 - (-155.0)))
        );
    } else {
        i64 d2 = static_cast<i64>(g_frameTime) - *ts;
        double t = static_cast<double>((d2 < 0 ? 0 : static_cast<u32>(d2)));
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
    SerBandPair(ar, mode, &m_timing);
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

RVA_COMPGEN(0x000087b0, 0x7, ??1CUserBase@@UAE@XZ)
RVA_COMPGEN(0x00008810, 0x20, ??_GCUserBase@@UAEPAXI@Z)

RVA_COMPGEN(0x00008860, 0x44, ??1CUserLogic@@UAE@XZ)
RVA_COMPGEN(0x00008a10, 0x1e, ??_GCUserLogic@@UAEPAXI@Z)
