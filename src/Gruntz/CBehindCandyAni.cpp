// CBehindCandyAni.cpp - a behind-candy eyecandy animation game-object
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
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/CBehindCandyAni.h>
#include <Gruntz/CAnimSink.h>
#include <Gruntz/CSerialObjRef.h> // the shared serialized-object-reference (Chain @0x8c00)

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). FireActivation invokes it __thiscall on the trigger.
typedef i32 (CBehindCandyAni::*BehindCandyHandler)();
struct CBehindCandyActEntry {
    BehindCandyHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645f98), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). CBehindCandyActReg
// is the shared <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its
// layout + ResolveEntry); it keeps its own placeholder name so the DATA-pinned
// global symbol is unchanged.
struct CBehindCandyActReg : public CActReg {};
DATA(0x00245f98)
extern CBehindCandyActReg g_behindCandyActReg; // 0x645f98

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Declared extern "C" here so the value-load reloc-masks against the
// already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// CBehindCandyAni::GetTypeTag @0x010030 - the vtable slot-2 logic-type id accessor
// (the 6-byte `mov eax,<id>; ret` archetype).
RVA(0x00010030, 0x6)
LogicTypeId CBehindCandyAni::GetTypeTag() {
    return LOGIC_BEHINDCANDYANI; // 0x3f3
}

// CBehindCandyAni::Serialize @0x010050 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x00010050, 0x47)
i32 CBehindCandyAni::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CBehindCandyAni::~CBehindCandyAni @0x0100f0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to the
// established leaf dtors; the empty body is enough for cl.
RVA(0x000100f0, 0x44)
CBehindCandyAni::~CBehindCandyAni() {}

// CBehindCandyAni::InitActReg @0x0ad7d0 - construct the class's activation-
// coordinate registry singleton (g_behindCandyActReg @0x645f98) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ad7d0, 0x15)
void CBehindCandyAni::InitActReg() {
    g_behindCandyActReg.Construct(2000, 2010);
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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CBehindCandyActEntry*)g_behindCandyActReg.ResolveEntry(id))->m_fn =
        &CBehindCandyAni::AdvanceAnim;
}

// CBehindCandyAni::AdvanceAnim @0x0adbb0 - re-target the bound object's
// animation sub-object (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc) and
// return 0. Byte-identical to CSimpleAnimation::AdvanceAnim save the call
// displacement.
RVA(0x000adbb0, 0x17)
i32 CBehindCandyAni::AdvanceAnim() {
    ((CAnimSink*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc);
    return 0;
}

#include <rva.h>
SIZE_UNKNOWN(CBehindCandyActEntry);
SIZE_UNKNOWN(CBehindCandyActReg);
SIZE_UNKNOWN(CBehindCandyAni);
