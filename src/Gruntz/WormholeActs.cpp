#include <rva.h>

#include <Gruntz/WormholeActs.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Wormhole.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

template<> DATA(0x002445c0)
CActReg CActRegPool<CExitTrigger>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA(0x0003f290, 0x102)
void CExitTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CExitTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

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
            if (list != NULL) {
                list->CString::CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CExitTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}
