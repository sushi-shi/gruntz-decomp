// SingleFrameMessage.cpp - a single-frame message eyecandy game-object
// (C:\Proj\Gruntz). One trace-discovered method:
//   RegisterActs @0x0ab710 - bind the per-frame handler to the "A" key.
//
// CSingleFrameMessage : CUserLogic. Only offsets / code bytes are load-bearing.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/SingleFrameMessage.h>

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CSingleFrameMessage::*SingleFrameHandler)();
struct CSingleFrameActEntry {
    SingleFrameHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x645ef0), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CSingleFrameActReg : public CActReg {};
DATA(0x00245ef0)
extern CSingleFrameActReg g_singleFrameActReg; // 0x645ef0

// CSingleFrameMessage::InitActReg @0x0ab530 - construct the class's activation-
// coordinate registry singleton (g_singleFrameActReg @0x645ef0) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000ab530, 0x15)
void CSingleFrameMessage::InitActReg() {
    ((CZDArrayDerived*)&g_singleFrameActReg)->Construct(2000, 2010);
}

// CSingleFrameMessage::RunAct @0x0ab5b0 - resolve the registry entry for id; if a
// handler is bound, re-resolve and invoke it as a PMF on this, else return the
// entry pointer. Same archetype as CAniCycle::RunAct.
RVA(0x000ab5b0, 0x102)
i32 CSingleFrameMessage::RunAct(i32 id) {
    CSingleFrameActEntry* e = (CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id);
    if (e->m_fn != 0) {
        return (this->*((CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id))->m_fn)();
    }
    return (i32)e;
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
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CSingleFrameActEntry*)g_singleFrameActReg.ResolveEntry(id))->m_fn =
        &CSingleFrameMessage::AdvanceAnim;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
SIZE_UNKNOWN(CSingleFrameActEntry);
SIZE_UNKNOWN(CSingleFrameActReg);
