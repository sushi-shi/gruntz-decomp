#include <rva.h>

#include <Gruntz/GruntToySprite.h>

#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0007f520, 0xa, CActRegPool<CGruntToySprite>::s_table)
RVA_DYNINIT(0x0007f540, 0x15, CActRegPool<CGruntToySprite>::s_table)
RVA_DYNINIT(0x0007f570, 0xe, CActRegPool<CGruntToySprite>::s_table)
RVA_DYNINIT(0x0007f590, 0x1f, CActRegPool<CGruntToySprite>::s_table)
template<> DATA(0x00244d58)
CActReg CActRegPool<CGruntToySprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00012280, 0x1e, ??_GCGruntToySprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000122b0, 0x44, ??1CGruntToySprite@@UAE@XZ)

RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageFrameByName("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", 0);
    SET_ANIMATION_ACT("A");
    Hide();
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_lastLayer = PICKUP_NONE;
}

RVA(0x0007f5c0, 0x102)
void CGruntToySprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))) != NULL) {
        (this->*(*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007f720, 0x18d)
void CGruntToySprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CGruntToySprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntToySprite::Update);
}

RVA(0x0007f920, 0x21)
i32 CGruntToySprite::BindToGrunt(i32 playerIndex, i32 unitIndex) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    return 1;
}

RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGrunt* e = g_gameReg->m_triggerMgr
                    ->m_units[m_gruntIdentity.m_playerIndex * 15 + m_gruntIdentity.m_unitIndex];
    if (e == NULL) {
        return 0;
    }
    PickupType layer = e->m_vehiclePickupType;
    if (m_lastLayer != layer) {
        CWwdSpriteObject* r = m_object;
        m_lastLayer = layer;
        CDDrawWorker* h = r->m_imageSet;
        if (h != NULL) {
            i32 layerIndex = IDX(layer);
            CImage* mapped = h->GetAt(layerIndex);
            r->m_frameImage = mapped;
            r->m_frameIndex = layerIndex;
        }
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = e->m_object->m_screenY - 0x20;
    return 0;
}

RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Write(&m_lastLayer, sizeof(m_lastLayer));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Read(&m_lastLayer, sizeof(m_lastLayer));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}
