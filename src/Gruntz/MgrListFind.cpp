#include <rva.h>

#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgrViews.h>

// @early-stop
RVA(0x000f0db0, 0x48)

i32 CellTargetable(i32 tileX, i32 tileY) {
    CPtrList& list = g_gameReg->m_cmdGrid->m_baseList;
    POSITION pos = list.GetHeadPosition();

    if (pos != 0) {
        do {
            CGruntPuddle* p = static_cast<CGruntPuddle*>(list.GetNext(pos));
            if (p->m_pending == 0) {
                i32 v54 = p->m_tileX;
                i32 v58 = p->m_tileY;
                if (v54 == tileX && v58 == tileY) {
                    return 1;
                }
            }
        } while (pos != 0);
    }
    return 0;
}
