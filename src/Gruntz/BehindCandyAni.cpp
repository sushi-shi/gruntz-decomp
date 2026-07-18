// BehindCandyAni.cpp - a behind-candy eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// Two trace-discovered CBehindCandyAni methods, defined in ascending retail-RVA
// order:
//   ~CBehindCandyAni @0x0100f0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   AdvanceAnim      @0x0adbb0 - the per-frame animation-advance (ret 0).
//
// CBehindCandyAni : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Image/CImage.h> // the +0x198 cached frame (ex CGameObjLayer view)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)

// The class's activation-coordinate registry singleton (@0x645f98), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CBehindCandyActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype (declared there among the
// concrete per-registry instances); it keeps its own placeholder name so the
// DATA-pinned global symbol is unchanged.
DATA(0x00245f98)
CBehindCandyActReg g_behindCandyActReg; // 0x645f98

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Declared extern "C" here so the value-load reloc-masks against the
// already-matched symbol.
extern "C" u32 g_engineFrameDelta;

// CBehindCandyAni::GetTypeTag (0x00010030) is now an inline member in the class header.

// CBehindCandyAni::Serialize @0x010050 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x00010050, 0x47)
i32 CBehindCandyAni::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, (CGameObject*)d) != 0;
}

// CBehindCandyAni::~CBehindCandyAni @0x0100f0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CBehindCandyAni() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CBehindCandyAni@@UAE@XZ 0x000100f0 0x44

// --- CBehindCandyAni (0x0ad540), vptr 0x5e838c --- the ctor anchors GetTypeTag
// @0x10030 + the ??_7CBehindCandyAni vtable in this TU. Folds the inline
// CUserLogic(obj) base + the shared z-clamp tail.
RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_1a0.m_14 == 0) {
        m_value = m_38->m_1a0.m_14;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_width >= g_buteMgr.GetInt("World", "BigActHeight")
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

// CBehindCandyAni::InitActReg @0x0ad7d0 - construct the class's activation-
// coordinate registry singleton (g_behindCandyActReg @0x645f98) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ad7d0, 0x15)
void CBehindCandyAni::InitActReg() {
    ((CZDArrayDerived*)&g_behindCandyActReg)->Construct(2000, 2010);
}

// CBehindCandyAni::RunAct @0x0ad850 - resolve the registry entry for id; if a
// handler is bound, re-resolve and invoke it as a PMF on this, else return the
// entry pointer. Same archetype as CAniCycle::RunAct.
RVA(0x000ad850, 0x102)
void CBehindCandyAni::FireActivation(i32 id) {
    CBehindCandyActEntry* e = (CBehindCandyActEntry*)g_behindCandyActReg.ResolveEntry(id);
    if (e->m_fn != 0) {
        (this->*((CBehindCandyActEntry*)g_behindCandyActReg.ResolveEntry(id))->m_fn)();
    }
}

// CBehindCandyAni::RegisterActs @0x0ad9b0 - bind the class's per-frame handler
// (AdvanceAnim @0x0adbb0) to the activation key "A" via the shared name registry.
// The SAME archetype as CSecretLevelTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ad9b0, 0x18d)
void CBehindCandyAni::RegisterActs() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=("A");
        g_typeCounter++;
    }
    ((CBehindCandyActEntry*)g_behindCandyActReg.ResolveEntry(id))->m_fn =
        (i32 (CUserLogic::*)())&CBehindCandyAni::AdvanceAnim;
}

// CBehindCandyAni::AdvanceAnim @0x0adbb0 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta) and
// return 0. Byte-identical to CSimpleAnimation::AdvanceAnim save the call
// displacement.
RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CBehindCandyActEntry);
SIZE_UNKNOWN(CBehindCandyActReg);
