#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/ZVec.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GruntVoiceActReg.h>
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

RVA(0x00119fa0, 0x2ac)
void RegisterGruntVoiceActions() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookupCallReport(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "A";
        g_typeCounter++;
    }

    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id) =
        static_cast<CActHandler>(&CGruntVoice::IdleHidden);

    i32 id2 = ActFindId("B");
    if (id2 == 0) {
        ActInsertId("B", g_typeCounter);
        id2 = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        FreeNameSlotNodes();
        *slot = "B";
        g_typeCounter++;
    }

    *CActRegPool<CGruntVoice>::s_table.ResolveEntryCallReport(id2) =
        static_cast<CActHandler>(&CGruntVoice::Update);
}
