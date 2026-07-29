#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/TypeKeyColl.h>     // s_codeA/s_actKeyB registration keys
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>          // the shared activation-registrar archetype (CActReg)
#include <Gruntz/SpotLightActReg.h> // CActRegPool<CSpotLight>::s_table decl
#include <Gruntz/SpotLight.h>

// CActRegPool<CSpotLight>::s_table (0x00246188): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
template<> DATA(0x00246188)
CActReg CActRegPool<CSpotLight>::s_table(2000, 2010);

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
// RegisterActs_646188 @0x0b1790 - bind handler "A" (0x4025db) and handler "B"
// (0x402414) into the per-class registry @0x646188.
// ===========================================================================
// Two-key registrar: cl5 spends its inline budget from the outside in, so only the
// SECOND key's name lookup expands the grow-fail report; the other three lookups keep
// it as the out-of-line zErrHandling::Report call.
// docs/patterns/act-registrar-report-outline-budget.md
RVA(0x000b1790, 0x2ac)
void RegisterActs_646188() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }
    // ILT 0x4025db -> 0x0b1af0 == CSpotLight::Tick.
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CSpotLight::Tick);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }
    // ILT 0x402414 -> 0x0b1ee0 == CSpotLight::Update.
    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CSpotLight::Update);
}
