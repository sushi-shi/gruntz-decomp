#include <Gruntz/SimpleAnimation.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Image/CImage.h>       // the +0x198 cached frame (ex CGameObjLayer view)
#include <Wap32/zBitVec.h>      // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Gruntz/TypeKeyColl.h> // g_typeCounter (the shared type-id counter)
#include <Gruntz/AniAdvanceCursor.h>

#include <Bute/ButeMgr.h>        // CButeTree (the shared registration key store)
#include <Mfc.h>                 // CString (the scratch name-vec element)
#include <Wap32/ZVec.h>          // zDArray<member-fn-ptr> dispatch table + zvec accessors
#include <Gruntz/LogicFnTable.h> // the shared CActReg dispatch-table shape
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <rva.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

RVA(0x0000f930, 0x47)
i32 CSimpleAnimation::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
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
RVA_COMPGEN(0x0000f9a0, 0x1e, ??_GCSimpleAnimation@@UAEPAXI@Z)
RVA_COMPGEN(0x0000f9d0, 0x44, ??1CSimpleAnimation@@UAE@XZ)

VTBL(CSimpleAnimation, 0x001e8544);
template<> DATA(0x00246038)
CActReg CActRegPool<CSimpleAnimation>::s_table(2000, 2010);

static inline CString* ResolveNameSlot(CTypeCollRuntime* v, i32 idx) {
    CString* r;
    v->m_grown = 0;
    if (idx >= v->m_lo && idx <= v->m_hi) {
        r = v->Elem(idx);
    } else if (v->GrowTo(idx, 0)) {
        r = v->Elem(idx);
    } else {
        void* sentinel = g_projActCache;
        g_retAddrBreadcrumb = GetRetAddr();
        v->m_errSink->Set(static_cast<void*>(v), sentinel, 0xc);
        r = v->Scratch();
    }
    CString* slot = v->Slots();
    i32 n = v->m_grown;
    while (n-- != 0) {
        if (slot) {
            slot->CString::CString();
        }
        slot++;
    }
    return r;
}

RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
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


RVA(0x000abc10, 0x102)
void CSimpleAnimation::FireActivation(i32 idx) {
    if (*CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx) != 0) {
        CActHandler fn = *CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
        (this->*fn)();
    }
}

// ===========================================================================
// RegisterSimpleAnimLogic  (0x0abd70)
// Register the logic handler into CActRegPool<CSimpleAnimation>::s_table: look the key up in the
// bute tree; if absent, Insert it under the running counter and cache the key name
// into the scratch zDArray<CString> slot (growing it), then bump the counter.
// Either way, resolve the dispatch-table slot for the key index and load it with
// the handler member-fn-ptr (0x4028b0).
// ---------------------------------------------------------------------------
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x000abd70, 0x18d)
void RegisterSimpleAnimLogic() {
    i32 idx = ActFindId("A");
    if (idx == 0) {
        ActInsertId("A", g_typeCounter);
        idx = g_typeCounter;
        CString* slot = ResolveNameSlot(&g_typeColl, g_typeCounter);
        *slot = "A";
        g_typeCounter++;
    }
    CActHandler* dslot = CActRegPool<CSimpleAnimation>::s_table.ResolveEntry(idx);
    *dslot = static_cast<CActHandler>(&CSimpleAnimation::AdvanceAnim);
}

RVA(0x000abf70, 0x17)
i32 CSimpleAnimation::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

