#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CurPlayer.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SBI_SideTab.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>

// @early-stop
RVA(0x00105070, 0x10e)
i32 CStatusBarMgr::BuildSideTabs() {
    i32 i = 0;
    for (i32 strid = 0xd9; strid < 0x1e7; strid += 0x12) {
        i32 geomBase;
        i32 geomVal;
        if (m_position == STATUSBAR_DOCK_RIGHT) {
            geomBase = m_rect10.left - 0x1c;
            geomVal = m_rect10.left;
        } else {
            geomBase = m_rect10.right;
            geomVal = m_rect10.right + 0x1c;
        }
        CSBI_SideTab* newobj = new CSBI_SideTab;

        i32 ok = newobj->BuildStatzTabStatusBar(
            this,
            g_gameReg->m_world,
            static_cast<SbiCommandId>(IDX(SBICMD_SIDE_TAB_FIRST) + i),
            TAB_CONTROLS,
            geomBase,
            strid - 0x11,
            geomVal,
            strid,
            "GAME_STATUSBAR_TABZ_STATZTAB_TAB",
            g_curPlayer,
            i,
            static_cast<StatusSampleMode>(m_statFlags[i]),
            m_position == STATUSBAR_DOCK_RIGHT
        );
        if (ok == 0) {
            delete newobj;
            return 0;
        }
        m_tabLists[0].AddTail(newobj);
        m_hitRects[i] = newobj;
        i++;
    }
    return 1;
}
