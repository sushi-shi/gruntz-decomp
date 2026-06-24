// CMulti.cpp - the multiplayer / lobby game-state (C:\Proj\Gruntz). Recovered
// from RTTI (.?AVCMulti@@, vtable 0x5e9fe4) after the dynamic-this tracer
// mis-attributed the 0x08d270 / 0x0b6110-0x0bc420 cluster to CPlay: every method
// touches the networking/lobby sub-object block at member offsets > +0x510
// (m_524 / m_580 / m_59c / m_5a0 / m_5bc / m_604) and the MULTI_JOIN /
// "Error: %s - %i" / "%s (%i)" lobby strings, which CPlay (layout tops at +0x510)
// has not got. CMulti : public CPlay, public CState (CHD numBaseClasses=3); the
// most-derived dtor walks down stamping CMulti -> CPlay -> CState vtables.
//
// Reconstructed in ascending retail-RVA order. Field names are placeholders;
// only the OFFSETS + the per-method call/branch structure are load-bearing
// (campaign carcass doctrine). Engine callees (SendNetStat / SendStatFlag / the
// m_4 logic object / the heap deleters / the MFC CString/CByteArray dtors) are
// external no-body fns -> their `call rel32` are reloc-masked.
#include <rva.h>
#include <Gruntz/CMulti.h>
#include <stdio.h> // engine sprintf (reloc-masked)

// ---------------------------------------------------------------------------
// The three retail vtables the most-derived dtor stamps as it walks the sub-
// objects (referenced by address as reloc-masked DATA externs; their contents
// are not reproduced in this TU, so a polymorphic model would emit a divergent
// ??_7 - the manual stamp is the transitional workaround).
// ---------------------------------------------------------------------------
DATA(0x001e9fe4)
extern void* g_vtbl_CMulti[]; // 0x5e9fe4
DATA(0x001ea0bc)
extern void* g_vtbl_CPlay[]; // 0x5ea0bc
DATA(0x001ea21c)
extern void* g_vtbl_CState[]; // 0x5ea21c

// ---------------------------------------------------------------------------
// The DirectPlay error globals (shared with CNetMgr::ReportError; same .data
// addresses, re-declared TU-local so the DIR32 operands reloc-mask).
// ---------------------------------------------------------------------------
DATA(0x002bf6fc)
extern "C" i32 g_code; // 0x6bf6fc  hr & 0xffff (the (%i) arg)
DATA(0x002bf700)
extern "C" char g_szCode[]; // 0x6bf700  error-code name buffer

// ---------------------------------------------------------------------------
// External engine helpers CMulti drives (reloc-masked rel32 calls). The names
// are placeholders; only the call shape (args / cleanup) is load-bearing.
// ---------------------------------------------------------------------------

// __stdcall free stat-channel writer (0x0b9290, ret 0xc): SendNetStat(chan, id, flag).
extern void __stdcall SendNetStat(i32 chan, i32 id, i32 flag);

// The engine heap free (CLobbyObjA/B teardown above pairs with it).
extern "C" void RezFree(void* p); // 0x005b9b82 (_RezFree, __cdecl)

// CState's base teardown (the dtor tail-calls it after stamping the CState
// vtable) - reloc-masked thiscall; modeled on a tiny helper so `mov ecx,esi; call`
// falls out with no stack cleanup.
class CMultiStateBase {
public:
    void BaseCleanup(); // 0x00403f53
};

// MFC member sub-object destructors (out-of-line NAFXCW; reloc-masked). Declared
// as the exact thiscall shapes so clang emits the right `mov ecx; call` bytes;
// the real CString/CByteArray dtors below are pulled from <Mfc.h>.

// The MULTI_JOIN dialog handler whose address is pushed into RunErrorDialog.
extern void MultiJoinHandler(); // 0x004b8020 (reloc-masked function address)

// ===========================================================================
// CMulti::~CMulti  @ 0x08d270  - the most-derived /GX dtor. Runs Teardown()
// first, then tears the CString/CByteArray run while stamping the CMulti ->
// CPlay -> CState vtables in turn over the sub-objects.
// ===========================================================================
// @early-stop
// EH-dtor wall (docs/patterns/eh-dtor-needs-base-subobject.md): the body is the
// correct, complete reconstruction - Teardown(), then the member CString/CByteArray
// teardown run in retail order while the CMulti->CPlay->CState vtable stamps and
// the CState BaseCleanup tail land at the right points. Retail emits a FLAT 0x124
// dtor (the CByteArray[4] at +0x3a4 torn via a single ??_M vector-dtor call, light
// register use), while our /GX lowering unrolls the array loop, saves ebx/ebp/edi,
// and splits the per-member cleanup into trailing EH unwind funclets - so the main
// body diverges in instruction selection + register allocation despite matching
// logic. Plus the /GX prologue reads fs:0 before push -1 vs our push-first order.
// Documented EH-state-machine wall; deferred to the final sweep.
RVA(0x0008d270, 0x124)
CMulti::~CMulti() {
    *(void**)this = g_vtbl_CMulti;
    Teardown();
    // CMulti sub-object teardown (high block).
    m_604.~CByteArray();
    m_5b8.~CString();
    m_5b4.~CString();
    m_5a0.~CString();
    m_59c.~CString();
    m_598.~CString();
    // CPlay sub-object.
    *(void**)this = g_vtbl_CPlay;
    CPlayDtorBody();
    m_488.~CByteArray();
    m_410.~CString();
    // the CByteArray[4] vector at +0x3a4.
    for (i32 i = 3; i >= 0; --i) {
        m_3a4[i].~CByteArray();
    }
    m_370.~CByteArray();
    m_1b4.~CString();
    // CState sub-object.
    *(void**)this = g_vtbl_CState;
    ((CMultiStateBase*)this)->BaseCleanup();
}

// ===========================================================================
// CMulti::Teardown  @ 0x0b6110  - drains the lobby state on teardown: if the
// join gate is fully armed (m_524 && m_5bc && m_520 && m_580) push the two final
// stat updates, then free the two heap lobby sub-objects (m_520, m_320), release
// the report gate object (m_524, via its vtable dtor), and hand m_590 back to the
// logic object (m_4->m_110).
// ===========================================================================
RVA(0x000b6110, 0xc7)
void CMulti::Teardown() {
    if (m_524 && m_5bc && m_520 && m_580) {
        SendNetStat(0x402, 0x4d2, 1);
        SendStatFlag(0x3ea, 1);
    }
    CLobbyObjB* p520 = m_520;
    if (p520) {
        p520->Teardown();
        RezFree(p520);
        m_520 = 0;
    }
    if (m_524) {
        m_524->ScalarDtor(1);
        m_524 = 0;
    }
    CLobbyObjA* p320 = m_320;
    if (p320) {
        p320->Teardown();
        RezFree(p320);
        m_320 = 0;
    }
    m_4->m_110 = m_590;
    CPlayDtorBody();
}

// ===========================================================================
// CMulti::ClearString59c  @ 0x0b76c0  - assign an empty CString into m_59c,
// return the caller-supplied reference.
// ===========================================================================
// @early-stop
// MFC-version CString()-empty wall: retail builds the empty temp inline (mov
// [temp],0, m_pchData = NULL) then operator=, while our toolchain's MFC
// CString::CString() -> Init() sets m_pchData = afxEmptyString.m_pchData and is
// emitted as an out-of-line ??0CString@@QAE@XZ call (the retail MFC's Init used
// _afxPchNil / 0). Plus the /GX prologue reads fs:0 before push -1 vs our push-
// first order. Operation is correct (operator= from an empty CString); the
// codegen gap is the MFC build divergence - documented in
// docs/patterns/cstring-empty-init-version-divergence.md, deferred to final sweep.
RVA(0x000b76c0, 0x4f)
CString& CMulti::ClearString59c(CString& s) {
    m_59c = CString();
    return s;
}

// ===========================================================================
// CMulti::ClearString5a0  @ 0x0b7730  - assign an empty CString into m_5a0.
// ===========================================================================
// @early-stop
// same MFC-version CString()-empty wall as ClearString59c (see above).
RVA(0x000b7730, 0x4f)
CString& CMulti::ClearString5a0(CString& s) {
    m_5a0 = CString();
    return s;
}

// ===========================================================================
// CMulti::GetString59c  @ 0x0b7a90  - return m_59c by value (NRVO copy).
// ===========================================================================
RVA(0x000b7a90, 0x23)
CString CMulti::GetString59c() {
    return m_59c;
}

// ===========================================================================
// CMulti::GetString5a0  @ 0x0b7ad0  - return m_5a0 by value (NRVO copy).
// ===========================================================================
RVA(0x000b7ad0, 0x23)
CString CMulti::GetString5a0() {
    return m_5a0;
}

// ===========================================================================
// CMulti::ReportVersionMsg  @ 0x0b7e30  - log a message line to the logic object
// (m_4->LogLine). With code > 0, formats "<msg> (<code>)" first; else logs msg.
// ===========================================================================
RVA(0x000b7e30, 0x63)
void CMulti::ReportVersionMsg(char* msg, i32 code) {
    char buf[512];
    if (msg && *msg && m_4) {
        if (code > 0) {
            sprintf(buf, "%s (%i)", msg, code);
            m_4->LogLine(buf);
        } else {
            m_4->LogLine(msg);
        }
    }
}

// ===========================================================================
// CMulti::ReportNetError  @ 0x0b7f60  - format the last DirectPlay error
// ("Error: <code-name> - <code>") and report it, unless the user cancelled
// (DPERR_USERCANCEL == 0x118).
// ===========================================================================
RVA(0x000b7f60, 0x52)
void CMulti::ReportNetError() {
    char buf[512];
    if (m_4 && g_code != 0x118) {
        sprintf(buf, "Error: %s - %i", g_szCode, g_code);
        ReportVersionMsg(buf, g_code);
    }
}

// ===========================================================================
// CMulti::JoinSession  @ 0x0b7fe0  - pop the MULTI_JOIN lobby dialog; on confirm
// push the join stat flag.
// ===========================================================================
RVA(0x000b7fe0, 0x2f)
i32 CMulti::JoinSession() {
    if (RunErrorDialog("MULTI_JOIN", (void*)&MultiJoinHandler, 0) == 0) {
        return 0;
    }
    SendStatFlag(0x3f7, 1);
    return 1;
}

// ===========================================================================
// CMulti::RunErrorDialog  @ 0x0bc250  - run the modal lobby dialog on the logic
// object (m_4): pre-hook m_4->m_60, run the 3-arg dialog, restore focus to
// m_4->m_4->m_4, then ack the join failure. Returns the dialog result.
// ===========================================================================
RVA(0x000bc250, 0x55)
i32 CMulti::RunErrorDialog(char* tmpl, void* handler, i32 lparam) {
    if (!m_4) {
        return 2;
    }
    m_4->m_60->PreDialog();
    i32 r = m_4->RunDialog(tmpl, handler, lparam);
    MultiRestoreFocus(m_4->m_4->m_4);
    AckJoinFailure();
    return r;
}

// ===========================================================================
// CMulti::AckJoinFailure  @ 0x0bc420  - if the join gate is armed
// (m_524 && m_5bc && m_580), push the join-failure stat flag.
// ===========================================================================
RVA(0x000bc420, 0x2b)
void CMulti::AckJoinFailure() {
    if (m_524 && m_5bc && m_580) {
        SendStatFlag(0x3f6, 1);
    }
}
