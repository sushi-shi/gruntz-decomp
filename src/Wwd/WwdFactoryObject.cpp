

#define WWDREGION_OOL_CTOR
#define CGAMEOBJECT_OOL_CTOR
#define ANIADVANCECURSOR_OOL_CTOR

#include <rva.h>

#include <Wwd/WwdFactoryObject.h>

#include <Mfc.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/Random.h>
#include <Gruntz/ResolveNode.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/WwdGameObject.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/AnimWorkerAct.h>
#include <Wwd/WwdAnimStepMode.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdObjMgr.h>

#include <string.h>

DATA(0x002c278c)
char g_rng2Seeded;

DATA(0x002c2798)
i32 g_rng2State;

VTBL(CWwdGameObjectC, 0x001effd0);
VTBL(CGameObject, 0x001f0020);
VTBL(CWwdGameObjectF, 0x001f0060);
VTBL(CWwdGameObjectA, 0x001f00a8);
VTBL(CWwdGameObject, 0x001f00e8);
VTBL(CAniAdvanceCursor, 0x001f0128);

RVA_COMPGEN(0x00154a50, 0x23, ??1CResolveNode@@UAE@XZ)

RVA(0x0015b2b0, 0xe)
WwdRegion::WwdRegion() {
    m_object = NULL;
}

RVA_COMPGEN(0x0015b2c0, 0x3d, ??0CResolveNode@@QAE@PAVCDDrawSurfaceMgr@@HH@Z)

RVA_COMPGEN(0x0015b300, 0x40, ??0AnimWorkerObj@@QAE@PAVCDDrawSurfaceMgr@@HH@Z)

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

// @early-stop
RVA(0x0015b390, 0x128)
CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CResolveNode(owner, id, stateFlags) {
    m_screenX = COORD_UNSET;
    m_posCache = NULL;
    m_animWorker = new AnimWorkerObj(owner, id, 0);
    m_carrier = NULL;
    m_hitWorker = NULL;
    m_attackWorker = NULL;
    m_collideWorker = NULL;
    m_objectId = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

// @early-stop
RVA(0x00154a80, 0x13)
void CResolveNode::Unload() {
    m_screenX = COORD_UNSET;
    m_dirty.m_rect.left = COORD_UNSET;
    m_dirty.m_armed = -1;
}

RVA_COMPGEN(0x0015b4c0, 0x1e, ??_GCGameObject@@UAEPAXI@Z)
RVA_COMPGEN(0x0015b4f0, 0xde, ??1CGameObject@@UAE@XZ)

// @early-stop
RVA(0x0015b650, 0x4d)
void CGameObject::Notify(void* p) {
    if (m_flags & 0x8) {
        i32 d = m_health - (static_cast<CGameObject*>(p))->m_damage;
        m_health = d;
        if (d <= 0) {
            m_animWorker->SetActKey(ACT_HEALTH_DEPLETED);
        }
    } else {
        AnimWorkerObj* h = m_hitWorker;
        if (h != NULL) {
            m_hitSource = static_cast<CGameObject*>(p);
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
    : CLoadable(field04, field08, owner) {
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
void CWwdGameObjectF::BltDirtyEx(CDDrawSurfacePair*, CDDrawSurfacePair*, CDDrawSurfacePair*) {}

RVA(0x0015baa0, 0x3)
void CWwdGameObjectF::BltDirtyRegions(CDDrawSurfacePair*, CDDrawSurfacePair*, CDDrawSurfacePair*) {}

// @early-stop
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
RVA(0x0015bd10, 0x1ef)
CWwdGameObject::~CWwdGameObject() {
    Unload();
}

RVA(0x0015bfb0, 0x4a)
i32 __stdcall RectsOverlap(CDDrawRect* a, CDDrawRect* b) {
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
    m_useElapsedTime = 1;

    m_consumeDraw = src->OwnerMgr()->m_flags & 0x40;
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
                i32 idx = c->m_frameIndex + 1;
                c->m_frameIndex = idx;
                c->m_layer = seq->GetFrame(idx);
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
                i32 idx = c->m_frameIndex + step;
                c->m_frameIndex = idx;
                c->m_layer = seq->GetFrame(idx);
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
                i32 idx = c->m_frameIndex - step;
                c->m_frameIndex = idx;
                c->m_layer = seq->GetFrame(idx);
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
        d = m_element;
        switch (d->m_positionMode) {
            case 1:
                m_boundObject->m_plotDX = d->m_positionDeltaX;
                m_boundObject->m_plotDY = d->m_positionDeltaY;
                break;
            case 2: {
                CWwdGameObjectA* c = m_boundObject;
                i32 x = c->m_screenX;
                if (c->m_stateFlags & SPRITE_STATE_MIRROR_X) {
                    i32 dy = d->m_positionDeltaY;
                    i32 dx = d->m_positionDeltaX;
                    c->m_screenX = x - dx;
                    c->m_screenY = c->m_screenY + dy;
                } else {
                    i32 dy = d->m_positionDeltaY;
                    i32 dx = d->m_positionDeltaX;
                    c->m_screenX = x + dx;
                    c->m_screenY = c->m_screenY + dy;
                }
                break;
            }
            case 3:
                m_boundObject->m_screenX = d->m_positionDeltaX;
                m_boundObject->m_screenY = d->m_positionDeltaY;
                break;
            default:
                break;
        }

        CWwdGameObjectA* c = m_boundObject;
        i32 fire = 1;
        if (!(c->m_flags & 0x2000000) && !(m_element->m_flags & 0x8)) {
            if (c->m_dirty.m_armed == -1) {
                fire = 0;
            }
        }
        if (fire) {
            CAniRecordView* dd = m_element;
            if (dd->m_flags & 0x4) {
                i32 cue = c->m_screenX;
                LeafCue** tbl;
                LeafCue* entry;
                if (dd->m_cueCount == 0) {
                    entry = NULL;
                } else {
                    tbl = dd->m_cues;
                    entry = tbl[Rng2Next() % dd->m_cueCount];
                }
                if (entry != NULL) {
                    entry->TriggerBlit(cue, 0, 0, 0);
                }
            } else {
                LeafCue** tbl;
                LeafCue* entry;
                if (dd->m_cueCount == 0) {
                    entry = NULL;
                } else {
                    tbl = dd->m_cues;
                    entry = tbl[Rng2Next() % dd->m_cueCount];
                }
                if (entry != NULL) {
                    entry->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }

        CAniRecordView* rd = m_element;
        i32 reload = rd->m_frameTime;
        m_frameTicksLeft = reload;
        m_useElapsedTime = (~rd->m_flags) & 1;

        if (m_scaleBits != 0x3f800000) {
            m_frameTicksLeft =
                static_cast<i32>((static_cast<double>(static_cast<u32>(reload)) * m_scale));
        }

        i32 modeWord = rd->m_loopMode;
        CAniElement* arr;
        i32 i;
        CAniRecordView* nd;
        switch (modeWord & 0xffff) {
            case 9:
                m_finished = 1;
                break;
            case 8: {
                if (m_animation != NULL) {
                    m_index = 0;
                    m_element = static_cast<CAniRecordView*>(m_animation->AtChecked(0));
                    m_finished = 0;
                    m_scale = 1.0f;
                    m_pendingDraw = m_element->m_drawValue;
                    m_curDraw = m_element->m_drawValue;
                }
                break;
            }
            case 7: {
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
            case 1: {
                CWwdGameObjectA* c2 = m_boundObject;
                if (c2->m_frameIndex == m_element->m_param) {
                    if (modeWord != 9) {
                        CAniElement* a = m_animation;
                        i32 j = m_index + 1;
                        m_index = j;
                        m_element = static_cast<CAniRecordView*>(a->AtChecked(j));
                        if (m_element == NULL) {
                            m_index = 0;
                            m_element = static_cast<CAniRecordView*>(a->AtChecked(0));
                        }
                        if (m_element != NULL) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case 2: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_minIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 3: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_maxIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 4: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_minIndex + 1) {
                    goto loop_restart;
                }
                break;
            }
            case 0:
            loop_restart:
                if (modeWord != 9) {
                    arr = m_animation;
                    i = m_index + 1;
                    m_index = i;
                    if (i >= 0 && i < arr->m_records.GetSize()) {
                        nd = static_cast<CAniRecordView*>(arr->m_records.GetAt(i));
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
            case 5: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_frameSet;
                if (c2->m_frameIndex == seq->m_maxIndex - 1) {
                    if (modeWord != 9) {
                        CAniElement* a = m_animation;
                        i32 j = m_index + 1;
                        m_index = j;
                        CAniRecordView* p;
                        if (j >= 0 && j < a->m_records.GetSize()) {
                            p = static_cast<CAniRecordView*>(a->m_records.GetAt(j));
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
        if (m_frameTicksLeft != 0) {
            i32 r = m_curDraw;
            m_curDraw = 0;
            return r;
        }
        i32 r = m_pendingDraw;
        m_pendingDraw = 0;
        return r;
    }
    if (m_frameTicksLeft != 0) {
        return m_curDraw;
    }
    return m_pendingDraw;
}

RVA(0x0015c900, 0x5c)
i32 CAniAdvanceCursor::Find(CFileMemBase* ar, SerialMode type, LogicTypeId typeId, void* self) {
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

// @early-stop
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

        void* out = 0;
        OwnerMgr()->m_animRegistry->m_animations.Lookup(buf, out);
        m_animation = static_cast<CAniElement*>(out);
    }
    CAniElement* w = m_animation;
    if (w != NULL) {
        CAniRecordView* e;
        if (m_index >= 0 && m_index < w->m_records.GetSize()) {
            e = static_cast<CAniRecordView*>(w->m_records.GetAt(m_index));
        } else {
            e = NULL;
        }
        m_element = e;
        if (e == NULL) {
            m_index = 0;
            if (w->m_records.GetSize() > 0) {
                e = static_cast<CAniRecordView*>(w->m_records.GetAt(0));
            } else {
                e = NULL;
            }
            m_element = e;
        }
        if (m_element != NULL) {
            m_frameTicksLeft = 0;
            m_finished = 0;
            m_curDraw = m_pendingDraw;
            m_pendingDraw = m_element->m_drawValue;
        }
    }
    return 1;
}

RVA(0x0015cbe0, 0x46)
i32 Rng2Next() {
    i32 seed;
    if (!(g_rng2Seeded & 1)) {
        g_rng2Seeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_rng2State;
    }
    g_rng2State = seed * 214013 + 2531011;
    return (g_rng2State >> 0x10) & 0x7fff;
}

RVA(0x0015cc30, 0x1e)
CImage* CDDrawWorker::GetFrame(i32 n) {
    if (n >= m_minIndex && n <= m_maxIndex) {
        return static_cast<CImage*>(m_items.GetAt(n));
    }
    return 0;
}

// @early-stop
RVA(0x0015cc50, 0x38)
void CWwdGameObjectA::ClampFirst() {
    CDDrawWorker* seq = m_frameSet;
    if (seq == NULL) {
        return;
    }
    i32 n = seq->m_minIndex;
    m_frameIndex = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_layer = static_cast<CImage*>(seq->m_items.GetAt(n));
    } else {
        m_layer = NULL;
    }
}

// @early-stop
RVA(0x0015cc90, 0x38)
void CWwdGameObjectA::ClampLast() {
    CDDrawWorker* seq = m_frameSet;
    if (seq == NULL) {
        return;
    }
    i32 n = seq->m_maxIndex;
    m_frameIndex = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_layer = static_cast<CImage*>(seq->m_items.GetAt(n));
    } else {
        m_layer = NULL;
    }
}

// Force-emit device, wall-blocked (docs/patterns/msvc5-variable-ctor-inline-depth.md):
// no natural out-of-line reference to the emitted symbol(s) exists in this TU, so
// removing this drops their labels. Dissolves when the inline-depth wall breaks.
static void* volatile g_forceEmitSink;
#pragma inline_depth(0)
void ForceEmitCResolveNodeCtor(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags) {
    g_forceEmitSink = new CResolveNode(owner, id, stateFlags);
}
#pragma inline_depth(0)
void ForceEmitAnimWorkerObjCtor(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags) {
    g_forceEmitSink = new AnimWorkerObj(owner, id, stateFlags);
}
#pragma inline_depth()
DATA(0x0024c22c)
char g_coinRolled;

DATA(0x0024c26c)
i32 g_coinValue;

DATA(0x002c127d)
u8 g_randSeeded;

DATA(0x002c1288)
i32 g_randSeed;

// @identity-TODO RandRange@CGruntzMgr - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (1 fns) came from the static library. It belongs to another compiland.
