#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
// SBI_SideTabBuild.cpp - the CSBI_SideTab "Build" factory (0x105070), re-homed
// from src/Stub/CSBI_SideTab.cpp (C:\Proj\Gruntz).
//
// 0x105070 was mislabeled ~CSBI_SideTab by the rtti-vptr heuristic; it is actually
// a builder method on the status-bar STATZTAB CONTAINER (the `this`/edi object):
// it loops the 15 stat slots (string-id 0xd9, step 0x12, up to 0x1e7), RezAlloc's
// a 0x5c-byte CSBI_SideTab child per slot, inlines its field-init + the 0x5eae3c
// vptr stamp, then runs CSBI_SideTab::BuildStatzTabStatusBar (0xe9600, via ILT
// 0x33c3) on it; on success it appends the child to the container's +0x2c CObList
// and stores it into the +0x150 slot array. On any build failure it scalar-deletes
// the child and bails. The mangled name is kept (?Build@CSBI_SideTab@@...); field
// names are placeholders, the OFFSETS + code bytes are the load-bearing facts.

// The engine block allocator (0x1b9b46 = operator new); __cdecl, caller-cleans.
void* RezAlloc(i32 size); // 0x1b9b46

// The built status-bar item (a CSBI_SideTab child, 0x5c bytes). BuildStatzTab-
// StatusBar (0xe9600, reached via ILT 0x33c3) populates it; on failure slot-0 (the
// scalar-deleting dtor) frees it. Only the touched offsets are modeled.
class CSbItem;
typedef void (CSbItem::*SbDeleteFn)(u32);
struct CSbItemVtbl {
    SbDeleteFn Delete; // [0x00] scalar-deleting dtor
};
class CSbItem {
public:
    // 0xe9600 (CSBI_SideTab::BuildStatzTabStatusBar), __thiscall ret 13 args.
    i32 BuildStatzTabStatusBar(
        CSbItem* parent,
        void* statusbar,
        i32 p3,
        i32 p4,
        i32 p5,
        i32 p6,
        i32 p7,
        i32 p8,
        const char* key,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 onLeft
    );
    void Delete(u32 flags) {
        (this->*(vptr->Delete))(flags);
    }

    CSbItemVtbl* vptr; // +0x00
    i32 m_4, m_8;      // +0x04, +0x08
    char m_pad0c[0x24 - 0x0c];
    i32 m_24, m_28; // +0x24, +0x28
    char m_pad2c[0x30 - 0x2c];
    i32 m_30, m_34; // +0x30, +0x34
    i32 m_38;       // +0x38
    char m_pad3c[0x44 - 0x3c];
    i32 m_44; // +0x44
};

// The CSBI_SideTab child vtable (0x5eae3c) stamped into each item; reloc-masked.
extern CSbItemVtbl g_vtbl_sideTab; // 0x5eae3c

// The current area index global the builder folds in as arg10 (0x644c54).
extern "C" i32 g_644c54; // 0x644c54

// The settings/registry singleton (0x64556c); its +0x30 is the level's status-bar
// owner passed as the StatzTab arg2.
struct CSbBuildSettings {
    char m_pad00[0x30];
    void* m_30; // +0x30
};
extern "C" CSbBuildSettings* g_mgrSettings; // 0x64556c

// ---------------------------------------------------------------------------
// CSBI_SideTab - modeled here as the STATZTAB CONTAINER Build runs on: a gate at
// +0x00, the two geometry-base pointers at +0x10/+0x18, the +0x2c child CObList,
// and the parallel +0x114 key / +0x150 child-slot arrays (15 entries, 0x3c apart).
class CSBI_SideTab {
public:
    i32 Build(); // 0x105070

    i32 m_0; // +0x00  gate (0 => geometry from m_10, else from m_18)
    char m_pad04[0x10 - 0x04];
    i32 m_10; // +0x10
    char m_pad14[0x18 - 0x14];
    i32 m_18; // +0x18
    char m_pad1c[0x2c - 0x1c];
    CObList m_2c; // +0x2c  child list (sizeof CObList == 0x1c -> ends +0x48)
    char m_pad48[0x114 - 0x48];
    i32 m_114[15];      // +0x114  per-slot key inputs
    CSbItem* m_150[15]; // +0x150  built child slots
};

// @early-stop
// ~73% (this/newobj callee-saved register-pinning wall, docs/patterns/
// zero-register-pinning.md): the loop body - geometry-base branch, the inlined
// CSBI_SideTab item field-init + 0x5eae3c vptr stamp, the 13-arg
// BuildStatzTabStatusBar call, the AddTail + slot store and the failure-path
// scalar-delete - is logic byte-faithful (every call/string/offset/immediate
// matches). The residual is a regalloc coin-flip: retail pins this->edi and
// newobj->esi (reusing the zeroed newobj as the `cmp [this],esi`/`cmp eax,esi`
// zero-constant), while cl pins this->esi / newobj->edi, so the m_0 / alloc-result
// tests materialize a separate zero in eax + the local stack-slot allocation
// shifts (0xc vs 0x14). No source lever flips the callee-saved assignment.
// Deferred to the final sweep.
RVA(0x00105070, 0x10e)
i32 CSBI_SideTab::Build() {
    i32 i = 0;
    for (i32 strid = 0xd9; strid < 0x1e7; strid += 0x12) {
        CSbItem* newobj = 0;
        i32 geomBase;
        i32 geomVal;
        if (m_0 == 0) {
            geomBase = m_10 - 0x1c;
            geomVal = m_10;
        } else {
            geomBase = m_18;
            geomVal = m_18 + 0x1c;
        }
        CSbItem* obj = (CSbItem*)RezAlloc(0x5c);
        if (obj) {
            obj->m_4 = 0;
            obj->m_8 = 0;
            obj->m_24 = 0;
            obj->m_28 = 0;
            obj->m_30 = 0;
            obj->m_34 = 0;
            obj->vptr = &g_vtbl_sideTab;
            obj->m_38 = -1;
            obj->m_44 = -1;
            newobj = obj;
        }
        i32 ok = newobj->BuildStatzTabStatusBar(
            (CSbItem*)this,
            g_mgrSettings->m_30,
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
            if (newobj) {
                newobj->Delete(1);
            }
            return 0;
        }
        m_2c.AddTail((CObject*)newobj);
        m_150[i] = newobj;
        i++;
    }
    return 1;
}
