#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/ActReg.h>          // the shared CActReg coordinate-registry archetype
#include <Gruntz/Wormhole.h>        // (kept: Wormhole.cpp shares this TU-header's registry note)
#include <Gruntz/ExitTrigger.h>     // the owning class (act dispatcher)
#include <Gruntz/UserLogic.h>

#include <rva.h>
#include <Wap32/ZVec.h>
#include <Gruntz/WormholeActs.h> // CActRegPool<CExitTrigger>::s_table decl

// CActRegPool<CExitTrigger>::s_table (0x002445c0): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x002445c0)
CActReg CActRegPool<CExitTrigger>::s_table(2000, 2010);

// CWormhole::FireAct @0x03f290 [@identity-TODO: retail says this is CExitTrigger's
// vtable slot 4 - ILT 0x42e6 `jmp 0x3f290` + CExitTrigger vtbl 0x1e822c slot 4 == 0x42e6;
// see the note in <Gruntz/Wormhole.h>] - look the activation coordinate up in the
// class registry (CActRegPool<CExitTrigger>::s_table); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same
// archetype as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x0003f290, 0x102)
void CExitTrigger::FireActivation(i32 coord) {
    CActHandler* e =
        (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CWormhole::RegisterActs @0x03f3f0 - bind the per-frame handler (AdvanceAnim
// @0x03f5f0) to the activation key "A" via the shared name registry. The SAME
// archetype as CGruntCreationPoint::RegisterActs.
//
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0003f3f0, 0x18d)
void CExitTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CExitTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}
