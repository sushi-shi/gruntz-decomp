#include <rva.h>

#include <Wwd/WwdFactoryObject.h>

#include <Mfc.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/LogicRecordCtorInline.h>
#include <Gruntz/AniAdvanceCursor.h>
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

RVA(0x0015b620, 0x2b)
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

RVA(0x0015b650, 0x1d)
i32 CGameObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015b670, 0x128)
CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 objectFlags)
    : CResolveNode(owner, id, objectFlags, CResolveNode::INLINE_SEED),
      m_region(WwdRegion::INLINE_SEED),
      m_shadow(WwdDirtyRect::INLINE_SEED) {
    AttachToOwner(owner, id);
}

RVA_COMPGEN(0x0015b7a0, 0x1e, ??_GCGameObject@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b7d0, 0xde, ??1CGameObject@@UAE@XZ)

RVA(0x0015b930, 0x4d)
void CGameObject::Notify(CGameObject* p) {
    if (m_flags & IDX(WWD_GAME_OBJECT_FLAG_DAMAGE_HEALTH_DIRECTLY)) {
        m_health -= p->m_damage;
        if (m_health <= 0) {
            m_logicRecord->SetLogicEvent(ACT_HEALTH_DEPLETED);
        }
    } else {
        CLogicRecord* h = m_hitLogic;
        if (h != NULL) {
            m_hitSource = p;
            h->m_dispatch(this);
        }
    }
}

RVA(0x0015b980, 0xb)
i32 CAniAdvanceCursor::IsLoaded() {
    return m_boundObject != NULL;
}

RVA_COMPGEN(0x0015b990, 0x1e, ??_GCAniAdvanceCursor@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b9b0, 0x5b, ??1CAniAdvanceCursor@@UAE@XZ)

RVA(0x0015ba10, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
    m_boundObject = NULL;
    m_animation = NULL;
    m_element = NULL;
}

RVA(0x0015ba40, 0x6)
LoadableClassId CWwdSpriteObject::GetClassId() {
    return CLASSID_WWD_SPRITE_OBJECT;
}

RVA_COMPGEN(0x0015ba50, 0x1e, ??_GCWwdSpriteObject@@UAEPAXI@Z)

RVA_COMPGEN(0x0015ba70, 0x1a6, ??1CWwdSpriteObject@@UAE@XZ)

RVA(0x0015bc20, 0x38)
i32 CWwdSpriteObject::Setup(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate) {
    m_soundCue = NULL;
    m_animationCursor.BindSprite(this);
    return CGameObject::Setup(x, y, sortKey, logicTemplate);
}

RVA(0x0015bd00, 0x1c)
void CWwdSpriteObject::Render(CDDrawSurfacePair* pair) {
    if (m_frameImage) {
        m_frameImage->RenderImage(this, pair);
    }
}

RVA(0x0015bd20, 0x1d)
i32 CWwdDeferredObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015bd40, 0x6)
LoadableClassId CWwdDeferredObject::GetClassId() {
    return CLASSID_WWD_DEFERRED_OBJECT;
}

RVA(0x0015bd50, 0x3)
void CWwdDeferredObject::Render(CDDrawSurfacePair*) {}

RVA(0x0015bd60, 0x3)
void CWwdDeferredObject::BltDirty(CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA(0x0015bd70, 0x3)
void CWwdDeferredObject::BltDirtyEx(CDrawSubWorker*, CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA(0x0015bd80, 0x3)
void CWwdDeferredObject::BltDirtyRegions(
    CDDrawSurfacePair*,
    CDDrawSurfacePair*,
    CDDrawSurfacePair*
) {}

RVA_COMPGEN(0x0015bd90, 0x1e, ??_GCWwdDeferredObject@@UAEPAXI@Z)
RVA(0x0015bdb0, 0x153)
CWwdDeferredObject::~CWwdDeferredObject() {
    Unload();
}

RVA(0x0015bf10, 0x16)
i32 CWwdDeferredObject::SetupDeferred(i32 sortKey, CLogicRecord* logicTemplate) {
    return CGameObject::Setup(0, 0, sortKey, logicTemplate);
}

RVA(0x0015bfb0, 0xb)
i32 CWwdGameObject::IsLoaded() {
    return m_logicRecord != NULL;
}

RVA(0x0015bfc0, 0x6)
LoadableClassId CWwdGameObject::GetClassId() {
    return CLASSID_WWD_CONTAINER_OBJECT;
}

RVA_COMPGEN(0x0015bfd0, 0x1e, ??_GCWwdGameObject@@UAEPAXI@Z)
// @early-stop
RVA(0x0015bff0, 0x1ef)
CWwdGameObject::~CWwdGameObject() {
    Unload();
}

RVA(0x0015c290, 0x4a)
i32 CDDrawChildGroup::RectsOverlap(CDDrawRect* a, CDDrawRect* b) {
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    return a->bottom >= b->top;
}

RVA(0x0015c2e0, 0x1d)
i32 CWwdDotObject::IsLoaded() {
    if (m_logicRecord == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015c300, 0x6)
LoadableClassId CWwdDotObject::GetClassId() {
    return CLASSID_WWD_DOT_OBJECT;
}

RVA(0x0015c310, 0x7)
u8 CWwdDotObject::GetDotColor() {
    return m_dotColor;
}

RVA(0x0015c320, 0xd)
void CWwdDotObject::SetDotColor(u8 dotColor) {
    m_dotColor = dotColor;
}

RVA_COMPGEN(0x0015c330, 0x1e, ??_GCWwdDotObject@@UAEPAXI@Z)
RVA(0x0015c350, 0x159)
CWwdDotObject::~CWwdDotObject() {
    Unload();
}

RVA(0x0015c4b0, 0x26)
i32 CWwdDotObject::SetupDot(i32 x, i32 y, i32 sortKey, CLogicRecord* logicTemplate, i32 dotColor) {
    m_dotColor = static_cast<u8>(dotColor);
    return CGameObject::Setup(x, y, sortKey, logicTemplate);
}

// @early-stop
RVA(0x0015c570, 0x2f)
void CAniAdvanceCursor::BindSprite(CWwdSpriteObject* src) {
    m_boundObject = src;
    m_finished = true;
    m_animation = NULL;
    m_scale = 1.0f;
    m_consumeDraw = src->OwnerMgr()->m_flags & 0x40;
    m_useElapsedTime = true;
}

RVA(0x0015c5a0, 0xc)
void CAniAdvanceCursor::Unload() {
    m_boundObject = NULL;
    m_animation = NULL;
    m_element = NULL;
}

RVA(0x0015c5b0, 0x45)
void CAniAdvanceCursor::SetAnimation(CAniElement* src) {
    CAniRecordView* e;
    i32 v;
    m_animation = src;
    if (!src) {
        return;
    }
    m_index = 0;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniRecordView*>(src->m_records.GetAt(0));
    } else {
        e = NULL;
    }
    m_element = e;
    m_frameTicksLeft = 0;
    m_finished = false;
    v = e->m_drawValue;
    m_pendingDraw = v;
    m_curDraw = v;
    {
        float f = src->m_scale;
        m_scale = f;
    }
}

RVA(0x0015c600, 0x40)

void CAniAdvanceCursor::RestartAnimation(i32 resetElapsedTime) {
    CAniElement* src = m_animation;
    if (src == NULL) {
        return;
    }
    m_index = 0;
    CAniRecordView* e;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniRecordView*>(src->m_records.GetAt(0));
    } else {
        e = NULL;
    }
    m_element = e;
    m_finished = false;
    i32 v = e->m_drawValue;
    m_scale = 1.0f;
    m_pendingDraw = v;
    m_curDraw = v;
    if (resetElapsedTime != 0) {
        m_frameTicksLeft = 0;
    }
}

// @early-stop
RVA(0x0015c640, 0x59c)
i32 CAniAdvanceCursor::Advance(u32 elapsed) {
    if (m_animation == NULL) {
        return -1;
    }

    if (m_frameTicksLeft > 0) {
        if (m_useElapsedTime != false) {
            if (elapsed >= m_frameTicksLeft) {
                m_frameTicksLeft = 0;
                m_curDraw = m_pendingDraw;
            } else {
                m_frameTicksLeft -= elapsed;
                return m_curDraw;
            }
        } else {
            m_frameTicksLeft -= 1;
            return m_curDraw;
        }
    } else {
        m_curDraw = m_pendingDraw;
    }

    if (m_finished == false) {
        CWwdSpriteObject* ctx = m_boundObject;
        CAniRecordView* d = m_element;

        switch (d->m_stepMode) {
            case WWDSTEP_NEXT: {
                CWwdSpriteObject* c = m_boundObject;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex + 1;
                c->m_frameImage = seq->GetFrame(c->m_frameIndex);
                if (c->m_frameImage == NULL) {
                    i32 first = c->m_imageSet->m_minIndex;
                    c->m_frameIndex = first;
                    c->m_frameImage = c->m_imageSet->GetFrame(first);
                }
                break;
            }
            case WWDSTEP_PREV: {
                CWwdSpriteObject* c = m_boundObject;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                i32 idx = c->m_frameIndex;
                if (idx == seq->m_minIndex) {
                    c->m_frameIndex = seq->m_maxIndex;
                } else {
                    c->m_frameIndex = idx - 1;
                }
                c->m_frameImage = seq->GetFrame(c->m_frameIndex);
                break;
            }
            case WWDSTEP_SET: {
                CWwdSpriteObject* c = m_boundObject;
                i32 frame = d->m_param;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameImage = seq->GetFrame(frame);
                c->m_frameIndex = frame;
                break;
            }
            case WWDSTEP_FIRST: {
                CWwdSpriteObject* c = m_boundObject;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                i32 first = seq->m_minIndex;
                c->m_frameIndex = first;
                c->m_frameImage = seq->GetFrame(first);
                break;
            }
            case WWDSTEP_LAST: {
                CWwdSpriteObject* c = m_boundObject;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                i32 last = seq->m_maxIndex;
                c->m_frameIndex = last;
                c->m_frameImage = seq->GetFrame(last);
                break;
            }
            case WWDSTEP_FORWARD_BY: {
                CWwdSpriteObject* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex + step;
                c->m_frameImage = seq->GetFrame(c->m_frameIndex);
                if (c->m_frameImage == NULL) {
                    c->ClampToLastFrame();
                }
                break;
            }
            case WWDSTEP_BACK_BY: {
                CWwdSpriteObject* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_imageSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex - step;
                c->m_frameImage = seq->GetFrame(c->m_frameIndex);
                if (c->m_frameImage == NULL) {
                    c->ClampToFirstFrame();
                }
                break;
            }
            default:
                break;
        }

        ctx = m_boundObject;
        ctx->m_plotDX = 0;
        ctx->m_plotDY = 0;
        switch (m_element->m_positionMode) {
            case WWDPOS_PLOT_OFFSET: {
                CAniRecordView* pd = m_element;
                CWwdSpriteObject* c = m_boundObject;
                c->m_plotDX = pd->m_positionDeltaX;
                c->m_plotDY = pd->m_positionDeltaY;
                break;
            }
            case WWDPOS_MOVE_RELATIVE: {
                CAniRecordView* pd = m_element;
                CWwdSpriteObject* c = m_boundObject;
                i32 x = c->m_screenX;
                i32 dy = pd->m_positionDeltaY;
                i32 dx = pd->m_positionDeltaX;
                if (HAS(c->m_stateFlags, SPRITE_STATE_MIRROR_X)) {
                    c->m_screenX = x - dx;
                    c->m_screenY = c->m_screenY + dy;
                } else {
                    c->m_screenX = x + dx;
                    c->m_screenY = c->m_screenY + dy;
                }
                break;
            }
            case WWDPOS_MOVE_ABSOLUTE:
                m_boundObject->m_screenX = m_element->m_positionDeltaX;
                m_boundObject->m_screenY = m_element->m_positionDeltaY;
                break;
            default:
                break;
        }

        CWwdSpriteObject* c = m_boundObject;
        b32 shouldPlayCue = true;
        if (HAS(static_cast<WwdGameObjectFlags>(c->m_flags),
                WWD_GAME_OBJECT_FLAG_CULL_SOUND_WHEN_NOT_DRAWN)
            || HAS(m_element->m_flags, ANI_RECORD_FLAG_CULL_CUE_WHEN_NOT_DRAWN)) {
            if (c->m_dirty.m_armed == -1) {
                shouldPlayCue = false;
            }
        }
        if (shouldPlayCue) {
            CAniRecordView* dd = m_element;
            if (HAS(dd->m_flags, ANI_RECORD_FLAG_POSITIONAL_CUE)) {
                i32 sourceX = c->m_screenX;
                SoundCue* soundCue;
                if (dd->m_cueCount == 0) {
                    soundCue = NULL;
                } else {
                    soundCue = dd->m_cues[dd->Rng2Next() % dd->m_cueCount];
                }
                if (soundCue != NULL) {
                    soundCue->PlaySpatialized(sourceX, 0, 0, 0);
                }
            } else {
                SoundCue* soundCue;
                if (dd->m_cueCount == 0) {
                    soundCue = NULL;
                } else {
                    soundCue = dd->m_cues[dd->Rng2Next() % dd->m_cueCount];
                }
                if (soundCue != NULL) {
                    soundCue->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
                }
            }
        }

        CAniRecordView* rd = m_element;
        i32 reload = rd->m_duration;
        m_frameTicksLeft = reload;
        m_useElapsedTime = static_cast<u8>(!HAS(rd->m_flags, ANI_RECORD_FLAG_FRAME_COUNT));

        if (m_scaleBits != ANI_SCALE_ONE_BITS) {
            m_frameTicksLeft =
                static_cast<i32>((static_cast<double>(static_cast<u32>(reload)) * m_scale));
        }

        i32 modeWord = IDX(rd->m_loopMode);
        CAniElement* arr;
        i32 i;
        CAniRecordView* nd;
        switch (static_cast<WwdAnimLoopMode>(modeWord & 0xffff)) {
            case WWDLOOP_FINISH:
                m_finished = true;
                break;
            case WWDLOOP_RESET_ANIMATION: {
                if (m_animation != NULL) {
                    m_index = 0;
                    m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(0));
                    m_finished = false;
                    m_scale = 1.0f;
                    m_curDraw = m_pendingDraw = m_element->m_drawValue;
                }
                break;
            }
            case WWDLOOP_RESTART_AT_SECOND: {
                m_index = 1;
                m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(1));
                if (m_element == NULL) {
                    m_index = 0;
                    m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(0));
                }
                if (m_element != NULL) {
                    m_finished = false;
                    m_frameTicksLeft = 0;
                    m_curDraw = m_pendingDraw;
                    m_pendingDraw = m_element->m_drawValue;
                }
                break;
            }
            case WWDLOOP_AT_PARAM: {
                if (m_boundObject->m_frameIndex == m_element->m_param) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        m_index = m_index + 1;
                        m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(m_index));
                        if (m_element == NULL) {
                            m_index = 0;
                            m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(0));
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case WWDLOOP_AT_FIRST: {
                CWwdSpriteObject* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_imageSet;
                if (c2->m_frameIndex == seq->m_minIndex) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec =
                            static_cast<CAniRecordView*>(GetAniElementAt(anim, m_index));
                        m_element = rec;
                        if (rec == NULL) {
                            m_index = 0;
                            m_element = static_cast<CAniRecordView*>(anim->AtChecked(0));
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case WWDLOOP_AT_LAST: {
                CWwdSpriteObject* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_imageSet;
                if (c2->m_frameIndex == seq->m_maxIndex) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec =
                            static_cast<CAniRecordView*>(GetAniElementAt(anim, m_index));
                        m_element = rec;
                        if (rec == NULL) {
                            m_index = 0;
                            m_element = static_cast<CAniRecordView*>(anim->AtChecked(0));
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case WWDLOOP_AFTER_FIRST: {
                CWwdSpriteObject* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_imageSet;
                if (c2->m_frameIndex == seq->m_minIndex + 1) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec =
                            static_cast<CAniRecordView*>(GetAniElementAt(anim, m_index));
                        m_element = rec;
                        if (rec == NULL) {
                            m_index = 0;
                            m_element = static_cast<CAniRecordView*>(anim->AtChecked(0));
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case WWDLOOP_NEXT:
                if (rd->m_loopMode != WWDLOOP_FINISH) {
                    arr = m_animation;
                    m_index = m_index + 1;
                    nd = static_cast<CAniRecordView*>(GetAniElementAt(arr, m_index));
                    m_element = nd;
                    if (nd == NULL) {
                        m_index = 0;
                        m_element = static_cast<CAniRecordView*>(arr->AtChecked(0));
                    }
                    if (m_element != NULL) {
                        m_curDraw = m_pendingDraw;
                        m_pendingDraw = m_element->m_drawValue;
                    }
                }
                break;
            case WWDLOOP_BEFORE_LAST: {
                CWwdSpriteObject* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_imageSet;
                if (c2->m_frameIndex == seq->m_maxIndex - 1) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* a = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* p =
                            static_cast<CAniRecordView*>(GetAniElementAt(a, m_index));
                        m_element = p;
                        if (p == NULL) {
                            m_index = 0;
                            i32 cnt = a->m_records.GetSize();
                            CAniRecordView* first;
                            if (cnt > 0) {
                                first = static_cast<CAniRecordView*>(a->m_records.GetAt(0));
                            } else {
                                first = NULL;
                            }
                            m_element = first;
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    if (m_consumeDraw != 0) {
        if (m_frameTicksLeft > 0) {
            i32 r = m_curDraw;
            m_curDraw = 0;
            return r;
        }
        i32 r = m_pendingDraw;
        m_pendingDraw = 0;
        return r;
    }
    if (m_frameTicksLeft > 0) {
        return m_curDraw;
    }
    return m_pendingDraw;
}

RVA(0x0015cbe0, 0x5c)
i32 CAniAdvanceCursor::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* self
) {
    if (ar == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_PRESAVE:
            return 1;
        case SERIAL_SAVE:
            if (Serialize(ar) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTSAVE:
            return 1;
        case SERIAL_PRELOAD:
            return 1;
        case SERIAL_LOAD:
            if (Deserialize(ar) == 0) {
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
RVA(0x0015cc40, 0xe)
i32 CAniAdvanceCursor::CanSerialize(CFileMemBase* ar) {
    return ar != NULL;
}

RVA(0x0015cc50, 0xfe)
i32 CAniAdvanceCursor::Serialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_index, sizeof(m_index));
    ar->Write(&m_frameTicksLeft, sizeof(m_frameTicksLeft));
    ar->Write(&m_useElapsedTime, sizeof(m_useElapsedTime));
    ar->Write(&m_finished, sizeof(m_finished));
    ar->Write(&m_consumeDraw, sizeof(m_consumeDraw));
    ar->Write(&m_pendingDraw, sizeof(m_pendingDraw));
    ar->Write(&m_curDraw, sizeof(m_curDraw));
    ar->Write(&m_scale, sizeof(m_scale));
    char buf[SERIAL_NAME_LEN];
    memset(buf, 0, sizeof(buf));
    if (m_animation != NULL) {

        strcpy(buf, OwnerMgr()->m_animRegistry->FindAnimationKey(m_animation));
    }
    ar->Write(buf, SERIAL_NAME_LEN);
    return 1;
}

static inline CAniElement* LookupAnimation(CMapStringToPtr& map, LPCTSTR name) {
    CAniElement* result = NULL;
    MapLookup(map, name, result);
    return result;
}

static inline CAniRecordView* RecordAt(CAniElement* anim, i32 index) {
    CAniRecordView* rec;
    if (index >= 0 && index < anim->m_records.GetSize()) {
        rec = static_cast<CAniRecordView*>(anim->m_records.GetAt(index));
    } else {
        rec = NULL;
    }
    return rec;
}

// @early-stop
RVA(0x0015cd50, 0x15b)
i32 CAniAdvanceCursor::Deserialize(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_index, sizeof(m_index));
    ar->Read(&m_frameTicksLeft, sizeof(m_frameTicksLeft));
    ar->Read(&m_useElapsedTime, sizeof(m_useElapsedTime));
    ar->Read(&m_finished, sizeof(m_finished));
    ar->Read(&m_consumeDraw, sizeof(m_consumeDraw));
    ar->Read(&m_pendingDraw, sizeof(m_pendingDraw));
    ar->Read(&m_curDraw, sizeof(m_curDraw));
    ar->Read(&m_scale, sizeof(m_scale));
    char buf[SERIAL_NAME_LEN];
    ar->Read(buf, SERIAL_NAME_LEN);
    if (strlen(buf) == 0) {
        m_animation = NULL;
    } else {
        m_animation = LookupAnimation(OwnerMgr()->m_animRegistry->m_animations, buf);
    }
    CAniElement* w = m_animation;
    if (w != NULL) {
        CAniRecordView* e = static_cast<CAniRecordView*>(GetAniElementAt(w, m_index));
        m_element = e;
        if (e == NULL) {
            m_index = 0;
            m_element = RecordAt(w, 0);
        }
        if (m_element != NULL) {
            m_finished = false;
            m_frameTicksLeft = 0;
            m_curDraw = m_pendingDraw;
            m_pendingDraw = m_element->m_drawValue;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015ceb0, 0xe)
i32 CAniAdvanceCursor::CanDeserialize(CFileMemBase* ar) {
    return ar != NULL;
}

RVA(0x0015cec0, 0x46)
i32 CAniRecordView::Rng2Next() {
    return GetRandomNumber();
}

RVA(0x0015cf10, 0x1e)
CImage* CDDrawWorker::GetFrame(i32 n) {
    return GetAt(n);
}

RVA(0x0015cf30, 0x38)
void CWwdSpriteObject::ClampToFirstFrame() {
    CDDrawWorker* seq = m_imageSet;
    if (seq != NULL) {
        i32 n = seq->m_minIndex;
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_frameImage = layer;
    }
}

RVA(0x0015cf70, 0x38)
void CWwdSpriteObject::ClampToLastFrame() {
    CDDrawWorker* seq = m_imageSet;
    if (seq != NULL) {
        i32 n = seq->m_maxIndex;
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_frameImage = layer;
    }
}
