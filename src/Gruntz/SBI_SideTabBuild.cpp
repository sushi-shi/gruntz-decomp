#include <rva.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/CurPlayer.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiSideTabBuildViews.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/GruntzMgr.h>

// @early-stop
RVA(0x00105070, 0x10e)
i32 CStatusBarMgr::BuildSideTabs() {
    i32 i = 0;
    for (i32 strid = 0xd9; strid < 0x1e7; strid += 0x12) {
        i32 geomBase;
        i32 geomVal;
        if (m_position == 0) {
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
            i + 0xb,
            0,
            geomBase,
            strid - 0x11,
            geomVal,
            strid,
            "GAME_STATUSBAR_TABZ_STATZTAB_TAB",
            g_curPlayer,
            i,
            m_statFlags[i],
            m_position == 0
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
