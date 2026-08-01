

#define WWDREGION_OOL_CTOR
#define CGAMEOBJECT_OOL_CTOR
#define ANIADVANCECURSOR_OOL_CTOR

#include <Mfc.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <rva.h>
#include <Ints.h>
#include <string.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Gruntz/WwdGameObject.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/ResolveNode.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <DDrawMgr/AniAdvance.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/SerialArchive.h>
#include <Wwd/WwdFactoryObject.h>
#include <Gruntz/LeafCue.h>

#include <DDrawMgr/DDrawSubMgr.h>
#include <Gruntz/SoundState.h>
#include <Wwd/WwdObjMgr.h>
#include <Gruntz/AniElement.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
namespace Rng {}

VTBL(CWwdGameObjectC, 0x001effd0);
VTBL(CGameObject, 0x001f0020);
VTBL(CWwdGameObjectF, 0x001f0060);
VTBL(CWwdGameObjectA, 0x001f00a8);
VTBL(CWwdGameObject, 0x001f00e8);
VTBL(CAniAdvanceCursor, 0x001f0128);

RVA_COMPGEN(0x00154a50, 0x23, ??1CResolveNode@@UAE@XZ)

RVA(0x0015b2b0, 0xe)
WwdRegion::WwdRegion() {
    m_object = 0;
}

RVA_COMPGEN(0x0015b2c0, 0x3d, ??0CResolveNode@@QAE@PAVCDDrawSurfaceMgr@@HH@Z)

RVA_COMPGEN(0x0015b300, 0x40, ??0AnimWorkerObj@@QAE@PAVCDDrawSurfaceMgr@@HH@Z)

RVA(0x0015b340, 0x2b)
i32 AnimWorkerObj::Consume(i32 amount) {
    i32 remaining = m_20;
    if (remaining == 0) {
        return remaining;
    }
    if (static_cast<u32>(amount) >= static_cast<u32>(remaining)) {
        m_20 = 0;
        return 0;
    }
    m_20 = remaining - amount;
    return 1;
}

RVA(0x0015b370, 0x1d)
i32 CGameObject::IsLoaded() {
    if (m_animWorker == 0) {
        return 0;
    }
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x0015b390, 0x128)
CGameObject::CGameObject(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CResolveNode(owner, id, stateFlags) {
    m_screenX = static_cast<i32>(0x80000000);
    m_posCache = 0;
    m_animWorker = new AnimWorkerObj(owner, id, 0);
    m_carrier = 0;
    m_hitWorker = 0;
    m_attackWorker = 0;
    m_collideWorker = 0;
    m_188 = g_wwdObjIdCounter;
    g_wwdObjIdCounter = g_wwdObjIdCounter + 1;
}

// @early-stop
RVA(0x00154a80, 0x13)
void CResolveNode::Unload() {
    m_screenX = static_cast<i32>(0x80000000);
    m_dirty.m_rect.left = static_cast<i32>(0x80000000);
    m_dirty.m_armed = -1;
}

RVA_COMPGEN(0x0015b4c0, 0x1e, ??_GCGameObject@@UAEPAXI@Z)
RVA_COMPGEN(0x0015b4f0, 0xde, ??1CGameObject@@UAE@XZ)

// @early-stop
RVA(0x0015b650, 0x4d)
void CGameObject::Notify(void* p) {
    if (m_flags & 0x8) {
        i32 d = m_placeMode - (static_cast<CGameObject*>(p))->m_120;
        m_placeMode = d;
        if (d <= 0) {
            m_animWorker->SetActKey(0x1c);
        }
    } else {
        AnimWorkerObj* h = m_hitWorker;
        if (h != 0) {
            m_84 = static_cast<CGameObject*>(p);
            h->m_notify(this);
        }
    }
}

RVA(0x0015b6a0, 0xb)
i32 CAniAdvanceCursor::IsLoaded() {
    return m_boundObject != 0;
}

RVA_COMPGEN(0x0015b6b0, 0x1e, ??_GCAniAdvanceCursor@@UAEPAXI@Z)
RVA(0x0015b6d0, 0x5b)
CAniAdvanceCursor::~CAniAdvanceCursor() {
    Unload();
    m_id = -1;
    m_flags = 0;
    m_ownerCtx = 0;
}

RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(CDDrawSurfaceMgr* owner, i32 field04, i32 field08)
    : CLoadable(field04, field08, owner) {
    m_boundObject = 0;
    m_14 = 0;
    m_element = 0;
}

RVA(0x0015b760, 0x6)
i32 CWwdGameObjectA::GetClassId() {
    return CLASSID_WWDOBJA;
}

RVA_COMPGEN(0x0015b770, 0x1e, ??_GCWwdGameObjectA@@UAEPAXI@Z)
RVA(0x0015b790, 0x1a6)
CWwdGameObjectA::~CWwdGameObjectA() {
    Unload();
}

RVA(0x0015b940, 0x38)
i32 CWwdGameObjectA::Setup(i32 x, i32 y, i32 sortKey, AnimWorkerObj* tmpl) {
    m_19c = 0;
    m_1a0.Construct(this);
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
    if (m_animWorker == 0) {
        return 0;
    }
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015ba60, 0x6)
i32 CWwdGameObjectF::GetClassId() {
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
    return m_animWorker != 0;
}

RVA(0x0015bce0, 0x6)
i32 CWwdGameObject::GetClassId() {
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
    if (m_animWorker == 0) {
        return 0;
    }
    if (m_ownerCtx != 0 && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x0015c020, 0x6)
i32 CWwdGameObjectC::GetClassId() {
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
    m_14 = 0;
    m_scale = 1.0f;
    m_24 = 1;

    m_2c = src->OwnerMgr()->m_flags & 0x40;
}

RVA(0x0015c2c0, 0xc)
void CAniAdvanceCursor::Unload() {
    m_boundObject = 0;
    m_14 = 0;
    m_element = 0;
}

RVA(0x0015c2d0, 0x45)
void CAniAdvanceCursor::Setup(CAniElement* src) {
    CAniDesc* e;
    i32 v;
    m_14 = src;
    if (!src) {
        return;
    }
    m_index = 0;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniDesc*>(src->m_records.GetAt(0));
    } else {
        e = 0;
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
    CAniElement* src = m_14;
    if (src == 0) {
        return;
    }
    m_index = 0;
    CAniDesc* e;
    if (src->m_records.GetSize() > 0) {
        e = static_cast<CAniDesc*>(src->m_records.GetAt(0));
    } else {
        e = 0;
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
    if (m_14 == 0) {
        return -1;
    }

    if (m_frameTicksLeft > 0) {
        if (m_24 != 0) {
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
        CAniDesc* d = m_element;

        switch (d->m_stepMode - 1) {
            case 0: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_190 + 1;
                c->m_190 = idx;
                c->m_layer = seq->GetFrame(idx);
                if (c->m_layer == 0) {
                    i32 first = c->m_sprite->m_minIndex;
                    c->m_190 = first;
                    c->m_layer = c->m_sprite->GetFrame(first);
                }
                break;
            }
            case 1: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_190;
                if (idx == seq->m_minIndex) {
                    c->m_190 = seq->m_maxIndex;
                } else {
                    c->m_190 = idx - 1;
                }
                c->m_layer = seq->GetFrame(c->m_190);
                break;
            }
            case 2: {
                CWwdGameObjectA* c = m_boundObject;
                i32 frame = d->m_param;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                c->m_layer = seq->GetFrame(frame);
                c->m_190 = frame;
                break;
            }
            case 3: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 first = seq->m_minIndex;
                c->m_190 = first;
                c->m_layer = seq->GetFrame(first);
                break;
            }
            case 4: {
                CWwdGameObjectA* c = m_boundObject;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 last = seq->m_maxIndex;
                c->m_190 = last;
                c->m_layer = seq->GetFrame(last);
                break;
            }
            case 5: {
                CWwdGameObjectA* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_190 + step;
                c->m_190 = idx;
                c->m_layer = seq->GetFrame(idx);
                if (c->m_layer == 0) {
                    c->ClampLast();
                }
                break;
            }
            case 6: {
                CWwdGameObjectA* c = m_boundObject;
                i32 step = d->m_param;
                CDDrawWorker* seq = c->m_sprite;
                if (seq == 0) {
                    break;
                }
                i32 idx = c->m_190 - step;
                c->m_190 = idx;
                c->m_layer = seq->GetFrame(idx);
                if (c->m_layer == 0) {
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
        switch (d->m_posMode) {
            case 1:
                m_boundObject->m_plotDX = d->m_posDX;
                m_boundObject->m_plotDY = d->m_posDY;
                break;
            case 2: {
                CWwdGameObjectA* c = m_boundObject;
                i32 x = c->m_screenX;
                if (c->m_stateFlags & 0x2) {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x - dx;
                    c->m_screenY = c->m_screenY + dy;
                } else {
                    i32 dy = d->m_posDY;
                    i32 dx = d->m_posDX;
                    c->m_screenX = x + dx;
                    c->m_screenY = c->m_screenY + dy;
                }
                break;
            }
            case 3:
                m_boundObject->m_screenX = d->m_posDX;
                m_boundObject->m_screenY = d->m_posDY;
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
            CAniDesc* dd = m_element;
            if (dd->m_flags & 0x4) {
                i32 cue = c->m_screenX;
                LeafCue** tbl;
                LeafCue* entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[Rng::Next2() % dd->m_randMod];
                }
                if (entry != 0) {
                    entry->TriggerBlit(cue, 0, 0, 0);
                }
            } else {
                LeafCue** tbl;
                LeafCue* entry;
                if (dd->m_randMod == 0) {
                    entry = 0;
                } else {
                    tbl = dd->m_randTable;
                    entry = tbl[Rng::Next2() % dd->m_randMod];
                }
                if (entry != 0) {
                    entry->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
                }
            }
        }

        CAniDesc* rd = m_element;
        i32 reload = rd->m_frameTime;
        m_frameTicksLeft = reload;
        m_24 = (~rd->m_flags) & 1;

        if (m_scaleBits != 0x3f800000) {
            m_frameTicksLeft =
                static_cast<i32>((static_cast<double>(static_cast<u32>(reload)) * m_scale));
        }

        i32 modeWord = rd->m_loopMode;
        CAniElement* arr;
        i32 i;
        CAniDesc* nd;
        switch (modeWord & 0xffff) {
            case 9:
                m_finished = 1;
                break;
            case 8: {
                if (m_14 != 0) {
                    m_index = 0;
                    m_element = static_cast<CAniDesc*>(m_14->AtChecked(0));
                    m_finished = 0;
                    m_scale = 1.0f;
                    m_pendingDraw = m_element->m_drawValue;
                    m_curDraw = m_element->m_drawValue;
                }
                break;
            }
            case 7: {
                m_index = 1;
                m_element = static_cast<CAniDesc*>(m_14->AtChecked(1));
                if (m_element == 0) {
                    m_index = 0;
                    m_element = static_cast<CAniDesc*>(m_14->AtChecked(0));
                }
                if (m_element != 0) {
                    m_finished = 0;
                    m_frameTicksLeft = 0;
                    m_curDraw = m_pendingDraw;
                    m_pendingDraw = m_element->m_drawValue;
                }
                break;
            }
            case 1: {
                CWwdGameObjectA* c2 = m_boundObject;
                if (c2->m_190 == m_element->m_param) {
                    if (modeWord != 9) {
                        CAniElement* a = m_14;
                        i32 j = m_index + 1;
                        m_index = j;
                        m_element = static_cast<CAniDesc*>(a->AtChecked(j));
                        if (m_element == 0) {
                            m_index = 0;
                            m_element = static_cast<CAniDesc*>(a->AtChecked(0));
                        }
                        if (m_element != 0) {
                            m_curDraw = m_pendingDraw;
                            m_pendingDraw = m_element->m_drawValue;
                        }
                    }
                }
                break;
            }
            case 2: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_sprite;
                if (c2->m_190 == seq->m_minIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 3: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_sprite;
                if (c2->m_190 == seq->m_maxIndex) {
                    goto loop_restart;
                }
                break;
            }
            case 4: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_sprite;
                if (c2->m_190 == seq->m_minIndex + 1) {
                    goto loop_restart;
                }
                break;
            }
            case 0:
            loop_restart:
                if (modeWord != 9) {
                    arr = m_14;
                    i = m_index + 1;
                    m_index = i;
                    if (i >= 0 && i < arr->m_records.GetSize()) {
                        nd = static_cast<CAniDesc*>(arr->m_records.GetAt(i));
                    } else {
                        nd = 0;
                    }
                    m_element = nd;
                    if (nd == 0) {
                        m_index = 0;
                        m_element = static_cast<CAniDesc*>(arr->AtChecked(0));
                    }
                    if (m_element != 0) {
                        m_curDraw = m_pendingDraw;
                        m_pendingDraw = m_element->m_drawValue;
                    }
                }
                break;
            case 5: {
                CWwdGameObjectA* c2 = m_boundObject;
                CDDrawWorker* seq = c2->m_sprite;
                if (c2->m_190 == seq->m_maxIndex - 1) {
                    if (modeWord != 9) {
                        CAniElement* a = m_14;
                        i32 j = m_index + 1;
                        m_index = j;
                        CAniDesc* p;
                        if (j >= 0 && j < a->m_records.GetSize()) {
                            p = static_cast<CAniDesc*>(a->m_records.GetAt(j));
                        } else {
                            p = 0;
                        }
                        m_element = p;
                        if (p == 0) {
                            m_index = 0;
                            i32 cnt = a->m_records.GetSize();
                            CAniDesc* first;
                            if (cnt > 0) {
                                first = static_cast<CAniDesc*>(a->m_records.GetAt(0));
                            } else {
                                first = 0;
                            }
                            m_element = first;
                        }
                        if (m_element != 0) {
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

    if (m_2c != 0) {
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
i32 CAniAdvanceCursor::Find(CFileMemBase* ar, i32 type, i32 typeId, void* self) {
    if (ar == 0) {
        return 0;
    }
    switch (type) {
        case 3:
            return 1;
        case 4:
            if (Serialize(ar) == 0) {
                return 0;
            }
            break;
        case 5:
            return 1;
        case 6:
            return 1;
        case 7:
            if (Deserialize(ar) == 0) {
                return 0;
            }
            break;
        case 8:
            return 1;
    }
    return 1;
}

RVA(0x0015c970, 0xfe)
i32 CAniAdvanceCursor::Serialize(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_index, 4);
    ar->Write(&m_frameTicksLeft, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_finished, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_pendingDraw, 4);
    ar->Write(&m_curDraw, 4);
    ar->Write(&m_scale, 4);
    char buf[0x80];
    memset(buf, 0, sizeof(buf));
    if (m_14 != 0) {

        strcpy(buf, OwnerMgr()->m_animRegistry->KeyOfValue(m_14));
    }
    ar->Write(buf, 0x80);
    return 1;
}

// @early-stop
RVA(0x0015ca70, 0x15b)
i32 CAniAdvanceCursor::Deserialize(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_index, 4);
    ar->Read(&m_frameTicksLeft, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_finished, 4);
    ar->Read(&m_2c, 4);
    ar->Read(&m_pendingDraw, 4);
    ar->Read(&m_curDraw, 4);
    ar->Read(&m_scale, 4);
    char buf[0x80];
    ar->Read(buf, 0x80);
    if (strlen(buf) == 0) {
        m_14 = 0;
    } else {

        void* out = 0;
        OwnerMgr()->m_animRegistry->m_10.Lookup(buf, out);
        m_14 = static_cast<CAniElement*>(out);
    }
    CAniElement* w = m_14;
    if (w != 0) {
        CAniDesc* e;
        if (m_index >= 0 && m_index < w->m_records.GetSize()) {
            e = static_cast<CAniDesc*>(w->m_records.GetAt(m_index));
        } else {
            e = 0;
        }
        m_element = e;
        if (e == 0) {
            m_index = 0;
            if (w->m_records.GetSize() > 0) {
                e = static_cast<CAniDesc*>(w->m_records.GetAt(0));
            } else {
                e = 0;
            }
            m_element = e;
        }
        if (m_element != 0) {
            m_frameTicksLeft = 0;
            m_finished = 0;
            m_curDraw = m_pendingDraw;
            m_pendingDraw = m_element->m_drawValue;
        }
    }
    return 1;
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
    CDDrawWorker* seq = m_sprite;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_minIndex;
    m_190 = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_layer = static_cast<CImage*>(seq->m_items.GetAt(n));
    } else {
        m_layer = 0;
    }
}

// @early-stop
RVA(0x0015cc90, 0x38)
void CWwdGameObjectA::ClampLast() {
    CDDrawWorker* seq = m_sprite;
    if (seq == 0) {
        return;
    }
    i32 n = seq->m_maxIndex;
    m_190 = n;
    if (n >= seq->m_minIndex && n <= seq->m_maxIndex) {
        m_layer = static_cast<CImage*>(seq->m_items.GetAt(n));
    } else {
        m_layer = 0;
    }
}

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
