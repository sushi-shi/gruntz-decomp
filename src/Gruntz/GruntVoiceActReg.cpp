#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>           // the shared activation-registrar archetype (CActReg)
#include <Gruntz/GruntVoiceActReg.h> // CActRegPool<CGruntVoice>::s_table decl
#include <Gruntz/GruntVoice.h>

static inline void FreeNameSlotNodes() {
    i32 n = g_typeColl.m_grown;
    CString* list = ActNameSlots();
    while (n-- != 0) {
        if (list != 0) {
            list->CString::~CString();
        }
        list++;
    }
}

// ===========================================================================
// RegisterActs_6514d8 @0x0119fa0 - bind handler "A" (0x4037bf) and handler "B"
// (0x402dd8) into the per-class registry @0x6514d8. (The registry's Construct
// already lives in GruntVoice.cpp, over [2000, 2010].)
// ===========================================================================
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x00119fa0, 0x2ac)
void RegisterActs_6514d8() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }
    // @identity-TODO a free `void()` registrant into a member-fn-ptr slot
    *reinterpret_cast<void**>(CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id)) =
        static_cast<void*>(&GruntVoiceActA);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }
    // @identity-TODO a free `void()` registrant into a member-fn-ptr slot
    *reinterpret_cast<void**>(CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id2)) =
        static_cast<void*>(&GruntVoiceActB);
}
