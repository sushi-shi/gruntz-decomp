#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/PathHazardActReg.h>
#include <Gruntz/PathHazard.h>

template<> DATA(0x00246250)
CActReg CActRegPool<CPathHazard>::s_table(2000, 2010);

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

RVA(0x000b3cc0, 0x2ac)
void RegisterPathHazardActions() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CPathHazard::ForwardTick);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CPathHazard>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CPathHazard::ForwardSiblingTick);
}
