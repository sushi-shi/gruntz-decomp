// SBI_SideTabBuild.cpp - CStatzTabBuilder::Build (C:\Proj\Gruntz), the STATZ side-tab builder.
//
// UN-MERGED back to its own TU (2026-07-13); see WarpStoneFly.cpp. Like MgrSettings this
// obj was flags="base" (no /GX) while SBI_RectOnly.cpp is flags="eh".
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiSideTabBuildViews.h> // CSBI_SideTab (ctor view) + CStatzTabBuilder + settings
#include <Gruntz/GruntzMgr.h>            // the *0x24556c singleton (CGruntzMgr)

extern "C" CGruntzMgr* g_gameReg;
extern "C" i32 g_curPlayer;

// @early-stop
// this/newobj callee-saved register-pinning wall (docs/patterns/
// zero-register-pinning.md) + the vptr-position wall: the loop body - geometry-base
// branch, the CSBI_SideTab item field-init + auto-stamped 0x5eae3c vptr, the 13-arg
// BuildStatzTabStatusBar call, the AddTail + slot store and the failure-path
// scalar-delete - is logic byte-faithful. Residuals: a regalloc coin-flip (retail pins
// this->edi and newobj->esi, reusing the zeroed newobj as a zero-constant; cl pins
// this->esi / newobj->edi) and the vptr stamped FIRST by the real ctor vs MIDDLE in
// retail's inline init. No source lever flips either. Deferred to the final sweep.
RVA(0x00105070, 0x10e)
i32 CStatzTabBuilder::Build() {
    i32 i = 0;
    for (i32 strid = 0xd9; strid < 0x1e7; strid += 0x12) {
        i32 geomBase;
        i32 geomVal;
        if (m_0 == 0) {
            geomBase = m_10 - 0x1c;
            geomVal = m_10;
        } else {
            geomBase = m_18;
            geomVal = m_18 + 0x1c;
        }
        CSBI_SideTab* newobj = new CSBI_SideTab;
        // `this` IS the builder - it is the parent the configure reads m_10/m_18 off. The
        // old view typed that param CSBI_SideTab* purely to compile, which forced this
        // cross-cast of a CStatzTabBuilder to an unrelated class.
        i32 ok = newobj->BuildStatzTabStatusBar(
            this,
            (CSbiConfigHost*)g_gameReg->m_world,
            i + 0xb,
            0,
            geomBase,
            strid - 0x11,
            geomVal,
            strid,
            "GAME_STATUSBAR_TABZ_STATZTAB_TAB",
            g_curPlayer,
            i,
            m_114[i],
            m_0 == 0
        );
        if (ok == 0) {
            delete newobj;
            return 0;
        }
        m_2c.AddTail((CObject*)newobj);
        m_150[i] = newobj;
        i++;
    }
    return 1;
}
