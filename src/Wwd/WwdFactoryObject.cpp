#include <rva.h>

#include <Wwd/WwdFactoryObject.h>

#include <Mfc.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/AnimWorkerObjCtorInline.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
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
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdAnimStepMode.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgr.h>

#include <string.h>

RVA(0x0015b340, 0x2b)
i32 AnimWorkerObj::Consume(i32 amount) {
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
    if (m_animWorker == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

// The pinned half of the two-entity split (docs/patterns/two-shapes-need-two-entities.md).
// Retail `call`s this from exactly three sites - CDDrawChildGroup::CreateContainerObject
// (through CWwdGameObject -> CWwdGameObjectA), CDDrawWorkerHost::ReadPlaneObjects and
// CWwdGameObject::CreateObject - and expands CGameObject(..., INLINE_BASE) in
// CreateSpriteObject / CreateDotObject / CreateDeferredObject.
// Retail's body calls neither 0x15b270 nor 0x15b2b0, so m_region and m_shadow take the
// INLINE_SEED siblings here; the expanded CGameObject(..., INLINE_BASE) leaves both as
// the `call`s the three factories show.
RVA(0x0015b390, 0x128)
CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CResolveNode(owner, id, stateFlags, CResolveNode::INLINE_SEED),
      m_region(WwdRegion::INLINE_SEED),
      m_shadow(WwdDirtyRect::INLINE_SEED) {
    AttachToOwner(owner, id);
}

RVA_COMPGEN(0x0015b4c0, 0x1e, ??_GCGameObject@@UAEPAXI@Z)
// The header's `~WwdRegion() {}` COMDAT - inlined away at every call site,
// but the unwind funclet for the member at +0x9c jumps to this address.

RVA_COMPGEN(0x0015b4f0, 0xde, ??1CGameObject@@UAE@XZ)

RVA(0x0015b650, 0x4d)
void CGameObject::Notify(CGameObject* p) {
    if (m_flags & 0x8) {
        m_health -= p->m_damage;
        if (m_health <= 0) {
            m_animWorker->SetWorkerAct(ACT_HEALTH_DEPLETED);
        }
    } else {
        AnimWorkerObj* h = m_hitWorker;
        if (h != NULL) {
            m_hitSource = p;
            h->m_notify(this);
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
CAniAdvanceCursor::CAniAdvanceCursor(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CWapObj(owner, field04, field08, CWapObj::NO_SEED) {
    m_boundObject = NULL;
    m_animation = NULL;
    m_element = NULL;
}

RVA(0x0015b760, 0x6)
LoadableClassId CWwdGameObjectA::GetClassId() {
    return CLASSID_WWDOBJA;
}

RVA_COMPGEN(0x0015b770, 0x1e, ??_GCWwdGameObjectA@@UAEPAXI@Z)

RVA_COMPGEN(0x0015b790, 0x1a6, ??1CWwdGameObjectA@@UAE@XZ)

RVA(0x0015b940, 0x38)
i32 CWwdGameObjectA::Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) {
    m_soundCue = NULL;
    m_animCursor.Construct(this);
    return CGameObject::Setup(x, y, sortKey, tmpl);
}

RVA(0x0015ba20, 0x1c)
void CWwdGameObjectA::Render(CDDrawSurfacePair* pair) {
    if (m_layer) {
        m_layer->RenderImage(this, pair);
    }
}

RVA(0x0015ba40, 0x1d)
i32 CWwdGameObjectF::IsLoaded() {
    if (m_animWorker == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015ba60, 0x6)
LoadableClassId CWwdGameObjectF::GetClassId() {
    return CLASSID_WWDOBJF;
}

RVA(0x0015ba70, 0x3)
void CWwdGameObjectF::Render(CDDrawSurfacePair*) {}

RVA(0x0015ba80, 0x3)
void CWwdGameObjectF::BltDirty(CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA(0x0015ba90, 0x3)
void CWwdGameObjectF::BltDirtyEx(CDrawSubWorker*, CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA(0x0015baa0, 0x3)
void CWwdGameObjectF::BltDirtyRegions(CDDrawSurfacePair*, CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA_COMPGEN(0x0015bab0, 0x1e, ??_GCWwdGameObjectF@@UAEPAXI@Z)
RVA(0x0015bad0, 0x153)
CWwdGameObjectF::~CWwdGameObjectF() {
    Unload();
}

RVA(0x0015bc30, 0x16)
i32 CWwdGameObjectF::SetupDeferred(i32 sortKey, AnimWorkerObj* tmpl) {
    return CGameObject::Setup(0, 0, sortKey, tmpl);
}

RVA(0x0015bcd0, 0xb)
i32 CWwdGameObject::IsLoaded() {
    return m_animWorker != NULL;
}

RVA(0x0015bce0, 0x6)
LoadableClassId CWwdGameObject::GetClassId() {
    return CLASSID_WWDOBJB;
}

RVA_COMPGEN(0x0015bcf0, 0x1e, ??_GCWwdGameObject@@UAEPAXI@Z)
// @early-stop
// BROKEN 2026-08-17 from 71.54 by deleting WORKER_FREE's do{...}while(0)
// wrapper: the wrapper's C1-only statement mass priced CGameObject::Unload out
// of the ~CGameObject sub-object site's /Ob1 budget, cascading the whole tail
// (docs/patterns/dowhile-macro-c1-mass-prices-out-the-inline-site.md).
// Residue: one nested step short - retail also expands ~CResolveNode's body and
// CALLS ??1CWapObj; our budget still declines ~CResolveNode (calls it) after
// paying for the Unload expansion. Writing m_dirty.Reset()'s stores directly in
// ~CResolveNode does not flip it and dents ~CWwdGameObjectF (99.71), so the
// member-call spelling stands; ~WwdDirtyRect's user-declared empty dtor is
// retail-real (0x15b290) and cannot be shaved.
RVA(0x0015bd10, 0x1ef)
CWwdGameObject::~CWwdGameObject() {
    Unload();
}

RVA(0x0015bfb0, 0x4a)
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

RVA(0x0015c000, 0x1d)
i32 CWwdGameObjectC::IsLoaded() {
    if (m_animWorker == NULL) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015c020, 0x6)
LoadableClassId CWwdGameObjectC::GetClassId() {
    return CLASSID_WWDOBJC;
}

RVA(0x0015c030, 0x7)
u8 CWwdGameObjectC::GetDotColor() {
    return m_dotColor;
}

RVA(0x0015c040, 0xd)
void CWwdGameObjectC::SetDotColor(u8 c8) {
    m_dotColor = c8;
}

RVA_COMPGEN(0x0015c050, 0x1e, ??_GCWwdGameObjectC@@UAEPAXI@Z)
RVA(0x0015c070, 0x159)
CWwdGameObjectC::~CWwdGameObjectC() {
    Unload();
}

RVA(0x0015c1d0, 0x26)
i32 CWwdGameObjectC::SetupFlagged(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl, i32 flag) {
    m_dotColor = static_cast<u8>(flag);
    return CGameObject::Setup(x, y, sortKey, tmpl);
}

// @early-stop
RVA(0x0015c290, 0x2f)
void CAniAdvanceCursor::Construct(CWwdGameObjectA* src) {
    m_boundObject = src;
    m_finished = 1;
    m_animation = NULL;
    m_scale = 1.0f;
    m_consumeDraw = src->OwnerMgr()->m_flags & 0x40;
    m_useElapsedTime = 1;
}

RVA(0x0015c2c0, 0xc)
void CAniAdvanceCursor::Unload() {
    m_boundObject = NULL;
    m_animation = NULL;
    m_element = NULL;
}

RVA(0x0015c2d0, 0x45)
void CAniAdvanceCursor::Setup(CAniElement* src) {
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
    m_finished = 0;
    v = e->m_drawValue;
    m_pendingDraw = v;
    m_curDraw = v;
    {
        float f = src->m_scale;
        m_scale = f;
    }
}

RVA(0x0015c320, 0x40)

void CAniAdvanceCursor::Recompute(i32 resetGate) {
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
    m_finished = 0;
    i32 v = e->m_drawValue;
    m_scale = 1.0f;
    m_pendingDraw = v;
    m_curDraw = v;
    if (resetGate != 0) {
        m_frameTicksLeft = 0;
    }
}

// @early-stop
// The AT_FIRST/AT_LAST/AFTER_FIRST arms carry their own COPIES of the advance
// body (retail emits all four; a shared `goto loop_restart` under-counted by
// ~150 insns - the copies survive cl's cross-jumper because each arm's entry
// register state differs), and the m_useElapsedTime mask is computed at u8
// width (`mov cl,[edi+4]; not cl`). Residue: per-copy register rotations, the
// WWDPOS arm seat swaps, and jump-table churn behind them.
RVA(0x0015c360, 0x59c)
i32 CAniAdvanceCursor::Advance(u32 elapsed) {
    if (m_animation == NULL) {
        return -1;
    }

    if (m_frameTicksLeft > 0) {
        if (m_useElapsedTime != 0) {
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

    if (m_finished == 0) {
        CWwdGameObjectA* ctx = m_boundObject;
        CAniRecordView* d = m_element;

        switch (d->m_stepMode) {
            case WWDSTEP_NEXT: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex + 1;
                c->m_layer = seq->GetFrame(c->m_frameIndex);
                if (c->m_layer == NULL) {
                    i32 first = c->m_frameSet->m_minIndex;
                    c->m_frameIndex = first;
                    c->m_layer = c->m_frameSet->GetFrame(first);
                }
                break;
            }
            case WWDSTEP_PREV: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                i32 idx = c->m_frameIndex;
                if (idx == seq->m_minIndex) {
                    c->m_frameIndex = seq->m_maxIndex;
                } else {
                    c->m_frameIndex = idx - 1;
                }
                c->m_layer = seq->GetFrame(c->m_frameIndex);
                break;
            }
            case WWDSTEP_SET: {
                CWwdGameObjectA* c = m_boundObject;
                i32 frame = d->m_param;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                c->m_layer = seq->GetFrame(frame);
                c->m_frameIndex = frame;
                break;
            }
            case WWDSTEP_FIRST: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                i32 first = seq->m_minIndex;
                c->m_frameIndex = first;
                c->m_layer = seq->GetFrame(first);
                break;
            }
            case WWDSTEP_LAST: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                i32 last = seq->m_maxIndex;
                c->m_frameIndex = last;
                c->m_layer = seq->GetFrame(last);
                break;
            }
            case WWDSTEP_FORWARD_BY: {
                CWwdGameObjectA* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex + step;
                c->m_layer = seq->GetFrame(c->m_frameIndex);
                if (c->m_layer == NULL) {
                    c->ClampLast();
                }
                break;
            }
            case WWDSTEP_BACK_BY: {
                CWwdGameObjectA* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_frameSet;
                if (seq == NULL) {
                    break;
                }
                c->m_frameIndex = c->m_frameIndex - step;
                c->m_layer = seq->GetFrame(c->m_frameIndex);
                if (c->m_layer == NULL) {
                    c->ClampFirst();
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
                CWwdGameObjectA* c = m_boundObject;
                c->m_plotDX = pd->m_positionDeltaX;
                c->m_plotDY = pd->m_positionDeltaY;
                break;
            }
            case WWDPOS_MOVE_RELATIVE: {
                CAniRecordView* pd = m_element;
                CWwdGameObjectA* c = m_boundObject;
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

        CWwdGameObjectA* c = m_boundObject;
        i32 fire = 1;
        if ((c->m_flags & 0x2000000) || (m_element->m_flags & 0x8)) {
            if (c->m_dirty.m_armed == -1) {
                fire = 0;
            }
        }
        if (fire) {
            CAniRecordView* dd = m_element;
            if (dd->m_flags & 0x4) {
                i32 cue = c->m_screenX;
                LeafCue* entry;
                if (dd->m_cueCount == 0) {
                    entry = NULL;
                } else {
                    entry = dd->m_cues[dd->Rng2Next() % dd->m_cueCount];
                }
                if (entry != NULL) {
                    entry->TriggerBlit(cue, 0, 0, 0);
                }
            } else {
                LeafCue* entry;
                if (dd->m_cueCount == 0) {
                    entry = NULL;
                } else {
                    entry = dd->m_cues[dd->Rng2Next() % dd->m_cueCount];
                }
                if (entry != NULL) {
                    entry->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }

        CAniRecordView* rd = m_element;
        i32 reload = rd->m_frameTime;
        m_frameTicksLeft = reload;
        m_useElapsedTime = static_cast<u8>(~rd->m_flags) & 1;

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
                m_finished = 1;
                break;
            case WWDLOOP_RESET_ANIMATION: {
                if (m_animation != NULL) {
                    m_index = 0;
                    m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(0));
                    m_finished = 0;
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
                    m_finished = 0;
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
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_minIndex) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec;
                        if (m_index >= 0 && m_index < anim->m_records.GetSize()) {
                            rec = static_cast<CAniRecordView*>(anim->m_records.GetAt(m_index));
                        } else {
                            rec = NULL;
                        }
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
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_maxIndex) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec;
                        if (m_index >= 0 && m_index < anim->m_records.GetSize()) {
                            rec = static_cast<CAniRecordView*>(anim->m_records.GetAt(m_index));
                        } else {
                            rec = NULL;
                        }
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
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_minIndex + 1) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* anim = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* rec;
                        if (m_index >= 0 && m_index < anim->m_records.GetSize()) {
                            rec = static_cast<CAniRecordView*>(anim->m_records.GetAt(m_index));
                        } else {
                            rec = NULL;
                        }
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
                    if (m_index >= 0 && m_index < arr->m_records.GetSize()) {
                        nd = static_cast<CAniRecordView*>(arr->m_records.GetAt(m_index));
                    } else {
                        nd = NULL;
                    }
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
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_maxIndex - 1) {
                    if (rd->m_loopMode != WWDLOOP_FINISH) {
                        CAniElement* a = m_animation;
                        m_index = m_index + 1;
                        CAniRecordView* p;
                        if (m_index >= 0 && m_index < a->m_records.GetSize()) {
                            p = static_cast<CAniRecordView*>(a->m_records.GetAt(m_index));
                        } else {
                            p = NULL;
                        }
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

RVA(0x0015c900, 0x5c)
i32 CAniAdvanceCursor::Find(
    CFileMemBase* ar,
    SerialMode type,
    LogicTypeId typeId,
    CGameObject* self
) {
    if (ar == NULL) {
        return 0;
    }
    switch (type) {
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
RVA(0x0015c960, 0xe)
i32 CAniAdvanceCursor::CanSerialize(CFileMemBase* ar) {
    return ar != NULL;
}

RVA(0x0015c970, 0xfe)
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

        strcpy(buf, OwnerMgr()->m_animRegistry->KeyOfValue(m_animation));
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
// Residue is one address-CSE too many: cl enregisters `&m_index` in EBP, so the
// four member addresses the tail still needs after EBX is rebound to m_element
// all spill, and the frame grows by a dword. Retail rematerialises m_index off
// the live `this` and gives EBP to `&m_pendingDraw`.
RVA(0x0015ca70, 0x15b)
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
        m_element = RecordAt(w, m_index);
        if (m_element == NULL) {
            m_index = 0;
            m_element = RecordAt(w, 0);
        }
        if (m_element != NULL) {
            m_finished = 0;
            m_frameTicksLeft = 0;
            m_curDraw = m_pendingDraw;
            m_pendingDraw = m_element->m_drawValue;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0015cbd0, 0xe)
i32 CAniAdvanceCursor::CanDeserialize(CFileMemBase* ar) {
    return ar != NULL;
}

RVA(0x0015cbe0, 0x46)
i32 CAniRecordView::Rng2Next() {
    return GetRandomNumber();
}

RVA(0x0015cc30, 0x1e)
CImage* CDDrawWorker::GetFrame(i32 n) {
    if (n >= m_minIndex && n <= m_maxIndex) {
        return static_cast<CImage*>(m_items.GetAt(n));
    }
    return 0;
}

RVA(0x0015cc50, 0x38)
void CWwdGameObjectA::ClampFirst() {
    CDDrawWorker* seq = m_frameSet;
    if (seq != NULL) {
        i32 n = seq->m_minIndex;
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_layer = layer;
    }
}

RVA(0x0015cc90, 0x38)
void CWwdGameObjectA::ClampLast() {
    CDDrawWorker* seq = m_frameSet;
    if (seq != NULL) {
        i32 n = seq->m_maxIndex;
        m_frameIndex = n;
        CImage* layer = seq->GetAt(n);
        m_layer = layer;
    }
}
