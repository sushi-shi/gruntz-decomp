#include <rva.h>

#include <Gruntz/FortressFlag.h>

#include <Bute/ButeTree.h>
#include <Enums.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicRecordDispatchInline.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarlordOwner.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>
#include <Wwd/LogicRecordEvent.h>

#include <stddef.h>

RVA_DYNINIT(0x000464a0, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x000464c0, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x000464f0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x00046510, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00046540, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00046560, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00046590, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x000465b0, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x000465e0, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x00046600, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x00046630, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00046650, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00046680, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x000466a0, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x000466d0, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x000466f0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x00046720, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00046740, 0x1a, s_gruntDirCenter)

RVA_DYNINIT(0x00045f00, 0xa, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00045f20, 0x15, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00045f50, 0xe, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00045f70, 0x1f, CActRegPool<CFortressFlag>::s_table)
template<> DATA(0x00245590)
CActReg CActRegPool<CFortressFlag>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x00046bb0, 0xa, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046bd0, 0x15, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046c00, 0xe, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046c20, 0x1f, CActRegPool<CParticlez>::s_table)
template<> DATA(0x002457c8)
CActReg CActRegPool<CParticlez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000471d0, 0xa, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x000471f0, 0x15, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x00047220, 0xe, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x00047240, 0x1f, CActRegPool<CExplosion>::s_table)
template<> DATA(0x00245750)
CActReg CActRegPool<CExplosion>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

static inline CActHandler* PartLookup(i32 coord) {
    return (CActRegPool<CParticlez>::s_table.ResolveEntry(coord));
}

RVA_COMPGEN(0x00010e70, 0x1e, ??_GCFortressFlag@@UAEPAXI@Z)
RVA_COMPGEN(0x00010ea0, 0x44, ??1CFortressFlag@@UAE@XZ)

RVA_COMPGEN(0x00012d70, 0x1e, ??_GCParticlez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012da0, 0x44, ??1CParticlez@@UAE@XZ)

RVA_COMPGEN(0x00012ea0, 0x1e, ??_GCExplosion@@UAEPAXI@Z)
RVA_COMPGEN(0x00012ed0, 0x44, ??1CExplosion@@UAE@XZ)

// @early-stop
RVA(0x00045c50, 0x220)
CFortressFlag::CFortressFlag(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    i32 v = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(o, v)
    switch (static_cast<WarlordOwner>(m_object->m_smarts)) {
        case WARLORDZ_KING:
            SetImageSetByName("GAME_FORTRESSFLAGZ_KING");
            break;
        case WARLORDZ_NAPOLEAN:
            SetImageSetByName("GAME_FORTRESSFLAGZ_NAPOLEAN");
            break;
        case WARLORDZ_PATTON:
            SetImageSetByName("GAME_FORTRESSFLAGZ_PATTON");
            break;
        case WARLORDZ_VIKING:
            SetImageSetByName("GAME_FORTRESSFLAGZ_VIKING");
            break;
        default:
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            return;
    }
    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE);
    i32 idx = IDX(g_gameReg->m_players[m_object->m_smarts].m_color);
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
    CWwdSpriteObject* spr = m_object;
    SET_DRAW_FILL(spr, SHADE_PAL_16, sel);
}

RVA(0x00045fa0, 0x102)
void CFortressFlag::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CFortressFlag>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CFortressFlag>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x00046100, 0x18d)
void CFortressFlag::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CFortressFlag>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CFortressFlag::AdvanceAnim);
}

RVA(0x00046300, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00046330, 0x92)
i32 CFortressFlag::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode == SERIAL_POSTLOAD) {
        CWwdSpriteObject* spr = m_object;
        i32 idx = IDX(g_gameReg->m_players[spr->m_smarts].m_color);
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        spr = m_object;
        SET_DRAW_FILL(spr, SHADE_PAL_16, sel);
    }
    return 1;
}

template<> RVA(0x00046400, 0x74)
CActHandler* zDArray<CActHandler>::Resolve(i32 id) {
    char* r;
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        r = m_base + (id - m_lo) * m_stride;
    } else if (GrowTo(id, 0)) {
        r = m_base + (id - m_lo) * m_stride;
    } else {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetRetAddr();
        m_errSink->Set(this, msg, 0xc);
        r = m_spare;
    }

    union {
        char* m_bytes;
        CActHandler* m_slot;
    } band;
    band.m_bytes = r;
    return band.m_slot;
}

RVA(0x00046770, 0xf1)
i32 DispatchParticlezLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CParticlez(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x000468b0, 0xf1)
i32 DispatchExplosionLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CUserLogic* sub = new CExplosion(owner);
            sub->Activate();
            record->m_userLogic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x000469f0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_BEHIND)
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00046c50, 0x102)
void CParticlez::FireActivation(i32 coord) {
    CActHandler* e = PartLookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = PartLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x00046db0, 0x18d)
void CParticlez::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((PartLookup(id)))) = static_cast<i32 (CUserLogic::*)()>(&CParticlez::Update);
}

RVA(0x00046fb0, 0x39)
i32 CParticlez::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* o = m_wwdObject;
    if (IsAniCursorComplete(&o->m_animationCursor)) {
        o->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 0;
}

// @early-stop
RVA(0x00047000, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXPLOSION");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00047270, 0x102)
void CExplosion::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CExplosion>::s_table.ResolveEntry(id));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CExplosion>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}

RVA(0x000473d0, 0x18d)
void RegisterExplosionActions() {
    ACT_NAME_ID(id, "A")
    *CActRegPool<CExplosion>::s_table.ResolveEntry(id) =
        static_cast<CActHandler>(&CExplosion::Update);
}
