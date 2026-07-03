#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiSideTabBuildViews.h> // CSBI_SideTab (ctor view) + CStatzTabBuilder + settings
// SBI_SideTabBuild.cpp - the STATZTAB "Build" factory (0x105070), re-homed
// from src/Stub/CSBI_SideTab.cpp (C:\Proj\Gruntz).
//
// 0x105070 was mislabeled ~CSBI_SideTab by the rtti-vptr heuristic; it is actually
// a builder method on the status-bar STATZTAB CONTAINER (the `this`/edi object,
// modeled here as CStatzTabBuilder): it loops the 15 stat slots (string-id 0xd9,
// step 0x12, up to 0x1e7), `new`s a 0x5c-byte REAL CSBI_SideTab child per slot
// (operator new == RezAlloc; MSVC auto-stamps ??_7CSBI_SideTab@@6B@ = 0x5eae3c, no
// manual stamp), then runs CSBI_SideTab::BuildStatzTabStatusBar (0xe9600, via ILT
// 0x33c3) on it; on success it appends the child to the container's +0x2c CObList
// and stores it into the +0x150 slot array. On any build failure it `delete`s the
// child (the slot-0 scalar-deleting dtor) and bails. Field names are placeholders;
// the OFFSETS + code bytes are the load-bearing facts.

// The engine block allocator (0x1b9b46 = operator new); __cdecl, caller-cleans.
// The throwing global operator new (_RezAlloc @0x1b9b46) is declared by <Mfc.h>.
void* RezAlloc(i32 size); // 0x1b9b46

// CSBI_SideTab (ctor/builder view), CSbBuildSettings + CStatzTabBuilder moved to
// <Gruntz/SbiSideTabBuildViews.h>.

// The current area index global the builder folds in as arg10 (0x644c54).
extern "C" i32 g_644c54; // 0x644c54

extern "C" CSbBuildSettings* g_mgrSettings; // 0x64556c

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
        i32 ok = newobj->BuildStatzTabStatusBar(
            (CSBI_SideTab*)this,
            g_mgrSettings->m_world,
            i + 0xb,
            0,
            geomBase,
            strid - 0x11,
            geomVal,
            strid,
            "GAME_STATUSBAR_TABZ_STATZTAB_TAB",
            g_644c54,
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
