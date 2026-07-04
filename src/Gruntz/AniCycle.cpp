// AniCycle.cpp - an animation-cycle eyecandy game-object (C:\Proj\Gruntz). One
// trace-discovered method:
//   RegisterActs @0x0ab0e0 - bind the per-frame handler to the "A" key.
//
// CAniCycle : CUserLogic. Only offsets / code bytes are load-bearing.
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/AniCycle.h>
#include <Gruntz/SerialObjRef.h> // the shared serialized-object-reference (Chain @0x8c00)

// The handler entry the per-class registry yields: its first dword receives the
// per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on this single-inheritance
// class).
typedef i32 (CAniCycle::*AniCycleHandler)();
struct CAniCycleActEntry {
    AniCycleHandler m_fn;
};

// The class's activation-coordinate registry singleton (@0x646088), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). Was a per-file
// duplicate of the <Gruntz/ActReg.h> CActReg archetype (layout + ResolveEntry); now
// derives from it, keeping its own placeholder name so the DATA-pinned global is
// unchanged.
struct CAniCycleActReg : public CActReg {};
DATA(0x00246088)
extern CAniCycleActReg g_aniCycleActReg; // 0x646088

// CAniCycle::GetTypeTag @0x00f450 - the vtable slot-2 logic-type id accessor
// (the 6-byte `mov eax,<id>; ret` archetype).
RVA(0x0000f450, 0x6)
LogicTypeId CAniCycle::GetTypeTag() {
    return LOGIC_ANICYCLE; // 0x3ea
}

// CAniCycle::Serialize @0x00f470 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 sub-object's
// chain; both run the same (ar, tag, c, d) tuple. Returns the second chain's success
// normalized to a bool. Byte-identical to CCursorSnapSprite::Serialize (0x011880)
// save the two call displacements.
RVA(0x0000f470, 0x47)
i32 CAniCycle::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CAniCycle::InitActReg @0x0aaf00 - construct the class's activation-coordinate
// registry singleton (g_aniCycleActReg @0x646088) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x000aaf00, 0x15)
void CAniCycle::InitActReg() {
    g_aniCycleActReg.Construct(2000, 2010);
}

// CAniCycle::RegisterActs @0x0ab0e0 - bind the class's per-frame handler
// (AdvanceAnim @0x0ab2e0) to the activation key "A" via the shared name registry.
// The SAME archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000ab0e0, 0x18d)
void CAniCycle::RegisterActs() {
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
    ((CAniCycleActEntry*)g_aniCycleActReg.ResolveEntry(id))->m_fn = &CAniCycle::AdvanceAnim;
}

#include <rva.h>
SIZE_UNKNOWN(CAniCycle);
SIZE_UNKNOWN(CAniCycleActEntry);
SIZE_UNKNOWN(CAniCycleActReg);
