#include <Gruntz/SimpleAnimation.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)
#include <Wap32/zBitVec.h>      // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Gruntz/TypeKeyColl.h> // g_typeCounter (the shared type-id counter)
#include <Gruntz/AniAdvanceCursor.h>

#include <Bute/ButeMgr.h>        // CButeTree (the shared registration key store)
#include <Mfc.h>                 // CString (the scratch name-vec element)
#include <Wap32/ZVec.h>          // zDArray<member-fn-ptr> dispatch table + zvec accessors
#include <Gruntz/LogicFnTable.h> // the shared LogicFnTable dispatch-table shape
#include <Globals.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

RVA(0x0000f930, 0x47)
i32 CSimpleAnimation::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}


// CSimpleAnimation::~CSimpleAnimation @0x00f9d0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CSimpleAnimation() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CSimpleAnimation@@UAE@XZ 0x0000f9d0 0x44

DATA(0x00246038)
extern LogicFnTable g_simpleAnimDispatch;

extern i32 SimpleAnimLogic_4028b0();

static inline char* ResolveNameSlot(_zdvec* v, i32 idx) {
    char* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = reinterpret_cast<i32>(g_projActCache);
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = reinterpret_cast<CString*>(v->m_alloc);
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

static inline char* ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = reinterpret_cast<i32>(g_projActCache);
    g_retAddrBreadcrumb = GetRetAddr();
    v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
    return v->m_spare;
}

RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CImage* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_height >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

RVA(0x000abb90, 0x15)
void InitSimpleAnimDispatch() {
    g_simpleAnimDispatch.Construct(0x7d0, 0x7da);
}

typedef i32 (CUserLogic::*LogicFn)();

RVA(0x000abc10, 0x102)
void CSimpleAnimation::FireActivation(i32 idx) {
    if (*reinterpret_cast<void**>(ResolveSlot(&g_simpleAnimDispatch, idx)) != 0) {
        LogicFn fn = *reinterpret_cast<LogicFn*>(ResolveSlot(&g_simpleAnimDispatch, idx));
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterSimpleAnimLogic  (0x0abd70)
// Register the logic handler into g_simpleAnimDispatch: look the key up in the
// bute tree; if absent, Insert it under the running counter and cache the key name
// into the scratch zDArray<CString> slot (growing it), then bump the counter.
// Either way, resolve the dispatch-table slot for the key index and load it with
// the handler member-fn-ptr (0x4028b0).
// ---------------------------------------------------------------------------
// @early-stop
// inlined _zdvec/zvec IndexToPtr regalloc wall (the documented ZVec family - see
// ZVec.cpp's IndexToPtr/GrowTo @early-stops + RegisterTextLogic/RegisterIconState
// ~96%): the two inlined accessors + the CString-ctor fixup loop are reconstructed
// faithfully, but cl pins the index/this/base across the grow branches differently
// than retail. Logic + the bute find/insert + the fn-ptr store are correct; the
// register assignment is not source-steerable.
RVA(0x000abd70, 0x18d)
void RegisterSimpleAnimLogic() {
    i32 idx = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (idx == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        char* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *reinterpret_cast<CString*>(slot) = "A";
        g_typeCounter++;
    }
    char* dslot = ResolveSlot(&g_simpleAnimDispatch, idx);
    *reinterpret_cast<void**>(dslot) = static_cast<void*>(&SimpleAnimLogic_4028b0);
}

RVA(0x000abf70, 0x17)
i32 CSimpleAnimation::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
