// DrawDebugStats.cpp - the in-game debug-overlay text renderer (0xcf770), a
// __thiscall HUD method on the PLAY game-state object. Recovered from the $SG
// string set ("Fps = %i ", " Objs = %i ", " Pos = %i,%i", " Timing = On ",
// " Sent = %i, Rcvd = %i, Frame = %i Counter = %lu") + the GDI call set
// (SetBkMode/SetTextColor/SetBkColor/DrawTextA/TextOutA).
//
// Structure: a debug-flags byte (g_debugFlags @0x6455f4) gates the whole thing
// (bit 0x20 = master off) and each text piece (0x10 Fps, 0x1 Objs, 0x4 Pos,
// 0x40 Timing, 0x80 elapsed-time, 0x2 net stats). Each piece sprintf's into a
// scratch buffer and inline-strcat's onto the accumulator; the 0x80 piece builds
// a CString (FormatElapsed @0x1190f0, "%i:%02i:%02i") which gives the routine its
// /GX exception frame, so it lives in an `eh` unit. The tail grabs a DC from a
// polymorphic source (m_c->m_4->m_14->m_2c->m_8; GetDC = vtable slot +0x44, Done
// = slot +0x68, both __stdcall taking the object explicitly), sets text attrs,
// runs the owner's post-setup virtual (slot +0x94), and draws the accumulated
// text bottom-aligned in the level's rect (g_dbgMgr->GetRect, then DrawTextA when
// rect.left>0 else TextOutA).
//
// CARCASS doctrine: the owning class + the m_4/m_c sub-objects are unmatched
// engine classes accessed by raw this+offset; every callee is a reloc-masked
// external __thiscall/__stdcall/__cdecl thunk; the format strings are $SG
// literals reloc-masked against the matched string symbols. Only the offsets /
// code bytes are load-bearing.

#include <Mfc.h>    // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde / += 0x1ba0c8)
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcat/strlen intrinsics (/O2)

#include <rva.h>

DATA(0x002455f4)
extern u8 g_debugFlags; // 0x6455f4  debug-overlay flag bits
DATA(0x00245588)
extern "C" u32 g_645588; // a wrap-safe draw/elapsed counter (FormatElapsed arg + %lu)

// FUN_001190f0 __cdecl: format the counter as "%i:%02i:%02i" into a returned
// CString (copy-construct into the caller's hidden return slot).
CString FormatElapsed(i32 count);

// The fps source (this->m_4).
struct DbgFpsSrc {
    char m_pad00[0x18];
    i32 m_18; // +0x18  fps
};
// The object-count source (this->m_c->m_8).
struct DbgObjsSrc {
    char m_pad00[0x1c];
    i32 m_1c; // +0x1c  active object count
};
// The position source (this->m_c->m_24->m_5c).
struct DbgPos {
    char m_pad00[0x84];
    i32 m_84; // +0x84  x
    i32 m_88; // +0x88  y
};
struct DbgPosRoot { // this->m_c->m_24
    char m_pad00[0x5c];
    DbgPos* m_5c; // +0x5c
};
// The DC source reached through this->m_c->m_4->m_14->m_2c->m_8: a polymorphic
// object whose vtable carries GetDC at slot +0x44 (#17) and Done at slot +0x68
// (#26), both __stdcall taking the object explicitly.
struct DbgDcSrc;
struct DbgDcVtbl {
    void* s0[0x11];                              // slots 0..16
    void(__stdcall* GetDC)(DbgDcSrc*, HDC* out); // slot 17 == +0x44
    void* s18[0x68 / 4 - 0x12];                  // slots 18..25
    void(__stdcall* Done)(DbgDcSrc*, HDC);       // slot 26 == +0x68
};
struct DbgDcSrc {
    DbgDcVtbl* m_vtbl; // +0x00
};
struct DbgDcHost { // this->m_c->m_4->m_14->m_2c
    char m_pad00[0x8];
    DbgDcSrc* m_8; // +0x08
};
struct DbgL14 { // this->m_c->m_4->m_14
    char m_pad00[0x2c];
    DbgDcHost* m_2c; // +0x2c
};
struct DbgDcRoot { // this->m_c->m_4
    char m_pad00[0x14];
    DbgL14* m_14; // +0x14
};
struct DbgMc { // this->m_c
    char m_pad00[0x4];
    DbgDcRoot* m_4;  // +0x04  DC chain root
    DbgObjsSrc* m_8; // +0x08  object-count source
    char m_pad0c[0x24 - 0xc];
    DbgPosRoot* m_24; // +0x24  position root
};
// The *0x64556c game-registry singleton, this method's typed alias: GetRect
// returns the level's text rect by out-param + value.
struct DbgMgr {
    RECT* GetRect(RECT* buf); // FUN_0008e3a0 __thiscall
};
DATA(0x0024556c)
extern DbgMgr* g_dbgMgr; // *0x64556c, this method's alias

struct DbgVtbl; // completed below (GetFrame @+0x6c, PostSetup @+0x94)
class CDbgView {
public:
    void DrawDebugStats(); // 0xcf770

    DbgVtbl* m_vptr; // +0x000
    DbgFpsSrc* m_4;  // +0x004
    char m_pad08[0xc - 0x8];
    DbgMc* m_c; // +0x00c
    char m_pad10[0x2d0 - 0x10];
    i32 m_2d0; // +0x2d0  packets received
    i32 m_2d4; // +0x2d4  packets sent
};
// The owner's own virtuals, modeled as 4-byte PMFs off the vtable so MSVC emits
// `mov edx,[this]; mov ecx,this; call [edx+slot]` - the class must be COMPLETE
// before the typedefs (see docs/patterns/pmf-complete-class-4byte.md).
typedef i32 (CDbgView::*DbgGetFrameFn)();
typedef void (CDbgView::*DbgPostSetupFn)(HDC);
struct DbgVtbl {
    char m_pad00[0x6c];
    DbgGetFrameFn GetFrame; // +0x6c  current frame number
    char m_pad70[0x94 - 0x70];
    DbgPostSetupFn PostSetup; // +0x94  per-draw text-attr setup
};

// @source: string-xref
// Code bytes are byte-EXACT vs retail (verified instruction-by-instruction with
// llvm-objdump -dr, base obj vs delinked target). The 99.55% fuzzy residual is
// entirely the reloc/EH/import scoring artifact: differently-NAMED symbol operands
// (sprintf vs _sprintf, FormatElapsed vs Unmatched_1190f0, CString op/dtor, the
// g_dbgMgr alias vs _g_mgrSettings), the Win32 __imp__ import-thunk vs direct-call
// delink form, and the /GX __except_list/__CxxFrameHandler EH-table relocs + the
// CString cleanup unwind funclet. No instruction-byte difference - green-enough (§2a).
RVA(0x000cf770, 0x35e)
void CDbgView::DrawDebugStats() {
    if (g_debugFlags & 0x20) {
        return;
    }

    char buf[0x1f0];
    char scratch[0x40];
    buf[0] = 0;

    if (g_debugFlags & 0x10) {
        sprintf(scratch, "Fps = %i ", m_4->m_18);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x1) {
        sprintf(scratch, " Objs = %i ", m_c->m_8->m_1c);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x4) {
        DbgPos* p = m_c->m_24->m_5c;
        sprintf(scratch, " Pos = %i,%i", p->m_84, p->m_88);
        strcat(buf, scratch);
    }
    if (g_debugFlags & 0x40) {
        strcat(buf, " Timing = On ");
    }
    if (g_debugFlags & 0x80) {
        CString t = FormatElapsed(g_645588);
        t += " ";
        strcat(buf, t);
        t += " ";
    }
    if (g_debugFlags & 0x2) {
        sprintf(
            scratch,
            " Sent = %i, Rcvd = %i, Frame = %i Counter = %lu",
            m_2d4,
            m_2d0,
            (this->*(m_vptr->GetFrame))(),
            g_645588
        );
        strcat(buf, scratch);
    }

    DbgDcHost* host = m_c->m_4->m_14->m_2c;
    HDC hdc = 0;
    host->m_8->m_vtbl->GetDC(host->m_8, &hdc);
    if (hdc == 0) {
        return;
    }
    SetBkMode(hdc, 1);
    SetTextColor(hdc, 0xffffff);
    SetBkColor(hdc, 0);
    (this->*(m_vptr->PostSetup))(hdc);

    if (buf[0] != 0) {
        RECT rb;
        RECT lr;
        CopyRect(&lr, g_dbgMgr->GetRect(&rb));
        RECT dr;
        dr.left = lr.left;
        dr.top = lr.bottom - 0x1c;
        dr.right = lr.right;
        dr.bottom = lr.bottom;
        if (lr.left > 0) {
            DrawTextA(hdc, buf, -1, &dr, 0x20);
        } else {
            TextOutA(hdc, 0, dr.top, buf, strlen(buf));
        }
    }
    host->m_8->m_vtbl->Done(host->m_8, hdc);
}

SIZE_UNKNOWN(DbgFpsSrc);
SIZE_UNKNOWN(DbgObjsSrc);
SIZE_UNKNOWN(DbgPos);
SIZE_UNKNOWN(DbgPosRoot);
SIZE_UNKNOWN(DbgDcVtbl);
SIZE_UNKNOWN(DbgDcSrc);
SIZE_UNKNOWN(DbgDcHost);
SIZE_UNKNOWN(DbgL14);
SIZE_UNKNOWN(DbgDcRoot);
SIZE_UNKNOWN(DbgMc);
SIZE_UNKNOWN(DbgMgr);
SIZE_UNKNOWN(CDbgView);
SIZE_UNKNOWN(DbgVtbl);
