#include <StdAfx.h>

#include <rva.h>

#include <Wwd/WwdFactoryObject.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordCtorInline.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AniElementInline.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/WapObj.h>
#include <Wwd/LogicRecordEvent.h>
#include <Wwd/WwdAnimStepMode.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgr.h>

#include <string.h>

namespace {
#include <Gruntz/GameRand.h>
}

RVA(0x0015b340, 0x2b)
i32 CLogicRecord::Consume(i32 amount) {
    i32 remaining = m_timeDelay;
    if (remaining == 0) {
        return remaining;
    }
    if (static_cast<u32>(amount) >= static_cast<u32>(remaining)) {
        m_timeDelay = 0;
        return 0;
    }
    m_timeDelay = remaining - amount;
    return 1;
}

RVA(0x0015b370, 0x1d)
i32 CGameObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015b390, 0x128)
CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
    : CRenderState(owner, id, objectFlags, CRenderState::INLINE_SEED),
      m_region(WwdRegion::INLINE_SEED),
      m_shadow(WwdDirtyRect::INLINE_SEED) {
    AttachToOwner(owner, id);
}

RVA_COMPGEN(0x0015b4c0, 0x1e, ??_GCGameObject@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b4f0, 0xde, ??1CGameObject@@UAE@XZ)

RVA(0x0015b650, 0x4d)
void CGameObject::Notify(CGameObject* p) {
    if (m_flags & IDX(WWD_GAME_OBJECT_FLAG_DAMAGE_HEALTH_DIRECTLY)) {
        m_health -= p->GetDamage();
        if (m_health <= 0) {
            m_logicRecord->SetLogicEvent(ACT_HEALTH_DEPLETED);
        }
    } else {
        CLogicRecord* h = m_hitLogic;
        if (h != NULL) {
            m_hitSource = p;
            h->Dispatch(this);
        }
    }
}

RVA(0x0015b6a0, 0xb)
i32 CAniAdvanceCursor::IsLoaded() {
    return m_boundObject != NULL;
}

RVA_COMPGEN(0x0015b6b0, 0x1e, ??_GCAniAdvanceCursor@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b6d0, 0x5b, ??1CAniAdvanceCursor@@UAE@XZ)

RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
    m_boundObject = NULL;
    m_animation = NULL;
    m_currentRecord = NULL;
}

RVA(0x0015b760, 0x6)
LoadableClassId CWwdSpriteObject::GetClassId() {
    return CLASSID_WWD_SPRITE_OBJECT;
}

RVA_COMPGEN(0x0015b770, 0x1e, ??_GCWwdSpriteObject@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b790, 0x1a6, ??1CWwdSpriteObject@@UAE@XZ)

RVA(0x0015b940, 0x38)
i32 CWwdSpriteObject::Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) {
    m_soundCue = NULL;
    m_animationCursor.BindSprite(this);
    return CGameObject::Setup(x, y, sortKey, logicTemplate);
}

RVA(0x0015ba20, 0x1c)
void CWwdSpriteObject::Render(CRenderBuffer* pair) {
    if (m_frameImage) {
        m_frameImage->RenderImage(this, pair);
    }
}

RVA(0x0015ba40, 0x1d)
i32 CWwdDeferredObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015ba60, 0x6)
LoadableClassId CWwdDeferredObject::GetClassId() {
    return CLASSID_WWD_DEFERRED_OBJECT;
}

RVA(0x0015ba70, 0x3)
void CWwdDeferredObject::Render(CRenderBuffer*) {}

RVA(0x0015ba80, 0x3)
void CWwdDeferredObject::BltDirty(CRenderBuffer*, CRenderBuffer*) {}

RVA(0x0015ba90, 0x3)
void CWwdDeferredObject::BltDirtyEx(CRenderSurface*, CRenderBuffer*, CRenderBuffer*) {}

RVA(0x0015baa0, 0x3)
void CWwdDeferredObject::BltDirtyRegions(CRenderBuffer*, CRenderBuffer*, CRenderBuffer*) {}

RVA_COMPGEN(0x0015bab0, 0x1e, ??_GCWwdDeferredObject@@UAEPAXI@Z)
RVA(0x0015bad0, 0x153)
CWwdDeferredObject::~CWwdDeferredObject() {
    Unload();
}

RVA(0x0015bc30, 0x16)
i32 CWwdDeferredObject::SetupDeferred(i32 sortKey, CLogicRecord* logicTemplate) {
    return CGameObject::Setup(0, 0, sortKey, logicTemplate);
}

RVA(0x0015bcd0, 0xb)
i32 CWwdGameObject::IsLoaded() {
    return m_logicRecord != NULL;
}

RVA(0x0015bce0, 0x6)
LoadableClassId CWwdGameObject::GetClassId() {
    return CLASSID_WWD_CONTAINER_OBJECT;
}

RVA_COMPGEN(0x0015bcf0, 0x1e, ??_GCWwdGameObject@@UAEPAXI@Z)
// @early-stop
RVA(0x0015bd10, 0x1ef)
CWwdGameObject::~CWwdGameObject() {
    Unload();
}

#include <Wwd/WwdRectOverlapInline.h>

RVA(0x0015bfb0, 0x4a)
i32 CDDrawChildGroup::RectsOverlap(RECT* a, RECT* b) {
    return CDDrawRectsOverlap(a, b);
}

RVA(0x0015c000, 0x1d)
i32 CWwdDotObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015c020, 0x6)
LoadableClassId CWwdDotObject::GetClassId() {
    return CLASSID_WWD_DOT_OBJECT;
}

RVA(0x0015c030, 0x7)
u8 CWwdDotObject::GetDotColor() {
    return m_dotColor;
}

RVA(0x0015c040, 0xd)
void CWwdDotObject::SetDotColor(u8 dotColor) {
    m_dotColor = dotColor;
}

RVA_COMPGEN(0x0015c050, 0x1e, ??_GCWwdDotObject@@UAEPAXI@Z)
RVA(0x0015c070, 0x159)
CWwdDotObject::~CWwdDotObject() {
    Unload();
}

RVA(0x0015c1d0, 0x26)
i32 CWwdDotObject::SetupDot(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate, i32 dotColor) {
    m_dotColor = static_cast<u8>(dotColor);
    return CGameObject::Setup(x, y, sortKey, logicTemplate);
}

// @early-stop
RVA(0x0015c290, 0x2f)
void CAniAdvanceCursor::BindSprite(CWwdSpriteObject* sprite) {
    m_boundObject = sprite;
    m_finished = true;
    m_animation = NULL;
    m_durationScale = 1.0f;
    m_consumeEvents =
        HAS(static_cast<DDrawSurfaceMgrFlags>(sprite->OwnerMgr()->m_flags),
            SURFACEMGR_CONSUME_ANIMATION_EVENTS);
    m_useElapsedTime = true;
}

RVA(0x0015c2c0, 0xc)
void CAniAdvanceCursor::Unload() {
    m_boundObject = NULL;
    m_animation = NULL;
    m_currentRecord = NULL;
}

RVA(0x0015c2d0, 0x45)
void CAniAdvanceCursor::SetAnimation(CAnimationSequence* animation) {
    CAniFrameRecord* firstRecord;
    i32 eventCode;
    m_animation = animation;
    if (!animation) {
        return;
    }
    m_recordIndex = 0;
    firstRecord = animation->RecordAt(0);
    m_currentRecord = firstRecord;
    m_recordDurationRemaining = 0;
    m_finished = false;
    eventCode = firstRecord->m_eventCode;
    m_pendingEventCode = eventCode;
    m_currentEventCode = eventCode;
    {
        float durationScale = animation->m_durationScale;
        m_durationScale = durationScale;
    }
}

RVA(0x0015c320, 0x40)

void CAniAdvanceCursor::RestartAnimation(i32 resetElapsedTime) {
    CAnimationSequence* animation = m_animation;
    if (animation == NULL) {
        return;
    }
    m_recordIndex = 0;
    CAniFrameRecord* firstRecord;
    firstRecord = animation->RecordAt(0);
    m_currentRecord = firstRecord;
    m_finished = false;
    i32 eventCode = firstRecord->m_eventCode;
    m_durationScale = 1.0f;
    m_pendingEventCode = eventCode;
    m_currentEventCode = eventCode;
    if (resetElapsedTime != 0) {
        m_recordDurationRemaining = 0;
    }
}

RVA(0x0015c360, 0x59c)
i32 CAniAdvanceCursor::Advance(u32 elapsedMs) {
    if (m_animation == NULL) {
        return -1;
    }

    if (m_recordDurationRemaining > 0) {
        if (m_useElapsedTime != false) {
            if (elapsedMs >= m_recordDurationRemaining) {
                m_recordDurationRemaining = 0;
                m_currentEventCode = m_pendingEventCode;
            } else {
                m_recordDurationRemaining -= elapsedMs;
                return m_currentEventCode;
            }
        } else {
            m_recordDurationRemaining -= 1;
            return m_currentEventCode;
        }
    } else {
        m_currentEventCode = m_pendingEventCode;
    }

    if (m_finished == false) {
        CWwdSpriteObject* boundSprite = m_boundObject;
        CAniFrameRecord* stepRecord = m_currentRecord;

        switch (stepRecord->m_stepMode) {
            case WWDSTEP_NEXT: {
                CWwdSpriteObject* sprite = m_boundObject;
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                sprite->m_frameIndex = sprite->m_frameIndex + 1;
                sprite->m_frameImage = imageSet->GetFrame(sprite->m_frameIndex);
                if (sprite->GetFrameImage() == NULL) {
                    i32 firstFrameIndex = sprite->GetImageSet()->GetMinIndex();
                    sprite->m_frameIndex = firstFrameIndex;
                    sprite->m_frameImage = sprite->GetImageSet()->GetFrame(firstFrameIndex);
                }
                break;
            }
            case WWDSTEP_PREV: {
                CWwdSpriteObject* sprite = m_boundObject;
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                i32 frameIndex = sprite->m_frameIndex;
                if (frameIndex == imageSet->GetMinIndex()) {
                    sprite->m_frameIndex = imageSet->GetMaxIndex();
                } else {
                    sprite->m_frameIndex = frameIndex - 1;
                }
                sprite->m_frameImage = imageSet->GetFrame(sprite->m_frameIndex);
                break;
            }
            case WWDSTEP_SET: {
                CWwdSpriteObject* sprite = m_boundObject;
                i32 frameIndex = stepRecord->GetFrameParameter();
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                sprite->m_frameImage = imageSet->GetFrame(frameIndex);
                sprite->m_frameIndex = frameIndex;
                break;
            }
            case WWDSTEP_FIRST: {
                CWwdSpriteObject* sprite = m_boundObject;
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                i32 firstFrameIndex = imageSet->GetMinIndex();
                sprite->m_frameIndex = firstFrameIndex;
                sprite->m_frameImage = imageSet->GetFrame(firstFrameIndex);
                break;
            }
            case WWDSTEP_LAST: {
                CWwdSpriteObject* sprite = m_boundObject;
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                i32 lastFrameIndex = imageSet->GetMaxIndex();
                sprite->m_frameIndex = lastFrameIndex;
                sprite->m_frameImage = imageSet->GetFrame(lastFrameIndex);
                break;
            }
            case WWDSTEP_FORWARD_BY: {
                CWwdSpriteObject* sprite = m_boundObject;
                i32 frameOffset = stepRecord->GetFrameParameter();
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                sprite->m_frameIndex = sprite->m_frameIndex + frameOffset;
                sprite->m_frameImage = imageSet->GetFrame(sprite->m_frameIndex);
                if (sprite->GetFrameImage() == NULL) {
                    sprite->ClampToLastFrame();
                }
                break;
            }
            case WWDSTEP_BACK_BY: {
                CWwdSpriteObject* sprite = m_boundObject;
                i32 frameOffset = stepRecord->GetFrameParameter();
                CImageSet* imageSet = sprite->GetImageSet();
                if (imageSet == NULL) {
                    break;
                }
                sprite->m_frameIndex = sprite->m_frameIndex - frameOffset;
                sprite->m_frameImage = imageSet->GetFrame(sprite->m_frameIndex);
                if (sprite->GetFrameImage() == NULL) {
                    sprite->ClampToFirstFrame();
                }
                break;
            }
            default:
                break;
        }

        boundSprite = m_boundObject;
        boundSprite->m_imageOffsetX = 0;
        boundSprite->m_imageOffsetY = 0;
        switch (m_currentRecord->m_positionMode) {
            case WWDPOS_PLOT_OFFSET: {
                CAniFrameRecord* positionRecord = m_currentRecord;
                CWwdSpriteObject* sprite = m_boundObject;
                sprite->m_imageOffsetX = positionRecord->m_positionParameterX;
                sprite->m_imageOffsetY = positionRecord->m_positionParameterY;
                break;
            }
            case WWDPOS_MOVE_RELATIVE: {
                CAniFrameRecord* positionRecord = m_currentRecord;
                CWwdSpriteObject* sprite = m_boundObject;
                i32 x = sprite->m_screenX;
                i32 dy = positionRecord->m_positionParameterY;
                i32 dx = positionRecord->m_positionParameterX;
                if (HAS(sprite->m_stateFlags, SPRITE_STATE_MIRROR_X)) {
                    sprite->m_screenX = x - dx;
                } else {
                    sprite->m_screenX = x + dx;
                }
                sprite->m_screenY = sprite->m_screenY + dy;
                break;
            }
            case WWDPOS_MOVE_ABSOLUTE:
                SET_SCREEN_POS(
                    m_boundObject,
                    m_currentRecord->m_positionParameterX,
                    m_currentRecord->m_positionParameterY
                );
                break;
            default:
                break;
        }

        CWwdSpriteObject* sprite = m_boundObject;
        b32 shouldPlayCue = true;
        if (HAS(static_cast<WwdGameObjectFlags>(sprite->m_flags),
                WWD_GAME_OBJECT_FLAG_CULL_SOUND_WHEN_NOT_DRAWN)
            || HAS(m_currentRecord->m_flags, ANI_RECORD_FLAG_CULL_CUE_WHEN_NOT_DRAWN)) {
            if (!sprite->m_dirty.IsValid()) {
                shouldPlayCue = false;
            }
        }
        if (shouldPlayCue) {
            CAniFrameRecord* cueRecord = m_currentRecord;
            if (HAS(cueRecord->m_flags, ANI_RECORD_FLAG_POSITIONAL_CUE)) {
                i32 sourceX = sprite->m_screenX;
                SoundCue* soundCue = cueRecord->PickCue();
                if (soundCue != NULL) {
                    soundCue->PlaySpatialized(sourceX, 0, 0, 0);
                }
            } else {
                SoundCue* soundCue = cueRecord->PickCue();
                if (soundCue != NULL) {
                    soundCue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                }
            }
        }

        CAniFrameRecord* timingRecord = m_currentRecord;
        i32 recordDuration = timingRecord->m_duration;
        m_recordDurationRemaining = recordDuration;
        m_useElapsedTime =
            static_cast<u8>(!HAS(timingRecord->m_flags, ANI_RECORD_FLAG_FRAME_COUNT));

        if (m_durationScaleBits != ANI_DURATION_SCALE_ONE_BITS) {
            m_recordDurationRemaining = static_cast<i32>(
                (static_cast<double>(static_cast<u32>(recordDuration)) * m_durationScale)
            );
        }

        i32 loopModeWord = IDX(timingRecord->m_loopMode);
        switch (static_cast<WwdAnimLoopMode>(loopModeWord & 0xffff)) {
            case WWDLOOP_FINISH:
                m_finished = true;
                break;
            case WWDLOOP_RESET_ANIMATION: {
                if (m_animation != NULL) {
                    m_recordIndex = 0;
                    m_currentRecord = static_cast<CAniFrameRecord*>(m_animation->AtChecked(0));
                    m_finished = false;
                    m_durationScale = 1.0f;
                    m_currentEventCode = m_pendingEventCode = m_currentRecord->m_eventCode;
                }
                break;
            }
            case WWDLOOP_RESTART_AT_SECOND: {
                m_recordIndex = 1;
                m_currentRecord = static_cast<CAniFrameRecord*>(m_animation->AtChecked(1));
                if (m_currentRecord == NULL) {
                    m_recordIndex = 0;
                    m_currentRecord = static_cast<CAniFrameRecord*>(m_animation->AtChecked(0));
                }
                if (m_currentRecord != NULL) {
                    m_finished = false;
                    m_recordDurationRemaining = 0;
                    m_currentEventCode = m_pendingEventCode;
                    m_pendingEventCode = m_currentRecord->m_eventCode;
                }
                break;
            }
            case WWDLOOP_AT_PARAM: {
                if (m_currentRecord->m_frameParameter == m_boundObject->m_frameIndex) {
                    if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                        m_recordIndex = m_recordIndex + 1;
                        m_currentRecord =
                            static_cast<CAniFrameRecord*>(m_animation->AtChecked(m_recordIndex));
                        if (m_currentRecord == NULL) {
                            m_recordIndex = 0;
                            m_currentRecord =
                                static_cast<CAniFrameRecord*>(m_animation->AtChecked(0));
                        }
                        if (m_currentRecord != NULL) {
                            m_currentEventCode = m_pendingEventCode;
                            m_pendingEventCode = m_currentRecord->m_eventCode;
                        }
                    }
                }
                break;
            }
            case WWDLOOP_AT_FIRST: {
                CWwdSpriteObject* loopSprite = m_boundObject;
                CImageSet* imageSet = loopSprite->GetImageSet();
                if (loopSprite->m_frameIndex == imageSet->GetMinIndex()) {
                    if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                        AdvanceToNextRecord();
                    }
                }
                break;
            }
            case WWDLOOP_AT_LAST: {
                CWwdSpriteObject* loopSprite = m_boundObject;
                CImageSet* imageSet = loopSprite->GetImageSet();
                if (loopSprite->m_frameIndex == imageSet->GetMaxIndex()) {
                    if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                        AdvanceToNextRecord();
                    }
                }
                break;
            }
            case WWDLOOP_AFTER_FIRST: {
                CWwdSpriteObject* loopSprite = m_boundObject;
                CImageSet* imageSet = loopSprite->GetImageSet();
                if (loopSprite->m_frameIndex == imageSet->GetMinIndex() + 1) {
                    if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                        AdvanceToNextRecord();
                    }
                }
                break;
            }
            case WWDLOOP_NEXT:
                if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                    AdvanceToNextRecord();
                }
                break;
            case WWDLOOP_BEFORE_LAST: {
                CWwdSpriteObject* loopSprite = m_boundObject;
                CImageSet* imageSet = loopSprite->GetImageSet();
                if (loopSprite->m_frameIndex == imageSet->GetMaxIndex() - 1) {
                    if (timingRecord->m_loopMode != WWDLOOP_FINISH) {
                        CAnimationSequence* animation = m_animation;
                        m_recordIndex = m_recordIndex + 1;
                        CAniFrameRecord* nextRecord = animation->RecordAt(m_recordIndex);
                        m_currentRecord = nextRecord;
                        if (nextRecord == NULL) {
                            m_recordIndex = 0;
                            m_currentRecord = animation->RecordAt(0);
                        }
                        if (m_currentRecord != NULL) {
                            m_currentEventCode = m_pendingEventCode;
                            m_pendingEventCode = m_currentRecord->m_eventCode;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    if (m_consumeEvents != 0) {
        if (m_recordDurationRemaining > 0) {
            i32 eventCode = m_currentEventCode;
            m_currentEventCode = 0;
            return eventCode;
        }
        i32 eventCode = m_pendingEventCode;
        m_pendingEventCode = 0;
        return eventCode;
    }
    if (m_recordDurationRemaining > 0) {
        return m_currentEventCode;
    }
    return m_pendingEventCode;
}

RVA(0x0015c900, 0x5c)
i32 CAniAdvanceCursor::SerializeDispatch(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    if (archive == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_PRESAVE:
            return 1;
        case SERIAL_SAVE:
            if (Serialize(archive) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTSAVE:
            return 1;
        case SERIAL_PRELOAD:
            return 1;
        case SERIAL_LOAD:
            if (Deserialize(archive) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            return 1;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015c960, 0xe)
i32 CAniAdvanceCursor::CanSerialize(CFileMemBase* archive) {
    return archive != NULL;
}

RVA(0x0015c970, 0xfe)
i32 CAniAdvanceCursor::Serialize(CFileMemBase* archive) {
    if (archive == NULL) {
        return 0;
    }
    archive->Write(&m_recordIndex, sizeof(m_recordIndex));
    archive->Write(&m_recordDurationRemaining, sizeof(m_recordDurationRemaining));
    archive->Write(&m_useElapsedTime, sizeof(m_useElapsedTime));
    archive->Write(&m_finished, sizeof(m_finished));
    archive->Write(&m_consumeEvents, sizeof(m_consumeEvents));
    archive->Write(&m_pendingEventCode, sizeof(m_pendingEventCode));
    archive->Write(&m_currentEventCode, sizeof(m_currentEventCode));
    archive->Write(&m_durationScale, sizeof(m_durationScale));
    char animationKey[SERIAL_NAME_LEN];
    memset(animationKey, 0, sizeof(animationKey));
    if (m_animation != NULL) {

        strcpy(animationKey, OwnerMgr()->GetAnimationRegistry()->FindAnimationKey(m_animation));
    }
    archive->Write(animationKey, SERIAL_NAME_LEN);
    return 1;
}

// @early-stop
RVA(0x0015ca70, 0x15b)
i32 CAniAdvanceCursor::Deserialize(CFileMemBase* archive) {
    if (archive == NULL) {
        return 0;
    }
    archive->Read(&m_recordIndex, sizeof(m_recordIndex));
    archive->Read(&m_recordDurationRemaining, sizeof(m_recordDurationRemaining));
    archive->Read(&m_useElapsedTime, sizeof(m_useElapsedTime));
    archive->Read(&m_finished, sizeof(m_finished));
    archive->Read(&m_consumeEvents, sizeof(m_consumeEvents));
    archive->Read(&m_pendingEventCode, sizeof(m_pendingEventCode));
    archive->Read(&m_currentEventCode, sizeof(m_currentEventCode));
    archive->Read(&m_durationScale, sizeof(m_durationScale));
    char animationKey[SERIAL_NAME_LEN];
    archive->Read(animationKey, SERIAL_NAME_LEN);
    if (strlen(animationKey) == 0) {
        m_animation = NULL;
    } else {
        m_animation = MapFind<CAnimationSequence>(
            OwnerMgr()->GetAnimationRegistry()->m_animations,
            animationKey
        );
    }
    CAnimationSequence* animation = m_animation;
    if (animation != NULL) {
        CAniFrameRecord* record = animation->RecordAt(m_recordIndex);
        m_currentRecord = record;
        if (record == NULL) {
            m_recordIndex = 0;
            m_currentRecord = animation->RecordAt(0);
        }
        if (m_currentRecord != NULL) {
            m_finished = false;
            m_recordDurationRemaining = 0;
            m_currentEventCode = m_pendingEventCode;
            m_pendingEventCode = m_currentRecord->m_eventCode;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015cbd0, 0xe)
i32 CAniAdvanceCursor::CanDeserialize(CFileMemBase* archive) {
    return archive != NULL;
}

RVA(0x0015cbe0, 0x46)
i32 CAniFrameRecord::NextRandomValue() {
    return GetRandomNumber();
}

RVA(0x0015cc30, 0x1e)
CImage* CImageSet::GetFrame(i32 n) {
    return GetAt(n);
}

RVA(0x0015cc50, 0x38)
void CWwdSpriteObject::ClampToFirstFrame() {
    CImageSet* seq = m_imageSet;
    if (seq != NULL) {
        i32 n = seq->GetMinIndex();
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_frameImage = layer;
    }
}

RVA(0x0015cc90, 0x38)
void CWwdSpriteObject::ClampToLastFrame() {
    CImageSet* seq = m_imageSet;
    if (seq != NULL) {
        i32 n = seq->GetMaxIndex();
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_frameImage = layer;
    }
}
