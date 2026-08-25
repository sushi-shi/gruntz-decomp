#include <rva.h>

#include <Gruntz/LightFx.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Gruntz/SerialArchive.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/ZVec.h>
#include <Wwd/LogicRecordEvent.h>

#include <stddef.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* result = NULL;
    MapLookup(map, name, result);
    return result;
}

RVA_DYNINIT(0x0009d120, 0xa, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d140, 0x15, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d170, 0xe, CActRegPool<CLightFx>::s_table)
RVA_DYNINIT(0x0009d190, 0x1f, CActRegPool<CLightFx>::s_table)
template<> DATA(0x00245ad0)
CActReg CActRegPool<CLightFx>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00012400, 0x1e, ??_GCLightFx@@UAEPAXI@Z)
RVA_COMPGEN(0x00012430, 0x44, ??1CLightFx@@UAE@XZ)

RVA(0x0009cdc0, 0xf1)
i32 DispatchLightFxLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED:
            record->SetLogicEvent(ACT_LIVE);
            {
                CLightFx* p = new CLightFx(obj);
                (static_cast<CUserLogic*>(p))->Activate();
                record->m_userLogic = p;
            }
            break;
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x0009cf00, 0x1a5)
CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_shadeTableIndex = 2;
    m_deleteWhenComplete = 1;
}
RVA(0x0009d1c0, 0x102)
void CLightFx::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CLightFx>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        (this->*(*((CActRegPool<CLightFx>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0009d320, 0x18d)
void CLightFx::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CLightFx>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CLightFx::AdvanceAnim);
}

// @early-stop
RVA(0x0009d520, 0xfd)
void CLightFx::Activate(
    const char* imageSetName,
    const char* animationName,
    i32 shadeTableIndex,
    i32 deleteWhenComplete
) {
    CDDrawWorker* imageSet = LookupWorker(
        m_ownerLogicRecord->m_ownerCtx->m_imageRegistry->m_workersByName,
        imageSetName
    );
    g_gameReg->m_lightFxMgr->ApplyShadeTable(imageSet, shadeTableIndex, SHADE_DST_BY_SRC_16);
    CWwdSpriteObject* object = m_wwdObject;
    if (imageSet != NULL) {

        i32 firstFrameIndex = imageSet->m_minIndex;

        object->m_imageSet = imageSet;
        CImage* firstFrame = imageSet->GetAt(firstFrameIndex);
        object->m_frameImage = firstFrame;
        object->m_frameIndex = firstFrameIndex;
    }
    CAniElement* node = NULL;
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    m_shadeTableIndex = shadeTableIndex;
    m_deleteWhenComplete = deleteWhenComplete;

    MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, animationName, node);
    if (node != NULL) {
        SwitchAnimation(
            LookupAnimation(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, animationName)
        );
        RebindNode();
    }
}

RVA(0x0009d660, 0xc8)
i32 CLightFx::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    switch (mode) {
        case SERIAL_SAVE:
            (ar)->Write(&m_shadeTableIndex, sizeof(m_shadeTableIndex));
            (ar)->Write(&m_deleteWhenComplete, sizeof(m_deleteWhenComplete));
            break;
        case SERIAL_LOAD:
            (ar)->Read(&m_shadeTableIndex, sizeof(m_shadeTableIndex));
            (ar)->Read(&m_deleteWhenComplete, sizeof(m_deleteWhenComplete));
            break;
        case SERIAL_POSTLOAD:
            g_gameReg
                ->m_lightFxMgr

                ->ApplyShadeTable(m_wwdObject->m_imageSet, m_shadeTableIndex, SHADE_DST_BY_SRC_16);
            break;
    }
    return 1;
}

RVA(0x0009d770, 0x25)
i32 CLightFx::RebindNode() {
    SET_ANIMATION_ACT("A");
    return 0;
}

RVA(0x0009d7b0, 0x40)
i32 CLightFx::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(
        IsAniCursorComplete(&m_wwdObject->m_animationCursor) && m_deleteWhenComplete
    )
    return 0;
}
