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

// Per-frame sub-objects driven by PumpA (0x0b6b40); reconstructed TU-local in
// CMulti.cpp (thiscall receivers, all out-of-line -> reloc-masked).
class CMultiSubDC;  // CMulti::m_2dc
class CMultiSubE4;  // CMulti::m_2e4
class CMultiSlot48; // CMultiLogic::m_48
class CMultiSub68;  // CMultiLogic::m_68
class CMultiSub70;  // CMultiLogic::m_70

// The CState owner/logic object at CMulti+0x04 (CState::m_4). CMulti's lobby /
// error-report methods drive it through three thiscall entry points (all
// out-of-line -> reloc-masked): a 1-arg log sink (the "%s (%i)" line), a 3-arg
// modal-dialog runner, and a chain to a Win32 focus restore. Modeled as a tiny
// helper so `mov ecx,[esi+4]; call rel32` falls out with no stack cleanup.
// The pre-dialog hook receiver (CMultiLogic::m_60); PreDialog() runs as a
// thiscall (ecx = the receiver) with no stack args (reloc-masked).
class CMultiDialogHook {
public:
    void PreDialog();      // 0x0051c7b0
    void StartTitleHook(); // 0x0011c7b0  (m_4->m_60 chain in StartTitle)
};

// A per-frame entry of CMultiLogic's +0x150 sub-object table (stride 0x238). The
// session-start path arms each entry's inner sub-object (+0x38) and conditionally
// pokes it; placeholder offsets, only the stride/offsets are load-bearing.
class CMultiLogicEntry {
public:
    char m_pad00_10[0x10];
    i32 m_10; // +0x10  passed to LoadSlotConfig
    i32 m_14; // +0x14
    char m_pad18_20[0x20 - 0x18];
    i32 m_20; // +0x20
    char m_pad24_38[0x38 - 0x24];
    char m_inner[0x238 - 0x38]; // +0x38  inner sub-object (FreeSlot / LoadSlotConfig / ArmSlot)
};

// The +0x6c list head's element type (CObList node, removed via RemoveHead).
class CMultiLogicNode {
public:
    char m_pad00_06[0x6];
    u8 m_6; // +0x06  parity byte
    char m_pad07_0c[0xc - 0x7];
    i32 m_c; // +0x0c  armed flag
};

// The +0x6c sub-object: a list-bearing manager (head at +0x1c, count latch at +0x28).
class CMultiLogicList {
public:
    char m_pad00_1c[0x1c];
    char m_head[0x28 - 0x1c];      // +0x1c  CObList head (RemoveHead via 0x5b4a03)
    i32 m_28;                      // +0x28  emptiness/count gate
    CMultiLogicNode* RemoveHead(); // 0x005b4a03 (MFC CObList::RemoveHead)
    void Step20b3(i32 v);          // per-frame poke (PumpA, thiscall)
};

// A small descriptor referenced through CMultiLogic+0xc4 (flags at +0x4, name at +0x8).
class CMultiLogicDesc {
public:
    void* m_0;
    u8 m_flags; // +0x04  bit 0x2 -> m_528 latch
    char m_pad5_8[0x8 - 0x5];
    char* m_8; // +0x08  host-name string
    char* m_c; // +0x0c  ... -> +0x8 string copied into a CString
};

class CMultiLogic {
public:
    void* m_0;        // +0x00
    CMultiLogic* m_4; // +0x04  the inner object (the focus-restore chain)
    char m_pad8_c[0xc - 0x8];
    i32 m_c; // +0x0c  active gate (PumpA)
    char m_pad10_48[0x48 - 0x10];
    CMultiSlot48* m_48; // +0x48  ambient slot manager (PumpA)
    char m_pad4c_5c[0x5c - 0x4c];
    CMultiLogicList* m_5c;  // +0x5c  list manager (StartSession poke target)
    CMultiDialogHook* m_60; // +0x60  the pre-dialog thiscall receiver
    char m_pad64_68[0x68 - 0x64];
    CMultiSub68* m_68;     // +0x68  per-frame sub (PumpA, Step3017)
    CMultiLogicList* m_6c; // +0x6c  the RemoveHead list manager
    CMultiSub70* m_70;     // +0x70  per-frame sub (PumpA, Step3562)
    char m_pad74_9c[0x9c - 0x74];
    i32 m_9c;        // +0x9c  zeroed at StartTitle entry
    void Step2d33(); // 0x??  per-frame finish (PumpA tail)
    char m_padA0_C0[0xc0 - 0xa0];
    i32 m_c0;              // +0xc0  must be non-null to proceed
    CMultiLogicDesc* m_c4; // +0xc4  host descriptor
    char m_padC8_110[0x110 - 0xc8];
    i32 m_110; // +0x110 receives CMulti::m_590 on teardown
    char m_pad114_150[0x150 - 0x114];
    CMultiLogicEntry m_150[4]; // +0x150  the 4-entry slot table (stride 0x238)

    // 1-arg log/append sink (reloc-masked); takes the formatted line.
    void LogLine(char* line);
    // the 3-arg modal-dialog runner (reloc-masked, thiscall on this=m_4).
    i32 RunDialog(char* tmpl, void* handler, i32 lparam);
    // session-start probe (3-arg thiscall): (mode, 0, 0) -> nonzero on success. 0x0048d780
    i32 ProbeSession(i32 mode, i32 a, i32 b);
    // report a netbind failure (2-arg thiscall: code, sub). 0x0008dc60
    void ReportError(i32 code, i32 sub);
    // resolve the host index from m_5c0 (1-arg thiscall) -> pointer or 0. 0x00492e80
    i32* ResolveHost(i32 host);
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
// method (ecx = the object, no stack args) then handed to the engine free. The
// CMulti session loop drives the lobby object (m_520) through a set of thiscall
// entry points (all out-of-line -> reloc-masked); placeholder names.
class CLobbyObjB { // m_520
public:
    void Teardown();                      // 0x004b6220 (NOT a CMulti method)
    void* FindSlot(i32 key);              // 0x004c0460  scan the +0x20 4x0x64 slot table
    void StartTick();                     // 0x000bf150
    i32 Step(i32 dt);                     // 0x000bf5a0
    i32 Drain();                          // 0x000bf9e0
    i32 IsBusy();                         // 0x000c01d0
    i32 IsStalled();                      // 0x000c04f0
    void ArmSlot(void* node, i32 parity); // 0x000c03f0
    void Step2437();                      // per-frame poke (PumpA, thiscall)

    char m_pad00_10[0x10];
    i32 m_10; // +0x10  slot-count / id base
};
class CLobbyObjA { // m_320
public:
    void Teardown(); // 0x004a3360
};

// A CLobbyObjB slot element (returned by FindSlot, stride 0x64) -- carries an
// inner manager at +0xc. Its FUN_004bc3f0 method (NOT a CMulti method) appends
// the host name into the inner manager. Placeholder.
class CLobbySlot {
public:
    // Fill `out` with the slot's host name and return it (FUN_004bc3f0; NOT a
    // CMulti method - it lives on this slot/player class).
    CString* BuildHostName(CString* out); // 0x004bc3f0
    char m_pad00_0c[0xc];
    void* m_c; // +0x0c  inner manager (sub-object at +0x18 read by the dropper)
};

// CMulti's own (manually-stamped) vtable. Only the two slots the per-frame Tick
// dispatches through are typed; the rest are opaque. The slots are thiscall in
// retail (ecx = this); modeled COM-style (explicit this arg) so MSVC 5.0 accepts
// the function-pointer types - the exact convention is part of Tick's documented
// codegen wall, so this expresses intent without perturbing a matched function.
class CMulti;
struct CMultiVtbl {
    char m_pad00_7c[0x7c];
    void(__stdcall* Redraw)(CMulti*, i32 a, i32 b, i32 c); // +0x7c
    char m_pad80_98[0x98 - 0x80];
    void(__stdcall* PostRedraw)(CMulti*); // +0x98
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

    // Reconstructed in this TU (ascending RVA).
    i32 StartSession(i32 mode, i32 unused); // 0x0b6580  arm the slot table + reseed timers
    i32 Connect(i32 mode);                  // 0x0b67f0  probe + wait for the session
    i32 Tick();                             // 0x0b6890  per-frame lobby pump
    i32 StartTitle();                       // 0x0b72c0  /GX: build "TITLE%d" + bind the net host
    void DropTimeout();                     // 0x0bc2d0  /GX: drop a timed-out player

    // External CMulti methods this TU calls but does not define (reloc-masked).
    void SendStatFlag(i32 code, i32 flag);  // 0x0b9240 (__thiscall, reads m_5bc)
    void SendNetStat3(i32 a, i32 b, i32 c); // 0x0b9290 (__thiscall, 3 args)
    void CPlayDtorBody();                   // 0x04c8700 (the CPlay sub-object teardown, thiscall)
    void OnDropPlayer();                    // 0x0bc110
    i32 RebindHost();                       // 0x0bc750  (also CNetMgr-shared)
    i32 RebindHostAlt();                    // 0x0bc460
    i32 ReadGroupSel();                     // 0x0b76a0
    i32 PumpA();                            // 0x0b6b40  (timeGetTime/wsprintf helper)
    i32 PumpAReady();                       // PumpA head probe (thiscall) 0x??
    void PumpAReset();                      // PumpA idle reset (thiscall) 0x??
    i32 PumpAIndex();                       // PumpA ambient index (thiscall) 0x??
    void PumpB();                           // 0x0b6e90
    void OnOutOfSync();                     // 0x0bae40
    void RefreshSlotTable();                // 0x021bd0  (free fn-ish thiscall on this)
    // m_4-side per-slot helpers used by StartSession (thiscall on the +0x38 inner).
    void FreeSlotInner();                                     // 0x025ca0 (on m_150[i]+0x38)
    i32 LoadSlotConfig(CMultiLogic* logic, i32 idx, i32 m10); // 0x025020 (on m_150[i]+0x38)
    void ArmSlotInner();                                      // 0x02ade0 (on m_150[i]+0x38)

    // --- layout (placeholder names; offsets are the load-bearing truth) ---
    CMultiVtbl* m_vtbl; // +0x000  vptr (Tick dispatches through +0x7c / +0x98)
    CMultiLogic* m_4;   // +0x004  the CState owner / logic object
    void* m_8;          // +0x008  string registry (FUN_0053c030 lookup)
    void* m_c;          // +0x00c  manager (vfn host, +0x24 chain, +0x20 sub-window)
    char m_pad10_2c[0x2c - 0x10];
    i32 m_2c; // +0x02c  saved/restored handle around the title build
    char m_pad30_1b4[0x1b4 - 0x30];
    CString m_1b4; // +0x1b4
    char m_pad1b8_1cc[0x1cc - 0x1b8];
    i32 m_1cc; // +0x1cc  reseeded to 0
    char m_pad1d0_2d0[0x2d0 - 0x1d0];
    i32 m_2d0;          // +0x2d0  Step() result
    i32 m_2d4;          // +0x2d4  Drain() result
    i32 m_2d8;          // +0x2d8  rng seed
    CMultiSubDC* m_2dc; // +0x2dc  per-frame sub (Step34bd)
    char m_pad2e0_2e4[0x2e4 - 0x2e0];
    CMultiSubE4* m_2e4; // +0x2e4  per-frame sub (Step2cc0)
    char m_pad2e8_320[0x320 - 0x2e8];
    CLobbyObjA* m_320; // +0x320  heap obj (thiscall teardown + _RezFree)
    char m_pad324_338[0x338 - 0x324];
    i32 m_338; // +0x338  ambient-window clock (int64 low, high at m_33c)
    i32 m_33c; // +0x33c
    i32 m_340; // +0x340  ambient interval (int64 low, high at m_344)
    i32 m_344; // +0x344
    i32 m_348; // +0x348  ambient-armed latch
    char m_pad34c_370[0x370 - 0x34c];
    CByteArray m_370; // +0x370
    char m_pad384_3a4[0x3a4 - 0x384];
    CByteArray m_3a4[4]; // +0x3a4  (vector dtor: 4 x 0x14)
    char m_pad3f4_410[0x410 - 0x3f4];
    CString m_410; // +0x410
    i32 m_414;     // +0x414  zeroed each Tick
    char m_pad418_488[0x488 - 0x418];
    CByteArray m_488; // +0x488
    char m_pad49c_520[0x520 - 0x49c];
    CLobbyObjB* m_520;       // +0x520  heap obj (thiscall teardown + _RezFree)
    CMultiReportGate* m_524; // +0x524  released via vtable +0x4 scalar dtor; the join/report gate
    i32 m_528;               // +0x528  is-host latch
    char m_pad52c_534[0x534 - 0x52c];
    i32 m_534; // +0x534  reset by Connect
    char m_pad538_564[0x564 - 0x538];
    i32 m_564; // +0x564  busy/active gate
    char m_pad568_574[0x574 - 0x568];
    i32 m_574; // +0x574  stall latch
    char m_pad578_57c[0x57c - 0x578];
    i32 m_57c; // +0x57c  pump-active reentrancy guard
    i32 m_580; // +0x580  gate flag
    char m_pad584_588[0x588 - 0x584];
    i32 m_588; // +0x588  armed flag
    char m_pad58c_590[0x590 - 0x58c];
    i32 m_590;     // +0x590  copied to m_4->m_110 on teardown
    i32 m_594;     // +0x594  ambient-enable gate
    CString m_598; // +0x598
    CString m_59c; // +0x59c
    CString m_5a0; // +0x5a0
    u8 m_5a4;      // +0x5a4  step shift base (used as byte/word; follows m_5a0)
    char m_pad5a5_5a8[0x5a8 - 0x5a5];
    i32 m_5a8; // +0x5a8  drain reload value
    char m_pad5ac_5b4[0x5b4 - 0x5ac];
    CString m_5b4; // +0x5b4
    CString m_5b8; // +0x5b8
    i32 m_5bc;     // +0x5bc  gate flag
    i32 m_5c0;     // +0x5c0  host index (ResolveHost arg)
    char m_pad5c4_5cc[0x5cc - 0x5c4];
    i32 m_5cc; // +0x5cc  current slot id
    i32 m_5d0; // +0x5d0
    i32 m_5d4; // +0x5d4  remaining-time accumulator
    i32 m_5d8; // +0x5d8  frame delta
    i32 m_5dc; // +0x5dc  last timeGetTime
    i32 m_5e0; // +0x5e0  accumulated time
    i32 m_5e4; // +0x5e4  second timeGetTime
    i32 m_5e8; // +0x5e8
    i32 m_5ec; // +0x5ec
    char m_pad5f0_604[0x604 - 0x5f0];
    CByteArray m_604; // +0x604
};

#endif // GRUNTZ_GRUNTZ_CMULTI_H
