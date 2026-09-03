#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/Blk6c.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdSpriteAnimationInline.h>

#include <ddraw.h>
#include <stdlib.h>
#include <string.h>

DATA(0x002bf674)
b32 g_logicTypesRegistered;

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* result = NULL;
    map.Lookup(name, result);
    return static_cast<CDDrawWorker*>(result);
}

RVA(0x001504d0, 0x6c)
void CWwdSpriteObject::SetImageFrameByName(const char* name, i32 frame) {
    CDDrawWorker* spr = LookupWorker(OwnerMgr()->m_imageRegistry->m_workersByName, name);
    m_imageSet = spr;
    if (spr) {
        CImage* f = spr->GetAt(frame);
        m_frameIndex = frame;
        m_frameImage = f;
    }
}

RVA(0x00150540, 0x65)
void CWwdSpriteObject::SetImageSetByName(const char* name) {
    CDDrawWorker* spr = LookupWorker(OwnerMgr()->m_imageRegistry->m_workersByName, name);
    m_imageSet = spr;
    if (spr) {
        i32 n = spr->m_minIndex;
        m_frameIndex = n;
        m_frameImage = spr->GetAt(n);
    }
}

static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* result = NULL;
    MapLookup(map, name, result);
    return result;
}

RVA(0x001505b0, 0x5e)
i32 CWwdSpriteObject::SetAnimationByName(const char* name, i32 advanceImmediately) {
    CAniElement* animation = LookupAnimation(OwnerMgr()->m_animRegistry->m_animations, name);
    if (!animation) {
        return 0;
    }
    SET_ANIMATION_AND_MAYBE_ADVANCE(this, animation, advanceImmediately)
    return 1;
}

static inline SoundCue* LookupSoundCue(CMapStringToPtr& map, LPCTSTR name) {
    SoundCue* result = NULL;
    MapLookup(map, name, result);
    return result;
}

RVA(0x00150610, 0x41)
i32 CWwdSpriteObject::SetSoundCueByName(const char* name) {
    SoundCue* cue = LookupSoundCue(OwnerMgr()->m_soundRegistry->m_cues, name);
    if (cue == NULL) {
        return 0;
    }
    m_soundCue = cue;
    return 1;
}

RVA(0x00150660, 0x49)
void CWwdSpriteObject::BltDirty(CDDrawSurfacePair* dst, CDDrawSurfacePair* src) {

    m_shadow = m_dirty;
    if (m_dirty.m_armed != -1) {
        RECT* r = &m_dirty.m_rect;
        dst->m_surface->BltFast(r->left, r->top, src->m_surface, r, 0x10);
        m_dirty.m_armed = -1;
    }
}

RVA(0x001506b0, 0x1ec)
void CWwdSpriteObject::BltDirtyEx(
    CDrawSubWorker* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        RECT ir;
        if (IntersectRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect)) {
            UnionRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect);
            Coord position(ir.left, ir.top);
            CSize size(ir.right - ir.left + 1, ir.bottom - ir.top + 1);
            dst->BlitDirtyRect(src, position, size);
        } else {
            dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
            dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
    }
}

RVA(0x001508a0, 0x117)
void CWwdSpriteObject::BltDirtyRegions(
    CDDrawSurfacePair* dst,
    CDDrawSurfacePair* src,
    CDDrawSurfacePair* restoreSrc
) {
    if (m_dirty.m_armed != -1 && m_shadow.m_armed != -1) {
        RECT ir;
        if (IntersectRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect)) {
            UnionRect(&ir, &m_dirty.m_rect, &m_shadow.m_rect);
            Coord position(ir.left, ir.top);
            CSize size(ir.right - ir.left + 1, ir.bottom - ir.top + 1);
            dst->BlitDirtyRect(src, position, size);
        } else {
            dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
            dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
        }
    } else if (m_dirty.m_armed != -1) {
        dst->BlitDirtyRect(src, m_dirty.m_lastPosition, m_dirty.m_size);
    } else if (m_shadow.m_armed != -1) {
        dst->BlitDirtyRect(src, m_shadow.m_lastPosition, m_shadow.m_size);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001509c0, 0xab)
i32 CWwdSpriteObject::IntersectsViewport() {
    if (m_frameImage == NULL) {
        return 0;
    }
    RECT bounds;
    SetRect(
        &bounds,
        m_screenPosition.m_x - m_frameImage->m_anchor.x,
        m_screenPosition.m_y - m_frameImage->m_anchor.y,
        m_screenPosition.m_x + m_frameImage->m_anchor.x,
        m_screenPosition.m_y + m_frameImage->m_anchor.y
    );
    if (HAS(static_cast<WwdGameObjectFlags>(m_flags), WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {

        RECT* r = &OwnerMgr()->m_level->m_mainPlane->m_planeViewRect;
        if (bounds.right < r->left) {
            return 0;
        }
        if (bounds.left > r->right) {
            return 0;
        }
        if (bounds.bottom < r->top) {
            return 0;
        }
        return bounds.top <= r->bottom;
    } else {

        CDDrawFrontSurface* g = OwnerMgr()->m_drawTarget->m_frontSurface;

        CSize surfaceSize(g->m_width, g->m_height);
        if (bounds.right < 0) {
            return 0;
        }
        if (bounds.left >= surfaceSize.cx) {
            return 0;
        }
        if (bounds.bottom < 0) {
            return 0;
        }
        return bounds.top < surfaceSize.cy;
    }
}

RVA(0x00150a70, 0x89)
i32 CWwdSpriteObject::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }
    if (m_animationCursor.SerializeDispatch(ar, mode, typeId, object) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (WriteSpriteState(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (ReadSpriteState(ar) == 0) {
                return 0;
            }
            break;
    }
    return CGameObject::SerializeDispatch(ar, mode, typeId, object) != 0;
}

RVA(0x00150b00, 0x12b)
i32 CWwdSpriteObject::WriteSpriteState(CFileMemBase* stream) {
    CFileMemBase* ar = stream;
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_reserved18c, sizeof(m_reserved18c));
    ar->Write(&m_frameIndex, sizeof(m_frameIndex));
    b32 hasFrameImage = false;
    if (m_frameImage != NULL) {
        hasFrameImage = true;
    }
    ar->Write(&hasFrameImage, sizeof(hasFrameImage));

    char tmp[0x100];
    memset(tmp, 0, SERIAL_NAME_LEN);
    if (m_imageSet != NULL) {
        strcpy(tmp, m_imageSet->m_name);
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, SERIAL_NAME_LEN);
    {
        strcpy(tmp, OwnerMgr()->m_soundRegistry->FindCueKey(m_soundCue));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);
    return 1;
}

RVA(0x00150c30, 0x130)
i32 CWwdSpriteObject::ReadSpriteState(CFileMemBase* stream) {
    CFileMemBase* ar = stream;
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_reserved18c, sizeof(m_reserved18c));
    ar->Read(&m_frameIndex, sizeof(m_frameIndex));
    b32 hasFrameImage;
    ar->Read(&hasFrameImage, sizeof(hasFrameImage));
    m_imageSet = NULL;

    char name[0x100];
    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {

        CDDrawWorker* found = NULL;
        CObject* foundOb = NULL;
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        mgr->m_imageRegistry->m_workersByName.Lookup(name, foundOb);
        found = static_cast<CDDrawWorker*>(foundOb);
        m_imageSet = found;
        if (found != NULL && hasFrameImage == true) {
            i32 idx = m_frameIndex;
            CImage* frame = found->GetAt(idx);
            m_frameImage = frame;
        }
    }

    m_soundCue = NULL;
    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {

        SoundCue* found = NULL;
        CDDrawSurfaceMgr* mgr = OwnerMgr();
        MapLookup(mgr->m_soundRegistry->m_cues, name, found);
        m_soundCue = found;
    }
    return 1;
}

// @early-stop
RVA(0x00150d60, 0x14d)
i32 CGameObject::Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) {
    CResolveNode::SetPosition(x, y);
    SetScreenPos(x, y);
    m_sortKey = sortKey;
    m_spawnPosition.Set(x, y);
    CLogicRecord* record = m_logicRecord;
    m_spawnSortKey = sortKey;
    m_stride.Set(10, 10);
    m_points = 0;
    m_score = 0;
    m_health = 0;
    m_smarts = 0;
    m_powerup = 0;
    m_damage = 0;
    m_direction = 0;
    m_faceDirection = 0;
    m_speed.Set(0, 0);
    m_reservede0 = 0;
    m_reserved180 = 0;

    if (record->Init(logicTemplate->m_dispatch, logicTemplate->m_flags) == 0) {
        return 0;
    }
    m_hitLogic = NULL;
    m_attackLogic = NULL;
    m_collisionLogic = NULL;
    m_hitSource = NULL;
    m_attackTarget = NULL;
    m_hitOther = NULL;
    m_objectType = 0;
    m_hitTypeFlags = 0;
    m_attackTypeMask = 0;
    m_collMask = 0;
    m_extent.left = COORD_UNSET;
    m_area.left = COORD_UNSET;
    m_switchRect.left = COORD_UNSET;
    m_region.m_object = this;
    m_region.m_position = m_screenPosition;
    LogicRecordFlags logicFlags = static_cast<LogicRecordFlags>(m_logicRecord->m_flags);
    if (HAS(logicFlags, LOGIC_RECORD_FLAG_LARGE_ACTIVE_REGION)) {
        m_flags |= IDX(WWD_GAME_OBJECT_FLAG_LARGE_ACTIVE_REGION);
        return 1;
    }
    if (HAS(logicFlags, LOGIC_RECORD_FLAG_SMALL_ACTIVE_REGION)) {
        m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION);
    }
    return 1;
}

RVA(0x00150eb0, 0x98)
i32 CGameObject::EnsureHitLogic(CLogicRecord* logicTemplate) {
    if (logicTemplate == NULL) {
        return 0;
    }
    if (m_hitLogic != NULL) {
        m_hitLogic->Unload();
    } else {
        m_hitLogic = new CLogicRecord(m_ownerCtx, m_id);
    }
    if (m_hitLogic == NULL) {
        return 0;
    }

    return m_hitLogic->Init(logicTemplate->m_dispatch, 0);
}

static inline CLogicRecord* LookupLogicTemplate(CMapStringToOb& map, LPCTSTR name) {
    CObject* result = NULL;
    map.Lookup(name, result);
    return static_cast<CLogicRecord*>(result);
}

RVA(0x00150f50, 0x35)
void CGameObject::AddLogicHit(char* key) {
    EnsureHitLogic(LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, key));
}

RVA(0x00150f90, 0x98)
i32 CGameObject::EnsureAttackLogic(CLogicRecord* logicTemplate) {
    if (logicTemplate == NULL) {
        return 0;
    }
    if (m_attackLogic != NULL) {
        m_attackLogic->Unload();
    } else {
        m_attackLogic = new CLogicRecord(m_ownerCtx, m_id);
    }
    if (m_attackLogic == NULL) {
        return 0;
    }

    return m_attackLogic->Init(logicTemplate->m_dispatch, 0);
}

RVA(0x00151030, 0x35)
void CGameObject::AddLogicAttack(char* key) {
    EnsureAttackLogic(LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, key));
}

RVA(0x00151070, 0x98)
i32 CGameObject::EnsureBumpLogic(CLogicRecord* logicTemplate) {
    if (logicTemplate == NULL) {
        return 0;
    }
    if (m_collisionLogic != NULL) {
        m_collisionLogic->Unload();
    } else {
        m_collisionLogic = new CLogicRecord(m_ownerCtx, m_id);
    }
    if (m_collisionLogic == NULL) {
        return 0;
    }

    return m_collisionLogic->Init(logicTemplate->m_dispatch, 0);
}

RVA(0x00151110, 0x35)
void CGameObject::AddLogicBump(char* key) {
    EnsureBumpLogic(LookupLogicTemplate(OwnerMgr()->m_logicRegistry->m_templatesByName, key));
}

static inline i32 NotifyLogicForEventCode(CGameObject* object, i32 eventCode) {
    CLogicRecord* record = object->m_logicRecord;
    if (!record) {
        return 0;
    }
    i32 savedEventCode = record->m_eventCode;
    record->SetEventCode(eventCode);
    object->m_logicRecord->m_dispatch(object);
    if (object->m_logicRecord->m_eventCode == eventCode) {
        object->m_logicRecord->SetEventCode(savedEventCode);
    }
    return 1;
}

// @early-stop
RVA(0x00151150, 0x190)
i32 CGameObject::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (ar == NULL) {
        return 0;
    }

    switch (mode) {
        case SERIAL_PRESAVE:
            m_carrierId = 0;
            if (m_carrier != NULL) {
                m_carrierId = m_carrier->m_objectId;
            }
            if (!NotifyLogicForEventCode(this, ACT_PREPARE_SAVE)) {
                goto fail;
            }
        default:
        dispatch:
            return m_logicRecord->SerializeDispatch(ar, mode, typeId, object) != 0;
        case SERIAL_SAVE:
            if (Serialize(ar) == 0) {
                return 0;
            }
            if (!NotifyLogicForEventCode(this, ACT_AFTER_SAVE)) {
                goto fail;
            }
            goto dispatch;
        case SERIAL_LOAD:
            if (SerializeObjectState(ar) == 0) {
                return 0;
            }
            if (!NotifyLogicForEventCode(this, ACT_AFTER_LOAD)) {
                goto fail;
            }
            goto dispatch;
        case SERIAL_POSTLOAD: {
            i32 node = m_carrierId;
            if (node != 0) {
                AddrWord<char> key;
                key.m_word = node;
                CWwdGameObject* found = NULL;
                if (MapLookup(
                        OwnerMgr()->m_childGroup->m_registeredGameObjectsById,
                        key.m_addr,
                        found
                    )
                    == false) {
                    found = NULL;
                }
                m_carrier = found;
            } else {
                m_carrier = NULL;
            }
            if (!NotifyLogicForEventCode(this, ACT_AFTER_LOAD_REFERENCES)) {
                goto fail;
            }
            goto dispatch;
        }
    }
fail:
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001512e0, 0x35)
i32 CGameObject::PrepareSave(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    m_carrierId = 0;
    if (m_carrier != NULL) {
        m_carrierId = m_carrier->m_objectId;
    }
    return 1;
}

RVA(0x00151320, 0x454)
i32 CGameObject::Serialize(CFileMemBase* arParam) {
    CFileMemBase* ar = arParam;
    if (ar == NULL) {
        return 0;
    }

    ar->Write(&m_shadow, sizeof(m_shadow));

    char tmp[SERIAL_NAME_LEN];
    memset(tmp, 0, sizeof(tmp));
    strcpy(tmp, m_name);
    ar->Write(tmp, SERIAL_NAME_LEN);

    ar->Write(&m_moveMode, sizeof(m_moveMode));
    ar->Write(&m_objectType, sizeof(m_objectType));
    ar->Write(&m_hitTypeFlags, sizeof(m_hitTypeFlags));
    ar->Write(&m_attackTypeMask, sizeof(m_attackTypeMask));
    ar->Write(&m_collMask, sizeof(m_collMask));
    ar->Write(&m_stride.m_x, sizeof(m_stride.m_x));
    ar->Write(&m_stride.m_y, sizeof(m_stride.m_y));
    ar->Write(&m_reserved100, sizeof(m_reserved100));
    ar->Write(&m_spawnPosition.m_x, sizeof(m_spawnPosition.m_x));
    ar->Write(&m_spawnPosition.m_y, sizeof(m_spawnPosition.m_y));
    ar->Write(&m_spawnSortKey, sizeof(m_spawnSortKey));
    ar->Write(&m_reserved110, sizeof(m_reserved110));
    ar->Write(&m_score, sizeof(m_score));
    ar->Write(&m_points, sizeof(m_points));
    ar->Write(&m_powerup, sizeof(m_powerup));
    ar->Write(&m_damage, sizeof(m_damage));
    ar->Write(&m_smarts, sizeof(m_smarts));
    ar->Write(&m_health, sizeof(m_health));
    ar->Write(&m_direction, sizeof(m_direction));
    ar->Write(&m_faceDirection, sizeof(m_faceDirection));
    ar->Write(&m_extent.left, sizeof(m_extent));
    ar->Write(&m_area.left, sizeof(m_area));
    ar->Write(&m_switchRect.left, sizeof(m_switchRect));
    ar->Write(&m_speed.m_x, sizeof(m_speed.m_x));
    ar->Write(&m_speed.m_y, sizeof(m_speed.m_y));
    ar->Write(&m_reserved16c, sizeof(m_reserved16c));
    ar->Write(&m_reserved170, sizeof(m_reserved170));
    ar->Write(&m_delta.m_x, sizeof(m_delta.m_x));
    ar->Write(&m_delta.m_y, sizeof(m_delta.m_y));
    ar->Write(&m_reserved17c, sizeof(m_reserved17c));
    ar->Write(&m_reserved180, sizeof(m_reserved180));
    ar->Write(&m_plotOffset.m_x, sizeof(m_plotOffset.m_x));
    ar->Write(&m_plotOffset.m_y, sizeof(m_plotOffset.m_y));
    ar->Write(&m_dirty, sizeof(m_dirty));
    ar->Write(&m_stateFlags, sizeof(m_stateFlags));
    ar->Write(&m_flashCountdown, sizeof(m_flashCountdown));
    ar->Write(&m_flashInterval, sizeof(m_flashInterval));
    ar->Write(&m_drawFillCmd, sizeof(m_drawFillCmd));
    ar->Write(&m_fillFraction, sizeof(m_fillFraction));
    ar->Write(&m_drawActive, sizeof(m_drawActive));
    ar->Write(&m_clip.left, sizeof(m_clip));
    ar->Write(&m_id, sizeof(m_id));
    ar->Write(&m_flags, sizeof(m_flags));
    ar->Write(&m_carrierId, sizeof(m_carrierId));

    memset(tmp, 0, sizeof(tmp));
    if (m_hitLogic != NULL) {
        strcpy(tmp, OwnerMgr()->m_logicRegistry->FindLogicTypeKey(m_hitLogic));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, sizeof(tmp));
    if (m_attackLogic != NULL) {
        strcpy(tmp, OwnerMgr()->m_logicRegistry->FindLogicTypeKey(m_attackLogic));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);

    memset(tmp, 0, sizeof(tmp));
    if (m_collisionLogic != NULL) {
        strcpy(tmp, OwnerMgr()->m_logicRegistry->FindLogicTypeKey(m_collisionLogic));
    }
    ar->Write(tmp, SERIAL_NAME_LEN);
    return 1;
}

RVA(0x00151780, 0x40d)
i32 CGameObject::SerializeObjectState(CFileMemBase* arParam) {
    CFileMemBase* ar = arParam;
    if (ar == NULL) {
        return 0;
    }

    ar->Read(&m_shadow, sizeof(m_shadow));

    char name[SERIAL_NAME_LEN];
    ar->Read(name, SERIAL_NAME_LEN);
    m_name = name;

    ar->Read(&m_moveMode, sizeof(m_moveMode));
    ar->Read(&m_objectType, sizeof(m_objectType));
    ar->Read(&m_hitTypeFlags, sizeof(m_hitTypeFlags));
    ar->Read(&m_attackTypeMask, sizeof(m_attackTypeMask));
    ar->Read(&m_collMask, sizeof(m_collMask));
    ar->Read(&m_stride.m_x, sizeof(m_stride.m_x));
    ar->Read(&m_stride.m_y, sizeof(m_stride.m_y));
    ar->Read(&m_reserved100, sizeof(m_reserved100));
    ar->Read(&m_spawnPosition.m_x, sizeof(m_spawnPosition.m_x));
    ar->Read(&m_spawnPosition.m_y, sizeof(m_spawnPosition.m_y));
    ar->Read(&m_spawnSortKey, sizeof(m_spawnSortKey));
    ar->Read(&m_reserved110, sizeof(m_reserved110));
    ar->Read(&m_score, sizeof(m_score));
    ar->Read(&m_points, sizeof(m_points));
    ar->Read(&m_powerup, sizeof(m_powerup));
    ar->Read(&m_damage, sizeof(m_damage));
    ar->Read(&m_smarts, sizeof(m_smarts));
    ar->Read(&m_health, sizeof(m_health));
    ar->Read(&m_direction, sizeof(m_direction));
    ar->Read(&m_faceDirection, sizeof(m_faceDirection));
    ar->Read(&m_extent.left, sizeof(m_extent));
    ar->Read(&m_area.left, sizeof(m_area));
    ar->Read(&m_switchRect.left, sizeof(m_switchRect));
    ar->Read(&m_speed.m_x, sizeof(m_speed.m_x));
    ar->Read(&m_speed.m_y, sizeof(m_speed.m_y));
    ar->Read(&m_reserved16c, sizeof(m_reserved16c));
    ar->Read(&m_reserved170, sizeof(m_reserved170));
    ar->Read(&m_delta.m_x, sizeof(m_delta.m_x));
    ar->Read(&m_delta.m_y, sizeof(m_delta.m_y));
    ar->Read(&m_reserved17c, sizeof(m_reserved17c));
    ar->Read(&m_reserved180, sizeof(m_reserved180));
    ar->Read(&m_plotOffset.m_x, sizeof(m_plotOffset.m_x));
    ar->Read(&m_plotOffset.m_y, sizeof(m_plotOffset.m_y));
    ar->Read(&m_dirty, sizeof(m_dirty));
    ar->Read(&m_stateFlags, sizeof(m_stateFlags));
    ar->Read(&m_flashCountdown, sizeof(m_flashCountdown));
    ar->Read(&m_flashInterval, sizeof(m_flashInterval));
    ar->Read(&m_drawFillCmd, sizeof(m_drawFillCmd));
    ar->Read(&m_fillFraction, sizeof(m_fillFraction));
    ar->Read(&m_drawActive, sizeof(m_drawActive));
    ar->Read(&m_clip.left, sizeof(m_clip));
    ar->Read(&m_id, sizeof(m_id));
    ar->Read(&m_flags, sizeof(m_flags));
    ar->Read(&m_carrierId, sizeof(m_carrierId));

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = NULL;
        OwnerMgr()->m_logicRegistry->m_templatesByName.Lookup(name, found);
        if (this->EnsureHitLogic(static_cast<CLogicRecord*>(found)) == 0) {
            return 0;
        }
    }

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = NULL;
        OwnerMgr()->m_logicRegistry->m_templatesByName.Lookup(name, found);
        if (this->EnsureAttackLogic(static_cast<CLogicRecord*>(found)) == 0) {
            return 0;
        }
    }

    ar->Read(name, SERIAL_NAME_LEN);
    if (strlen(name) != 0) {
        CObject* found = NULL;
        OwnerMgr()->m_logicRegistry->m_templatesByName.Lookup(name, found);
        if (this->EnsureBumpLogic(static_cast<CLogicRecord*>(found)) == 0) {
            return 0;
        }
    }
    return 1;
}

static inline BOOL LookupLinkedObject(CMapPtrToPtr& map, i32 id, CWwdGameObject*& out) {
    out = NULL;
    AddrWord<char> key;
    key.m_word = id;
    MapOutRef<CWwdGameObject> dst;
    dst.m_asTyped = &out;
    return map.Lookup(key.m_addr, *dst.m_asVoid);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00151b90, 0x70)
i32 CGameObject::ResolveLinkedObject(b32 gate) {
    if (gate == false) {
        return 0;
    }
    CWwdGameObject* found;
    if (m_carrierId != 0) {
        if (LookupLinkedObject(
                OwnerMgr()->m_childGroup->m_registeredGameObjectsById,
                m_carrierId,
                found
            )
            != false) {
            m_carrier = found;
            return 1;
        }
        m_carrier = NULL;
        return 1;
    }
    m_carrier = NULL;
    return 1;
}

RVA(0x00151c00, 0x118)
i32 CGameObject::WriteSnapshot(CFileMemBase* dst, LogicTypeId unused) {
    CFileMemBase* ar = dst;
    if (ar == NULL) {
        return 0;
    }
    CLogicRecord* record = m_logicRecord;
    if (record == NULL) {
        return 0;
    }
    if (record->m_eventCode == 0) {
        record->m_dispatch(this);
    }

    i32 serialTypeId = 0;

    if (this->GetClassId() == CLASSID_CALLBACKOBJ) {
        serialTypeId = static_cast<CWwdGameObjectSerial*>(this)->GetSerialTypeId();
    }

    record = m_logicRecord;
    CUserLogic* logic = record->m_userLogic;
    LogicTypeId logicTypeId = LOGIC_UNSET;
    if (logic != NULL) {
        logicTypeId = logic->GetTypeTag();
    }

    WwdSnapshot snapshot;
    snapshot.m_id = m_id;
    snapshot.m_classId = this->GetClassId();
    snapshot.m_objectId = m_objectId;
    snapshot.m_screenPosition = ScreenPos();
    snapshot.m_sortKey = m_sortKey;
    snapshot.m_serialTypeId = serialTypeId;
    snapshot.m_logicTypeId = logicTypeId;

    {
        strcpy(
            snapshot.m_logicTypeName,
            OwnerMgr()->m_logicRegistry->FindLogicTypeKey(m_logicRecord)
        );
    }
    ar->Write(&snapshot, sizeof(snapshot));
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00151d20, 0x3a)
i32 CGameObject::NotifyForEventCode(i32 eventCode) {
    return NotifyLogicForEventCode(this, eventCode);
}

RVA(0x00151d60, 0xb)
i32 CLogicRecord::IsLoaded() {
    return m_dispatch != NULL;
}

RVA(0x00151d70, 0x6)
LoadableClassId CLogicRecord::GetClassId() {
    return CLASSID_LOGICRECORD;
}

RVA_COMPGEN(0x00151d80, 0x1e, ??_GCLogicRecord@@UAEPAXI@Z)

RVA(0x00151da0, 0x80)
CLogicRecord::~CLogicRecord() {
    m_dispatch = NULL;
    if (m_payload) {
        delete[] m_payload;
        m_payload = NULL;
        m_payloadSize = 0;
    }
    if (m_userLogic) {
        delete m_userLogic;
        m_userLogic = NULL;
    }
    m_target = NULL;
}

RVA(0x00151e20, 0x46)
i32 CLogicRecord::Init(LogicRecordDispatchFn dispatch, i32 flags) {
    if (dispatch == NULL) {
        return 0;
    }
    m_dispatch = dispatch;
    m_flags = flags;
    m_payload = NULL;
    m_userLogic = NULL;
    m_timeDelay = 0;
    m_frameDelay = 0;
    m_minX = 0;
    m_minY = 0;
    m_maxX = 0;
    m_maxY = 0;
    m_positionedSound = NULL;
    m_reserved16c = 0;
    m_userFlags = 0;
    return 1;
}

RVA(0x00151e70, 0x3b)
void CLogicRecord::Unload() {
    m_dispatch = NULL;
    if (m_payload) {
        delete[] m_payload;
        m_payload = NULL;
        m_payloadSize = 0;
    }
    if (m_userLogic) {
        delete m_userLogic;
        m_userLogic = NULL;
    }
    m_target = NULL;
}

RVA(0x00151eb0, 0x43)
void CDDrawWorker::Unload() {
    for (i32 i = 0; i < m_items.GetSize(); i++) {
        CImage* el = static_cast<CImage*>(m_items.GetAt(i));
        if (el != NULL) {
            delete el;
        }
    }
    m_items.SetSize(0, -1);

    m_minIndex = 99999;
    m_maxIndex = 0;
}

#define ADD_FRAME_AT(elem, index)                                                                  \
    m_items.SetAtGrow(index, elem);                                                                \
    if (index < m_minIndex) {                                                                      \
        m_minIndex = index;                                                                        \
    }                                                                                              \
    if (index > m_maxIndex) {                                                                      \
        m_maxIndex = index;                                                                        \
    }

RVA(0x00151f00, 0xa4)
CImage* CDDrawWorker::InsertFrame(CRezItm* src, i32 n, i32 mode) {
    if (n < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(n)) != NULL) {
        return NULL;
    }

    CImage* worker = new CImage(n, Owner());
    if (!worker->Resolve(src, mode)) {
        if (worker) {
            delete worker;
        }
        return NULL;
    }
    ADD_FRAME_AT(static_cast<CObject*>(worker), n)
    return worker;
}

RVA(0x00151fb0, 0xa4)
CImage* CDDrawWorker::LoadFrame(char* path, i32 index, i32 keyed) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return NULL;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->Create(path, keyed) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return NULL;
    }

    ADD_FRAME_AT(static_cast<CObject*>(nf), index)
    return nf;
}

RVA(0x00152060, 0xab)
CImage*
CDDrawWorker::CreateDescriptorFrame(PidHeader* desc, FileImageFormat mode, i32 index, u32 size) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return NULL;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->LoadDispatch(desc, mode, size, 1) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return NULL;
    }

    ADD_FRAME_AT(static_cast<CObject*>(nf), index)
    return nf;
}

RVA(0x00152110, 0xa9)
CImage* CDDrawWorker::CreateBlankFrame(i32 width, i32 height, i32 index, i32 keyed) {
    if (index < m_items.GetSize() && static_cast<CImage*>(m_items.GetAt(index)) != NULL) {
        return NULL;
    }

    CImage* nf = new CImage(index, Owner());

    if (nf->CreateBlankSurface(width, height, keyed) == 0) {
        if (nf != NULL) {
            delete nf;
        }
        return NULL;
    }

    ADD_FRAME_AT(static_cast<CObject*>(nf), index)
    return nf;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001521c0, 0x2b)
void CDDrawWorker::AddFrameAt(CObject* elem, i32 index){ADD_FRAME_AT(elem, index)}

RVA(0x001521f0, 0xbc)
i32 CDDrawWorker::BuildFramesFromArchive(CRezDir* tab) {
    i32 count = 0;
    CRezTyp* sym = tab->GetFirstType();
    while (sym != NULL) {
        CRezItm* val = tab->GetFirstItem(sym);
        while (val != NULL) {
            char* p = val->GetName();
            while (*p != 0) {
                if (*p >= '0' && *p <= '9') {
                    break;
                }
                p++;
            }
            i32 fi = atoi(p);
            if (InsertFrame(val, fi, 1) != NULL) {
                count++;
            }
            val = tab->GetNextItem(val);
            if ((OwnerMgr()->m_flags & SURFACEMGR_SINGLE_FRAME_WORKERS) && count > 0) {
                val = NULL;
            }
        }
        sym = tab->GetNextType(sym);
        if ((OwnerMgr()->m_flags & SURFACEMGR_SINGLE_FRAME_WORKERS) && count > 0) {
            sym = NULL;
        }
    }
    return count;
}

RVA(0x001522b0, 0xf7)
i32 CDDrawWorker::ValidateFramesFromArchive(CRezDir* tab) {

    i32 matched = 0;
    i32 liveFrames = 0;
    i32 n = m_items.GetSize();
    for (i32 i = 0; i < n; i++) {
        CImage* el;
        if (DDRAW_WORKER_FRAME_IN_RANGE(this, i)) {
            el = DDRAW_WORKER_FRAME_AT_UNCHECKED(this, i);
        } else {
            el = NULL;
        }
        if (el != NULL) {
            liveFrames++;
        }
    }
    CRezTyp* sym = tab->GetFirstType();
    while (sym != NULL) {
        CRezItm* val = tab->GetFirstItem(sym);
        while (val != NULL) {
            GZ_ENUM_RETURN(RezTypeTag, u32)
            tag = (static_cast<CRezItm*>(val))->GetType();
            if (tag == IMGTAG_XCP || tag == IMGTAG_PMB || tag == IMGTAG_DIR || tag == IMGTAG_DIP) {
                char* p = val->GetName();
                while (*p != 0) {
                    if (*p >= '0' && *p <= '9') {
                        break;
                    }
                    p++;
                }
                i32 fi = atoi(p);
                if (0 == ReloadFrame(static_cast<CRezItm*>(val), fi, 1)) {
                    return -1;
                }
                matched++;
            }
            val = tab->GetNextItem(val);
        }
        sym = tab->GetNextType(sym);
    }
    return (matched >= liveFrames) ? matched : -1;
}

RVA(0x001523b0, 0x3b)
i32 CDDrawWorker::ReloadFrame(CRezItm* rec, i32 n, i32 flag) {
    CImage* el = GetAt(n);
    if (el == NULL) {
        return 0;
    }
    return el->Reload(rec, flag) != 0;
}

RVA(0x001523f0, 0x82)
i32 CDDrawWorker::GetMemoryUsage(i32 raw) {
    i32 sum = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame) {
            i32 size = frame->m_height * frame->m_width;
            if (frame->m_surface && frame->m_surface->m_bitDepth == BPP_RGB_16) {
                size += size;
            }
            if (frame->m_surface && frame->m_surface->m_bitDepth == BPP_RGB_24) {
                size = size * 3;
            }
            if (frame->m_owned) {
                size = frame->m_owned->m_rleLen;
            }
            if (raw == 0) {
                size += 0x34;
            }
            sum += size;
        }
    }
    return sum;
}

RVA(0x00152480, 0x4e)
i32 CDDrawWorker::SetAllTypes(ShadeMode type) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->Select(type, NULL);
            count++;
        }
    }
    return count;
}

RVA(0x001524d0, 0x41)
i32 CDDrawWorker::SetAllLightLevels(i32 value) {
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_light = value;
            count++;
        }
    }
    return count;
}

RVA(0x00152520, 0x4b)
i32 CDDrawWorker::SetAllFormats(CShadeTable* format) {
    if (!format) {
        return 0;
    }
    i32 count = 0;
    for (i32 i = m_minIndex; i <= m_maxIndex; i++) {
        CImage* frame = GetAt(i);
        if (frame && frame->m_owned) {
            frame->m_owned->m_palDescr = format;
            count++;
        }
    }
    return count;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00152570, 0x24)
ShadeMode CDDrawWorker::GetFirstFrameState() {
    CImage* frame = static_cast<CImage*>(m_items.GetAt(m_minIndex));
    if (frame == NULL) {
        return SHADE_COPY;
    }
    CDDrawShadeBlit* fmt = frame->m_owned;
    if (fmt == NULL) {
        return SHADE_COPY;
    }
    return fmt->m_drawType;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001525a0, 0x1f)
i32 CDDrawWorker::GetFirstFrameLightLevel() {
    CImage* frame = static_cast<CImage*>(m_items.GetAt(m_minIndex));
    if (frame == NULL) {
        return 1;
    }
    CDDrawShadeBlit* fmt = frame->m_owned;
    if (fmt == NULL) {
        return 0;
    }
    return fmt->m_light;
}

RVA(0x001525c0, 0x76)
i32 CDDrawWorker::FindFrame(CImage* frame, char* outName, i32* outIndex) {
    if (frame) {
        for (i32 i = 0; i < m_items.GetSize(); i++) {
            CImage* cur = static_cast<CImage*>(m_items.GetAt(i));
            if (cur && cur == frame) {
                if (outName) {
                    strcpy(outName, m_name);
                }
                if (outIndex) {
                    *outIndex = i;
                }
                return 1;
            }
        }
    }
    return 0;
}
