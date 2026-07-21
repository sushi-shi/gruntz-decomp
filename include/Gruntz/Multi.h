#ifndef GRUNTZ_GRUNTZ_CMULTI_H
#define GRUNTZ_GRUNTZ_CMULTI_H

#include <rva.h>
#include <Gruntz/Play.h>
#include <Mfc.h>

class CGameApp;              // WAP32 app (<Wap32/Wap32.h>); CGruntzMgr::m_owner (+0x08)
class CTileTriggerContainer; // CMulti::m_2e4
class CGruntzSoundZ;         // CGruntzMgr::m_sound (+0x48; PlayByName/FindBank)
class CFontConfig;           // CGruntzMgr::m_chatLog (+0x5c; the chat/text-input config)
class CChatBoxOwner;         // CMulti::m_2e0 (per-frame LoadChatBoxSprite sub)
#include <Gruntz/MapMgr.h>   // CBrickzGrid IS CMapMgr (a typedef now - a fwd decl
class CWorldSoundSet;        // CGruntzMgr::m_inputState (+0x54; Retune @0xbd60)
class CNetMgr;          // CMulti::m_netGate/+0x524 pointee (net-stat/session wrappers)
struct CNetGameMgr;     // the network facet of CState::m_4 (m_wnd/m_6c/m_channels/m_38)
struct CNetPlayerEntry; // the local-player descriptor stored at +0x5bc
struct CNetStatPacket; // the 0x10-byte stat packet the Send* family ships
struct CNetCtrlMsg;    // control-message arg (HandleControlMsg)
struct CNetVersionMsg; // version-check message arg (HandleVersionCheck)
class GruntzPlayer;    // the 0x238 per-player/channel record (BroadcastOneChannel)
struct CNetSession;    // the +0x520 command-session facet (Session() accessor)

class CMultiLogicDesc {
public:
    u32 m_0;    // +0x00  the DPLCONNECTION dwSize dword (the desc overlays the blob)
    u8 m_flags; // +0x04  bit 0x2 -> m_isHost latch
    char m_pad5_8[0x8 - 0x5];
    char* m_8;            // +0x08  host-name string
    CMultiLogicDesc* m_c; // +0x0c  linked descriptor; its m_8 host-name is copied into a CString
};

void SetActiveAndFocus(void* hwnd); // 0x00518930

// CMultiPlayer DISSOLVED: it was a fake view of CNetPlayerListNode (<Net/NetMgr.h>) -
// OpenPlayer @0x5786d0 RezAlloc's a 0x58-byte node with vptr 0x5f0760 (the real
// CNetPlayerListNode), and GroupName @0xb76a0 just reads m_desc.m_lpszName @+0x34.
class CNetPlayerListNode; // <Net/NetMgr.h>  OpenPlayer result

class InterfaceObject; // <Net/InterfaceObject.h>

class CMultiReportGate {
public:
    // Slot identities from the real ??_7CNetMgr @0x1ea42c (5 slots, the MFC CObject
    // scheme): [0] GetRuntimeClass (0x1bef01), [1] the scalar-deleting dtor.
    virtual void GetRuntimeClass();       // [0] CObject slot (0x1bef01)
    virtual ~CMultiReportGate();          // slot 1 (deleting dtor -> cl-emitted ??_G)
    i32 Bind(i32* tmpl);                  // 0x578170  bind to host template -> nonzero ok
    void Activate();                      // 0x578750
    CNetPlayerListNode* OpenPlayer(char* name); // 0x5786d0 -> the opened player (0 fail)
    void M178a80(void* h, i32 a);         // 0x578a80  watchdog re-probe (EnumGroupsRange)
    // Create a session player from `name` (the CNetMgr wrapper method, RVA 0x178cb0);
    // returns the player record pointer (0 = fail). Reloc-masked.
    i32 CreatePlayer(void* name, i32 b, i32 c); // 0x178cb0
    char m_pad04[0x70 - 0x4];
    InterfaceObject* m_playerInfo; // +0x70  the DirectPlay service-provider node (IsInterface1-5 probes)
    CNetPlayerListNode* m_player;   // +0x74  the player StartTitle opened (OpenPlayer result)
    i32 m_78; // +0x78  reset to 0 by CMultiStartDlg::DoDataExchange (load pass)
};

class CMulti;

class CMulti : public CPlay {
public:
    // Realized real-polymorphic: a virtual dtor makes cl emit ??_7CMulti@@6B@. The
    // vptr occupies +0x00, so every matched CMulti method is codegen-neutral. ~CMulti's
    // leading manual vptr
    // stamp is dropped so cl's implicit entry vptr-store survives (a leading manual
    // stamp would dead-store-eliminate the implicit one -> no ??_7). The mid-dtor
    // CPlay/CState restamps are realized via local dtor-view classes in CMulti.cpp.
    // Constructed by CGruntzMgr::TransitionState (`new CMulti`, state id 17, `push 0x660`);
    // defined out-of-line in GruntzMgr.cpp, the only TU that builds one.
    CMulti();
    virtual ~CMulti() OVERRIDE; // slot 0  0x08d270 (most-derived /GX dtor; the ~CPlay->~CState
                                // base chain tears the CPlay/CState sub-objects)
    // The 13 vtable slots CMulti overrides on CState/CPlay (RTTI vtbl 0x1e9fe4, 43 slots).
    // Own-body slots carry the canonical family name (one name per slot, RTTI+ILT-proven);
    // the rest are declared-only anchors (impls unreconstructed / reloc-masked).
    // slot 1 (CState): the multiplayer connect/init driver @0x0b5460, /GX 18-EH-state
    // (Multi.cpp; retail ??_7CMulti slot 1 = ILT 0x3fb2 -> 0xb5460, ex
    // "SetupMultiplayerSession") - runs the whole "start a networked game" sequence:
    // peer CNetMgr + session + interface + command manager new'd and wired, then the
    // connect wait + first poll. Returns 1 on a fully-established session.
    virtual i32 LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) OVERRIDE;
    // slot 2 (CState): the lobby-drain teardown @0x0b6110 (Multi.cpp; retail slot 2 =
    // ILT 0x2ef5 -> 0xb6110, ex "Teardown") - final stat pushes, free the session/
    // report-gate/overlay sub-objects, then chain CPlay::ReleaseResources.
    virtual void ReleaseResources() OVERRIDE;
    virtual GameStateId Update() OVERRIDE; // slot 4  (CState)
    // slot 5 (CState): the per-frame lobby pump @0x0b6890 (Multi.cpp; retail slot 5 =
    // ILT 0x2dab -> 0xb6890, ex "Tick").
    virtual i32 Render() OVERRIDE;
    virtual i32 Vslot09(i32) OVERRIDE;      // slot 9  (CState)
    virtual i32 FrameSlot28(i32) OVERRIDE;  // slot 10 (CState)
    virtual i32 Vslot0b(i32, i32) OVERRIDE; // slot 11 (CState)
    virtual i32 Vslot15() OVERRIDE;         // slot 21 (CState)
    virtual i32 Vslot1a() OVERRIDE;         // slot 26 (CPlay)
    virtual i32 GetFrame() OVERRIDE;        // slot 27 (CPlay)
    // slot 30 (CPlay): the per-mode session/level (re)starter @0x0b6580 (Multi.cpp;
    // retail slot 30 = ILT 0x449e -> 0xb6580, ex "StartSession") - resolve the chosen
    // host, reseed RNG + frame timers, prime the per-slot config table, load the
    // level via CPlay::LoadByMode, re-arm the session.
    virtual i32 LoadByMode(i32 mode, i32 unused) OVERRIDE;
    virtual void OnExit() OVERRIDE;        // slot 32 (CPlay; ex "Vslot20")
    virtual void TickStateMgrs() OVERRIDE; // slot 38 (CPlay; 0x0bd3c0, ex "Vslot26")
    // The owner back-ptr (CState::m_4) IS the real CGruntzMgr - the game-manager
    // singleton the lobby methods drive. No downcast: LogLine/RunDialog/... resolve to
    // the canonical
    // EnterModalUI/RunModalDialog/... at their real rvas.
    CGruntzMgr* Mgr() {
        return m_mgr;
    }
    // The +0x524 join/report gate IS the real small CNetMgr (the DirectPlay session
    // wrapper, ??1 @0xb6000). The 0xb9xxx net-stat/session methods (below) reach it as
    // such.
    CNetMgr* Peer() {
        return reinterpret_cast<CNetMgr*>(m_netGate);
    }
    // The local player descriptor at +0x5bc (the roster/dialog TUs view the slot as an
    // i32 token, so the canonical member stays i32; this accessor is the typed view).
    CNetPlayerEntry* LocalPlayer() {
        return reinterpret_cast<CNetPlayerEntry*>(m_5bc);
    }
    // The game-manager (CState::m_4) as its network facet (m_wnd/m_6c/m_channels/m_38);
    // the net methods reach it as CNetGameMgr.
    CNetGameMgr* NetGameMgr() {
        return reinterpret_cast<CNetGameMgr*>(m_mgr);
    }
    // The resync WM_COMMAND lParam latch (the inherited CState::m_levelIndex slot, role-reused).
    i32& ResyncLParam() {
        return m_levelIndex; // the CState +0x1c slot reused as the resync latch
    }
    // The +0x520 session (command-slot + lobby-sync facets are the same CNetSession).
    CNetSession* Session() {
        return m_session;
    }

    // (The dtor-run lobby drain @0xb6110, ex "Teardown", IS the slot-2
    // ReleaseResources override declared above - one slot, one name.)
    CString& ClearString59c(CString& s); // 0x0b76c0  (assign m_groupName <- empty)
    CString& ClearString5a0(CString& s); // 0x0b7730  (assign m_hostName <- empty)
    CString GetString59c();              // 0x0b7a90 (out-of-line: return m_groupName by value)
    RVA(0x000b7ad0, 0x23)
    CString GetString5a0() {
        return m_hostName;
    }
    // The two config-name accessors (moved off the conflated CNetMgr in the
    // netmgr-vs-cmulti split): return the m_5b4 / m_5b8 CStrings by value. `this`
    // is a CMulti at every call site (SetupMultiplayerSession/OnJoinConfirm/...).
    RVA(0x000b6090, 0x23)
    CString GetConfigNameA() {
        return m_5b4;
    }
    RVA(0x000b60d0, 0x23)
    CString GetConfigNameB() {
        return m_5b8;
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
    //  reached with a null recipient.)

    // Reconstructed in this TU (ascending RVA). (StartSession @0xb6580 is the
    // slot-30 LoadByMode override; Tick @0xb6890 is the slot-5 Render override -
    // both declared with the vtable slots above.)
    i32 Connect(i32 mode); // 0x0b67f0  probe + wait for the session
    i32 StartTitle();      // 0x0b72c0  /GX: build "TITLE%d" + bind the net host
    void DropTimeout();    // 0x0bc2d0  /GX: drop a timed-out player
    // 0x0bc910  /GX: latch session params, create the host player, register the channel.
    i32 OpenHostChannel(void* a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

    // External CMulti methods this TU calls but does not define (reloc-masked).
    void SendStatFlag(i32 code, i32 flag); // 0x0b9240 (__thiscall, reads m_5bc)
    // Build a {id, value} stat packet and ship it through the +0x524 CNetMgr
    // (the NetMgr.h-side name/signature, same RVA).
    void SendNetStat(i32 id, u32 value, i32 flag); // 0x0b9290 (__thiscall, 3 args)
    // The multiplayer net-game / lobby-watchdog dialog helpers reach these on the
    // g_multiState singleton (Ghidra labels them CNetMgr::, the same-object dual-view).
    void DropPlayer(i32 id);        // 0x0bb510 (roster-side declared-only alias)
    i32 DropChannelPlayer(i32 idx); // 0x0bb510
    i32 Poll(i32 token);            // 0x0bba10  (via ILT 0x1249; verify-custom-level poll)
    i32 ResolveLocalPlayer();       // 0x0ba7d0
    void ReportAckLatency();        // 0x0bd000
    i32 VerifyCustomLevel(void* h, i32 token); // 0x0b8fc0
    i32 PollSession();                         // 0x0b95f0 (drain the receive queue; ret i32)
    void AutoTuneCmdDelay();                   // 0x0bcc10
    // (CPlay::ReleaseResources @0xc8700, the CPlay slot-2 body ex "CPlayDtorBody",
    // is inherited - CMulti::ReleaseResources chains it with the qualified CPlay::
    // spelling, binding the real CPlay method.)
    void OnDropPlayer(); // 0x0bc110
    // (The ex "RebindHost"/"RebindHostAlt" alias decls @0xbc750/0xbc460 are GONE -
    // they ARE CreateLocalPlayer / SetupTcpIpConfig below, one symbol per body.)
    // The connect-drive helpers the Net-side coordinator (NetMgrMisc.cpp) reaches
    // off the g_multiState singleton. In the 0xb6110-0xbc420 lobby cluster; Ghidra labels
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
    // (The ex "LoadLevelByMode" @0xca200 alias is GONE - that body is the inherited
    // CPlay::LoadByMode slot-30 default, reached with the qualified CPlay:: spelling
    // since CMulti overrides the slot. The ex "LoadTitleScreen" @0xfa350 alias is
    // GONE - that body is the inherited CState::RunTitleSeq. The ten CPlay per-frame
    // helper re-decls (StepInputA/StepC/LoadScrollSpeedOptions/StepScroll/
    // NotifyVisibleEntities/StepGridWalk/winapi_0d0b30_CopyRect/DrawDebugStats/
    // OnRegion1/OnRegion2) are GONE too - CMulti : public CPlay inherits the real
    // methods from <Gruntz/Play.h>, so the calls bind the canonical CPlay symbols.)

    // ---- The 0xb5xxx-0xbdxxx network/lobby method cluster ----
    // Recovered from the Frankenstein <Net/NetMgr.h> CNetMgr onto their true owner
    // (this CMulti): retail runs each on this=g_curMulti (a CMulti at offset 0). The
    // small real CNetMgr is reached via Peer() (+0x524). Defined in Multi.cpp.
    // (SetupMultiplayerSession @0xb5460 is the slot-1 LoadGameAssetNamespaces
    // override declared with the vtable slots above.)
    i32 Open(); // 0x0b77a0
    i32 SetupServices();                                 // 0x0b78b0
    i32 DetectConnectionConfig();                        // 0x0b82e0
    void ApplyCmdDelayDefaults();                        // 0x0b85a0
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
    void WriteTag(const char* tag); // 0x0bd4a0 (reloc-masked)
    // Drop/wait helpers (moved off the conflated CNetMgr in the netmgr-vs-cmulti
    // split): both run on `this`==CMulti (they read the +0x520 session, the +0x604
    // drop-id CDWordArray, m_534, m_hostIndex).
    void RecordDropPlayer2(i32 a, i32 id);                                       // 0x0bb5e0
    i32 WaitForOtherPlayers();                                                   // 0x0bb700
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
    void SetServiceName(CString s);                // 0xb7730 (external)
    void PopulateGroupList(void* hList, i32 flag); // 0x1784be (external)
    // (ReportError/SetReportMode are CNetMgr statics reached directly as
    //  CNetMgr::ReportError - <Net/NetMgr.h> is included by Multi.cpp.)

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
    i32 m_levelVerifyResult;     // +0x53c  level-verify response latch (VerifyCustomLevel/Poll)
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
    // +0x604  the drop-id / vote scratch array. It is a CDWordArray (dword
    // elements): WaitForOtherPlayers drives it via CDWordArray SetSize
    // @0x1b4bad / SetAtGrow @0x1b4d7c, and RecordDropPlayer2 reads GetData()
    // (+0x608) / GetSize() (+0x60c) as the pending-drop id list.
    CDWordArray m_604; // +0x604
    // Tail padding to the TRUE retail size: TransitionState `push 0x660; call ??2` @0x8bd24,
    // then the inline `mov [esi],??_7CMulti@@6B@` (0x5e9fe4) stamp.
    char m_pad618[0x660 - 0x618];
};

extern CMulti* g_multiState;
extern CString g_sessionName;
extern "C" i32 g_optionsCursor;

#endif // GRUNTZ_GRUNTZ_CMULTI_H
