// CSingleFrameMessage.cpp - a single-frame message eyecandy game-object
// (C:\Proj\Gruntz). One trace-discovered method:
//   RegisterActs @0x0ab710 - bind the per-frame handler to the "A" key.
//
// CSingleFrameMessage : CUserLogic. Only offsets / code bytes are load-bearing.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/CSingleFrameMessage.h>

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CSingleFrameMessage::*SingleFrameHandler)();
struct CSingleFrameActEntry {
    SingleFrameHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645ef0). Same
// [2000,2010] fixed-range shape / layout as CBehindCandyActReg, built by the
// shared registry ctor (0x408710). The id->entry resolve folds the VActLookup
// archetype; the slow Insert is __thiscall on m_coll2.
struct CSingleFrameActReg {
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
DATA(0x00245ef0)
extern CSingleFrameActReg g_singleFrameActReg; // 0x645ef0

// CSingleFrameMessage::InitActReg @0x0ab530 - construct the class's activation-
// coordinate registry singleton (g_singleFrameActReg @0x645ef0) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ab530, 0x15)
void CSingleFrameMessage::InitActReg() {
    g_singleFrameActReg.Construct(2000, 2010);
}

// CSingleFrameMessage::RegisterActs @0x0ab710 - bind the class's per-frame handler
// (AdvanceAnim @0x0ab910) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ab710, 0x18d)
void CSingleFrameMessage::RegisterActs() {
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
    ((CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id))->m_fn = &CSingleFrameMessage::AdvanceAnim;
}
