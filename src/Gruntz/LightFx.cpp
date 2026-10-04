#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/LightFx.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Image/ImageSet.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CLightFx>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

i32 DispatchLightFxLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CLightFx)
}

CLightFx::CLightFx(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    m_shadeTableIndex = 2;
    m_deleteWhenComplete = true;
}

void CLightFx::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void CLightFx::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CLightFx>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CLightFx::AdvanceAnim);
}

void CLightFx::Activate(
    const std::string& imageSetName,
    const std::string& animationName,
    i32 shadeTableIndex,
    b32 deleteWhenComplete
) {
    CDDrawWorker* imageSet = m_ownerLogicRecord->m_ownerCtx->FindWorker(imageSetName);
    g_gameReg->m_lightFxMgr->ApplyShadeTable(imageSet, shadeTableIndex, SHADE_DST_BY_SRC_16);
    CWwdSpriteObject* object = m_wwdObject;
    if (imageSet != NULL) {

        i32 firstFrameIndex = imageSet->GetMinIndex();

        object->m_imageSet = imageSet;
        object->SetImageFrame(firstFrameIndex);
    }
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    m_shadeTableIndex = shadeTableIndex;
    m_deleteWhenComplete = deleteWhenComplete;

    CAniElement* node =
        m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(animationName);
    if (node != NULL) {
        SwitchAnimation(
            m_wwdObject->OwnerMgr()->m_animRegistry->FindAnimation(animationName)
        );
        RebindNode();
    }
}

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

i32 CLightFx::RebindNode() {
    SET_ANIMATION_ACT("A");
    return 0;
}

i32 CLightFx::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    MARK_OBJECT_COMPLETE_IF(m_wwdObject->m_animationCursor.IsComplete() && m_deleteWhenComplete)
    return 0;
}
