// CMulti.h - the multiplayer / lobby game-state (C:\Proj\Gruntz). RTTI
// (.?AVCMulti@@) gives the most-derived shape: `CMulti : public CPlay, public
// CState` (CHD numBaseClasses=3, bases CMulti/CPlay/CState). The destructor at
// 0x08d270 stamps the three retail vtables in turn (CMulti 0x5e9fe4 -> CPlay
// 0x5ea0bc -> CState 0x5ea21c) as it walks down the sub-objects, tearing down a
// run of CString / CByteArray members > +0x510 (the networking/lobby block).
//
// CARCASS doctrine: only the member OFFSETS + the per-method call/branch
// structure are load-bearing. Field names are placeholders (m_<hexoffset>);
// the unmatched engine callees (SendNetStat / SendStatFlag / the m_4 logic
// object's methods / the heap deleters) are external no-body fns, so their
// `call rel32` are reloc-masked. CMulti is modeled SELF-CONTAINED with its own
// CString/CByteArray members at the retail offsets and a manual vtable-stamp
// (the three retail vtables referenced by address as reloc-masked DATA externs)
// since CPlay/CState's vtable contents are not reproduced in this TU.
#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>
// <Mfc.h> brings the real MFC CString / CByteArray (member sub-objects) plus the
// Win32 sprintf / DialogBoxParamA / SetActiveWindow surface CMulti dispatches to.
#include <Mfc.h>

// The CState owner/logic object at CMulti+0x04 (CState::m_4). CMulti's lobby /
// error-report methods drive it through three thiscall entry points (all
// out-of-line -> reloc-masked): a 1-arg log sink (the "%s (%i)" line), a 3-arg
// modal-dialog runner, and a chain to a Win32 focus restore. Modeled as a tiny
// helper so `mov ecx,[esi+4]; call rel32` falls out with no stack cleanup.
// The pre-dialog hook receiver (CMultiLogic::m_60); PreDialog() runs as a
// thiscall (ecx = the receiver) with no stack args (reloc-masked).
class CMultiDialogHook {
public:
    void PreDialog(); // 0x0051c7b0
};

class CMultiLogic {
public:
    void* m_0;        // +0x00
    CMultiLogic* m_4; // +0x04  the inner object (the focus-restore chain)
    char m_pad8[0x60 - 0x8];
    CMultiDialogHook* m_60; // +0x60  the pre-dialog thiscall receiver
    char m_pad64[0x110 - 0x64];
    i32 m_110; // +0x110 receives CMulti::m_590 on teardown

    // 1-arg log/append sink (reloc-masked); takes the formatted line.
    void LogLine(char* line);
    // the 3-arg modal-dialog runner (reloc-masked, thiscall on this=m_4).
    i32 RunDialog(char* tmpl, void* handler, i32 lparam);
};

// Win32 focus restore on the innermost window (m_4->m_4->m_4); __cdecl, reloc-masked.
void MultiRestoreFocus(void* hwnd); // 0x00518930

// The report-gate object at CMulti+0x524: released via its scalar-deleting
// destructor (vtable slot +0x04, thiscall, arg 1). Declarations only -> no ??_7.
class CMultiReportGate {
public:
    virtual void Slot00();            // +0x00
    virtual i32 ScalarDtor(i32 flag); // +0x04  scalar-deleting destructor
};

// The two lobby heap sub-objects (m_520 / m_320): each torn down by a thiscall
// method (ecx = the object, no stack args) then handed to the engine free.
class CLobbyObjB { // m_520
public:
    void Teardown(); // 0x004b6220
};
class CLobbyObjA { // m_320
public:
    void Teardown(); // 0x004a3360
};

class CMulti {
public:
    ~CMulti(); // 0x08d270 (most-derived /GX dtor; stamps 3 vtables, tears the
               // CString/CByteArray run)

    // Teardown helper run first by the dtor (and standalone @0xb6110): drains the
    // lobby sub-objects and pushes the final stat flags.
    void Teardown();                                           // 0x0b6110
    CString& ClearString59c(CString& s);                       // 0x0b76c0  (assign m_59c <- empty)
    CString& ClearString5a0(CString& s);                       // 0x0b7730  (assign m_5a0 <- empty)
    CString GetString59c();                                    // 0x0b7a90  (return m_59c by value)
    CString GetString5a0();                                    // 0x0b7ad0  (return m_5a0 by value)
    void ReportVersionMsg(char* msg, i32 code);                // 0x0b7e30
    void ReportNetError();                                     // 0x0b7f60
    i32 JoinSession();                                         // 0x0b7fe0
    i32 RunErrorDialog(char* tmpl, void* handler, i32 lparam); // 0x0bc250 (_MultiDispatch)
    void AckJoinFailure();                                     // 0x0bc420

    // External CMulti methods this TU calls but does not define (reloc-masked).
    void SendStatFlag(i32 code, i32 flag); // 0x0b9240 (__thiscall, reads m_5bc)
    void CPlayDtorBody();                  // 0x04c8700 (the CPlay sub-object teardown, thiscall)

    // --- layout (placeholder names; offsets are the load-bearing truth) ---
    char m_pad00_004[0x4 - 0x0];
    CMultiLogic* m_4; // +0x004  the CState owner / logic object
    char m_pad08_1b4[0x1b4 - 0x8];
    CString m_1b4; // +0x1b4
    char m_pad1b8_320[0x320 - 0x1b8];
    CLobbyObjA* m_320; // +0x320  heap obj (thiscall teardown + _RezFree)
    char m_pad324_370[0x370 - 0x324];
    CByteArray m_370; // +0x370
    char m_pad384_3a4[0x3a4 - 0x384];
    CByteArray m_3a4[4]; // +0x3a4  (vector dtor: 4 x 0x14)
    char m_pad3f4_410[0x410 - 0x3f4];
    CString m_410; // +0x410
    char m_pad414_488[0x488 - 0x414];
    CByteArray m_488; // +0x488
    char m_pad49c_520[0x520 - 0x49c];
    CLobbyObjB* m_520;       // +0x520  heap obj (thiscall teardown + _RezFree)
    CMultiReportGate* m_524; // +0x524  released via vtable +0x4 scalar dtor; the join/report gate
    char m_pad528_580[0x580 - 0x528];
    i32 m_580; // +0x580  gate flag
    char m_pad584_590[0x590 - 0x584];
    i32 m_590; // +0x590  copied to m_4->m_110 on teardown
    char m_pad594_598[0x598 - 0x594];
    CString m_598; // +0x598
    CString m_59c; // +0x59c
    CString m_5a0; // +0x5a0
    char m_pad5a4_5b4[0x5b4 - 0x5a4];
    CString m_5b4; // +0x5b4
    CString m_5b8; // +0x5b8
    i32 m_5bc;     // +0x5bc  gate flag
    char m_pad5c0_604[0x604 - 0x5c0];
    CByteArray m_604; // +0x604
};

#endif // GRUNTZ_GRUNTZ_CMULTI_H
