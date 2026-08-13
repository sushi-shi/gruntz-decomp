#include <rva.h>

#include <Gruntz/WormholeActs.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Wormhole.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0003f1f0, 0xa, int)
RVA_DYNINIT(0x0003f210, 0x15, int)
RVA_DYNINIT(0x0003f240, 0xe, int)
RVA_DYNINIT(0x0003f260, 0x1f, int)
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
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CExitTrigger>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CExitTrigger::AdvanceAnim);
}
