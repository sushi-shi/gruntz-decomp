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
#include <Gruntz/CBehindCandyAni.h>

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class). FireActivation invokes it __thiscall on the trigger.
typedef i32 (CBehindCandyAni::*BehindCandyHandler)();
struct CBehindCandyActEntry {
    BehindCandyHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645f98). Same
// [2000,2010] fixed-range shape as CSecretActReg, built by the shared registry
// ctor (0x408710, __thiscall ret 8). The id->entry resolve (ResolveEntry) folds
// the VActLookup archetype; the slow Insert is __thiscall on m_coll2.
struct CBehindCandyActReg {
    void* m_vptr;       // +0x00
    CActColl2* m_coll2; // +0x04
    i32 m_lo;           // +0x08
    i32 m_hi;           // +0x0c
    char* m_base;       // +0x10
    char* m_cur;        // +0x14
    i32 m_stride;       // +0x18
    char m_pad1c[0x20 - 0x1c];
    i32 m_scratch; // +0x20

    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8)

    char* ResolveEntry(i32 id) {
        m_scratch = 0;
        if (id >= m_lo && id <= m_hi) {
            return m_base + (id - m_lo) * m_stride;
        }
        if (((CActColl*)this)->Find(id, 0)) {
            return m_base + (id - m_lo) * m_stride;
        }
        void* item = g_actCache;
        g_actAllocResult = (void*)ActAlloc();
        m_coll2->Insert(this, item, 0xc);
        return m_cur;
    }
};
DATA(0x00245f98)
extern CBehindCandyActReg g_behindCandyActReg; // 0x645f98

// The animation sub-object embedded at CGameObject+0x1a0 (the bound object is
// CUserLogic::m_38). Its setter (0x15c360, __thiscall, 1 arg) re-targets the
// active animation to the draw-delta passed in; modeled NO-body so the call
// reloc-masks. The SAME engine method CSimpleAnimation / CGruntPuddle / the
// projectile render-obj all call.
struct CAnimSink {
    i32 SetAnim(u32 ctx); // 0x15c360
};

// The global the advance hands the sink (_g_6bf3bc; the per-frame draw-delta
// mirror). Declared extern "C" here so the value-load reloc-masks against the
// already-matched symbol.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

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
    ((CBehindCandyActEntry*)g_behindCandyActReg.ResolveEntry(id))->m_fn = &CBehindCandyAni::AdvanceAnim;
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
