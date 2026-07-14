// SingleAnimation.cpp - a single-shot eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// One trace-discovered CSingleAnimation method:
//   ~CSingleAnimation @0x010540 - the /GX leaf dtor (folds the CUserLogic teardown).
//
// CSingleAnimation : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance class).
typedef i32 (CSingleAnimation::*SingleAnimHandler)();
struct CSingleAnimActEntry {
    SingleAnimHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645f70), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CSingleAnimActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype; keeps its placeholder name so
// the DATA-pinned global symbol is unchanged.
struct CSingleAnimActReg : public CActReg {};
DATA(0x00245f70)
CSingleAnimActReg g_singleAnimActReg; // 0x645f70

// The per-frame draw-delta mirror (_g_6bf3bc) the advance hands the sink; declared
// extern "C" so the value-load reloc-masks against the already-matched symbol.
extern "C" u32 g_engineFrameDelta;

// CSingleAnimation::Serialize @0x104a0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, then (only on success) the +0x34 sub-object's
// chain. Returns the second chain's success normalized to a bool.
RVA(0x000104a0, 0x47)
i32 CSingleAnimation::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CSingleAnimation::~CSingleAnimation @0x010540 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x00010540, 0x44)
CSingleAnimation::~CSingleAnimation() {}

// --- CSingleAnimation (0x0ae7f0), vptr 0x5e745c --- the ctor anchors the
// ??_7CSingleAnimation vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x000ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// CSingleAnimation::InitActReg @0x0ae9a0 - construct the class's activation-
// coordinate registry singleton (g_singleAnimActReg @0x645f70) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ae9a0, 0x15)
void CSingleAnimation::InitActReg() {
    ((CZDArrayDerived*)&g_singleAnimActReg)->Construct(2000, 2010);
}

// CSingleAnimation::RunAct @0x0aea20 - resolve the registry entry for id; if a
// handler is bound, re-resolve and invoke it as a PMF on this, else return the entry
// pointer. ResolveEntry is inlined twice (side-effectful; no CSE). Same archetype as
// CAniCycle::RunAct.
RVA(0x000aea20, 0x102)
i32 CSingleAnimation::RunAct(i32 id) {
    CSingleAnimActEntry* e = (CSingleAnimActEntry*)g_singleAnimActReg.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CSingleAnimActEntry*)g_singleAnimActReg.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
}

// CSingleAnimation::RegisterActs @0x0aeb80 - bind the class's per-frame handler
// (AdvanceAnim @0x0aed80) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000aeb80, 0x18d)
void CSingleAnimation::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CSingleAnimActEntry*)g_singleAnimActReg.ResolveEntry(id))->m_fn =
        &CSingleAnimation::AdvanceAnim;
}

// CSingleAnimation::AdvanceAnim @0x0aed80 - advance the bound object's +0x1a0 anim
// cursor by the frame counter, then, if the anim sub-mgr is active (m_1c8) and its
// idle flag is clear (m_1c0 == 0), mark the object dirty (m_flags |= 0x10000).
// Returns 0. (Sibling of CMenuSparkle::AdvanceAnim without the flicker countdown.)
RVA(0x000aed80, 0x39)
i32 CSingleAnimation::AdvanceAnim() {
    ((CAniAdvanceCursor*)((char*)m_38 + 0x1a0))->Advance(g_engineFrameDelta);
    if (m_38->m_1c8 != 0 && m_38->m_1c0 == 0) {
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CSingleAnimActEntry);
SIZE_UNKNOWN(CSingleAnimActReg);
