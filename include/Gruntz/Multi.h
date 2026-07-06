// Multi.h - the multiplayer / lobby game-state (C:\Proj\Gruntz). RTTI
// (.?AVCMulti@@) gives the most-derived shape: `CMulti : public CPlay, public
// CState` (CHD numBaseClasses=3, bases CMulti/CPlay/CState). The destructor at
// 0x08d270 stamps the three retail vtables in turn (CMulti 0x5e9fe4 -> CPlay
// 0x5ea0bc -> CState 0x5ea21c) as it walks down the sub-objects, tearing down a
// run of CString / CByteArray members > +0x510 (the networking/lobby block).
//
// CARCASS doctrine: only the member OFFSETS + the per-method call/branch
// structure are load-bearing. Field names are placeholders (m_<hexoffset>);
// the unmatched engine callees (SendNetStat / SendStatFlag / the m_logic
// object's methods / the heap deleters) are external no-body fns, so their
// `call rel32` are reloc-masked. CMulti is modeled SELF-CONTAINED with its own
// CString/CByteArray members at the retail offsets and a manual vtable-stamp
// (the three retail vtables referenced by address as reloc-masked DATA externs)
// since CPlay/CState's vtable contents are not reproduced in this TU.
//
// INHERITANCE HELD (vtable_hierarchy audit: CMulti -> derive CPlay; the true chain
// is CMulti : CPlay : CState, single-chain not a diamond - CPlay itself derives
// CState per <Gruntz/Play.h>). We DELIBERATELY do NOT declare `: public CPlay`
// here: CMulti's low-offset members (m_logic +0x04, m_stateReg +0x08, m_view +0x0c,
// m_curState +0x2c, ...) occupy exactly the CPlay/CState sub-object range and are
// read by ~30 methods across this TU. A real `: public CPlay` (CPlay.h pulls Mfc /
// CGameRegistry / CSpriteFactoryHolder / CState) forces removing every one of those members and
// re-mapping each access through the base sub-objects - reshuffling the /O2 regalloc
// of a 73%-partial TU whose functions already sit on documented EH/regalloc walls.
// It also cannot help the sole inheritance-shaped function, ~CMulti @0x8d270, whose
// residual is the EH-state-machine / vector-dtor wall below (0.65%, not a base issue:
// the CState/CPlay sub-object restamps are already reproduced via local dtor-views +
// CPlayDtorBody). So the real-chain modeling stays a final-sweep task, held here.
#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>
// <Mfc.h> brings the real MFC CString / CByteArray (member sub-objects) plus the
// Win32 sprintf / DialogBoxParamA / SetActiveWindow surface CMulti dispatches to.
#include <Mfc.h>

// OWNERSHIP VERDICT (netmgr-vs-cmulti, 2026-07-05; full proof in <Net/NetMgr.h>):
// this CMulti owns the WHOLE +0x2d8..+0x60c network/lobby field block and the
// 0xb5xxx-0xbdxxx method cluster (PollSession/SendNetStat/BroadcastChatLine/...).
// <Net/NetMgr.h>'s CNetMgr models the same fields/methods under the conflated
// name; the REAL CNetMgr (RTTI CNetMgr:CObject, ??1 @0xb6000) is the small
// DirectPlay wrapper THIS class holds at +0x524 (m_netGate below).
//
// Per-frame sub-objects driven by PumpA (0x0b6b40); reconstructed TU-local in
// CMulti.cpp (thiscall receivers, all out-of-line -> reloc-masked).
class CGameApp;              // WAP32 app (<Wap32/Wap32.h>); CMultiMgr::m_owner (+0x08)
class CMultiSubDC;           // CMulti::m_fxOverlay
class CTileTriggerContainer; // CMulti::m_2e4
class CMultiSoundZ;          // CMultiMgr::m_48
class CMultiSub68;           // CMultiMgr::m_68
class CChatBoxOwner;         // CMulti::m_2e0 (per-frame LoadChatBoxSprite sub)
class CBrickzGrid;           // CMultiMgr::m_70
class CMultiMgr;             // CState owner at CMulti+0x04 (fwd for CSlotConfig::Load)
class CLobbySlot;            // CNetSession2 slot element (fwd for FindSlot's return)
class PBOutput;              // CMultiMgr::m_54 output sink (defined TU-local in CMulti.cpp)

// CMultiMgr IS the CGruntzMgr game-manager singleton (*0x64556c, the object
// <Gruntz/GruntzMgr.h> / <Gruntz/GameRegistry.h> canonically model) viewed by the
// multiplayer/lobby subsystem: the CState owner at CMulti+0x04 (CState::m_4). Its
// +0x48 sound object and its +0x150 4-entry 0x238-stride options table are the
// SAME slots the canonical class names (m_sound / m_options[4]); the state factory
// CGruntzMgr::TransitionState passes `this`=this manager to each state's activate.
//
// @early-stop (keystone follow-up): this facet is the ONE CGruntzMgr view not yet
// dissolved into the canonical class. It types SEVEN per-mode REUSED slots for
// multiplayer mode (m_54 output sink, m_5c/m_6c list managers, m_60 dialog hook,
// m_68/m_70 per-frame subs, m_c4 host descriptor) and dispatches five reloc-masked
// lobby methods (LogLine/RunDialog/ProbeSession/ResolveHost + the Ack error path)
// that are meaningless outside multiplayer - folding them onto the 60-TU canonical
// would fabricate a per-mode mega-type (the anti-pattern CGameRegistry.h warns
// against), and re-casting ~30 sites in this 73%-partial TU risks shuffling its
// documented regalloc walls. So it stays a TU-local, honestly-NAMED per-mode
// downcast-view of the singleton (the CGruntzMgr/CGameRegistry MFC/Win32 split
// precedent), no longer masquerading under the canonical `CGruntzMgr` name. The
// final sweep can dissolve it once the multiplayer sub-object types are modeled.
//
// CMulti's lobby / error-report methods drive it through thiscall entry points (all
// out-of-line -> reloc-masked): a 1-arg log sink (the "%s (%i)" line), a 3-arg
// modal-dialog runner, and a chain to a Win32 focus restore.
// The pre-dialog hook receiver (CMultiMgr::m_60); PreDialog() runs as a
// thiscall (ecx = the receiver) with no stack args (reloc-masked).
class CMultiDialogHook {
public:
    void PreDialog();      // 0x0051c7b0
    void StartTitleHook(); // 0x0011c7b0  (m_logic->m_60 chain in StartTitle)
};

// CMultiMgrOptions's inner slot-config sub-object (+0x38). StartSession drives it
// per-slot through three thiscall entry points (ecx = &m_inner; out-of-line ->
// reloc-masked); only the +0x38 offset + 0x200 span are load-bearing.
class CSlotConfig {
public:
    void FreeSlot();                              // 0x025ca0
    i32 Load(CMultiMgr* logic, i32 idx, i32 m10); // 0x025020
    void Arm();                                   // 0x02ade0
    char m_pad[0x238 - 0x38];
};

// A per-frame entry of CMultiMgr's +0x150 sub-object table (stride 0x238). The
// session-start path arms each entry's inner sub-object (+0x38) and conditionally
// pokes it; placeholder offsets, only the stride/offsets are load-bearing.
class CMultiMgrOptions {
public:
    char m_pad00_10[0x10];
    i32 m_10; // +0x10  passed to CSlotConfig::Load
    i32 m_14; // +0x14
    char m_pad18_20[0x20 - 0x18];
    i32 m_20; // +0x20
    char m_pad24_38[0x38 - 0x24];
    CSlotConfig m_inner; // +0x38  inner slot-config sub-object
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

// A small descriptor referenced through CMultiMgr+0xc4 (flags at +0x4, name at +0x8).
class CMultiLogicDesc {
public:
    void* m_0;
    u8 m_flags; // +0x04  bit 0x2 -> m_isHost latch
    char m_pad5_8[0x8 - 0x5];
    char* m_8;            // +0x08  host-name string
    CMultiLogicDesc* m_c; // +0x0c  linked descriptor; its m_8 host-name is copied into a CString
};

class CMultiMgr {
public:
    void* m_0;      // +0x00
    CMultiMgr* m_4; // +0x04  the inner object (the focus-restore chain)
    // +0x08 == WAP32::CGameMgr::m_owner (the owning CGameApp; its +0x0c
    // m_hInstance feeds LoadStringA in CMulti::ReportStatusId @0xb7ec0).
    CGameApp* m_owner; // +0x08
    i32 m_c;           // +0x0c  active gate (PumpA)
    char m_pad10_48[0x48 - 0x10];
    CMultiSoundZ* m_48; // +0x48  ambient slot manager (PumpA)
    char m_pad4c_54[0x54 - 0x4c];
    PBOutput* m_54; // +0x54  output sink (PumpB 2-arg blit)
    char m_pad58_5c[0x5c - 0x58];
    CMultiLogicList* m_5c;  // +0x5c  list manager (StartSession poke target)
    CMultiDialogHook* m_60; // +0x60  the pre-dialog thiscall receiver
    char m_pad64_68[0x68 - 0x64];
    CMultiSub68* m_68;     // +0x68  per-frame sub (PumpA, Step3017)
    CMultiLogicList* m_6c; // +0x6c  the RemoveHead list manager
    CBrickzGrid* m_70;     // +0x70  per-frame sub (PumpA, Step3562)
    char m_pad74_9c[0x9c - 0x74];
    i32 m_9c;        // +0x9c  zeroed at StartTitle entry
    void Step2d33(); // 0x??  per-frame finish (PumpA tail)
    char m_padA0_C0[0xc0 - 0xa0];
    i32 m_c0;              // +0xc0  must be non-null to proceed
    CMultiLogicDesc* m_c4; // +0xc4  host descriptor
    char m_padC8_110[0x110 - 0xc8];
    i32 m_110; // +0x110 receives CMulti::m_590 on teardown
    char m_pad114_150[0x150 - 0x114];
    CMultiMgrOptions m_150[4]; // +0x150  the 4-entry slot table (stride 0x238)

    // 1-arg log/append sink (reloc-masked); takes the formatted line.
    void LogLine(char* line);
    // the 3-arg modal-dialog runner (reloc-masked, thiscall on this=m_logic).
    i32 RunDialog(char* tmpl, void* handler, i32 lparam);
    // session-start probe (3-arg thiscall): (mode, 0, 0) -> nonzero on success. 0x0048d780
    i32 ProbeSession(i32 mode, i32 a, i32 b);
    // report a netbind failure (2-arg thiscall: code, sub). 0x0008dc60
    void ReportError(i32 code, i32 sub);
    // resolve the host index from m_hostIndex (1-arg thiscall) -> pointer or 0. 0x00492e80
    i32* ResolveHost(i32 host);
};

// Win32 focus restore on the innermost window (m_logic->m_4->m_4); __cdecl, reloc-masked.
void MultiRestoreFocus(void* hwnd); // 0x00518930

// The opened-player object OpenPlayer returns (group-name accessor @0x4b76a0).
class CMultiPlayer;

// The report gate's player-info sub-object (gate->m_70): per-slot occupancy probes
// the multiplayer-start dialog reads to derive the player count. Reloc-masked
// __thiscall leaves in the DirectPlay session cluster (0x1794xx).
class CMultiPlayerInfo {
public:
    i32 Q1794b0(); // 0x1794b0  slot-1 occupancy probe
    i32 Q1794e0(); // 0x1794e0  slot-2
    i32 Q179510(); // 0x179510  slot-3
    i32 Q179540(); // 0x179540  slot-4
};
SIZE_UNKNOWN(CMultiPlayerInfo);

// The join/report gate at CMulti+0x524 - IDENTITY RESOLVED (netmgr-vs-cmulti):
// this IS the real CNetMgr (RTTI CNetMgr : CObject, ??_7 @0x1ea42c, virtual dtor
// ??1 @0xb6000 = the ScalarDtor slot below; <Net/NetMgr.h> models it in full,
// conflated with CMulti's own fields - see its header verdict). The methods here
// are CNetMgr's 0x178xxx wrappers viewed with divergent signatures (Bind ==
// Init@0x178170, Activate == 0x178750, OpenPlayer == AddPlayerNode@0x1786d0,
// M178a80 == EnumGroupsRange@0x178a80); the +0x70/+0x74 typing also diverges
// (pointers here vs i32 selection latches there). FOLD DEFERRED until those
// signature/typing conflicts are reconciled against the retail bytes in the
// CNetMgr split - do not re-diverge further.
// Released via its scalar-deleting destructor (vtable slot +0x04, thiscall,
// arg 1). ONE struct (was split into a separate CMultiNetGate net-bind view,
// and the per-TU WatchSess524 / CMultiRegSub lens sub-object views folded
// here): StartTitle drives its net-bind entry points (non-virtual,
// reloc-masked thiscall) and stashes the opened player at +0x74; the lobby
// watchdog re-probes it via M178a80, and the start-dialog reads its +0x70
// player-info sub-object's per-slot occupancy probes.
class CMultiReportGate {
public:
    virtual void Slot00();                // +0x00
    virtual i32 ScalarDtor(i32 flag);     // +0x04  scalar-deleting destructor
    i32 Bind(i32* tmpl);                  // 0x578170  bind to host template -> nonzero ok
    void Activate();                      // 0x578750
    CMultiPlayer* OpenPlayer(char* name); // 0x5786d0 -> the opened player (0 fail)
    void M178a80(void* h, i32 a);         // 0x578a80  watchdog re-probe (EnumGroupsRange)
    char m_pad04[0x70 - 0x4];
    CMultiPlayerInfo* m_70; // +0x70  per-slot player-info sub-object (start-dialog probes)
    CMultiPlayer* m_player; // +0x74  the player StartTitle opened (OpenPlayer result)
};

// The two lobby heap sub-objects (m_session / m_attractOverlay): each torn down by a thiscall
// method (ecx = the object, no stack args) then handed to the engine free. The
// CMulti session loop drives the lobby object (m_session) through a set of thiscall
// entry points (all out-of-line -> reloc-masked); placeholder names.
// IDENTITY-RECOVERY TODO (deferred multi-class reconciliation, flagged - NOT a keep):
// this CMulti::m_session (+0x520) lobby-session object is the SAME 0x20bb0 object the
// canonical CNetSession (<Net/NetMgr.h>) models - StartTick@0xbf150 == CNetSession::Reset,
// IsStalled@0xc04f0 == CNetSession::Verify, CheckLatency@0xc04a0 == CNetSession::CheckLatency.
// But its Step/Drain/IsBusy methods resolve to a THIRD class, CLobbySync (Poll@0xbf5a0,
// Tick@0xbf9e0, Advance@0xc01d0), so m_session's dispatch spans CNetSession + CLobbySync.
// Fully folding this (add the lobby/CLobbySync methods to CNetSession, delete this view,
// retype m_session) needs a dedicated CNetSession<->CLobbySync reconciliation and pulls
// the heavy <Net/NetMgr.h> into the widely-included Multi.h - deferred, not done here.
class CNetSession2 { // m_session
public:
    void Teardown();                      // 0x004b6220 (NOT a CMulti method)
    CLobbySlot* FindSlot(i32 key);        // 0x004c0460  scan the +0x20 4x0x64 slot table
    void StartTick();                     // 0x000bf150 (== CNetSession::Reset)
    i32 Step(i32 dt);                     // 0x000bf5a0 (== CLobbySync::Poll)
    i32 Drain();                          // 0x000bf9e0 (== CLobbySync::Tick)
    i32 IsBusy();                         // 0x000c01d0 (== CLobbySync::Advance)
    i32 IsStalled();                      // 0x000c04f0 (== CNetSession::Verify)
    void ArmSlot(void* node, i32 parity); // 0x000c03f0
    i32 CheckLatency(i32 cap);            // 0x000c04a0 (thunk 0x148d)  == CNetSession::CheckLatency
    void Step2437();                      // per-frame poke (PumpA, thiscall)

    char m_pad00_10[0x10];
    i32 m_10; // +0x10  slot-count / id base
};
SIZE_UNKNOWN(CNetSession2); // CMulti lobby-session view (identity-recovery TODO above)
class CLobbyObjA {          // m_attractOverlay
public:
    void Teardown(); // 0x004a3360
};

// A CNetSession2 slot element (returned by FindSlot, stride 0x64) -- carries an
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
    // Realized real-polymorphic: a virtual dtor makes cl emit ??_7CMulti@@6B@. The
    // vptr occupies +0x00 (where the old CMultiVtbl* m_vtbl lived - same offset, so
    // every matched CMulti method is codegen-neutral). ~CMulti's leading manual vptr
    // stamp is dropped so cl's implicit entry vptr-store survives (a leading manual
    // stamp would dead-store-eliminate the implicit one -> no ??_7). The mid-dtor
    // CPlay/CState restamps are realized via local dtor-view classes in CMulti.cpp.
    // Tick dispatches through vtbl() (reads the vptr as a CMultiVtbl*), preserving
    // its +0x7c/+0x98 indirect-call bytes.
    virtual ~CMulti(); // 0x08d270 (most-derived /GX dtor; stamps CPlay/CState, tears
                       // the CString/CByteArray run)
    CMultiVtbl* vtbl() {
        return *(CMultiVtbl**)this;
    }

    // Teardown helper run first by the dtor (and standalone @0xb6110): drains the
    // lobby sub-objects and pushes the final stat flags.
    void Teardown();                     // 0x0b6110
    CString& ClearString59c(CString& s); // 0x0b76c0  (assign m_groupName <- empty)
    CString& ClearString5a0(CString& s); // 0x0b7730  (assign m_hostName <- empty)
    CString GetString59c();              // 0x0b7a90  (return m_groupName by value)
    CString GetString5a0();              // 0x0b7ad0  (return m_hostName by value)
    CString Name42ff();                  // ILT 0x0042ff -> GetConfigNameB 0x0b60d0 (MultiColorDlg)
    CString Name31d4();                  // ILT 0x0031d4 -> GetConfigNameA 0x0b6090 (MultiColorDlg)
    void ReportVersionMsg(char* msg, i32 code); // 0x0b7e30
    // Status-bar diagnostic by string-resource id: LoadStringA through
    // m_logic->m_owner->m_hInstance then ReportVersionMsg. Defined in
    // Net/LobbyDialogs.cpp (its RVA-order home).
    void ReportStatusId(u32 strId, i32 level);                 // 0x0b7ec0
    void ReportNetError();                                     // 0x0b7f60
    i32 JoinSession();                                         // 0x0b7fe0
    i32 RunErrorDialog(char* tmpl, void* handler, i32 lparam); // 0x0bc250 (_MultiDispatch)
    void AckJoinFailure();                                     // 0x0bc420
    void Commit3ada(i32); // ILT 0x0003ada -> 0x0bccd0  replay-name commit (ApiCallers OnReset)

    // Reconstructed in this TU (ascending RVA).
    i32 StartSession(i32 mode, i32 unused); // 0x0b6580  arm the slot table + reseed timers
    i32 Connect(i32 mode);                  // 0x0b67f0  probe + wait for the session
    i32 Tick();                             // 0x0b6890  per-frame lobby pump
    i32 StartTitle();                       // 0x0b72c0  /GX: build "TITLE%d" + bind the net host
    void DropTimeout();                     // 0x0bc2d0  /GX: drop a timed-out player

    // External CMulti methods this TU calls but does not define (reloc-masked).
    void SendStatFlag(i32 code, i32 flag); // 0x0b9240 (__thiscall, reads m_5bc)
    // Build a {id, value} stat packet and ship it through the +0x524 CNetMgr
    // (was SendNetStat3; the NetMgr.h-side name/signature, same RVA).
    void SendNetStat(i32 id, u32 value, i32 flag); // 0x0b9290 (__thiscall, 3 args)
    // The multiplayer net-game / lobby-watchdog dialog helpers reach these on the
    // g_64bd5c singleton (Ghidra labels them CNetMgr::, the same-object dual-view).
    // Folded here from the former per-TU CNetSession / WatchSess lens types.
    void DropPlayer(i32 id);  // 0x0bb510  (CNetMgr::DropChannelPlayer)
    i32 Poll(i32 token);      // 0x0bba10  (via ILT 0x1249; verify-custom-level poll)
    i32 ResolveLocalPlayer(); // 0x0ba7d0
    void ReportAckLatency();  // 0x0bd000
    i32 VerifyCustomLevel(void* h, i32 token); // 0x0b8fc0
    i32 PollSession();                         // 0x0b95f0 (drain the receive queue; ret i32)
    void AutoTuneCmdDelay();                   // 0x0bcc10
    void CPlayDtorBody(); // 0x04c8700 (the CPlay sub-object teardown, thiscall)
    void OnDropPlayer();  // 0x0bc110
    i32 RebindHost();     // 0x0bc750  (also CNetMgr-shared)
    i32 RebindHostAlt();  // 0x0bc460
    // The connect-drive helpers the Net-side coordinator (NetMgrMisc.cpp) reaches
    // off the g_64bd5c singleton. In the 0xb6110-0xbc420 lobby cluster; Ghidra labels
    // them CNetMgr:: (Broadcast*), a heuristic mis-attribution of this CMulti cluster.
    i32 BroadcastChannelTable(i32 entry); // 0x0ba810
    i32 BroadcastOneChannel(i32 chan);    // 0x0baf00
    // Assemble + broadcast one chat line (stat 0x3f0), optionally appending it
    // to the hWnd chat edit (the lobby DlgProcs' chat submit path).
    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, HWND hWnd); // 0x0bb190
    i32 ReadGroupSel();                                                    // 0x0b76a0
    i32 PumpA();             // 0x0b6b40  (timeGetTime/wsprintf helper)
    i32 PumpAReady();        // PumpA head probe (thiscall) 0x??
    void PumpAReset();       // PumpA idle reset (thiscall) 0x??
    i32 PumpAIndex();        // PumpA ambient index (thiscall) 0x??
    void PumpB();            // 0x0b6e90
    void OnOutOfSync();      // 0x0bae40
    void RefreshSlotTable(); // 0x021bd0  (free fn-ish thiscall on this)
    // Out-of-line CMulti bodies reached by thiscall on `this` (reloc-masked): the
    // mode-driven level loader (StartSession) and the title-screen loader (StartTitle).
    i32 LoadLevelByMode(i32 mode, i32 flags);                          // 0x000ca200
    i32 LoadTitleScreen(const char* name, i32 a, i32 b, i32 c, i32 d); // 0x004fa350
    // Inherited CPlay per-frame helpers PumpB dispatches to (CMulti : public
    // CPlay). CMulti.cpp is modeled self-contained (no CPlay.h), so the helpers
    // are re-declared here; all thiscall on `this`, out-of-line -> reloc-masked.
    // Names + RVAs are the canonical CPlay bodies (src/Gruntz/Play.cpp).
    void StepInputA();             // 0x0d11e0  latch the per-axis scroll/input block
    void StepC();                  // 0x0d8d90  m_480-driven region-flip step
    void LoadScrollSpeedOptions(); // 0x0d12b0  butemgr Min/MaxScrollSpeed "Optionz"
    void StepScroll();             // 0x0d1ac0  recompute the scroll origin (m_4e4)
    void NotifyVisibleEntities();  // 0x0d9050  push the visible-clip rect to entities
    void StepGridWalk(u32 clock);  // 0x0d0a60  advance the grid-walk countdown
    void CopyRect(void* h);        // 0x0d0b30  compute+clamp the pane copy rect (g_pCopyRect)
    void DrawDebugStats();         // 0x0cf770  Fps/Objs/Pos/Timing/Sent debug overlay
    void OnRegion2(i32 v); // 0x0d8a00  arm overlay-A (m_overlayAActive, deadline m_430/+0x438)
    void OnRegion1(i32 v); // 0x0d8aa0  arm overlay-B (m_overlayBActive, deadline m_440/+0x448)

    // --- layout (placeholder names; offsets are the load-bearing truth) ---
    // +0x000  vptr (compiler ??_7CMulti; Tick dispatches via vtbl() -> +0x7c/+0x98)
    CMultiMgr* m_logic; // +0x004  the CState owner / game-manager (logic) object
    void* m_stateReg;   // +0x008  string/state registry (FUN_0053c030 lookup)
    void* m_view;       // +0x00c  view manager (vfn host, +0x24 chain, +0x20 sub-window)
    char m_pad10_2c[0x2c - 0x10];
    i32 m_curState; // +0x02c  current-state slot saved/restored around the title build
    char m_pad30_150[0x150 - 0x30];
    i32 m_150; // +0x150  Tick redraw region arg b
    i32 m_154; // +0x154  Tick redraw region arg c
    char m_pad158_1b4[0x1b4 - 0x158];
    CString m_1b4; // +0x1b4
    char m_pad1b8_1cc[0x1cc - 0x1b8];
    i32 m_1cc; // +0x1cc  reseeded to 0
    char m_pad1d0_2d0[0x2d0 - 0x1d0];
    i32 m_stepResult;             // +0x2d0  Step() result
    i32 m_drainResult;            // +0x2d4  Drain() result
    i32 m_rngSeed;                // +0x2d8  rng seed
    CMultiSubDC* m_fxOverlay;     // +0x2dc  per-frame sub (Step34bd)
    CChatBoxOwner* m_2e0;         // +0x2e0  per-frame sub (PBSub2e0::Step2bfd)
    CTileTriggerContainer* m_2e4; // +0x2e4  per-frame sub (Step2cc0)
    char m_pad2e8_30c[0x30c - 0x2e8];
    i32 m_paletteActive;           // +0x30c  overlay-active flag (PumpB)
    char m_palette[0x320 - 0x310]; // +0x310  blit palette/param block (passed by address)
    CLobbyObjA* m_attractOverlay;  // +0x320  heap obj (thiscall teardown + _RezFree)
    char m_pad324_338[0x338 - 0x324];
    // +0x338 ambient-window start + +0x340 interval: each a 64-bit clock stored as
    // an aligned DWORD pair and read via *(i64*)& (a true __int64 member forces
    // 8-byte struct alignment, which is NOT matching-neutral -- authentic shape).
    i32 m_338;          // +0x338  ambient-window start (int64 low, high at m_33c)
    i32 m_33c;          // +0x33c
    i32 m_340;          // +0x340  ambient interval (int64 low, high at m_344)
    i32 m_344;          // +0x344
    i32 m_ambientArmed; // +0x348  ambient-armed latch
    char m_pad34c_370[0x370 - 0x34c];
    CByteArray m_370; // +0x370
    char m_pad384_3a4[0x3a4 - 0x384];
    CByteArray m_3a4[4]; // +0x3a4  (vector dtor: 4 x 0x14)
    char m_pad3f4_410[0x410 - 0x3f4];
    CString m_410; // +0x410
    i32 m_414;     // +0x414  zeroed each Tick
    char m_pad418_430[0x430 - 0x418];
    // +0x430/+0x440 overlay-A/B deadline + +0x438/+0x448 interval: 64-bit clocks
    // stored as aligned DWORD pairs, read via *(i64*)& (authentic -- a real __int64
    // member forces 8-byte struct alignment and regresses every method).
    i32 m_430; // +0x430  overlay-A deadline clock (int64 low, high at m_434)
    i32 m_434; // +0x434
    i32 m_438; // +0x438  overlay-A interval (int64 low, high at m_43c)
    i32 m_43c; // +0x43c
    i32 m_440; // +0x440  overlay-B deadline clock (int64 low, high at m_444)
    i32 m_444; // +0x444
    i32 m_448; // +0x448  overlay-B interval (int64 low, high at m_44c)
    i32 m_44c; // +0x44c
    char m_pad450_470[0x470 - 0x450];
    i32 m_overlayAActive; // +0x470  overlay-A enable flag
    i32 m_overlayBActive; // +0x474  overlay-B enable flag
    char m_pad478_488[0x488 - 0x478];
    CByteArray m_488; // +0x488
    char m_pad49c_520[0x520 - 0x49c];
    CNetSession2* m_session;     // +0x520  heap obj (thiscall teardown + _RezFree)
    CMultiReportGate* m_netGate; // +0x524  THE REAL CNetMgr (see the CMultiReportGate
                                 //         identity note); released via its virtual
                                 //         scalar-deleting dtor (slot +0x4, ??1 @0xb6000)
    i32 m_isHost;                // +0x528  is-host latch (net-game dlg: active-session flag)
    i32 m_sessionTerminated;     // +0x52c  "the game session has been terminated" flag
                                 //         (lobby watchdog + NetDlgSessionStop banner gate)
    i32 m_530;                   // +0x530  custom-level verified flag (net-game dlg)
    i32 m_534;                   // +0x534  reset by Connect
    i32 m_538;                   // +0x538  player-removed flag (lobby watchdog)
    i32 m_53c;                   // +0x53c  version-mismatch flag (net-game dlg verify)
    char m_pad540_564[0x564 - 0x540];
    i32 m_pollAbort; // +0x564  set => PollSession stops pumping / Tick finishes
                     //         (abnormal-termination gate; busy/active gate)
    i32 m_568;       // +0x568  selection-taken flag (lobby watchdog)
    i32 m_56c;       // +0x56c  session-full flag (lobby watchdog)
    i32 m_570;       // +0x570  version-mismatch flag (lobby watchdog)
    i32 m_574;       // +0x574  stall latch
    char m_pad578_57c[0x57c - 0x578];
    i32 m_pumpGuard;     // +0x57c  pump-active reentrancy guard
    i32 m_connected;     // +0x580  gate flag
    i32 m_584;           // +0x584  state word cleared on dispatch-handler entry (normal-exit
                         //         mark; NetDlgSessionStop's clean-exit gate)
    i32 m_588;           // +0x588  armed flag
    i32 m_58c;           // +0x58c  stat-reset gate (lobby watchdog)
    i32 m_590;           // +0x590  copied to m_logic->m_110 on teardown
    i32 m_594;           // +0x594  ambient-enable gate
    CString m_598;       // +0x598
    CString m_groupName; // +0x59c
    CString m_hostName;  // +0x5a0
    // +0x5a4 is a DWORD (NetMgr stores m_cmdDelay here; the net color-config dialog
    // reads it + m_5a8 as a color word pair). Tick reads it as a DWORD (m_curSlotId
    // + m_5a4*2) and again as its low byte ((u8)m_5a4 << 1), so the byte uses cast.
    i32 m_5a4;          // +0x5a4  cmd-delay / step-shift base / color word (DWORD)
    i32 m_drainReload;  // +0x5a8  drain reload value (net color-config: 2nd color word)
    i32 m_5ac;          // +0x5ac  game-closed flag (lobby watchdog; == NetMgr m_gameClosed)
    i32 m_5b0;          // +0x5b0  lobby current-selection index / custom-level id (net-game
                        //          dlg: BuildRezPath arg cast (void*)m_5b0)
    CString m_5b4;      // +0x5b4
    CString m_5b8;      // +0x5b8
    i32 m_5bc;          // +0x5bc  gate flag
    i32 m_hostIndex;    // +0x5c0  host index (ResolveHost arg)
    i32 m_lastSenderId; // +0x5c4  sender-id latch (dispatch id 0x402 records the
                        //         sender; NetDlgSessionStop's EndDialog result)
    char m_pad5c8_5cc[0x5cc - 0x5c8];
    i32 m_curSlotId;  // +0x5cc  current slot id
    i32 m_5d0;        // +0x5d0
    i32 m_drainTimer; // +0x5d4  remaining-time accumulator
    i32 m_frameDelta; // +0x5d8  frame delta
    i32 m_lastTime;   // +0x5dc  last timeGetTime
    i32 m_accumTime;  // +0x5e0  accumulated time
    i32 m_5e4;        // +0x5e4  second timeGetTime
    i32 m_5e8;        // +0x5e8
    i32 m_5ec;        // +0x5ec
    char m_pad5f0_600[0x600 - 0x5f0];
    i32 m_600;        // +0x600  committed / abort gate (start dlg + watchdog; == NetMgr m_600)
    CByteArray m_604; // +0x604
};

#endif // GRUNTZ_GRUNTZ_CMULTI_H
