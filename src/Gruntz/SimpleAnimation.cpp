// SimpleAnimation.cpp - a simple eyecandy animation game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CSimpleAnimation methods, defined in ascending retail-RVA
// order:
//   ~CSimpleAnimation @0x00f9d0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   AdvanceAnim       @0x0abf70 - the per-frame animation-advance (ret 0).
//
// CSimpleAnimation : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/SimpleAnimation.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>

#include <Bute/ButeMgr.h>        // CButeTree (the shared registration key store)
#include <Mfc.h>                 // CString (the scratch name-vec element)
#include <Wap32/ZVec.h>          // zDArray<member-fn-ptr> dispatch table + zvec accessors
#include <Gruntz/LogicFnTable.h> // the shared LogicFnTable dispatch-table shape
#include <Gruntz/NameVec.h>      // g_buteNameVec's scratch zDArray<CString> view
#include <Globals.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// CSimpleAnimation::Serialize @0x00f930 - the vtable slot-1 override: base CUserLogic
// chain + the +0x34 sub-object chain. Byte-identical to CEyeCandy::Serialize (0x00fcc0).
RVA(0x0000f930, 0x47)
i32 CSimpleAnimation::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Defined in SpriteResource.cpp/Projectile.cpp; declared extern "C"
// here so the value-load reloc-masks against the already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CSimpleAnimation::~CSimpleAnimation @0x00f9d0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x0000f9d0, 0x44)
CSimpleAnimation::~CSimpleAnimation() {}

// ===========================================================================
// The file-scope CSimpleAnimation-logic registration thunks (proximity-attributed
// to CSimpleAnimation, but really the shared CUserLogic dispatch-table
// registration the engine emits per game-object class - the same archetype as
// RegisterIconState (CInGameIcon.cpp) / RegisterTextLogic (CInGameText.cpp) /
// RegisterWormholeLogic). The class-specific member-fn-ptr table is
// g_simpleAnimDispatch (0x646038); the handler is the logic method at 0x4028b0.
// The bute key store, running counter, scratch name-vec, key string and the error
// globals are the SHARED registration infrastructure (same symbols every per-class
// register thunk uses).
// ===========================================================================

// The shared registration key store (?g_buteTree@@3VCButeTree@@A @ 0x6bf620).
extern CButeTree g_buteTree;

// The running registration index (0x61aea8) bumped on each fresh insert.
DATA(0x0021aea8)
extern i32 g_logicRegCounter;

// The scratch name-vec (zDArray<CString> @ 0x6bf650): the registration path
// IndexToPtr's it (growing + CString-constructing fresh slots) to stash the key.
// NameVec is the shared def in <Gruntz/NameVec.h>.
DATA(0x002bf650)
extern NameVec g_buteNameVec;

// The zvec error globals + the capture helper the inlined accessor touches on a
// bounds miss (the same set ZVec.cpp models).
extern void* GetRetAddr(); // 0x16d990

// The logic registration key (the .data string constant @ 0x60a454, the SAME key
// string every per-class register thunk inserts).
DATA(0x0020a454)
extern const char s_simpleAnimLogicKey[];

// The CSimpleAnimation-logic dispatch table (a zDArray<int (CUserLogic::*)(void)>
// @ 0x646038). The 0x15 thunk constructs it over the index band [0x7d0, 0x7da].
// Shared shape: <Gruntz/LogicFnTable.h>.
DATA(0x00246038)
extern LogicFnTable g_simpleAnimDispatch;

// The handler member function loaded into the dispatch slot (LAB_004028b0).
// Referenced by address so its DIR32 operand reloc-masks.
extern i32 SimpleAnimLogic_4028b0();

// The zDArray<CString> accessor inlined WITH the per-slot CString-ctor fixup over
// the freshly-grown region (the zDArray::IndexToPtr body).
static inline i32 ResolveNameSlot(NameVec* v, i32 idx) {
    i32 r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else if (v->GrowTo(idx, 0)) {
        r = v->m_base + (idx - v->m_lo) * v->m_stride;
    } else {
        i32 sentinel = g_zvecErrSentinel;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_err->Set((void*)v, sentinel, 0xc);
        r = v->m_spare;
    }
    CString* slot = (CString*)v->m_alloc;
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

// The plain _zvec accessor inlined (no fixup) - the dispatch-table slot resolver.
static inline i32 ResolveSlot(_zvec* v, i32 idx) {
    i32 lo = v->m_lo;
    v->m_grown = 0;
    if (idx >= lo && idx <= v->m_hi) {
        return v->m_base + (idx - lo) * v->m_stride;
    }
    if (v->GrowTo(idx, 0)) {
        return v->m_base + (idx - v->m_lo) * v->m_stride;
    }
    i32 sentinel = g_zvecErrSentinel;
    g_retAddrBreadcrumb = GetRetAddr();
    v->m_err->Set((void*)v, sentinel, 0xc);
    return v->m_spare;
}

// --- CSimpleAnimation (0x0ab940), vptr 0x5e8544 --- the ctor anchors the
// ??_7CSimpleAnimation vtable in this TU. Folds the inline CUserLogic(obj) base +
// the shared z-clamp tail.
RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// ===========================================================================
// InitSimpleAnimDispatch  (0x0abb90)
// File-scope static-init thunk: construct the logic dispatch table over the index
// band [0x7d0, 0x7da].
// ===========================================================================
RVA(0x000abb90, 0x15)
void InitSimpleAnimDispatch() {
    ((CZDArrayDerived*)&g_simpleAnimDispatch)->Construct(0x7d0, 0x7da);
}

// The stored handler is a CUserLogic member-fn-ptr (the same shape the whole
// LogicFnTable family dispatches; see CInGameText::Dispatch).
typedef i32 (CUserLogic::*LogicFn)();

// ===========================================================================
// CSimpleAnimation::Dispatch  (0x0abc10)
// Index g_simpleAnimDispatch by idx; if the resolved slot holds a non-null member
// function, invoke it on this. The bounds-check + grow of the table accessor is
// inlined (ResolveSlot), computed once for the null-test and once for the call.
// Same archetype as CInGameText::Dispatch.
// ===========================================================================
RVA(0x000abc10, 0x102)
void CSimpleAnimation::Dispatch(i32 idx) {
    if (*(void**)ResolveSlot(&g_simpleAnimDispatch, idx) != 0) {
        LogicFn fn = *(LogicFn*)ResolveSlot(&g_simpleAnimDispatch, idx);
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
// inlined zDArray/zvec IndexToPtr regalloc wall (the documented ZVec family - see
// ZVec.cpp's IndexToPtr/GrowTo @early-stops + RegisterTextLogic/RegisterIconState
// ~96%): the two inlined accessors + the CString-ctor fixup loop are reconstructed
// faithfully, but cl pins the index/this/base across the grow branches differently
// than retail. Logic + the bute find/insert + the fn-ptr store are correct; the
// register assignment is not source-steerable.
RVA(0x000abd70, 0x18d)
void RegisterSimpleAnimLogic() {
    i32 idx = (i32)g_buteTree.Find(s_simpleAnimLogicKey);
    if (idx == 0) {
        g_buteTree.Insert(s_simpleAnimLogicKey, (void*)g_logicRegCounter);
        i32 slot = ResolveNameSlot(&g_buteNameVec, g_logicRegCounter);
        *(CString*)slot = s_simpleAnimLogicKey;
        g_logicRegCounter++;
    }
    i32 dslot = ResolveSlot(&g_simpleAnimDispatch, idx);
    *(void**)dslot = (void*)&SimpleAnimLogic_4028b0;
}

// CSimpleAnimation::AdvanceAnim @0x0abf70 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0. Same archetype as CGruntPuddle's remove-path notify and
// CProjectile::DetachRenderObj's SetAnim(g_6bf3bc).
RVA(0x000abf70, 0x17)
i32 CSimpleAnimation::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance_15c360(g_6bf3bc);
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZDArrayDerived.h>
