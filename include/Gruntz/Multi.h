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
// `call rel32` are reloc-masked.
//
// INHERITANCE MODELED (the devs' true shape): `class CMulti : public CPlay`
// (single chain CMulti : CPlay : CState; CPlay derives CState per <Gruntz/Play.h>).
// CMulti's low-offset members are the CState/CPlay sub-object fields, accessed by
// their canonical names/types: the owner back-ptr is CState::m_4, the real CGruntzMgr
// the lobby methods drive; the shared-offset scalar/pointer views
// (m_2c asset slot, m_hudRect inline RECT, m_hitTest, m_guts, m_beginMarker as a
// CTileTriggerContainer view, m_overlayActive as a CLobbyObjA view, the region/ambient
// timers, the cue fields) fold onto CPlay's named members. CMulti keeps only its own
// multiplayer block (+0x520..+0x604). ~CMulti tears down that block; the compiler-chained
// ~CPlay -> ~CState destructors do the base sub-objects. The 14 overridden vtable slots
// (0,1,2,4,5,9,10,11,21,26,27,30,32,38) are declared OVERRIDE below (vtable_hierarchy
// audit: INHERIT/OVERRIDE/MISSING all clear).
#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>
#include <Gruntz/Play.h>
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
class CGameApp;              // WAP32 app (<Wap32/Wap32.h>); CGruntzMgr::m_owner (+0x08)
class CTileTriggerContainer; // CMulti::m_2e4
class CGruntzSoundZ;         // CGruntzMgr::m_sound (+0x48; PlayByName/FindBank)
class CFontConfig;           // CGruntzMgr::m_chatLog (+0x5c; the chat/text-input config)
class CChatBoxOwner;         // CMulti::m_2e0 (per-frame LoadChatBoxSprite sub)
#include <Gruntz/MapMgr.h>   // CBrickzGrid IS CMapMgr (a typedef now - a fwd decl
                             // of it would be a redefinition, C2371)
class CWorldSoundSet;        // CGruntzMgr::m_inputState (+0x54; Retune @0xbd60)
// The small real CNetMgr (the +0x524 DirectPlay wrapper) and the network views of
// CMulti's base sub-objects, forward-declared so the accessors below compile without
// pulling the heavy <Net/NetMgr.h> into every Multi.h includer (Multi.cpp has it).
class CNetMgr;          // CMulti::m_netGate/+0x524 pointee (net-stat/session wrappers)
struct CNetGameMgr;     // the network facet of CState::m_4 (m_wnd/m_6c/m_channels/m_38)
struct CNetPlayerEntry; // the local-player descriptor stored at +0x5bc
// (the ex-CSndSubMgr facet of CState::m_c is gone - the holder's m_28 IS the CSndHost)
struct CNetStatPacket; // the 0x10-byte stat packet the Send* family ships
struct CNetCtrlMsg;    // control-message arg (HandleControlMsg)
struct CNetVersionMsg; // version-check message arg (HandleVersionCheck)
struct CNetChannel;    // per-channel record (BroadcastOneChannel)
struct CNetSession;    // the +0x520 command-session facet (Session() accessor)

// DISSOLVED (RELOC-Multi): the ex-`CMultiMgr` view IS the CGruntzMgr game-manager
// singleton (*0x64556c) - the CState owner at CMulti+0x04 (CState::m_4, already typed
// CGruntzMgr* in <Gruntz/State.h>). Every one of its "own" methods was a fake alias of a
// real CGruntzMgr method at the SAME rva (disasm-proven): LogLine == EnterModalUI@0x8ef10
// (it forwards the text to CGameApp::ShowMessage@0x80c00), RunDialog ==
// RunModalDialog@0x90260, ProbeSession == PassClickToPlayState@0x8d780, ResolveHost ==
// FindOptionsSlot@0x92e80, ReportError == ReportError@0x8dc60, Step2d33 ==
// AdvanceOptionsCycle@0x933e0 (ILT 0x2d33). Its members are CGruntzMgr's canonical slots
// (m_48 == m_sound, m_54 == m_inputState, m_5c == m_chatLog, m_60 == m_timer, m_9c ==
// m_lobbyResult, m_c0 == m_lobby, m_150 == m_options[4], m_0c == CGameMgr::m_frameGate).
// Mgr() below now returns the REAL CGruntzMgr*, so those calls reloc-bind.
//
// The ex-`CMultiDialogHook` (+0x60 pre-dialog receiver) IS CGruntzMgr::m_timer, the real
// CGruntSpawnConfig: its PreDialog/StartTitleHook were both aliases of
// CGruntSpawnConfig::DtorBody@0x11c7b0 (the 2-iter pair teardown; CGruntzMgr::EnterModalUI
// itself calls it through ILT 0x20a4 on the same +0x60 slot).
//
// STILL DEFERRED (blocked on the GruntzMgr lane): five CGruntzMgr members whose canonical
// type in <Gruntz/GruntzMgr.h> is still a placeholder (m_cmdGrid/CTriggerMgr,
// m_cmdSubMgr/CmdSink, m_cmdNotify/CmdSinkV, m_connSettings/void*, m_options/
// CGruntzMgrOptions - the last three are only declared, or padded, in that header). The
// multiplayer facets those slots wear are modeled below and reached by a documented
// reinterpret at the (9) use sites in Multi.cpp; once GruntzMgr.h types them, the facets
// fold away too.

// (CSlotConfig + CMultiMgrOptions are GONE - two more names for one class. The entry is
// the real GruntzPlayer (<Gruntz/GruntzPlayer.h>) and its "inner slot-config sub-object"
// at +0x38 is that class's real CBattlezMapConfig member: the three thiscalls the
// session-start path drove on &m_inner are LoadConfig / FreeArrays / Clear_02ade0, which
// is exactly the CBattlezMapConfig interface every other consumer already cast to. See
// the 6-way conflation proof in GruntzPlayer.h.)

// The +0x6c list head's element type (CPtrList node, removed via RemoveHead).
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
    char m_head[0x28 - 0x1c];      // +0x1c  CPtrList head (RemoveHead via 0x5b4a03)
    i32 m_28;                      // +0x28  emptiness/count gate
    CMultiLogicNode* RemoveHead(); // 0x005b4a03 (MFC CPtrList::RemoveHead)
    void Step20b3(i32 v);          // per-frame poke (PumpA, thiscall)
};

// The DirectPlay launch connection-settings buffer CGruntzMgr::m_connSettings (+0xc4)
// points at (flags at +0x4, player/host name at +0x8) - the multiplayer facet of that
// still-`void*` canonical member.
class CMultiLogicDesc {
public:
    void* m_0;
    u8 m_flags; // +0x04  bit 0x2 -> m_isHost latch
    char m_pad5_8[0x8 - 0x5];
    char* m_8;            // +0x08  host-name string
    CMultiLogicDesc* m_c; // +0x0c  linked descriptor; its m_8 host-name is copied into a CString
};

// Win32 focus restore on the innermost window (m_logic->m_gameWnd->m_hwnd); __cdecl,
// reloc-masked.
void SetActiveAndFocus(void* hwnd); // 0x00518930 (ex MultiRestoreFocus)

// The opened-player object OpenPlayer returns (group-name accessor @0x4b76a0).
class CMultiPlayer;

// The report gate's player-info sub-object (gate->m_70): per-slot occupancy probes
// the multiplayer-start dialog reads to derive the player count. Reloc-masked
// __thiscall leaves in the DirectPlay session cluster (0x1794xx).
class CMultiPlayerInfo {
public:
};
SIZE_UNKNOWN(CMultiPlayerInfo);

// The join/report gate at CMulti+0x524 - IDENTITY RESOLVED (netmgr-vs-cmulti):
// this IS the real CNetMgr (RTTI CNetMgr : CObject, ??_7 @0x1ea42c, virtual dtor
// ??1 @0xb6000 = the scalar-dtor slot below; <Net/NetMgr.h> models it in full,
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
    // Slot identities from the real ??_7CNetMgr @0x1ea42c (5 slots, the MFC CObject
    // scheme): [0] GetRuntimeClass (0x1bef01), [1] the scalar-deleting dtor.
    virtual void GetRuntimeClass();       // [0] CObject slot (0x1bef01)
    virtual ~CMultiReportGate();          // slot 1 (deleting dtor -> cl-emitted ??_G)
    i32 Bind(i32* tmpl);                  // 0x578170  bind to host template -> nonzero ok
    void Activate();                      // 0x578750
    CMultiPlayer* OpenPlayer(char* name); // 0x5786d0 -> the opened player (0 fail)
    void M178a80(void* h, i32 a);         // 0x578a80  watchdog re-probe (EnumGroupsRange)
    // Create a session player from `name` (the CNetMgr wrapper method, RVA 0x178cb0);
    // returns the player record pointer (0 = fail). Reloc-masked.
    i32 CreatePlayer(void* name, i32 b, i32 c); // 0x178cb0
    char m_pad04[0x70 - 0x4];
    CMultiPlayerInfo* m_70; // +0x70  per-slot player-info sub-object (start-dialog probes)
    CMultiPlayer* m_player; // +0x74  the player StartTitle opened (OpenPlayer result)
    i32 m_78;               // +0x78  reset to 0 by CMultiStartDlg::DoDataExchange (load pass)
};

// The +0x520 lobby-session object (m_session) and the +0x320 attract overlay: each
// torn down by a thiscall method then handed to the engine free (see CMulti::Teardown).
// IDENTITY RESOLVED (op REHOME MF-Net): m_session IS the canonical CNetSession
// (<Net/NetMgr.h>) - `new CNetSession`, dtor @0xb6220. Its command-session methods
// (Reset/Verify/CheckLatency/CreateSlot/FindCmdSlot) and lobby-sync methods
// (Poll/Tick/Advance/Reconcile) run on the SAME object; the ex-views CNetSession2 /
// CLobbyObjB / CLobbySync are folded onto CNetSession, its 0x64-byte slots onto
// CNetCmdSlot. m_session is typed CNetSession* below (fwd-decl only; Multi.cpp pulls
// the full <Net/NetMgr.h>). The +0x320 overlay's teardown is CLightFxRender::Ctor
// @0xa3360 - bound directly in CMulti::Teardown (the ex-CLobbyObjA view is dissolved).

// CMulti's own (manually-stamped) vtable. Only the two slots the per-frame Tick
// dispatches through are typed; the rest are opaque. The slots are thiscall in
// retail (ecx = this); modeled COM-style (explicit this arg) so MSVC 5.0 accepts
// the function-pointer types - the exact convention is part of Tick's documented
// codegen wall, so this expresses intent without perturbing a matched function.
class CMulti;
struct CMultiSlotView {
    char m_pad00_7c[0x7c];
    void(__stdcall* Redraw)(CMulti*, i32 a, i32 b, i32 c); // +0x7c
    char m_pad80_98[0x98 - 0x80];
    void(__stdcall* PostRedraw)(CMulti*); // +0x98
};

class CMulti : public CPlay {
public:
    // Realized real-polymorphic: a virtual dtor makes cl emit ??_7CMulti@@6B@. The
    // vptr occupies +0x00 (where the old CMultiSlotView* m_vtbl lived - same offset, so
    // every matched CMulti method is codegen-neutral). ~CMulti's leading manual vptr
    // stamp is dropped so cl's implicit entry vptr-store survives (a leading manual
    // stamp would dead-store-eliminate the implicit one -> no ??_7). The mid-dtor
    // CPlay/CState restamps are realized via local dtor-view classes in CMulti.cpp.
    // Tick dispatches through vtbl() (reads the vptr as a CMultiSlotView*), preserving
    // its +0x7c/+0x98 indirect-call bytes.
    // Constructed by CGruntzMgr::TransitionState (`new CMulti`, state id 17, `push 0x660`);
    // defined out-of-line in GruntzMgr.cpp, the only TU that builds one.
    CMulti();
    virtual ~CMulti() OVERRIDE; // slot 0  0x08d270 (most-derived /GX dtor; the ~CPlay->~CState
                                // base chain tears the CPlay/CState sub-objects)
    // The 13 vtable slots CMulti overrides on CState/CPlay (RTTI vtbl 0x1e9fe4, 43 slots).
    // Declared-only anchors (the real impls are the RVA-bound methods below / reloc-masked).
    virtual i32 Vfunc1(i32, i32, i32) OVERRIDE; // slot 1  (CState)
    virtual void ReleaseResources() OVERRIDE;   // slot 2  (CState)
    virtual GameStateId Update() OVERRIDE;      // slot 4  (CState)
    virtual i32 Render() OVERRIDE;              // slot 5  (CState)
    virtual i32 Vslot09(i32) OVERRIDE;          // slot 9  (CState)
    virtual i32 FrameSlot28(i32) OVERRIDE;      // slot 10 (CState)
    virtual i32 Vslot0b(i32, i32) OVERRIDE;     // slot 11 (CState)
    virtual i32 Vslot15() OVERRIDE;             // slot 21 (CState)
    virtual i32 Vslot1a() OVERRIDE;             // slot 26 (CPlay)
    virtual i32 GetFrame() OVERRIDE;            // slot 27 (CPlay)
    virtual i32 LoadByMode(i32, i32) OVERRIDE;  // slot 30 (CPlay; the per-mode level loader)
    virtual void OnExit() OVERRIDE;             // slot 32 (CPlay; ex "Vslot20")
    virtual void Vslot26() OVERRIDE;            // slot 38 (CPlay)
    CMultiSlotView* vtbl() {
        return *(CMultiSlotView**)this;
    }
    // The owner back-ptr (CState::m_4) IS the real CGruntzMgr - the game-manager
    // singleton the lobby methods drive. No downcast: the ex-CMultiMgr view is dissolved
    // (see the note above), so LogLine/RunDialog/... now resolve to the canonical
    // EnterModalUI/RunModalDialog/... at their real rvas.
    CGruntzMgr* Mgr() {
        return m_4;
    }
    // The +0x524 join/report gate IS the real small CNetMgr (the DirectPlay session
    // wrapper, ??1 @0xb6000). The 0xb9xxx net-stat/session methods (below) reach it as
    // such; was the CMultiReportGate view / ((CNetMgr*)m_netGate) casts.
    CNetMgr* Peer() {
        return (CNetMgr*)m_netGate;
    }
    // The local player descriptor at +0x5bc (the roster/dialog TUs view the slot as an
    // i32 token, so the canonical member stays i32; this accessor is the typed view).
    CNetPlayerEntry* LocalPlayer() {
        return (CNetPlayerEntry*)m_5bc;
    }
    // The game-manager (CState::m_4) as its network facet (m_wnd/m_6c/m_channels/m_38);
    // the net methods reach it as CNetGameMgr (was the Frankenstein CNetMgr::m_4 view).
    CNetGameMgr* NetGameMgr() {
        return (CNetGameMgr*)m_4;
    }
    // The resync WM_COMMAND lParam latch (CState +0x1c base-class field, unmodeled here).
    i32& ResyncLParam() {
        return *(i32*)((char*)this + 0x1c);
    }
    // The +0x520 session (command-slot + lobby-sync facets are the same CNetSession).
    CNetSession* Session() {
        return m_session;
    }

    // Teardown helper run first by the dtor (and standalone @0xb6110): drains the
    // lobby sub-objects and pushes the final stat flags.
    void Teardown();                     // 0x0b6110
    CString& ClearString59c(CString& s); // 0x0b76c0  (assign m_groupName <- empty)
    CString& ClearString5a0(CString& s); // 0x0b7730  (assign m_hostName <- empty)
    CString GetString59c();              // 0x0b7a90 (out-of-line: return m_groupName by value)
    RVA(0x000b7ad0, 0x23)
    CString GetString5a0() {
        return m_hostName;
    }
    CString Name42ff(); // ILT 0x0042ff -> GetConfigNameB 0x0b60d0 (MultiColorDlg)
    CString Name31d4(); // ILT 0x0031d4 -> GetConfigNameA 0x0b6090 (MultiColorDlg)
    void ReportVersionMsg(char* msg, i32 code); // 0x0b7e30
    // Status-bar diagnostic by string-resource id: LoadStringA through
    // m_logic->m_owner->m_hInstance then ReportVersionMsg. Defined in
    // Net/LobbyDialogs.cpp (its RVA-order home).
    void ReportStatusId(u32 strId, i32 level);                 // 0x0b7ec0
    void ReportNetError(i32 level);                            // 0x0b7f60
    i32 JoinSession();                                         // 0x0b7fe0
    i32 RunErrorDialog(char* tmpl, void* handler, i32 lparam); // 0x0bc250 (_MultiDispatch)
    void AckJoinFailure();                                     // 0x0bc420
    // (0x0bccd0 replay-name/config commit is the canonical CMulti::SaveConfig below,
    //  reached with a null recipient; the former Commit3ada view is dissolved onto it.)

    // Reconstructed in this TU (ascending RVA).
    i32 StartSession(i32 mode, i32 unused); // 0x0b6580  arm the slot table + reseed timers
    i32 Connect(i32 mode);                  // 0x0b67f0  probe + wait for the session
    i32 Tick();                             // 0x0b6890  per-frame lobby pump
    i32 StartTitle();                       // 0x0b72c0  /GX: build "TITLE%d" + bind the net host
    void DropTimeout();                     // 0x0bc2d0  /GX: drop a timed-out player
    // 0x0bc910  /GX: latch session params, create the host player, register the channel.
    i32 OpenHostChannel(void* a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

    // External CMulti methods this TU calls but does not define (reloc-masked).
    void SendStatFlag(i32 code, i32 flag); // 0x0b9240 (__thiscall, reads m_5bc)
    // Build a {id, value} stat packet and ship it through the +0x524 CNetMgr
    // (was SendNetStat3; the NetMgr.h-side name/signature, same RVA).
    void SendNetStat(i32 id, u32 value, i32 flag); // 0x0b9290 (__thiscall, 3 args)
    // The multiplayer net-game / lobby-watchdog dialog helpers reach these on the
    // g_64bd5c singleton (Ghidra labels them CNetMgr::, the same-object dual-view).
    // Folded here from the former per-TU CNetSession / WatchSess lens types.
    void DropPlayer(i32 id);        // 0x0bb510 (roster-side declared-only alias)
    i32 DropChannelPlayer(i32 idx); // 0x0bb510
    i32 Poll(i32 token);            // 0x0bba10  (via ILT 0x1249; verify-custom-level poll)
    i32 ResolveLocalPlayer();       // 0x0ba7d0
    void ReportAckLatency();        // 0x0bd000
    i32 VerifyCustomLevel(void* h, i32 token); // 0x0b8fc0
    i32 PollSession();                         // 0x0b95f0 (drain the receive queue; ret i32)
    void AutoTuneCmdDelay();                   // 0x0bcc10
    // CPlayDtorBody @0xc8700 is CPlay::CPlayDtorBody (inherited - no CMulti redecl, so
    // Teardown's call binds to the real CPlay method).
    void OnDropPlayer(); // 0x0bc110
    i32 RebindHost();    // 0x0bc750  (also CNetMgr-shared)
    i32 RebindHostAlt(); // 0x0bc460
    // The connect-drive helpers the Net-side coordinator (NetMgrMisc.cpp) reaches
    // off the g_64bd5c singleton. In the 0xb6110-0xbc420 lobby cluster; Ghidra labels
    // them CNetMgr:: (Broadcast*), a heuristic mis-attribution of this CMulti cluster.
    i32 BroadcastChannelTable(CNetPlayerEntry* recipient); // 0x0ba810
    i32 BroadcastOneChannel(i32 chan);                     // 0x0baf00
    // Register a channel from a host template (CNetMgr-shared, RVA 0x0baa90); the
    // host-open path calls it on `this`. Reloc-masked.
    i32 RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f); // 0x0baa90
    // Assemble + broadcast one chat line (stat 0x3f0), optionally appending it
    // to the hWnd chat edit (the lobby DlgProcs' chat submit path).
    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd); // 0x0bb190
    i32 ReadGroupSel();                                                     // 0x0b76a0
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

    // ---- The 0xb5xxx-0xbdxxx network/lobby method cluster (wave5-F2 fold) ----
    // Recovered from the Frankenstein <Net/NetMgr.h> CNetMgr onto their true owner
    // (this CMulti): retail runs each on this=g_curMulti (a CMulti at offset 0). The
    // small real CNetMgr is reached via Peer() (+0x524). Defined in Multi.cpp.
    i32 SetupMultiplayerSession(i32 a1, i32 a2, i32 a3); // 0x0b5460
    i32 Open();                   // 0x0b77a0  (was the NetSessionOpener this-view)
    i32 SetupServices();          // 0x0b78b0
    i32 DetectConnectionConfig(); // 0x0b82e0
    void ApplyCmdDelayDefaults(); // 0x0b85a0
    // 0x0b86c0 (/GX): pop the multiplayer-start dialog over the suspended game. On a
    // cancel it clears the host's options slot + rebroadcasts the channel table, or
    // ships the guest's 0x3ea abort stat; on OK it applies the cmd-delay defaults
    // (host) or fires the GAME_KEY cue + a 250-tick active wait (guest). Was the fake
    // CNetMgrLite view's method.
    i32 ShowMultiStartDlg();
    i32 JoinAndRegisterChannel();                                                // 0x0b8b10
    i32 OnJoinConfirm(void* hDlg);                                               // 0x0b8cf0
    i32 PollSessionGated(i32 a1, i32 a2);                                        // 0x0b9180
    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);                              // 0x0b91f0
    i32 SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c);                         // 0x0b92e0
    i32 SendStatPair(CNetPlayerEntry* recipient, CNetStatPacket* pkt, i32 c);    // 0x0b9330
    i32 SendStatTo(CNetPlayerEntry* recipient, i32 id, i32 c);                   // 0x0b93a0
    i32 SendStat3(i32 id, u32 value, i32 flag);                                  // 0x0b9410
    i32 SendNetStatTo(CNetPlayerEntry* recipient, i32 id, u32 value, i32 c);     // 0x0b9490
    i32 SendStatPairRaw(CNetPlayerEntry* recipient, void* pkt, i32 size, i32 c); // 0x0b9500
    i32 SendStatValue(i32 id, i32 statId, i32 value, i32 flag);                  // 0x0b9570
    i32 DispatchRecvMsg(i32 sender, char* buf, i32 size);                        // 0x0b9750
    i32 HandleControlMsg(CNetCtrlMsg* msg, i32 arg2);                            // 0x0ba1a0
    i32 OnPlayerLeft(i32 playerId);                                              // 0x0ba3b0
    void AckDropPlayer(i32 id);                                                  // 0x0ba590
    i32 LoadMenuSelectSprite(void* evp);                                         // 0x0ba620
    i32 ParseChannelTable(void* packet);                                         // 0x0ba980
    i32 RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e); // 0x0baac0
    i32 RegisterChannelRec(void* rec);                                           // 0x0bac40
    i32 RemoveChannel(i32 idx);                                                  // 0x0bac90
    i32 OnPauseChannel();                                                        // 0x0bad00
    void OnMultiPause();                                                         // 0x0bad40
    void OnMultiOptions();                                                       // 0x0badd0
    i32 ParseOneChannel(void* rec);                                              // 0x0baff0
    i32 SendChannelStat422();                                                    // 0x0bb0b0
    i32 SendChannelStat423();                                                    // 0x0bb120
    i32 CreateSession();                                                         // 0x0bbc90
    u32 FrameSyncWait();                                                         // 0x0bc070
    i32 SetupTcpIpConfig();                                                      // 0x0bc460
    i32 CreateLocalPlayer();                                                     // 0x0bc750
    i32 WaitForConnect();                                                        // 0x0bca50
    i32 SaveConfig(CNetPlayerEntry* recipient);                                  // 0x0bccd0
    i32 LoadConfig(void* cfg);                                                   // 0x0bce80
    i32 ResetPlayerCommands(i32 id);                                             // 0x0bcf20
    u32 GetMaxAckLatency();                                                      // 0x0bd030
    void HandleVersionCheck(CNetVersionMsg* msg);                                // 0x0bd0b0
    void AnnounceVersion(i32 param);                                             // 0x0bd180
    // External thunked helpers the cluster fires (no body here so the call reloc-masks).
    i32 MeasurePing();            // round-trip sample
    i32 ProbeLatency(i32 flag);   // secondary latency probe
    void WriteCmdDelay(i32 flag); // persist m_5a4/m_drainReload
    void SendStatPacket(i32 param, const void* packet, i32 size, i32 flag); // stat dispatcher
    void ShowChatLine(void* hWnd, const char* text);                        // 0xbb3e0 (external)
    void HandleSpriteMsg(CNetCtrlMsg* msg);                                 // 0xba620 (external)
    void RejoinIfNeeded(i32 flag);                                          // 0xba810 (external)
    void ReportConnectFailed(i32);                                          // 0xb7f60 (external)
    i32 DispatchServices(const char* cmd, i32 flag, void* cb);              // 0xbc250 (external)
    CString GetGameName();                         // 0xb7a90 (external; == GetString59c)
    void ApplyDynSetting(CString s);               // 0xb76c0 (external)
    i32 GetAmbientId();                            // 0xda200 (external)
    void SetServiceName(CString s);                // 0xb7730 (external)
    void PopulateGroupList(void* hList, i32 flag); // 0x1784be (external)
    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);   // netmgrerror (static)
    static void SetReportMode(i32 log, i32 msgBox, i32 beep, i32 third); // netmgrerror (static)

    // --- layout (placeholder names; offsets are the load-bearing truth) ---
    // --- CMulti-own multiplayer block (after CPlay base @0x518) ---
    char _padMp[0x520 - 0x51c];
    CNetSession* m_session;      // +0x520  the DirectPlay command/lobby-sync session
    CMultiReportGate* m_netGate; // +0x524
    i32 m_isHost;                // +0x528  (== Frankenstein m_useChannelLatency)
    i32 m_sessionTerminated;     // +0x52c
    i32 m_530;                   // +0x530
    i32 m_534;                   // +0x534
    i32 m_538;                   // +0x538  (Frankenstein m_removedFromGame)
    i32 m_53c;                   // +0x53c  (Frankenstein m_levelVerifyResult)
    i32 m_verifyDone;     // +0x540  Poll's exit gate (gap-named; == Frankenstein m_verifyDone)
    i32 m_recordAcked[4]; // +0x544  Poll's per-record ack latch (gap-named)
    i32 m_recordToken[4]; // +0x554  Poll's per-record vote/token latch (gap-named)
    i32 m_pollAbort;      // +0x564
    i32 m_568;            // +0x568
    i32 m_56c;            // +0x56c  (Frankenstein m_gameFull)
    i32 m_570;            // +0x570  (Frankenstein m_versionMismatch)
    i32 m_574;            // +0x574  (Frankenstein m_outOfSyncGuard)
    i32 m_syncGate;       // +0x578  frame-sync long-frame toggle gate (gap-named)
    i32 m_pumpGuard;      // +0x57c  (== Frankenstein m_57c reconnect/rejoin gate)
    i32 m_connected;      // +0x580
    i32 m_584;            // +0x584
    i32 m_588;            // +0x588
    i32 m_58c;            // +0x58c  (Frankenstein m_admitted)
    i32 m_590;            // +0x590
    i32 m_594;            // +0x594
    CString m_598;        // +0x598  (Frankenstein m_configSection)
    CString m_groupName;  // +0x59c
    CString m_hostName;   // +0x5a0
    i32 m_5a4;            // +0x5a4  (Frankenstein m_cmdDelay)
    i32 m_drainReload;    // +0x5a8  (Frankenstein m_resend)
    i32 m_5ac;            // +0x5ac  (Frankenstein m_gameClosed)
    i32 m_5b0;            // +0x5b0
    CString m_5b4;        // +0x5b4  config name CString A
    CString m_5b8;        // +0x5b8  config name CString B
    i32 m_5bc;            // +0x5bc  local player descriptor (typed via LocalPlayer())
    i32 m_hostIndex;      // +0x5c0  (== Frankenstein m_localPlayerId)
    i32 m_lastSenderId;   // +0x5c4
    char _p5c4[0x5cc - 0x5c8];
    i32 m_curSlotId;         // +0x5cc  (== Frankenstein m_resyncTick)
    i32 m_5d0;               // +0x5d0
    i32 m_drainTimer;        // +0x5d4
    i32 m_frameDelta;        // +0x5d8
    i32 m_lastTime;          // +0x5dc
    i32 m_accumTime;         // +0x5e0  (== Frankenstein m_lastFrameDelta)
    i32 m_5e4;               // +0x5e4  (Frankenstein m_lastFrameTime)
    i32 m_5e8;               // +0x5e8
    i32 m_5ec;               // +0x5ec
    i32 m_channelLatency[4]; // +0x5f0  per-channel ack-latency values (gap-named)
    i32 m_600;               // +0x600
    CByteArray m_604; // +0x604  (drop-id array; net methods view m_pData/m_nSize as dropIds/count)
    // Tail padding to the TRUE retail size: TransitionState `push 0x660; call ??2` @0x8bd24,
    // then the inline `mov [esi],??_7CMulti@@6B@` (0x5e9fe4) stamp.
    char m_pad618[0x660 - 0x618];
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_GRUNTZ_CMULTI_H
