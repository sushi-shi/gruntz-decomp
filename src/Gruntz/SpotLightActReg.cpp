#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/SpotLightActReg.h>
#include <Gruntz/SpotLight.h>

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

    *CActRegPool<CSpotLight>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CSpotLight::Update);
}
