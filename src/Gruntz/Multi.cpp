// Multi.cpp - the multiplayer TU (C:\Proj\Gruntz), interval 0x0b5380-0x0bd35d
// (+ out-of-band strays). ONE original TU per docs/exe-map/interval-dossiers.md
// #4b: our multi + netmgrgame units (plus 11 dossier stray re-homes) were slices
// of this single game-side multiplayer file - ~15 multi/netmgrgame block
// alternations span the interval (impossible for two first-link objs), the
// c:\proj\incs\netmgr.h header-inline assert emits at ?PollSession@CNetMgr
// @0x0b95f0 (this TU includes netmgr.h), and CMulti::LoadByMode(slot 30)/Render(slot 5)/PumpA/
// PumpB drive CNetMgr::CreateSession/DispatchRecvMsg/... as one cooperating
// session layer. (??1CMultiStartDlg @0xb8960 stays in ShowMultiDlg.cpp:
// COMDAT-at-usage of the dialog whose file is 0xc16b0.)
//
// Functions in strictly ascending retail-RVA order. Field names are placeholders;
// only the OFFSETS + the per-method call/branch structure are load-bearing.
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/GameRegPtr.h>
#include <rva.h>
#include <Gruntz/CurPlayer.h> // g_curPlayer
#include <Rez/FrameClock.h>   // the frame-clock/timer band the session loop reads/pumps
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (CWorld::m_68; ClearGridRange)
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <Io/FileStream.h> // CFileIO (the static MFC CFile global at 0x646778; 0x0b5400 ctor)
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/SoundStream.h> // SoundStream : SoundDevice - the REAL m_c->m_soundStream (+0x20)
#include <Gruntz/SoundCue.h> // CSndHost - the REAL m_c->m_soundRegistry (name->cue map + emit gate)
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h> // renderer B - the real CDDrawWorkerList (PruneWorkers)
#include <DDrawMgr/DDrawChildGroup.h> // the m_c->m_childGroup object manager
#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/Multi.h>
#include <Gruntz/Attract.h> // g_attractStateCount (attract-title-index divisor)

#include <Gruntz/GruntzMgr.h>        // CGruntzMgr - the REAL CState::m_4 game mgr
#include <Gruntz/GruntSpawnConfig.h> // CGruntSpawnConfig - CGruntzMgr::m_timer (+0x60; DtorBody)
#include <Gruntz/Dialogs.h> // CMultiStartDlg (stack-constructed by ShowMultiStartDlg @0xb86c0)
#include <Gruntz/LightFxRender.h> // CLightFxRender (the +0x320 attract overlay; teardown Ctor @0xa3360)
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegistry.h> // g_gameReg singleton (0x24556c) canonical view
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSubMgrPages (m_c->m_drawTarget->m_overlayPair->m_surface Fill)
#include <Wap32/EngStr.h>             // THE canonical EngStr_DrawText (0x115440) lean decl
#include <stdio.h>                    // engine sprintf (reloc-masked)
#include <stdlib.h>                   // srand (reloc-masked)
#include <Globals.h>

// ---------------------------------------------------------------------------
// Engine globals the session loop touches (re-declared TU-local with their
// retail .data addresses so the DIR32 operands reloc-mask).
// ---------------------------------------------------------------------------
DATA(0x002455fc)
i32 g_optionsCursor = 0; // decl in Multi.h
// g_lastNow (0x245580 draw clock) comes from <Rez/FrameClock.h>.

// The game-manager singleton + a divisor for the TITLE%d index.
// g_attractStateCount (0x645534) is declared in <Gruntz/Attract.h> (included below).

// The DirectPlay session-name CString global (assigned in StartTitle).
DATA(0x002473d8)
extern CString g_sessionName; // 0x6473d8

// ===========================================================================
// (former NetMgrGame.cpp preamble - the game-side CNetMgr protocol context)
// ===========================================================================
#include <Net/InterfaceObject.h> // the shared DirectPlay group-node class (Find/predicates)
// (`Cdb200` is gone. Its ONE call site below holds a GruntzPlayer* and reads back the very
// +0x08 field the method writes, so 0xdb200 IS GruntzPlayer::SwapChannel - the xref that
// closes Play.cpp's @identity-TODO. Its `M(void*)` was a PHANTOM (?M@Cdb200@@QAEHPAX@Z,
// defined by no obj); the call now binds to the real ?SwapChannel@GruntzPlayer@@QAEHH@Z.)
#include <Gruntz/LeafCue.h>
#include <Bute/SymParser.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/StatusBarMgr.h>
#include <Net/NetMgr.h>

// The DirectPlay create-context singleton (0x648cf4). Homed here from <Net/NetMgr.h>:
// a header cannot carry a DATA() binding, so it had none. Name kept honest - the only
// proven fact is its TYPE (CNetCreateCtx*, +0x74 pinned); its role is unrecovered.
DATA(0x00248cf4)
extern "C" CNetCreateCtx* g_netCreateCtx;
#include <Net/NetPackets.h> // the fixed-layout stat-0x3f9 / stat-0x416 wire structs
#include <rva.h>
#include <string.h> // memset (inlined rep stosl for the version packet)
#include <stdio.h>  // sprintf (the chat-line formatter)
#include <stdlib.h> // atoi (0x11ffb0) / srand (0x11fed0)

#include <Gruntz/GruntzPlayer.h> // OnPlayerLeft derefs the leaving player's slot
#include <Gruntz/GruntzCmdMgr.h> // CNetGameMgr::m_6c command manager (ResetPlayerCommands Dispatch)
#include <Gruntz/SoundCue.h>     // DispatchRecvMsg's chat cue (m_c sound sub-mgr -> "GAME_CHAT")
#include <Bute/SymParser.h>      // the REAL CSymParser (CState::m_8; ResolvePath @0x13c030)
// (The former local CGruntzMgr shadow is gone: m_4's game-mgr methods are now declared
// directly on CNetGameMgr in <Net/NetMgr.h>, so m_4->Method() needs no cross-cast.)
// LoadGameAssetNamespaces (0xf9ea0) is now CState::LoadGameAssetNamespaces; CMulti
// (: CPlay : CState) inherits it and calls it cast-free (the CAssetLoader this-view
// AUTHENTIC-FLOOR NOTE (cast audit): the casts remaining in this TU are intentional -
//   (The m_4 game-mgr / m_5c chat-log helpers are now real methods on CNetGameMgr /
//     CNetChatLog; CSymParser is the real <Bute/SymParser.h> class (m_8 typed);
//     the CNetConnectThis shell and the CNetConnectSlotView PMF vtable-slot view are
//     DISSOLVED - the four connect-driver dispatches are the real CState/CPlay-chain
//     virtuals, see LoadGameAssetNamespaces, the slot-1 driver.)
//   * (char*)static_cast<const char*>(aCString): MFC CString -> LPCTSTR (operator) -> char* to feed a
//     char*-taking engine API; both casts are required.
//   * (IDirectPlay4Z*)m_releaseIface etc.: DirectPlay COM downcast off the abstract
//     INetReleasable slot; DirectX interfaces are foreign SDK types.
//   * manual vtable stamps (*(void**)this=&g_netMgrVtbl) and the +0x1c/+0x38/+0x54 CObList
//     offset dtors are documented terminal @early-stop walls (vtbl un-catalogued / member-
//     by-value modeling deferred to the final sweep).


// Owner-TU global DEFINITIONS (this multiplayer TU owns them). Each is referenced
// only from this TU (data_home: private); the single reference extern lives in
// <Net/NetMgr.h> / <Globals.h>. DATA() rides the DEFINITION (reloc-masked DIR32 at
// the use sites). Ordered ascending by RVA per section (.data then .bss).
// g_localVersion was extern-only tree-wide (here + <Net/NetMgr.h>), while CRezSync::Init
// wrote the SAME address under a private `void* g_60fa70` - so one variable had two
// symbols and no storage. It is the .bute [General] "RezSync" dword (RezSync::Init loads
// it) that Net compares against the host packet's m_1c. Defined here, beside its
// adjacent sibling g_remoteVersion.
DATA(0x0020fa70)
extern "C" {
    // = 1, read straight out of the retail image: [0x20fa70,+4) holds 01 00 00 00,
    // file-backed in .data's raw span (which ends at 0x229400), matching its
    // adjacent sibling g_remoteVersion. It was `= 0` here, which cl folds into
    // .bss - so the base emitted no bytes where retail has an initializer.
    i32 g_localVersion = 1; // 0x20fa70  local protocol/rez-sync version word
}
DATA(0x0020fa74)
i32 g_remoteVersion = 1; // 0x20fa74  protocol version word (local build = 1)
DATA(0x0020fab8)
i32 g_dplayAppGuid[4] = {
    static_cast<i32>(0xf41cf640),
    0x11d191b2,
    static_cast<i32>(0x6000fc8d),
    0x1ea89f97
}; // 0x20fab8  DirectPlay app GUID / net-bind template
DATA(0x00211d88)
i32 g_dropPlayerId = -999; // 0x211d88  saved dropped-player id (sentinel -999)
DATA(0x00211ec4)
char s_GameKey[] = "GAME_KEY"; // 0x211ec4  registry value-name literal
DATA(0x00246378)
u8 g_chanStat423_flag; // 0x246378
DATA(0x0024637c)
i32 g_chanStat423_id; // 0x24637c
DATA(0x00246380)
i32 g_chanStat423_val; // 0x246380
DATA(0x00246fd8)
u8 g_chanStat422_flag; // 0x246fd8
DATA(0x00246fdc)
i32 g_chanStat422_id; // 0x246fdc
DATA(0x00246fe0)
i32 g_chanStat422_val; // 0x246fe0
DATA(0x00248d04)
i32 g_pauseGuard; // 0x248d04  OnMultiPause reentrancy guard
DATA(0x00248d08)
i32 g_optionzGuard; // 0x248d08  OnMultiOptionz reentrancy guard
DATA(0x00248d0c)
i32 g_syncToggle; // 0x248d0c  FrameSyncWait alternating low-bit flag
DATA(0x00248d10)
i32 g_dropGuard; // 0x248d10  OnDropPlayer reentrancy guard
DATA(0x00248d14)
u32 g_ackThrottleDeadline; // 0x248d14  drop-throttle deadline

// Non-worklist siblings referenced here (shared / homed elsewhere): kept as plain
// extern refs carrying their DATA fallback (no private definition of them here).
// DEFINED HERE (owner TU; only LobbyDialogs also reads it). This TU used to declare the
// SAME cell twice - as the extern-"C" g_sharedFlag here and again as a C++ `g_dlgResultSink`
// further down (Globals.cpp/Globals.h carried that second name too) - so 0x248ce0 had two
// symbols and no storage. It is the modal chat-sink handle (ShowChatLine consumes it, and
// every modal exit path clears it). .bss, zero-init.
DATA(0x00248ce0)
extern "C" {
    i32 g_sharedFlag = 0; // 0x248ce0
}

// MultiDispatch outcome codes the message handlers switch on (engine command
// dispatcher result space; names reconstructed from the branches, values
// load-bearing). Pause/OutOfSync forward the resync WM_COMMAND on DISPATCH_RESYNC;
// DropPlayer resets buffers on DISPATCH_RESET, resets+aborts on DISPATCH_ABORT,
// and reports/broadcasts the leaving player on DISPATCH_PLAYERLEFT.
enum {
    DISPATCH_RESYNC = 0x4cc,     // post the resync WM_COMMAND
    DISPATCH_RESET = 0x4cd,      // reset command buffers only
    DISPATCH_ABORT = 0x4ce,      // reset buffers + post the abort WM_COMMAND
    DISPATCH_PLAYERLEFT = 0x4ea, // report + broadcast the leaving player
};

// Engine stat-dispatcher ids the Net cluster ships (names reconstructed from
// use; values load-bearing).
enum {
    STAT_DROP_ANNOUNCE = 0x3e8,    // RecordDropPlayer2: announce a pending drop (sent twice)
    STAT_CHAT = 0x3f0,             // BroadcastChatLine: a chat line
    STAT_CHANNEL_TABLE = 0x3f8,    // BroadcastChannelTable: the whole four-channel table
    STAT_PLAYER_JOINED = 0x3f9,    // CreateLocalPlayer: local player join announce
    STAT_CHANNEL_ONE = 0x3fa,      // BroadcastOneChannel: a single channel descriptor
    STAT_CHANNEL_LEFT = 0x3fb,     // DropChannelPlayer: a channel's player has left
    STAT_PAUSE = 0x407,            // OnPauseChannel: announce a pause
    STAT_PLAYERLEFT = 0x410,       // broadcast: a player has left
    STAT_PLAYERLEFT_LOCAL = 0x411, // report: the local view of the leaving player
    STAT_CONNECTING = 0x415,       // announce: connection attempt in progress
    STAT_CONFIG = 0x416,           // SaveConfig: the command-timing config blob
    STAT_VERSIONPACKET = 0x417,    // the version-announce packet stat id
    STAT_VERSIONMISMATCH = 0x418,  // announce: host/client version mismatch
    STAT_ACKLATENCY = 0x421,       // report: current worst ack latency
};

// (The DirectPlay DLL imports DirectPlayCreate/DirectPlayEnumerate + the
// EnumProviderCb service-provider enum callback moved with the engine CNetMgr to
// src/Net/NetMgr.cpp; they are not referenced by this game-side half.)

// SetupServices' referents (all reloc-masked). g_hostServicesMode selects the
// host-vs-join branch; g_serviceId is the selected service id (0x3e7 == "none").
extern "C" i32 g_hostServicesMode; // 0x648cf0
extern "C" i32 g_serviceId;        // 0x611d8c
// The multiplayer-command dispatch fired with a services callback (0xbc250,
// __thiscall) + that callback (address-taken -> reloc-masked).
extern "C" void ServicesDispatchCb(); // 0x401a19

// The engine config store reached through m_4->+0x38: writes the selected service /
// player-name / game-name into the registry section. Two __thiscall setters
// (0x139460 / 0x1393b0), external/no-body so the `call rel32` reloc-masks.

// The DirectPlay service-provider node (group-list payload) is the shared
// InterfaceObject class (its five GUID predicates select the connection class -
// IPX/TcpIp/Modem/Serial/...; external __thiscall, reloc-masked). Used by Find +
// DetectConnectionConfig below - now the real class, not a per-TU method-only shadow.

// JoinAndRegisterChannel's referents. Two cdecl config-string builders (0xf9280
// seeds the section, 0xf93b0 appends a "key=value"), two engine CNetMgr* globals
// (the group-enum target / the connect-report target), and the channel-table name
// getter reached through m_4->+0x150. All reloc-masked (external/no-body).
extern "C" i32 Cfg_SetSection(char* buf, const char* fmt, i32 arg);   // 0xf9280
extern "C" i32 Cfg_AppendKeyVal(char* buf, const char* key, i32 val); // 0xf93b0
extern "C" CNetMgr* g_groupEnumMgr;                                   // 0x648cf4
extern "C" CMulti* g_connectRptMgr;                                   // 0x648cf8
// The channel-table base at CNetGameMgr+0x150 is m_4->m_channels[0] (CNetChannel);
// JoinAndRegisterChannel seeds its name CString (+0x4) / id (m_8) directly.

// OnJoinConfirm's referents: the game's cached GetDlgItem import pointer, the
// key=value config parser (0xf9160) + its int parse (0x11ffb0). All reloc-masked.
extern "C" i32 Cfg_GetKey(char* out, const char* src, const char* key); // 0xf9160

// NetSetupDlgProc's referents (the multiplayer host/join setup dialog). The engine
// caches the USER32 imports as function-pointer globals + runs a shared base dialog
// proc first; the settings singleton supplies the Player_Name/Game_Name defaults.
extern "C" HWND g_setupDlgHwnd;                                    // 0x64557c
extern "C" i32 BaseDlgProc(HWND, u32 msg, u32 wParam, i32 lParam); // 0x1192d0
extern "C" i32(WINAPI* g_pEndDialog)(HWND, i32);                   // 0x6c44ac
extern "C" u32(WINAPI* g_pGetDlgItemTextA)(HWND, i32, char*, i32); // 0x6c448c
extern "C" i32(WINAPI* g_pMessageBeep)(u32);                       // 0x6c4534

// The MULTI_JOIN wait dialog's cached player listbox HWND (GetDlgItem(hDlg,0x3fc)).
DATA(0x00248d00)
extern "C" HWND g_netPlayerListHwnd; // 0x648d00
// 0xb8af0: 1-byte no-op player-row refresh helper (cdecl(hDlg,hList)); rel32-masked call.
extern "C" void RefreshPlayerRow(HWND hDlg, HWND hList); // 0xb8af0
// The DirectPlay player-enumeration list + its listbox filler (both defined later in
// this TU at 0xb89e0); forward-declared so MultiJoinDlgProc (0xb8020) can call them.
void FillPlayerList(HWND hList, CNetMgr* sess); // 0x0b89e0  (walks CNetMgr's +0x38 player list)

// (CNetJoinPacket moved to <Net/NetPackets.h> - a fully-known wire struct has no
// business being DEFINED in a .cpp.)

#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages (CMulti::Open m_c->m_drawTarget)
// The cached timeGetTime import fn-ptr (0x6c4650);
// pinned in a callee-saved reg by CMulti::Vslot09.
// The DirectPlay application GUID (DAT_0060fab8) lives at 0x20fab8 as g_dplayAppGuid
// (i32[4], bound in Globals.cpp); the Open path casts it to GUID for InitFromProvider.
DATA(0x00248cf0)
i32 g_isHost_648cf0;

//  CMulti::Open @0xb77a0, and its +0xc holder is the inherited CState::m_c
//  (CDDrawSurfaceMgr), +0x524 the real CNetMgr via Peer().)
// The menu-select event the handler is handed IS the received control message
// (MenuSelectEvent, now in <Net/NetMgr.h> - xref-proven the CNetCtrlMsg passed by
// HandleControlMsg @0xba1ed).

// The session/player manager at CNetMgr+0x524 IS the shared CNetMgr (<Net/NetMgr.h>),
// reached by casting m_peer each use (LoadMenuSelectSprite); no separate view.

// The "ready options" count is CNetGameMgr::CountActiveChannels @0x492e30 (via the
// 0x38cd ILT thunk; __thiscall ret 4) - the SAME method the channel cluster gates
// on.
// The custom-level verify-vote stat ids Poll ships (values load-bearing).
enum {
    STAT_VERIFY_REQUEST = 0x41c,  // guest -> host: request the level-verify vote
    STAT_VERIFY_AGREE = 0x41d,    // host: all players agree on the level
    STAT_VERIFY_DISAGREE = 0x41e, // host: the players disagree on the level
};

// (g_gameReg - the 0x64556c singleton - is declared once above as the
// CNetGameMgr*; the Wait views cast it per use.)

// The frame-clock base stamp WaitForOtherPlayers republishes on exit (0x648ce8).
extern "C" i32 g_scoreTimeBase; // 0x648ce8

// (EngStr_DrawText comes from <Wap32/EngStr.h> - the ONE canonical lean decl; the
// former TU-local (void*, CString*, RECT*) spelling mangled to a symbol the
// definition never emits, leaving this unit's call reloc UNBOUND.)

//   * WaitDWordArr (this+0x604, "SetSize @0x1b4bad / SetAtGrow @0x1b4d7c") IS the MFC
//     CDWordArray it always said it was. Its two declared-only methods mangled as
//     ?SetSize@WaitDWordArr@@... - PHANTOMS no obj and no .LIB defines; the real
//     CDWordArray binds them out of NAFXCW, and its inline GetSize() is the same single
//     load the fake +0x8 field was.
//   * WaitLogic (this->m_4, "+0x48 ambient sound sub-mgr") IS CNetGameMgr - m_4's real
//     type - whose +0x48 is now the typed m_sound member (the same CGruntzSoundZ* slot
//     <Gruntz/GruntzMgr.h> calls CGruntzMgr::m_sound; CNetGameMgr IS *g_gameReg).
//   * WaitSettings (g_gameReg) IS CGameRegistry, already the declared type of the
//     singleton in this TU: m_14 / m_30 / m_modeW / m_modeH are its own members, so the
//     view was casting the class out of its own singleton.
// (CCueEmitter + CNetCueRec are gone too - see the two call sites. CNetCueRec was a
// CONFLATION of two unrelated classes at one name: at ShowMultiStartDlg's first use it is
// the GruntzPlayer that FindOptionsSlot returns (+0x08 / +0x20 = m_008 / m_020), and at
// its second it is the LeafCue a CMapStringToOb::Lookup hands back (+0x10 DSoundCloneInst* /
// +0x14 clock / +0x18 interval - literally LeafCue's field set, which the OTHER cue site
// in this very file already used the canonical LeafCue for). CCueEmitter was LeafCue::m_10,
// i.e. DSoundCloneInst, whose ConfigureItem @0x1360d0 the comment already named.)
// The embedded registry/bute object at (m_c->m_soundRegistry + 0x10) is the REAL CSndHost
// (<Gruntz/SoundCue.h>): its +0x10 IS that class's name->cue map and its +0x30 the
// m_emitGate. The two shells that stood here (CNetCfg / CNetCfgSub) were CState::m_c
// (the canonical CDDrawSurfaceMgr) and its m_28.
//
// That map is a ::CMapStringToPtr, NOT a CMapStringToOb - every call from a CSndHost
// enters 0x1b8438, and `mfc_class 0x1b8438` names its band [0x1b8247, 0x1b85b1) (ctor
// stamps ??_7CMapStringToPtr@@6B@ @0x1eb014). CMapStringToOb is the SEPARATE band
// [0x1b7e17, 0x1b8247) whose Lookup is 0x1b8008. There is NO COMDAT fold (MSVC5 has no
// /OPT:ICF) - two classes, identical code, which is why every FID row there is AMBIG.
// SoundCue.h declared CMapStringToOb while its own comment cited 0x1b8438: a WRONG-CLASS
// binding (mfc_class --audit bucket A/B) that objdiff cannot see because it reloc-masks.
// Fixed at the member (SoundCue.h), so the out-param is a void*& now.
// cdecl ILT-thunk helper (ActiveWait is declared with the CMulti externs above).
void NetCueReset_3bbb(i32 a, i32 b); // 0x3bbb

// CNetMgrLite was CMulti itself: every field it declared is a canonical member -
// +0x04 == CState::m_4 (CGruntzMgr), +0x0c == CState::m_c, +0x528 == m_isHost,
// +0x538 == m_538, +0x5c0 == m_hostIndex - and its three "self-call" thunks are
// already-named CMulti methods: 0x1d70 -> BroadcastChannelTable (0xba810), 0x2e82 ->
// SendStatFlag (0xb9240), 0x386e -> ApplyCmdDelayDefaults (0xb85a0).
// (g_killCueClock is declared with the CMulti PumpA externs above.)
// The engine 3-arg keyed string-format helper at 0x0f9160 (__cdecl): formats a
// value for `key` into `out` and returns nonzero when it produced a string (else
// the caller falls back to the raw source). External (reloc-masked).
extern "C" i32 NetFormatKeyed(char* out, void* src, const char* key);

// (ShowCursor comes from the real USER32 import; 0x6c44c4 is its IAT slot, not a global.)

// (The ex "RegistryFind" extern @0x13c030 is GONE - retail's call is a plain
// __thiscall CSymParser::ResolvePath on m_8 (`mov ecx,[this+8]; push key`),
// spelled m_8->ResolvePath(...) like the 100%-exact sibling state loaders.)

// The player record OpenPlayer returns (stashed in m_netGate->m_player); its
// group-name accessor is read in StartTitle. 0x004b76a0. (The net-bind entry
// points Bind/Activate/OpenPlayer are now methods of CMultiReportGate in CMulti.h;
// the opened-player class CMultiPlayer{GroupName} now lives in <Gruntz/Multi.h>.)

// The "sub-window" at m_c->+0x20 is the REAL SoundStream (<Dsndmgr/SoundStream.h>,
// `class SoundStream : public SoundDevice`): its two per-frame ticks are
// SoundDevice::PurgeVoiceList (0x136e20) and SoundStream::TickSubManagers (0x137ac0),
// BOTH already 100%% EXACT in the tree. CDDrawSurfaceMgr::m_frameProfiler now
// carries that type, so the calls are cast-free.
// Tick's present-finish wait (cdecl free fn). 0x0013dfe0
extern void ActiveWait(i32 phase);

// The render-sub object reached via m_c->m_level->m_5c is the level's MAIN PLANE, and
// its "SubTick" (0x163300) is CPlaneRender::CenterScrollA - the same class + rva
// GameLevel.cpp already drives on m_mainPlane.

// ---------------------------------------------------------------------------
// The CState sub-object dtor-view the most-derived ~CMulti walks: a virtual dtor
// whose auto vptr-restore stamps ??_7CState@@6B@ (0x5ea21c, name-matching
// config/vtable_names.csv) then runs the CState base teardown (CState::ReleaseResources). ~CMulti
// keeps the explicit member teardown in retail order. (The CPlay sub-object
// teardown is CPlay::ReleaseResources, the slot-2 body @0xc8700 the slot-2
// override chains; CMulti : public CPlay is modeled for real now.)
// (~CMulti itself already auto-stamps ??_7CMulti at dtor entry via CMulti's virtual dtor.)

// ---------------------------------------------------------------------------
// The DirectPlay error globals (shared with CNetMgr::ReportError; same .data
// addresses, re-declared TU-local so the DIR32 operands reloc-mask).
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// External engine helpers CMulti drives (reloc-masked rel32 calls). The names
// are placeholders; only the call shape (args / cleanup) is load-bearing.
// ---------------------------------------------------------------------------

// (The 0x0b9290 stat writer is CMulti::SendNetStat - a __thiscall member, declared
// in Multi.h. The
// ReleaseResources (ex "Teardown") call site relies on entry ecx still holding `this` - retail emits no
// mov ecx before the call - which the member spelling reproduces.)

// The engine heap free (CLobbyObjA/B teardown above pairs with it).
// The engine's global free (0x1b9b82) IS MFC's ::operator delete (??3@YAXPAX@Z,
// library-labelled); call it directly so the reloc binds (no fake RezFree view).

// (CMultiStateBase is gone - a DEAD view with zero uses left. Its "base-cleanup
// @0x403f53" is the ILT thunk 0x3f53 -> 0x0fa150 == ?ReleaseResources@CState@@UAEXXZ,
// the canonical <Gruntz/GameModeBase.h> method CState's inline dtor already calls.)

// MFC member sub-object destructors (out-of-line NAFXCW; reloc-masked). Declared
// as the exact thiscall shapes so clang emits the right `mov ecx; call` bytes;
// the real CString/CByteArray dtors below are pulled from <Mfc.h>.

// The MULTI_JOIN dialog handler whose address is pushed into RunErrorDialog. The
// `push &MultiJoinHandler` reloc targets the ILT jmp-thunk (0x222f), not the body
// (that body is Gap_0b8020 @0xb8020); bind the thunk rva to the referenced symbol so
// the delinked datum name pairs (R66/GruntzApp _ErrorDialogProcThunk idiom).
// @data-symbol: ?MultiJoinHandler@@YAXXZ 0x0000222f
extern void MultiJoinHandler(); // thunk 0x222f -> body 0xb8020 (Gap_0b8020)

// The four On*-handler callbacks (declared in <Net/NetMgr.h>, address-taken in the
// OnMulti* handlers below) likewise push their ILT jmp-thunks; bind each thunk rva to
// the referenced symbol (@data-symbol is text-scanned from this .cpp, so it lives here).
// @data-symbol: _MultiPauseCallback 0x0000113b
// @data-symbol: _MultiOptionzCallback 0x000027fc
// @data-symbol: _MultiOutOfSyncCallback 0x0000301c
// @data-symbol: _MultiDropPlayerCallback 0x00003387

// (0x788d0 ?PositionUpdate@CSnd788d0 is merged into src/Gruntz/TriggerMgr.cpp
// per dossier 10b - embedded in the [0x77f80..0x7d7ca] one-TU interval.)

// ===========================================================================
// CMulti::~CMulti  @ 0x08d270  - the most-derived /GX dtor. Runs the slot-2
// ReleaseResources (ex "Teardown") first, then tears the CString/CByteArray run while stamping the CMulti ->
// CPlay -> CState vtables in turn over the sub-objects.
// ===========================================================================
// @early-stop
// EH-dtor wall (docs/patterns/eh-dtor-needs-base-subobject.md): the body is the
// correct, complete reconstruction - ReleaseResources(), then the member CString/CByteArray
// teardown run in retail order while the CMulti->CPlay->CState vtable stamps and
// the CState::ReleaseResources tail land at the right points. Retail emits a FLAT 0x124
// dtor (the CByteArray[4] at +0x3a4 torn via a single ??_M vector-dtor call, light
// register use), while our /GX lowering unrolls the array loop, saves ebx/ebp/edi,
// and splits the per-member cleanup into trailing EH unwind funclets - so the main
// body diverges in instruction selection + register allocation despite matching
// logic. Plus the /GX prologue reads fs:0 before push -1 vs our push-first order.
// Documented EH-state-machine wall; deferred to the final sweep.
RVA(0x0008d270, 0x124)
CMulti::~CMulti() {
    // cl's implicit vptr store (??_7CMulti) stamps here at dtor entry (CMulti's own
    // virtual dtor); no manual stamp is needed at the most-derived level.
    CMulti::ReleaseResources(); // 0xb6110 (own slot-2 override, ex "Teardown";
                                // in-dtor static bind -> direct rel32)
    // CMulti sub-object teardown (high block).
    m_604.~CDWordArray();
    m_5b8.~CString();
    m_5b4.~CString();
    m_hostName.~CString();
    m_groupName.~CString();
    m_598.~CString();
    // CPlay/CState sub-objects (m_488, m_cueText, m_3a4[], m_startMarkers, m_1b4, ...)
    // are torn down by the compiler-chained ~CPlay -> ~CState base destructors now that
    // CMulti : CPlay. No manual base teardown here.
}

// InitStr6473d8 @0x0b5380 - the dynamic initializer that default-constructs the global
// CString g_sessionName in place (explicit-ctor-call tail-jmp).
RVA(0x000b5380, 0xa)
void InitStr6473d8() {
    g_sessionName.CString::CString();
}

// -------------------------------------------------------------------------
// 0x0b5400. Re-run
// the ctor of the static CFileIO global at 0x646778 in place via the explicit-
// ctor-call tail-jmp (its canonical DATA pin lives in src/Io/FileStream.cpp).
extern CFileIO g_obj646778;
RVA(0x000b5400, 0xa)
void ConstructFileIOGlobal() {
    g_obj646778.CFile::CFile();
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs.
// =========================================================================
// CMulti::LoadGameAssetNamespaces  (0xb5460, slot-1 override, ex "SetupMultiplayerSession";
// __thiscall, /GX 18-EH-state) - the multiplayer
// connect/init DRIVER, reconstructed LEAF-FIRST. It operator-new's four objects
// (the 0x8c peer CNetMgr + 3 CObLists, the 0x1c interface object, the 0x630
// CStatusBarMgr session, the 0x78 command manager); each `new`+ctor below is a
// file-local view whose members drive the exact /GX EH-state chain (peer 1..3,
// level-path CStrings 4..6, session 7..0xa, cmd-mgr 0xb..0x12).
//
// CLASS-CONFLATION NOTE: the driver's real owner is the big connect-manager whose
// fields (0x40..0x60c) only partially overlap the header's consolidated CNetMgr
// view (e.g. this+0x8 is a CSymParser*, not the header's CString m_8). The fields
// it writes are almost all un-named padding, so this body uses deliberate offset
// access (TF/MF macros) - the offset is the load-bearing fact, not the name.
// =========================================================================

// (g_lastNow is in <Rez/FrameClock.h>; g_frameDelta/g_frameTime are declared in the CMulti preamble above.)
extern "C" void ChannelSlots_InitAll(); // 0x2da1 (thunk) - no `this` (stale-ecx callee)

// The four connect-driver vtable dispatches (+0x08 / +0x74 / +0x78 / +0x90 off the
// `this` vptr) are the REAL CState/CPlay-chain virtual slots - CMulti is polymorphic
// over the full chain now, so the ex-CNetConnectSlotView PMF view is DISSOLVED
// (2026-07-16). Slot identities read from retail ??_7CMulti @0x1e9fe4 (RTTI
// .?AVCMulti@@ : CPlay : CState, 43 slots):
//   +0x08 slot  2 = ReleaseResources (ex "Abort"; CMulti overrides it, ILT 0x2ef5)
//   +0x74 slot 29 = LoadImageBanks   (ex "OnStart"; inherited CPlay, ILT 0x1a41 -> 0x0cffe0)
//   +0x78 slot 30 = LoadByMode       (ex "OnConnect"; CMulti overrides it, ILT 0x449e)
//   +0x90 slot 36 = Vslot24          (ex "OnReady"; inherited CPlay, ILT 0x1d9d)
// A plain unqualified virtual call on `this` lowers to the identical
// `mov reg,[this]; mov ecx,this; call [reg+off]` dispatch the PMF loads emitted.

// The external `this`-methods the driver calls (all resolved to real classes;
// the empty CNetConnectThis shell is gone):
//   InitConnect IS CState::LoadGameAssetNamespaces (the slot-1 base default; chained qualified).
//   StartTitle IS CMulti::StartTitle (called cast-free from the slot-1 driver).
//   Open IS CMulti::Open @0xb77a0 (called cast-free; the NetSessionOpener view is gone).
//   ShowMultiStartDlg IS CMulti::ShowMultiStartDlg (called cast-free).
//   LoadCursorSprites IS CPlay::LoadCursorSprites; cast at the call.
// The m_4 game-mgr lobby helpers (ResetClockGlobals/ClearOptionsSlots/
// InitLobbySettings/GetWorldFileName) and the chat-log FreeNodes are declared
// directly on their real classes (CNetGameMgr / CNetChatLog in NetMgr.h).
//
// CSymParser is the REAL <Bute/SymParser.h> class now (included above): m_8 is the
// typed CState::m_8 CSymParser*, so ResolvePath ("STATEZ_MULTI") is a plain member
// call. (The old "g_emptyString C2373 collision" that blocked the include is dead -
// the shared decl was unified into <EmptyString.h>, which SymParser.h itself pulls.)
// (1) the 0x8c-byte peer object: the REAL CNetMgr (netmgr-vs-cmulti split DONE). It
// is the small DirectPlay wrapper (RTTI CNetMgr : CObject, ??_7 @0x5ea42c/0x1ea42c,
// ??1 @0xb6000) CMulti holds at +0x524. `new CNetMgr()` reproduces the inlined ctor
// at 0xb560e byte-for-byte: cl stamps the CObject base vptr (0x5e8cb4), runs the
// three by-value CObList member ctors (m_groups/m_players/m_sessions, nBlockSize 10),
// stamps the derived CNetMgr vptr (0x5ea42c), then the inline ctor body zeroes
// +0x14/+0x18. CNetPeer is realized in <Net/NetMgr.h> at its true 0x8c size with its
// real base and cl-emitted vtable.

// (2) the 0x1c interface object IS a CChatBoxOwner (<Gruntz/ChatBoxOwner.h>):
// same size, same seven fields in the same store order (retail inlines its
// header-inline ctor at 0xb583d), and every method the site runs on it
// (Attach/Configure) is a CChatBoxOwner method. The CNetIface view is gone.
//
// (3) the 0x630 session IS the canonical CStatusBarMgr (<Gruntz/StatusBarMgr.h>,
// SIZE 0x630): the 8 CPtrLists @+0x2c are m_tabLists (vector-ctor 0x11f5a0), the
// +0x530 array is m_ptrPool, and the two non-default stamps are m_barFrameGate
// (+0x614 = 0x1e0) and m_544 (= 1). Retail inlines the (header-inline) ctor's
// ~400-byte scalar init - that init is NOT yet recovered (only the two non-zero
// stamps are), so the stamps live at the site. The failure path runs Teardown +
// the member dtors (the view's ~CNetSess). The CNetSess view is gone.

// (4) the 0x78 command manager: 4 CPtrLists + a flag at +0x74. The dtor runs a base
// cleanup (0x2207) then the 4 members reverse-destruct (states 0xf..0x12).
// @early-stop
// ~71% (0%->71.2%): a COMPLETE, correct reconstruction - the full 18-EH-state connect
// sequence, all 4 object constructions, and every call/control-flow arm are byte-
// structurally present and verified against retail with llvm-objdump -dr (the peer
// CObList ctors, the dialog flow + rep-stos, the vtable slot PMF dispatches, the
// level-path operator+ "custom\\"+GetConfigNameB [mangled PBDABV0 confirms the arg
// order], the iface/session/cmd-mgr new+teardown, and the whole tail all match). Three
// documented walls cap the byte-match:
//  1. ZERO-REGISTER-PINNING (dominant, docs/patterns/zero-register-pinning.md): retail
//     pins {this,0,1} in {ebx,ebp,esi}; our cl picks {esi,ebx,ebp} - a proven non-
//     steerable coin-flip that permutes the reg operand of ~every field store.
//  2. PEER FINAL-VPTR/EH-STATE residual: the peer is now `CNetPeer : public
//     CObject`, so cl emits its base-phase vptr stamp (reloc-masks 0x5e8cb4) at
//     ctor entry and drives the 3 CObList /GX new-cleanup states itself. Only the
//     FINAL stamp 0x5ea42c stays manual (it is CNetMgr's own, un-catalogued vtable
//     that cl cannot re-emit here). A residual /GX state-numbering delta remains
//     around that manual final stamp until CNetMgr's own vtable is catalogued.
//     ~400-byte scalar ctor init (3 stride-0x18 sub-loops + 3 rep-stos regions) is
//     still unrecovered - retail inlines that (header-inline) ctor here, ours emits
//     only the member ctors + the two site stamps. Likewise the iface/session/cmd-mgr
//     failure paths: retail INLINES the (header-inline) dtors, ours calls the
//     out-of-line ??1s (~CTileTriggerContainer is the 0xc8640 COMDAT of exactly that
//     inline dtor - moving it header-side is the interleaved-COMDAT fix, deferred).
//     71.2 -> 66.6 from these shape deltas; the structure (real classes, real sizes,
//     real teardown order) is now correct per drive-to-0.
RVA(0x000b5460, 0x914)
i32 CMulti::LoadGameAssetNamespaces(i32 a1, i32 a2, i32 a3) {
    // Connect-state fields reached cast-free through the real classes: `this` is a
    // CMulti (its CPlay/CState base carries the 0x2c..0x4b8 connect-state members and
    // CMulti owns 0x520..0x600), and NetGameMgr() is the CState::m_4 game-mgr's network
    // facet (CNetGameMgr, the 0xac/0x110/0x114/0x12c guards). Both were the TF()/MF()
    // offset-access macros; the offsets are now named members (see Multi.h/Play.h/State.h,
    // NetMgr.h).
    g_gameReg->m_134 = 2;
    if (a1 == 0) {
        return 0;
    }
    // Chain the base default (0xf9ea0) - qualified -> direct rel32 (retail ILT 0x43a9).
    if ((CState::LoadGameAssetNamespaces(a1, a2, a3), 0)) {
        return 0;
    }
    g_connectRptMgr = this;

    // --- zero the connect-state field block (disasm order) ---
    m_region0Gate = 0;
    m_region1Gate = 0;
    m_region2Gate = 0;
    m_viewMode = 0;
    m_hudSuppressed = 1;
    m_49c = -1;
    m_snapshotActive = 0;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_530 = 0;
    m_534 = 0;
    m_sessionTerminated = 0;
    m_538 = 0;
    m_5ac = 0;
    m_pollAbort = 0;
    m_568 = 0;
    m_56c = 0;
    m_574 = 0;
    m_40 = 0;
    m_1c0 = 0;
    m_syncGate = 0;
    m_connected = 0;
    m_pumpGuard = 0;
    m_584 = 0;
    m_588 = 0;
    m_570 = 0;
    m_1c4 = 1;
    m_5bc = 0;
    m_hostIndex = 0;
    m_5a4 = 0;
    m_600 = 1;
    m_drainReload = 0;
    m_lightFx = 0;
    m_savedClock = 0;
    m_rngSeed = static_cast<i32>(::timeGetTime());
    m_58c = 0;
    m_594 = 0;

    // m_channelLatency[0..3] + the four g_gameReg slots (+0x37c / +0x380)
    i32* clat = m_channelLatency;
    for (i32 k = 0; k < 0x8e0; k += 0x238) {
        *clat++ = 0;
        i32* slot = (i32*)((char*)g_gameReg + k + 0x37c);
        slot[0] = 0;
        slot[1] = 0;
    }

    NetGameMgr()->m_114 = 0;
    Mgr()->ResetClockGlobals();
    Mgr()->ClearOptionsSlots();
    ChannelSlots_InitAll();

    CNetMgr* peer = new CNetMgr();
    m_netGate = (CMultiReportGate*)peer;
    g_groupEnumMgr = peer;

    NetGameMgr()->m_ac = 1;
    if (Mgr()->InitializeLobbyConnectionSettings() != 0) {
        if (StartTitle() != 0) {
            NetGameMgr()->m_ac = 0;
            ReleaseResources(); // slot 2 (+0x08) virtual dispatch, ex "Abort"
            return 0;
        }
    } else {
        if (Open() != 0) {
            NetGameMgr()->m_ac = 0;
            while (::ShowCursor(0) >= 0) {
            }
            return 0;
        }
    }

    if (m_isHost != 0) {
        m_58c = 1;
    }
    NetGameMgr()->m_ac = 0;
    // rep stos: zero 0x40 dwords from this+0x1d0
    {
        i32* p = (i32*)((char*)this + 0x1d0);
        for (i32 i = 0; i < 0x40; i++) {
            p[i] = 0;
        }
    }
    m_590 = NetGameMgr()->m_110;
    NetGameMgr()->m_110 = 1;
    if (LoadImageBanks() == 0) { // slot 29 (+0x74) virtual dispatch, ex "OnStart"
        return 0;
    }
    Vslot24(); // slot 36 (+0x90) virtual dispatch, ex "OnReady"
    m_2c = static_cast<CResSource*>(m_8->ResolvePath("STATEZ_MULTI"));
    if (m_2c == 0) {
        return 0;
    }
    if (ShowMultiStartDlg() == 0) {
        return 0;
    }
    while (::ShowCursor(0) >= 0) {
    }
    if (CreateSession() == 0) {
        return 0;
    }

    // --- custom-level path ---
    if (m_5b0 != 0) {
        NetGameMgr()->m_12c = 0;
        *(CString*)((char*)NetGameMgr() + 0xc8) = "custom\\" + GetConfigNameB();
    } else {
        NetGameMgr()->m_12c = 1;
        *(CString*)((char*)NetGameMgr() + 0xc8) = GetConfigNameA();
    }
    if (Mgr()->GetWorldFileName().GetLength() == 0) {
        return 0;
    }

    // (2) interface object - a CChatBoxOwner (the CNetIface view is gone)
    CChatBoxOwner* iface = new CChatBoxOwner();
    m_hitTest = iface;
    // CChatBoxOwner::Attach RETURNS i32 (constant 1), and retail TESTS it here - VERIFIED
    // at 0xb5460+0x349: `call 0x3e77; test eax,eax; jne <continue>` with the same
    // Deactivate+RezFree+return-0 teardown CPlay::LoadGameAssetNamespaces has. The Attach `== 0` guard
    // is REAL: Attach returns a failure signal (see ChatBoxOwner.cpp).
    if (iface->Attach(m_c, NetGameMgr()->m_5c) == 0) {
        CChatBoxOwner* io = m_hitTest;
        if (io == 0) {
            return 0;
        }
        io->Deactivate();
        ::operator delete(io);
        m_hitTest = 0;
        return 0;
    }
    m_hitTest->m_10 = 0;
    m_hitTest->Configure(1);

    // (3) session - the 0x630 CStatusBarMgr (the CNetSess view is gone). The two
    // out-of-band stamps that used to sit here (m_barFrameGate = 0x1e0; m_544 = 1) are
    // GONE: they are ctor initialisers, and CStatusBarMgr now has its real inline ctor
    // (<Gruntz/StatusBarMgr.h>), which this `new` expands exactly as retail does.
    CStatusBarMgr* sess = new CStatusBarMgr;
    m_guts = sess;
    if (sess->LoadBattlezItemConfig(m_c) == 0) {
        CStatusBarMgr* so = m_guts;
        if (so == 0) {
            return 0;
        }
        so->Teardown(); // the view's ~CNetSess body; the members then tear down
        delete so;
        m_guts = 0;
        return 0;
    }

    // (4) command manager - the tile-trigger CONTAINER (retail: `push 0x78; call
    // ??2` + four inlined CPtrList(0xa) ctors at +0x00/+0x1c/+0x38/+0x54 +
    // `[+0x74]=0`; size 0x78, not CTileTriggerSwitchLogic's 0x8c).
    CTileTriggerContainer* cmd = new CTileTriggerContainer();
    m_beginMarker = cmd;
    if (cmd->GetFlag74() == 0) {
        CTileTriggerContainer* co = m_beginMarker;
        if (co == 0) {
            return 0;
        }
        delete co;
        m_beginMarker = 0;
        return 0;
    }

    // --- kick off the connect wait + first poll ---
    if (LoadByMode(1, 1) == 0) { // slot 30 (+0x78) virtual dispatch, ex "OnConnect"
        return 0;
    }
    m_pumpGuard = 1;
    m_534 = 0;
    i32 wr = WaitForOtherPlayers();
    m_pumpGuard = 0;
    if (wr == 0) {
        return 0;
    }
    if ((static_cast<CPlay*>(this))->LoadCursorSprites(0, 0) == 0) {
        return 0;
    }
    PollSession();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    NetGameMgr()->m_5c->FreeNodes();
    m_connected = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::~CNetMgr  (0x0b6000, __thiscall ??1) - the managed-list teardown run of
// the destructor. Fully cl-emitted vtable stamps now: CNetMgr is a real polymorphic
// class (own ??_7CNetMgr@@6B@ @0x1ea42c) deriving from CObject, so the compiler
// writes the own vptr at dtor entry (masks 0x1ea42c) AND folds the CObject
// grand-base restamp (masks 0x5e8cb4) at the tail - no manual `*(void**)this = &g_*`
// store. The body runs the session Destroy then the compiler auto-destructs the three
// managed CObLists at +0x54/+0x38/+0x1c (reverse decl order) and restamps the CObject
// base - now that they are real by-value CObList members (m_groups/m_players/m_sessions
// in <Net/NetMgr.h>, netmgr-vs-cmulti split), cl emits the /GX unwind frame with the
// descending EH-state cookies (2/1/0) retail has, so the manual offset-cast dtors are
// gone.
RVA(0x000b6000, 0x6d)
CNetMgr::~CNetMgr() {
    Destroy();
}

// ===========================================================================
// CMulti::ReleaseResources  @ 0x0b6110  (slot-2 override, ex "Teardown") - drains
// the lobby state on teardown: if the
// join gate is fully armed (m_netGate && m_5bc && m_session && m_connected) push the two final
// stat updates, then free the two heap lobby sub-objects (m_session, m_attractOverlay), release
// the report gate object (m_netGate, via its vtable dtor), and hand m_590 back to the
// logic object (m_logic->m_110).
// ===========================================================================
RVA(0x000b6110, 0xc7)
void CMulti::ReleaseResources() {
    if (m_netGate && m_5bc && m_session && m_connected) {
        SendNetStat(0x402, 0x4d2, 1);
        SendStatFlag(0x3ea, 1);
    }
    // The +0x520 session object is destroyed as ~CNetSession (0xb6220, non-virtual) - its
    // real dtor (ResetSync + vector-destroy the 4 CNetCmdSlot slots) - then the engine free.
    CNetSession* p520 = m_session;
    if (p520) {
        delete p520;
        m_session = 0;
    }
    if (m_netGate) {
        delete m_netGate;
        m_netGate = 0;
    }
    // The +0x320 attract overlay is destroyed via CLightFxRender::Ctor (0xa3360) - the
    // engine reuses the field-zeroing "ctor" as the pre-free cleanup - then the engine
    // free. (Was the CLobbyObjA decl-only view; m_lightFx is the i32 gate reused
    // as the CLightFxRender* here.)
    CLightFxRender* p320 = m_lightFx;
    if (p320) {
        p320->Ctor();
        ::operator delete(p320);
        m_lightFx = 0;
    }
    Mgr()->m_isEffectsEnabled = m_590;
    // Chain CPlay's slot-2 body (0xc8700, ex "CPlayDtorBody") - qualified -> direct.
    CPlay::ReleaseResources();
}

// ---------------------------------------------------------------------------
// 0x0b6220: ~CNetSession. Runs the lobby-sync drain (ResetSync @0xbf000) then
// vector-destroys the embedded 4x CNetCmdSlot slot table at +0x20 (per-element dtor
// = ~CNetCmdSlot). The /GX frame + ResetSync call + 'eh vector destructor iterator'
// fall out of the member array (eh-dtor-model-members-as-destructible.md). 100%.
//
// reloc-fidelity note (NOT a defect): the tool reports the DIR32 at +0x2b as MISBOUND
// (ours 0xb62a0, retail 0x1636). It is the same function. +0x2b is the `push
// offset <element dtor>` argument to the MFC 'eh vector destructor iterator'
// (0x11f640); retail's operand is the /INCREMENTAL linker's ILT thunk at 0x401636,
// whose 5 bytes are `e9 65 4c 0b 00` = jmp 0xb62a0 = ??1CNetCmdSlot@@QAE@XZ - exactly
// the symbol we emit. reloc_fidelity chases ILT jmp-thunks for CALL operands but not
// for an address-taken DIR32, so the thunk address reads as a different target. Nothing
// to fix in source: the reference names the right function and links.
RVA(0x000b6220, 0x54)
CNetSession::~CNetSession() {
    ResetSync();
}

// ---------------------------------------------------------------------------
// 0x0b62a0: ~CNetCmdSlot. Runs ResetAll (0xc0bb0) then tears down the +0x20 CObList
// member (m_cmds). The /GX frame + ResetAll call + member ~CPtrList fall out of the
// destructible member (eh-dtor-model-members-as-destructible.md). 100%.
RVA(0x000b62a0, 0x4a)
CNetCmdSlot::~CNetCmdSlot() {
    ResetAll();
}

// CMulti::Vslot09 (0x0b6330): chain the base ResetForMode gate; on success run the
// manager per-frame tick, restore the saved game clock, and re-seed the net drain /
// frame timer block (two timeGetTime samples), then push a net stat when connected.
// @early-stop
// ~98.6%: logic + every instruction byte-faithful. Residual is a single callee-saved
// register-NAMING swap: retail pins the cached ::timeGetTime fn-ptr in ebx and the
// zero in edi; cl picks edi for the fn-ptr and ebx for the zero. Invariant to
// materialization order (tried the tg decl before/after the first zero store) and
// to the permuter - a non-steerable regalloc coin-flip between the two pushed
// callee-saved regs.
RVA(0x000b6330, 0x89)
i32 CMulti::Vslot09(i32 arg) {
    if (CPlay::Vslot09(arg) == 0) { // qualified: the BASE leg, not this override
        return 0;
    }
    m_4->RefreshGameClock(); // 0x8f620 direct (thunk 0x3d23)
    g_frameTime = m_savedClock;
    DWORD(WINAPI * tg)(void) = ::timeGetTime;
    m_drainTimer = 0;
    m_lastTime = tg();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = tg();
    if (m_connected != 0) {
        SendNetStat(0x402, 0x4d2, 1);
    }
    return 1;
}

// The shared HUD message-sprite helper (0x1154b0, __cdecl); reloc-masked.
void ShowHudMessage(
    void* sink,
    CString* text,
    RECT* rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0

// ===========================================================================
// CMulti::FrameSlot28  @ 0x0b63f0  (vtable slot 10 / +0x28) - the HUD status/
// pause overlay.  BYTE-IDENTICAL to CPlay::FrameSlot28 (Play.cpp): both freeze
// m_60, stash the game clock, (m_40) run the notify, clear the present surface,
// then draw the LoadString(0x81a9) banner + tick the status message.
// ===========================================================================
// @early-stop
// /GX EH-frame wall (92.75%): full+correct logic (byte-identical body to CPlay's).
// Same residual as CPlay::FrameSlot28 - SEH scope-table representation + a 4-byte
// /GX RECT+CString frame-packing difference (0x14 vs retail 0x10). See Play.cpp.
RVA(0x000b63f0, 0x11b)
i32 CMulti::FrameSlot28(i32 arg) {
    m_4->m_timer->DtorBody(); // 0x20a4 -> CGruntSpawnConfig::DtorBody @0x11c7b0
    m_savedClock = static_cast<i32>(g_frameTime);
    if (m_40) {
        QuitToMenu();
    }
    if (arg == 9) {
        return 1;
    }
    RECT r;
    m_c->m_drawTarget->m_overlayPair->m_surface->Fill(0);
    CString s;
    s.LoadString(0x81a9);
    r.right = m_4->m_modeW;
    r.bottom = m_4->m_modeH;
    r.left = 0;
    r.top = 0;
    ShowHudMessage(m_c, &s, &r, 0x78, 1, 0xff, 0xff, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1); // 0xfa8f0 CState::RetireScene (inherited via CPlay, cast-free)
    if (m_4 && m_4->m_cmdGrid) {
        m_4->m_cmdGrid->ClearGridRange(5); // 0x41b0 -> CTriggerMgr::ClearGridRange @0x6bd40
    }
    return 1;
}

// FUN_00021bd0 is invoked both on `this` (CMulti) and on m_logic->m_5c; one symbol,
// so it is modeled on a neutral helper and reached by cast at both sites.

// ===========================================================================
// CMulti::LoadByMode  @ 0x0b6580  (slot-30 override, ex "StartSession") - resolve
// the chosen host, reseed the RNG +
// the frame timers, prime the per-slot config table (m_logic->m_150[0..3]), load the
// level, then re-arm everything for the live session. Returns 1 on success.
// ===========================================================================
RVA(0x000b6580, 0x1eb)
i32 CMulti::LoadByMode(i32 mode, i32 unused) {
    g_optionsCursor = 0;
    // FindOptionsSlot's OptionsSlot is defined in GruntzMgr.cpp; only its +0x00 field is
    // read here (g_curPlayer = *host), so the row is taken as i32*.
    i32* host = (i32*)Mgr()->FindOptionsSlot(m_hostIndex);
    if (!host) {
        return 0;
    }
    g_curPlayer = *host;
    srand(m_rngSeed);
    g_activePlayerCount = 0;
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    m_5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = timeGetTime();
    m_574 = 0;
    m_curSlotId = m_session->m_tick - 1;
    // The CPlay slot-30 default body (0xca200) - qualified -> direct rel32 (this
    // method IS the CMulti override of that slot; ex the "LoadLevelByMode" alias).
    if (CPlay::LoadByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        GruntzPlayer* e = &Mgr()->m_options[i];
        if (e == 0) {
            return 0;
        }
        e->m_038.FreeArrays();
        if (e->m_038.LoadConfig((CLevelInfo*)Mgr(), i, e->m_configId) == 0) {
            return 0;
        }
        if (e->m_014 && e->m_liveGate) {
            e->m_038.Clear_02ade0();
        }
    }
    this->RefreshSlotTable();
    srand(m_rngSeed);
    g_frameDelta = 0;
    g_lastNow = 0;
    g_frameTime = 0;
    m_savedClock = 0;
    m_5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = timeGetTime();
    m_curSlotId = m_session->m_tick - 1;
    m_574 = 0;
    Mgr()->m_chatLog->FreeNodes();
    m_session->Reset(); // 0xbf150
    Mgr()->m_timer->DtorBody();
    return 1;
}

// ===========================================================================
// CMulti::Connect  @ 0x0b67f0  - probe the chosen session on m_logic; on failure
// report the netbind error, else run the connect-wait pump (m_pumpGuard reentrancy
// guard) and mark m_connected on success.
// ===========================================================================
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the body
// is the complete, correct reconstruction. Retail pins edi=0 once (xor edi,edi)
// and reuses it for the two arg pushes AND the m_connected/m_534 stores, while our /O2
// emits immediate `push $0` + `mov [esi+N],$0`. Structure + offsets are
// byte-exact; only the constant-0 materialization differs, and no source lever
// (`int z=0;`, reorder) forces the pinning under /O2. Deferred to the final sweep.
RVA(0x000b67f0, 0x74)
i32 CMulti::Connect(i32 mode) {
    m_connected = 0;
    m_534 = 0;
    if (Mgr()->PassClickToPlayState(mode, 0, 0) == 0) {
        Mgr()->ReportError(0x8005, 0x446);
        return 0;
    }
    m_pumpGuard = 1;
    i32 r = PumpA();
    m_pumpGuard = 0;
    if (r == 0) {
        return 0;
    }
    m_connected = 1;
    return 1;
}

// ===========================================================================
// CMulti::Render  @ 0x0b6890  (slot-5 override, ex "Tick") - the per-frame lobby
// pump: redraw, advance the
// frame clock off timeGetTime, step the lobby object, arm the next slot, then
// either keep ticking, flag out-of-sync, or finish + present.
// ===========================================================================
// @early-stop
// regalloc / scheduling wall: the body is the complete, correct reconstruction
// (the redraw vfn call, the timeGetTime delta math into m_frameDelta/m_accumTime, the slot
// arm via m_session->ArmSlot, the Step/Drain clamp, then the busy/stall branch and
// the present tail). MSVC pins ebp=this and threads the timeGetTime import ptr
// through a callee-saved reg across the whole body; our /O2 lowering picks a
// different this/scratch allocation and reorders the m_5dx stores, so the
// instruction stream diverges despite identical logic. >512 B; deferred to the
// final sweep (no NEW idea closes it here).
RVA(0x000b6890, 0x21b)
i32 CMulti::Render() {
    m_drewThisFrame = 0;
    HandleDragMove(0, m_cursorX, m_cursorY); // slot 31 (+0x7c) virtual dispatch
    i32 oldT = m_lastTime;
    i32 t = timeGetTime();
    m_lastTime = t;
    m_frameDelta = t - oldT;
    m_accumTime += (t - oldT);
    i32 newId = m_session->m_tick;
    if (m_curSlotId != newId) {
        m_curSlotId = newId;
        CMultiLogicList* lst = (CMultiLogicList*)Mgr()->m_cmdSubMgr;
        CMultiLogicNode* node;
        if (lst->m_28 == 0) {
            node = 0;
        } else {
            node = lst->RemoveHead();
        }
        if (node) {
            node->m_c = 1;
            i32 v = m_curSlotId + static_cast<i32>(m_5a4) * 2;
            i32 s = v < 0 ? -v : v;
            s &= 0x7f;
            node->m_6 = static_cast<u8>((v < 0 ? -s : s));
        }
        m_session->ArmSlot(node, static_cast<i32>(static_cast<u8>((static_cast<u8>(m_5a4) << 1))));
    }
    i32 dt = m_frameDelta;
    if (static_cast<u32>(dt) >= g_frameDelta) {
        dt = static_cast<i32>(g_frameDelta);
    }
    m_packetsRcvd = m_session->Poll(dt); // 0xbf5a0
    m_packetsSent = 0;
    if (static_cast<u32>(m_frameDelta) < static_cast<u32>(m_drainTimer)) {
        m_drainTimer = m_drainTimer - m_frameDelta;
    } else {
        m_drainTimer = 0;
    }
    if (m_drainTimer == 0) {
        m_packetsSent = m_session->Tick(); // 0xbf9e0
        m_drainTimer = m_drainReload;
    }
    i32 fin = 0;
    if (m_session->Advance() && m_pollAbort == 0) { // 0xc01d0
        fin = 1;
    }
    TickStateMgrs(); // slot 38 (+0x98) virtual dispatch (ex the view's "PostRedraw")
    CLevelPlane* mainPlane = m_c->m_level->m_mainPlane; // the level MAIN plane
    if (mainPlane) {
        mainPlane->CenterScrollA(); // 0x163300
    }
    if (fin == 0) {
        if (m_session->Verify() == 0 && m_574 == 0) { // 0xc04f0
            if (m_isHost != 0) {
                SendStatFlag(0x404, 1);
                OnOutOfSync();
                PumpA();
                m_drainTimer = 0;
                return 1;
            }
            SendStatFlag(0x403, 1);
        }
        PumpA();
        m_drainTimer = 0;
        return 1;
    }
    PumpB();
    DropTimeout();
    SoundStream* win = m_c->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->PurgeVoiceList(now);  // 0x136e20 (SoundDevice base)
        win->TickSubManagers(now); // 0x137ac0
    }
    ActiveWait(2); // FUN_0013dfe0 ActiveWait(2)
    return 1;
}

// ===========================================================================
// CMulti::PumpA  @ 0x0b6b40  - the ambient-timer service: advance the shared
// kill-cue clock, and once the ambient window elapses, format an "AMBIENT%d"
// cue name and register/trigger it; then decay the five ambient stat timers,
// pump the redraw sub-objects, and finish the frame. Reads timeGetTime /
// wsprintfA through the game import slots (reloc-masked). Placeholder helper
// types; only member offsets + the call/branch structure are load-bearing.
// ---------------------------------------------------------------------------

// The global ambient/kill-cue clock state (retail .data addresses -> DIR32
// operands reloc-mask).
// The 0x24558c..0x2455a0 countdown band (g_frameTicks/g_timer32/g_timer100/g_timer200/
// g_timer400/g_timer500) comes from <Rez/FrameClock.h>. NOTE: the pump below proves
// 0x245598 == g_timer200 (seed 0xc8 countdown).

// dispatches m_c->m_childGroup's slot 9 (+0x24, ONE arg = the frame delta) then slot 16
// (+0x40, no arg). The class whose RTTI slot map (vtbl 0x1efdc0, 17 slots) carries
// BOTH - at those exact offsets, with those exact arities - is the one
// CDDrawChildGroup: slot 9 = TickKillCues_159a70 (0x159a70, ret 4) and slot 16 = Slot40
// (0x159f00, ret 0). Both bodies already exist in the tree. No new slot was needed
// (the earlier "CRenderer has no slot 16" blocker read the dispatch off the WRONG
// member - it is the object-manager at +0x08, not the renderer at +0x0c).
// The 3-way conflation is RESOLVED: the ex CSpriteFactory / ex
// CWwdObjMgr twins are merged onto the one CDDrawChildGroup, and m_8 is typed to
// it - the cast is gone.]

// Per-frame receivers (thiscall, out-of-line -> reloc-masked).
// CGruntzMgr::m_sound (+0x48) IS the real CGruntzSoundZ (<Dsndmgr/GruntzSoundZ.h>);
// the +0x1c inner (m_pCurrent) are CGruntzSoundZ's own members.
//
// CTriggerMgr itself (m_cmdGrid's declared type) - its "Step3017" is
// LoadTeleporterGooConfig (ILT 0x3017 -> 0x6eb80), "Fire1398" is
// ScrollToActiveRecord (ILT 0x1398 -> 0x788d0), "Reset2b85" is OverlayRelease
// (ILT 0x2b85 -> 0x79b00), and the +0x230 armed gate is m_armed. The "DC"
// overlay was CStatusBarMgr itself (m_guts's declared type: m_0 == m_position,
// m_mode(+0x10c) == m_activeTab).]

// @early-stop
// large-body regalloc/scheduling wall (~88%). Prologue, the m_594/m_logic->m_c/ready
// early-out, the shared-clock advance and frame size (sub esp,0x44) are byte-exact
// (llvm-objdump -dr); the residual is MSVC5's register/branch choices across the
// five stat-timer decay blocks + the m_view->m_8 vfn-host dispatch on this 670-byte
// body (0-in-ebp reuse, g_frameDelta single-load hoisting), not steerable from source.
RVA(0x000b6b40, 0x29e)
i32 CMulti::PumpA() {
    i32 ready = PumpAReady();
    if (m_594 == 0 && Mgr()->m_frameGate != 0 && ready == 0) {
        PumpAReset();
        return 1;
    }
    g_lastNow += 0x21;
    g_frameTime += 0x21;
    g_frameDelta = 0x21;
    g_killCueClock = g_lastNow;
    g_engineFrameDelta = 0x21;
    if (m_ambientInitDone == 0) {
        if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *(i64*)&m_ambientTimerLo >= *(i64*)&m_ambientInterval) {
            char name[0x40];
            wsprintfA(name, "AMBIENT%d", PumpAIndex());
            if (g_gameReg->m_14 != 0) {
                Mgr()->m_sound->PlayByName(name, 1);
            } else {
                CGruntzSoundInnerZ* p = Mgr()->m_sound->FindBank(name);
                if (p) {
                    Mgr()->m_sound->m_pCurrent = p;
                }
                if (Mgr()->m_sound->m_pCurrent) {
                    Mgr()->m_sound->m_pCurrent->SetLoop(1);
                }
            }
            m_ambientInitDone = 1;
        }
    }
    ((CMultiLogicList*)Mgr()->m_cmdSubMgr)->Step20b3(m_curSlotId % 128);
    m_session->Step2437();
    g_frameTicks++;
    u32 t1 = g_timer32 ? g_timer32 : 0x32;
    if (g_frameDelta < t1) {
        g_timer32 = t1 - g_frameDelta;
    } else {
        g_timer32 = 0;
    }
    u32 t2 = g_timer100 ? g_timer100 : 0x64;
    if (g_frameDelta < t2) {
        g_timer100 = t2 - g_frameDelta;
    } else {
        g_timer100 = 0;
    }
    u32 t3 = g_timer200 ? g_timer200 : 0xc8;
    if (g_frameDelta < t3) {
        g_timer200 = t3 - g_frameDelta;
    } else {
        g_timer200 = 0;
    }
    u32 t4 = g_timer400 ? g_timer400 : 0x190;
    if (g_frameDelta < t4) {
        g_timer400 = t4 - g_frameDelta;
    } else {
        g_timer400 = 0;
    }
    u32 t5 = g_timer500 ? g_timer500 : 0x1f4;
    if (g_frameDelta < t5) {
        g_timer500 = t5 - g_frameDelta;
    } else {
        g_timer500 = 0;
    }
    m_c->m_childGroup->TickKillCues_159a70(g_frameDelta);
    m_c->m_childGroup->CollideBroadcast();
    Mgr()->m_cmdGrid->LoadTeleporterGooConfig(static_cast<i32>(g_frameDelta));
    m_guts->LoadDestructButtonSprite(g_frameDelta);
    SoundStream* win = m_c->m_soundStream;
    if (win) {
        i32 now = timeGetTime();
        win->PurgeVoiceList(now);  // 0x136e20 (SoundDevice base)
        win->TickSubManagers(now); // 0x137ac0
    }
    m_beginMarker->FilterList2((void*)g_frameDelta);
    ((CBrickzGrid*)Mgr()->m_tileGrid)
        ->UpdateDiagonals(reinterpret_cast<i32>(Mgr())); // CBrickzGrid is a view of CGruntzMapMgr (+0x70)
    if (ready == 0) {
        PumpAReset();
    }
    Mgr()->AdvanceOptionsCycle();
    return 1;
}

// ===========================================================================
// CMulti::PumpB  @ 0x0b6e90  - the lobby/attract render pump. A light path when
// the game is idle (m_594==0 && m_logic->m_c!=0) drives just the compositor + the
// primary pane; otherwise the full frame runs: two deadline-gated FX, the
// m_view manager sub-tree (panes m_10/m_14, the +0xc vfn host, the +0x24 chain),
// the ambient overlays (m_fxOverlay/m_2e0/m_attractOverlay) and two int64 clock gates against
// g_frameTime. All callees are out-of-line (reloc-masked); PumpB's members not in
// CMulti.h are reached through dedicated view structs / documented offsets.
// ---------------------------------------------------------------------------

//   PBMgr      == CDDrawSurfaceMgr (<Gruntz/GameRegistry.h>) - it IS CState::m_c,
//                 already typed there; every member lines up (m_4 draw target, m_8
//                 object factory/manager, m_c renderer B, m_24 level/view).
//   PBVfnHost  == CRenderer (<Gruntz/View.h>) - its "Blit34" (+0x34) IS CRenderer::
//                 Present, slot 13, already modeled at that offset with the same
//                 2-arg thiscall shape.
//   PBSub4     == CDDrawSubMgrPages (<Gruntz/ResMgr.h>) - the +0x10/+0x14/+0x18 page triple.
// TWO CONFLATIONS remain visible as casts at the sites below, and they are REAL
// (do not paper over them):
//   (1) CDDrawSubMgrPages types +0x10/+0x14/+0x18 as its own SurfaceA*/SurfaceB* nested
//       pages, while this TU's uses (->m_surface Flip/Fill, the VisitVisible target)
//       are CDDrawSurfacePair* operations. Both readings agree on the OFFSETS and on
//       a +0x2c CDDSurface; they disagree on the page CLASS. Settling that needs the
//       page ctor/new-site (unreconstructed), so the pages keep the pair type here
//       via cast and the disagreement is reported, not guessed away.
//   (2) CDDrawSurfaceMgr::m_24 is typed CGameViewport there but is used as
//       CGameLevel here (VisitVisible / m_mainPlane) - the same +0x24 slot, two
//       names; CDDrawSurfaceMgr's slot map says +0x24 is the level. Same fold.]

// The output sink hung off CGruntzMgr::m_inputState (+0x54; thiscall 2-arg blit).
// CLightFxRender::Resize @0xa3460 / ComputeRect @0xa3820, dispatched on the
// typed m_lightFx below - same receiver, same thunks 0x1fa0/0x14dd.)
// The compositor refresh helper (__cdecl free fn). 0x00002356
extern "C" void PumpBRefresh2356(void* reg, void* fx, i32 flag);

// @early-stop
// large-body regalloc/scheduling wall (~83%). All branch structure is byte-exact
// (the two int64 deadline gates' jl/jg/jb triples, the small/big split, the
// m_attractOverlay render sub-block all align in llvm-objdump -dr base vs target); the
// residual is MSVC5 reordering the push/mov/call scheduling across this 845-byte
// body (prologue reg-save order, arg-eval interleave) plus the else-branch's
// redundant m_90 rc.top store the retail optimizer keeps (rc escaped to SetRect)
// - not steerable from source. Sibling of the PumpA (~88%) wall.
RVA(0x000b6e90, 0x34d)
void CMulti::PumpB() {
    CDDrawSurfaceMgr* mgr = m_c;
    if (m_594 == 0 && Mgr()->m_frameGate != 0) {
        StepInputA();
        mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
        mgr->m_workerList->PruneWorkers(
            mgr->m_drawTarget->m_backPair,
            mgr->m_drawTarget->m_overlayPair
        );
        m_guts->LoadMainStatusBarSprite();
        CDDrawSurfacePair* h = (CDDrawSurfacePair*)mgr->m_drawTarget->m_backPair;
        if (h == 0) {
            return;
        }
        StepGridWalk(g_frameDelta);
        winapi_0d0b30_CopyRect(reinterpret_cast<i32>(h));
        ((CDDrawSurfacePair*)mgr->m_drawTarget->m_frontPair)->m_surface->Flip(0);
        return;
    }
    StepInputA();
    StepC();
    if (m_region0Gate != 0) {
        ((CDDrawSurfacePair*)mgr->m_drawTarget->m_backPair)->m_surface->Fill(0);
        m_guts->Deactivate();
    }
    if (m_worldReady == 0) {
        if (Mgr()->m_cmdGrid->m_armed != 0) {
            Mgr()->m_cmdGrid->ScrollToActiveRecord();
        } else {
            LoadScrollSpeedOptions();
        }
    }
    StepScroll();
    Mgr()->m_inputState->Retune(
        (mgr->m_level->m_mainPlane)->m_snappedX,
        (mgr->m_level->m_mainPlane)->m_snappedY
    );
    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        mgr->m_level->VisitVisible(mgr->m_drawTarget->m_backPair, mgr->m_childGroup);
        mgr->m_workerList->PruneWorkers(
            mgr->m_drawTarget->m_backPair,
            mgr->m_drawTarget->m_overlayPair
        );
    }
    m_guts->LoadMainStatusBarSprite();
    if (m_lightFx != 0) {
        CStatusBarMgr* fx = m_guts;
        if (fx->m_position != 2 && fx->m_activeTab != 5) {
            RECT rc;
            if (fx->m_position == 1) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                i32 cx = g_gameReg->m_modeH;
                i32 cy = g_gameReg->m_modeW;
                rc.top = cx;
                SetRect(&rc, cy - 140, 5, cy - 20, 125);
            }
            m_lightFx->Resize(static_cast<i32>(g_frameDelta), 0);
            m_lightFx->ComputeRect(
                (CDDrawSurfacePair*)mgr->m_drawTarget->m_backPair,
                (LfxRect*)&rc
            );
        }
    }
    Mgr()->m_chatLog->Scroll(g_frameDelta);
    CDDrawSurfacePair* h = (CDDrawSurfacePair*)mgr->m_drawTarget->m_backPair;
    if (h == 0) {
        return;
    }
    m_hitTest->LoadChatBoxSprite(reinterpret_cast<i32>(h));
    DrawDebugStats();
    Mgr()->m_cmdGrid->OverlayRelease();
    StepGridWalk(g_frameDelta);
    winapi_0d0b30_CopyRect(reinterpret_cast<i32>(h));
    if (m_worldReady != 0) {
        h->DrawBox((i32*)&m_hudRect, 0xff);
    }
    ((CDDrawSurfacePair*)mgr->m_drawTarget->m_frontPair)->m_surface->Flip(0);
    PumpBRefresh2356(g_gameReg, m_guts, m_region0Gate);
    if (mgr->m_level->m_mainPlane != 0) {
        (mgr->m_level->m_mainPlane)->CenterScrollB();
    }
    if (m_region0Gate != 0) {
        if (static_cast<i64>(g_frameTime) - *(i64*)&m_region0TimerLo >= *(i64*)&m_region0Interval) {
            OnRegion2(0);
        }
    }
    if (m_region1Gate != 0) {
        if (static_cast<i64>(g_frameTime) - *(i64*)&m_region1TimerLo >= *(i64*)&m_region1Interval) {
            OnRegion1(0);
        }
    }
}

// ===========================================================================
// CMulti::StartTitle  @ 0x0b72c0  - /GX: pick a randomized "TITLE%d" backdrop,
// load it, reset the view + cursor, then bind the DirectPlay host (m_netGate) to the
// resolved descriptor, open the local player, and stash the host/group strings.
// Returns 1 on a fully-bound session.
// ===========================================================================
// @early-stop
// MFC CString temp + /GX EH wall: the body is the complete, correct
// reconstruction (the TITLE%d Format, the RunTitleSeq gate, the view/cursor
// reset, the m_netGate Bind/Activate/OpenPlayer net chain, and the two CString stash
// helpers). Retail interleaves the EH-state stores (the [esp+...]=-1 funclet
// indices) and the inline strlen+rep-movs CString constructions with a register
// allocation our MSVC5 /O2 lowering reorders, and the empty-CString Init differs
// (the documented cstring-empty-init-version-divergence wall). >512 B; logic is
// correct, byte-match deferred to the final sweep.
RVA(0x000b72c0, 0x30b)
i32 CMulti::StartTitle() {
    Mgr()->m_lobbyResult = 0;
    m_588 = 1;
    if (!m_netGate) {
        return 0;
    }
    CResSource* saved = m_2c;
    CResSource* st = static_cast<CResSource*>(m_8->ResolvePath("STATEZ_ATTRACT")); // 0x13c030
    m_2c = st;
    if (!st) {
        return 0;
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString title;
    title.Format("TITLE%d", idx);
    // 0xfa350 CState::RunTitleSeq (inherited; ex the "LoadTitleScreen" alias).
    if (RunTitleSeq(title, 0, 0, 1, 0) == 0) {
        m_2c = saved;
        return 0;
    }
    // m_c->m_drawTarget IS a CDDrawSubMgrPages (its 0x158dc0 leaf is that class's
    // Method_158dc0); retail loads the VALUE at [m_c+4], not its address.
    m_c->m_drawTarget->Method_158dc0();
    void* vobj = *(void**)(*(void**)((char*)m_c + 0x1c));
    (*(void(__stdcall**)(void*))((char*)*(void**)vobj + 0x28))(vobj); // vfn +0x28(vobj)
    m_2c = saved;
    while (::ShowCursor(1) < 0) {
    }
    if (!Mgr()->m_lobby) {
        return 0;
    }
    CMultiLogicDesc* desc = (CMultiLogicDesc*)Mgr()->m_connSettings;
    if (!desc) {
        return 0;
    }
    m_isHost = (desc->m_flags & 2) ? 1 : 0;
    i32 tmpl[4];
    tmpl[0] = g_dplayAppGuid[0];
    tmpl[1] = g_dplayAppGuid[1];
    tmpl[2] = g_dplayAppGuid[2];
    tmpl[3] = g_dplayAppGuid[3];
    if (m_netGate->Bind(tmpl) == 0) {
        return 0;
    }
    m_netGate->Activate();
    CMultiPlayer* player = m_netGate->OpenPlayer(desc->m_8);
    if (player == 0) {
        return 0;
    }
    m_netGate->m_player = player;
    CString hostName(desc->m_c->m_8);
    ClearString5a0(hostName); // clear m_hostName, return the temp
    char* grp = player->GroupName();
    CString grpName(grp);
    ClearString59c(grpName); // clear m_groupName, return the temp
    // 0xbc460 SetupTcpIpConfig (host) / 0xbc750 CreateLocalPlayer (guest) - the
    // real bodies below (ex the "RebindHostAlt"/"RebindHost" alias decls).
    i32 r = m_isHost ? SetupTcpIpConfig() : CreateLocalPlayer();
    return r ? 1 : 0;
}

// ===========================================================================
// CMulti::ClearString59c  @ 0x0b76c0  - assign an empty CString into m_groupName,
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
    m_groupName = CString();
    return s;
}

// ===========================================================================
// CMulti::ClearString5a0  @ 0x0b7730  - assign an empty CString into m_hostName.
// ===========================================================================
// @early-stop
// same MFC-version CString()-empty wall as ClearString59c (see above).
RVA(0x000b7730, 0x4f)
CString& CMulti::ClearString5a0(CString& s) {
    m_hostName = CString();
    return s;
}

// CMulti::Open @0xb77a0 (__thiscall): roll the
// "BACKGND" title fade, init the DDraw sub-mgr pages, build the session services, connect
// via DirectPlay using the app GUID, then host- or join-start per g_isHost. The +0x524
// join gate is the small real CNetMgr (Peer()); Configure/Build/HostStart/JoinStart were
// the fake view's names for RunTitleSeq(CState)/SetupServices/DetectConnectionConfig/
// JoinSession (all real CMulti/CState methods now - the calls bind).
RVA(0x000b77a0, 0xb5)
i32 CMulti::Open() {
    if (!Peer()) {
        return 0;
    }
    RunTitleSeq("BACKGND", 0, 0, 1, 0); // 0xfa350 (CState base)
    m_c->m_drawTarget->Method_158dc0(); // m_c->m_4
    i32 descriptor = SetupServices();   // 0xb78b0
    if (!descriptor) {
        return 0;
    }
    if (!Peer()->InitFromProvider((void*)descriptor, *(const GUID*)g_dplayAppGuid)) {
        return 0;
    }
    if (g_isHost_648cf0) {
        m_isHost = 1;
        if (!DetectConnectionConfig()) { // 0xb82e0
            return 0;
        }
    } else {
        m_isHost = 0;
        if (!JoinSession()) { // 0xb7fe0
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::SetupServices  (__thiscall; ret 0 args; /GX EH frame).
// Enumerates the peer's DirectPlay service providers (m_peer->EnumServiceProviders);
// on failure reports the connect error and returns 0. On success it dispatches the
// host or join services command (per g_hostServicesMode) and, when that succeeds and
// the m_4->+0x38 config store exists, persists the config: the selected service id
// (unless "none" == 0x3e7), the local player name, and - host only - the game name.
// Returns the peer's selected provider (m_groupSel).
// @early-stop
// entropy-tail plateau (~98.2%): logic + both branches + the two scoped CString
// temps under the /GX frame are byte-faithful; the ~2% residual is a scheduling
// nuance in the config-store write block. §2a scoring-tail. Final sweep.
RVA(0x000b78b0, 0x17f)
i32 CMulti::SetupServices() {
    if (Peer()->EnumServiceProviders(0) != 0) {
        ReportConnectFailed(0);
        return 0;
    }

    if (g_hostServicesMode != 0) {
        if (DispatchServices("MULTI_HOSTSERVICES", 0, (void*)&ServicesDispatchCb) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_configStore;
            if (store != 0 && g_serviceId != 0x3e7) {
                store->SetValueDword("Service", g_serviceId);
                {
                    CString name = GetString5a0();
                    store->SetValueString("Player_Name", const_cast<char*>(static_cast<const char*>((name))));
                }
                {
                    CString gameName = GetGameName();
                    store->SetValueString("Game_Name", const_cast<char*>(static_cast<const char*>((gameName))));
                }
            }
        }
    } else {
        if (DispatchServices("MULTI_JOINSERVICES", 0, (void*)&ServicesDispatchCb) != 0) {
            Utils::RegistryHelper* store = NetGameMgr()->m_configStore;
            if (store != 0) {
                if (g_serviceId != 0x3e7) {
                    store->SetValueDword("Service", g_serviceId);
                }
                CString name = GetString5a0();
                store->SetValueString("Player_Name", const_cast<char*>(static_cast<const char*>((name))));
            }
        }
    }
    return Peer()->m_groupSel;
}

// GetString59c (0x0b7a90): return m_groupName by value (CString copy into the
// return slot). Out-of-line (retail emits it standalone; the inline member folded
// into its callers and never emitted).
RVA(0x000b7a90, 0x23)
CString CMulti::GetString59c() {
    return m_groupName;
}

// ---------------------------------------------------------------------------
// NetSetupDlgProc  (Win32 DialogProc, __stdcall; ret 0x10, 4 args).
// The multiplayer host/join service-setup dialog. Runs the shared base proc first;
// on WM_INITDIALOG it fills the service-provider combo (control 0x3fc), selects the
// current service, and seeds the Player_Name/Game_Name edits (0x51b/0x51c) from the
// settings config; on WM_COMMAND it handles Cancel (2 -> EndDialog(0)) and OK (1):
// validates the service name (0x51b, beep on empty), records it (host mode also
// records the game name), latches the selected service id, reads the group
// selection, and closes the dialog (EndDialog(1)).
// @early-stop
// regalloc callee-save-count wall (~77%): the base-proc dispatch, the switch(msg)
// (sub 0x110/je/dec/jne, matching retail), the GetString(key,buf,&maxlen,default)
// config reads (int* maxlen reusing one slot, cfg re-derived per call), both
// Cancel/OK paths and the by-value CString(name) arg-slot construction are all
// reproduced. Residual: retail keeps GetDlgItem's fn-ptr in a 4th callee-saved reg
// (ebp) across INITDIALOG, so retail saves ebx/ebp/esi/edi (sub esp,0x50) while
// our /O2 uses only ebx/esi/edi (sub esp,0x58); the extra reg + 4-byte frame delta
// cascades every stack offset and re-orders the return-0 tail blocks. Final sweep.
RVA(0x000b7b10, 0x27c)
i32 __stdcall NetSetupDlgProc(HWND hDlg, u32 msg, u32 wParam, i32 lParam) {
    g_setupDlgHwnd = hDlg;
    if (BaseDlgProc(hDlg, msg, wParam, lParam) != 0) {
        return 1;
    }

    switch (msg) {
        case 0x110: {
            HWND combo = ::GetDlgItem(hDlg, 0x3fc);
            g_groupEnumMgr->m_groupSel = 0;
            g_groupEnumMgr->PopulateGroupList(combo, 0);
            if (g_serviceId == 0x3e7) {
                ::SendMessageA(combo, 0x186, 0, 0);
            } else if (static_cast<i32>(::SendMessageA(combo, 0x186, g_serviceId, 0)) == -1) {
                ::SendMessageA(combo, 0x186, 0, 0);
            }

            char nameBuf[0xa];
            char gameBuf[0x40];
            i32 cap = 0xa;
            ((Utils::RegistryHelper*)*(void**)((char*)g_gameReg + 0x38))
                ->GetValueString(
                    const_cast<char*>(static_cast<const char*>(("Player_Name"))),
                    nameBuf,
                    (u32*)&cap,
                    "Player"
                );
            cap = 0x40;
            ((Utils::RegistryHelper*)*(void**)((char*)g_gameReg + 0x38))
                ->GetValueString(
                    const_cast<char*>(static_cast<const char*>(("Game_Name"))),
                    gameBuf,
                    (u32*)&cap,
                    "Multiplayer_Gruntz"
                );
            ::SendMessageA(::GetDlgItem(hDlg, 0x51b), 0xc5, 9, 0);
            ::SetDlgItemTextA(hDlg, 0x51b, nameBuf);
            ::SendMessageA(::GetDlgItem(hDlg, 0x51c), 0xc5, 0x3f, 0);
            ::SetDlgItemTextA(hDlg, 0x51c, gameBuf);
            return 1;
        }
        case 0x111:
            break;
        default:
            return 0;
    }

    if (wParam == 2) {
        g_pEndDialog(hDlg, 0);
        return 1;
    }
    if (wParam != 1) {
        return 0;
    }

    char name[0xa];
    g_pGetDlgItemTextA(hDlg, 0x51b, name, 0xa);
    if (name[0] == 0) {
        g_pMessageBeep(0);
        return wParam;
    }
    g_connectRptMgr->SetServiceName(CString(name));

    if (g_hostServicesMode != 0) {
        char gname[0x40];
        g_pGetDlgItemTextA(hDlg, 0x51c, gname, 0x40);
        if (gname[0] == 0) {
            g_pMessageBeep(0);
            return 1;
        }
        g_connectRptMgr->ApplyDynSetting(CString(gname));
    }

    HWND combo = ::GetDlgItem(hDlg, 0x3fc);
    i32 svc = static_cast<i32>(::SendMessageA(combo, 0x188, 0, 0));
    if (svc != -1) {
        g_serviceId = svc;
    }
    g_groupEnumMgr->ReadGroupSel(::GetDlgItem(hDlg, 0x3fc));
    g_pEndDialog(hDlg, 1);
    return 1;
}

// CMulti::GetString5a0 (0x000b7ad0) is now an inline member in the header.

// ===========================================================================
// CMulti::ReportVersionMsg  @ 0x0b7e30  - log a message line to the logic object
// (m_logic->LogLine). With code > 0, formats "<msg> (<code>)" first; else logs msg.
// ===========================================================================
RVA(0x000b7e30, 0x63)
void CMulti::ReportVersionMsg(char* msg, i32 code) {
    char buf[512];
    if (msg && *msg && Mgr()) {
        if (code > 0) {
            sprintf(buf, "%s (%i)", msg, code);
            Mgr()->EnterModalUI(buf);
        } else {
            Mgr()->EnterModalUI(msg);
        }
    }
}

// __thiscall(id, dest): load string `strId` from the app instance
// (m_logic->m_owner->m_hInstance), defaulting to "Error", then push it through
// ReportVersionMsg. A CMulti method (receiver-proven: called by WaitForConnect /
// the lobby watchdog on the same `this`); its RVA-order home is this TU.
RVA(0x000b7ec0, 0x7d)
void CMulti::ReportStatusId(u32 strId, i32 level) {
    char buf[0x12a];
    if (Mgr() && Mgr()->m_owner->m_hInstance) {
        if (!LoadStringA(Mgr()->m_owner->m_hInstance, strId, buf, 0xfa)) {
            strcpy(buf, "Error");
        }
        ReportVersionMsg(buf, level);
    }
}

// ===========================================================================
// CMulti::ReportNetError  @ 0x0b7f60  - format the last DirectPlay error
// ("Error: <code-name> - <code>") and report it at the given `level`, unless the
// user cancelled (DPERR_USERCANCEL == 0x118).
// ===========================================================================
RVA(0x000b7f60, 0x52)
void CMulti::ReportNetError(i32 level) {
    char buf[512];
    if (Mgr() && g_code != 0x118) {
        sprintf(buf, "Error: %s - %i", g_szCode, g_code);
        ReportVersionMsg(buf, level);
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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMulti);
SIZE_UNKNOWN(CState); // local dtor-view (stamps ??_7CState in ~CMulti)
SIZE_UNKNOWN(CMultiLogicDesc);
SIZE_UNKNOWN(CMultiMgrOptions);
SIZE_UNKNOWN(CSlotConfig);
SIZE_UNKNOWN(CMultiLogicList);
SIZE_UNKNOWN(CMultiLogicNode);
SIZE_UNKNOWN(CMultiReportGate);
SIZE_UNKNOWN(CRefresh21bd0);
SIZE_UNKNOWN(PBListSink);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

// ---------------------------------------------------------------------------
// MultiJoinDlgProc (0x0b8020) - the MULTI_JOIN "wait for players" modeless dialog
// proc (Win32 DialogProc, __stdcall; ret 0x10, 4 args). Its address is registered
// through the MultiJoinHandler ILT thunk (0x222f) above. Runs the shared base proc
// first (screensaver/monitor-power swallow); then:
//   * WM_INITDIALOG (0x110): cache the player listbox (GetDlgItem 0x3fc), clear the
//     create-context's player-sel latch (+0x74), arm a 2500 ms poll timer, and
//     self-post the first WM_TIMER. Bails (EndDialog(0)) if the listbox or context
//     is missing.
//   * WM_COMMAND (0x111): Cancel(2) -> KillTimer + EndDialog(0); OK(1) -> KillTimer,
//     confirm the join (OnJoinConfirm) -> EndDialog(1) on success, else beep + re-arm
//     the poll timer and stay open.
//   * WM_TIMER (0x113): re-enumerate the session players (EnumPlayersInto); on a
//     lost/errored session close the dialog, else refresh the player list, restore
//     the selection, and re-arm the timer (2000 ms, or 5000 ms on a slow-link
//     provider - InterfaceObject::IsInterface2).
// The g_pSendMessageA/GetDlgItem imports go through the engine's cached fn-ptr
// globals (::SendMessageA / ::GetDlgItem); EndDialog/MessageBeep via g_p*; KillTimer/
// SetTimer are direct USER32 imports. The re-arm-timeout probe (+0x70 IsInterface2)
// recurs in the WM_TIMER and WM_COMMAND-OK-fail paths.
// @early-stop
// regalloc block-duplication wall (~76%; same family as the sibling NetSetupDlgProc
// 0xb7b10 ~77%): logic byte-exact - the base-proc dispatch, the switch(msg) compare
// chain (sub 0x110/je/dec/je/sub 2/jne, matching retail with the WM_TIMER body as the
// fallthrough), the shared EndDialog(0)+return-1 tail (close:), the EnumPlayersInto/
// FillPlayerList/SetCurSel refresh, the OnJoinConfirm join and both IsInterface2
// slow-link timeout probes are all reproduced. Residual: retail SHARES one return-0
// block (0xb81e3, two predecessors: switch-miss + wParam!=1) so both reach it via
// `jne` (OK body fallthrough), while our MSVC5 /O2 DUPLICATES the 6-byte return-0
// (inlining one copy at wParam!=1, `je OK`) and reuses known-value registers
// (`push ebx`/`push eax` where we materialize immediates) - non-source-steerable
// block-layout + register-reuse heuristics. Final sweep.
RVA(0x000b8020, 0x22f)
i32 __stdcall MultiJoinDlgProc(HWND hDlg, u32 msg, u32 wParam, i32 lParam) {
    g_setupDlgHwnd = hDlg;
    if (BaseDlgProc(hDlg, msg, wParam, lParam) != 0) {
        goto ret_true;
    }
    switch (msg) {
        case 0x110: // WM_INITDIALOG
            g_netPlayerListHwnd = ::GetDlgItem(hDlg, 0x3fc);
            if (g_netPlayerListHwnd == 0) {
                goto close;
            }
            if (g_netCreateCtx == 0) {
                goto close;
            }
            g_netCreateCtx->m_74 = 0;
            SetTimer(hDlg, 1, 0x9c4, 0);
            ::SendMessageA(hDlg, 0x113, 0, 0);
            return 1;
        case 0x111: // WM_COMMAND
            if (wParam == 2) {
                KillTimer(hDlg, 1);
                g_pEndDialog(hDlg, 0);
                return 1;
            }
            if (wParam != 1) {
                goto ret_false;
            }
            KillTimer(hDlg, 1);
            if (((CMulti*)g_connectRptMgr)->OnJoinConfirm(hDlg) != 0) {
                g_pEndDialog(hDlg, 1);
                return 1;
            }
            g_pMessageBeep(0);
            {
                i32 t = 0x7d0;
                InterfaceObject* io = g_netCreateCtx->m_serviceProvider;
                if (io && io->IsInterface2()) {
                    t = 0x1388;
                }
                SetTimer(hDlg, 1, t, 0);
            }
            return 0;
        case 0x113: // WM_TIMER
            KillTimer(hDlg, 1);
            {
                i32 sel = static_cast<i32>(::SendMessageA(g_netPlayerListHwnd, 0x188, 0, 0)); // LB_GETCURSEL
                i32 hr = g_groupEnumMgr->EnumPlayersInto(0, 0);
                if (hr == static_cast<i32>(0x88770118)) {
                    goto close;
                }
                if (hr != 0) {
                    if (g_connectRptMgr == 0) {
                        goto close;
                    }
                    ((CMulti*)g_connectRptMgr)->ReportNetError(0);
                    g_pEndDialog(hDlg, 0);
                    return 1;
                }
                FillPlayerList(g_netPlayerListHwnd, g_groupEnumMgr);
                if (sel != -1) {
                    ::SendMessageA(g_netPlayerListHwnd, 0x186, sel, 0); // LB_SETCURSEL
                } else {
                    ::SendMessageA(g_netPlayerListHwnd, 0x186, 0, 0);
                }
                RefreshPlayerRow(hDlg, g_netPlayerListHwnd);
                i32 t = 0x7d0;
                InterfaceObject* io = g_netCreateCtx->m_serviceProvider;
                if (io && io->IsInterface2()) {
                    t = 0x1388;
                }
                SetTimer(hDlg, 1, t, 0);
            }
            return 1;
    }
ret_false:
    return 0;
close:
    g_pEndDialog(hDlg, 0);
ret_true:
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::DetectConnectionConfig  (__thiscall; ret 0 args; /GX EH frame).
// Resolves the connection class from the peer's selected service provider
// (m_groupSel): the InterfaceObject GUID predicates pick IPX / TcpIp / Modem /
// Serial (else "Other"), seeding the command-timing defaults (m_cmdDelay/m_resend).
// It then overrides those from the config store's "<section>_CmdDelay/_Resend" keys
// (if present), latches the local player name into the channel table, and joins the
// session (JoinAndRegisterChannel); on success it records the result into the peer's
// player-selection latch and returns 1, else 0.
// @early-stop
// /GX CString-temp cluster wall: the type detection, the three operator+ config-key
// builds + two GetInt reads, the channel-name latch and the JoinAndRegisterChannel
// tail are all reproduced, but retail's EH-state cookie sequence around the four
// scoped CString temps + their stack-slot packing is not expressible from clean
// C++ scopes. Same family as SetupTcpIpConfig / JoinAndRegisterChannel. Final sweep.
RVA(0x000b82e0, 0x230)
i32 CMulti::DetectConnectionConfig() {
    m_5ac = 0;
    InterfaceObject* provider = (InterfaceObject*)Peer()->m_groupSel;
    if (provider == 0) {
        return 0;
    }

    m_598 = "Other";
    if (provider->IsInterface1()) {
        m_598 = "IPX";
        m_5a4 = 2;
        m_drainReload = 0xa;
    } else if (provider->IsInterface2()) {
        m_598 = "TcpIp";
        m_5a4 = 3;
        m_drainReload = 0xa;
    } else if (provider->IsInterface3()) {
        m_598 = "Modem";
        m_5a4 = 4;
        m_drainReload = 0x1e;
    } else if (provider->IsInterface4()) {
        m_598 = "Serial";
        m_5a4 = 2;
        m_drainReload = 0xa;
    } else {
        m_5a4 = 2;
        m_drainReload = 0xa;
    }

    Utils::RegistryHelper* cfg = NetGameMgr()->m_configStore;
    CString kDelay = m_598 + "_CmdDelay";
    CString kResend = m_598 + "_Resend";
    CString kDyn = m_598 + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_5a4 = cd;
        m_drainReload = rs;
    }

    CNetChannel* ch0 = NetGameMgr()->m_channels;
    {
        CString name = GetString5a0();
        ch0->m_name = name;
    }
    ch0->m_slotId = 0;

    i32 r = JoinAndRegisterChannel();
    if (r != 0) {
        Peer()->m_playerSel = r;
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::ApplyCmdDelayDefaults  (C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_configSection + _Suffix" via operator+
// (each a stack CString temp), writes m_cmdDelay under "_CmdDelay" and m_resend under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
RVA(0x000b85a0, 0xd2)
void CMulti::ApplyCmdDelayDefaults() {
    Utils::RegistryHelper* reg = ((CGameMgr*)g_gameReg)->m_38;

    CString cmdDelayName = m_598 + "_CmdDelay";
    CString resendName = m_598 + "_Resend";
    CString dynCmdName = m_598 + "_DynCmdDelay";

    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(cmdDelayName)), m_5a4);
    reg->SetValueDword(const_cast<char*>(static_cast<const char*>(resendName)), m_drainReload);
}

// ~CMultiStartDlg @0x0b8960 - the COMPILER-GENERATED dtor (destroy CStringList
// m_74, then CString m_70, then chain the NAFXCW ~CDialog base; all reloc-masked).
// CMultiStartDlg declares no dtor (see Dialogs.h), so there is no source body to
// hang an RVA() on: cl emits it as a COMDAT in every using obj, and THIS obj is
// one - ShowMultiStartDlg below stack-constructs the dialog, which is exactly why
// retail placed the COMDAT here at 0xb8960, right after ShowMultiStartDlg's
// 0xb86c0+0x206. (Was defined in a dedicated src/Gruntz/ShowMultiDlg.cpp holding
// TU; that fiction existed only to host the user-declared dtor, and the
// declaration was itself the vptr-restamp mis-model. Deleted.)
// docs/patterns/eh-dtor-vptr-restamp-presence.md
//
// @rva-symbol: ??1CMultiStartDlg@@UAE@XZ 0x000b8960 0x59
RVA(0x000b86c0, 0x206)
i32 CMulti::ShowMultiStartDlg() {
    CMultiStartDlg dlg(reinterpret_cast<i32>(m_4), 0);
    i32 r = m_4->ExitModalUI(&dlg, 0);
    g_sharedFlag = 0;
    if (r != 1) {
        if (m_isHost != 0) {
            GruntzPlayer* rec = m_4->FindOptionsSlot(m_hostIndex);
            if (rec == 0) {
                return 0;
            }
            rec->m_liveGate = 0;
            NetCueReset_3bbb(rec->m_008, 1);
            BroadcastChannelTable(0); // 0xba810 (ILT 0x1d70)
        }
        if (m_isHost == 0 && m_538 == 0) {
            SendStatFlag(0x3ea, 1); // 0xb9240 (ILT 0x2e82)
        }
        return 0;
    }
    // r == 1
    if (m_isHost != 0) {
        ApplyCmdDelayDefaults(); // 0xb85a0 (ILT 0x386e)
    } else {
        if (m_c->m_soundRegistry->m_emitGate == 0) {
            void* rec_ob = 0; // CMapStringToPtr::Lookup takes a void*& out-param
            m_c->m_soundRegistry->m_10.Lookup(s_GameKey, rec_ob);
            LeafCue* rec = (LeafCue*)rec_ob;
            if (rec != 0) {
                i32 snd = g_sndEnabled;
                i32 cue = g_sndCueTag;
                if (snd != 0) {
                    i32 clk = g_killCueClock;
                    if (static_cast<u32>((clk - rec->m_14)) >= static_cast<u32>(rec->m_18)) {
                        rec->m_14 = clk;
                        rec->m_10->ConfigureItem(cue, 0, 0, 0);
                    }
                }
            }
        }
        ActiveWait(0xfa);
    }
    return 1;
}

// The player record held in each list node: it carries its profile/name source
// at +0x34 (NetFormatKeyed reads NAME out of it, else it is used raw as the text).
// (The former PlayerRecord / PlayerNode / Session views ARE the canonical
// CNetPlayerDesc / CNetListNode / CNetMgr from <Net/NetMgr.h>: FillPlayerList walks
// the CNetMgr +0x38 player CObList exactly as CNetMgr::PopulatePlayerList does. See
// FillPlayerList below.)

// ---------------------------------------------------------------------------
// FillPlayerList() - 0x0b89e0. __stdcall(HWND hList, CNetMgr* sess): clear the
// listbox, then walk the CNetMgr +0x38 player CObList (head @+0x3c via
// GetHeadPosition, running cursor cached in m_playerSelId @+0x80) - for each node's
// CNetPlayerDesc payload (+0x8) format its NAME key out of the +0x34 profile
// (NetFormatKeyed) into a scratch buffer, add that string (or the raw profile on a
// format miss), and stash the payload as the new item's data. No-op if either arg
// is null. The standalone twin of CNetMgr::PopulatePlayerList (0x178790) - same list
// walk, same canonical CNetListNode / CNetPlayerDesc types; the Session/PlayerNode/
// PlayerRecord views it used to carry ARE this CNetMgr + its player-list node/payload.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc/aliasing wall (~91.5%): logic byte-exact except the advance block -
// retail reloads m_playerSelId into a 2nd register (ecx) for the ->next read while
// keeping the old position in eax for ->m_data (`mov ecx,[m_80]; mov eax,[eax+8];
// mov edx,[ecx]`); the recompile keeps the position in one reg and derefs both
// fields from it. Aliasing-conservatism choice MSVC made in retail, not
// source-steerable (cf. linked-list-walk-node-eax-rotation.md). Deferred.
RVA(0x000b89e0, 0xc8)
void FillPlayerList(HWND hList, CNetMgr* sess) {
    char buf[256];
    if (!hList) {
        return;
    }
    if (!sess) {
        return;
    }
    ::SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    CNetListNode* node = (CNetListNode*)sess->m_players.GetHeadPosition();
    sess->m_playerSelId = node;
    CNetPlayerDesc* player;
    if (node) {
        sess->m_playerSelId = node->m_next;
        player = (CNetPlayerDesc*)node->m_data;
    } else {
        player = 0;
    }
    while (player) {
        const char* str;
        if (NetFormatKeyed(buf + 4, player->m_profile, "NAME")) {
            str = buf;
        } else {
            str = player->m_profile;
        }
        i32 idx = static_cast<i32>(::SendMessageA(hList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str)));
        if (idx != -1) {
            ::SendMessageA(hList, LB_SETITEMDATA, idx, reinterpret_cast<LPARAM>(player));
        }
        CNetListNode* pos = sess->m_playerSelId;
        if (pos) {
            player = (CNetPlayerDesc*)pos->m_data;
            sess->m_playerSelId = pos->m_next;
        } else {
            player = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::JoinAndRegisterChannel  (__thiscall; ret 0 args; /GX EH frame).
// Builds the command-timing config string in a 0x100 stack buffer (section seed +
// "CMDDELAY"/"RESEND"/"LEVEL" appends), enumerates the host group into it (via the
// group-enum mgr's EnumGroupsInto), and on success creates the local player
// (m_peer->CreatePlayer) and registers the local channel (RegisterChannelFrom over
// the channel-table name at m_4+0x150). Returns the enum result iff the channel
// registered, else 0; a failed enum / player-create reports the connect error.
// @early-stop
// branchless-mask + CString-temp wall (~?%): the config-string build, the
// EnumGroupsInto + CreatePlayer + RegisterChannelFrom sequence and the local-player
// latch are reproduced, but retail folds the final "channel-registered ? enumResult
// : 0" into a split neg/sbb/and mask carried across the name CString's /GX dtor,
// which a clean ternary won't reproduce exactly. Final sweep.
RVA(0x000b8b10, 0x175)
i32 CMulti::JoinAndRegisterChannel() {
    char buf[0x100];
    buf[0] = g_emptyString[0];
    memset(&buf[1], 0, 0xff);
    Cfg_SetSection(buf, "%s", *(i32*)((char*)this + 0x59c));
    Cfg_AppendKeyVal(buf, "CMDDELAY", m_5a4);
    Cfg_AppendKeyVal(buf, "RESEND", m_drainReload);
    Cfg_AppendKeyVal(buf, "LEVEL", ResyncLParam());

    i32 enumResult = g_groupEnumMgr->EnumGroupsInto((void*)4, buf, 0, reinterpret_cast<i32>(g_emptyString));
    if (enumResult == 0) {
        g_connectRptMgr->ReportConnectFailed(0);
        return 0;
    }

    void* lp = (void*)Peer()->CreatePlayer((void*)"Player", reinterpret_cast<i32>(g_emptyString), 0);
    m_5bc = reinterpret_cast<i32>((CNetPlayerEntry*)lp);
    if (lp == 0) {
        ReportConnectFailed(0);
        return 0;
    }

    m_hostIndex = *(i32*)((char*)lp + 4);
    CNetChannel* ch0 = NetGameMgr()->m_channels;
    i32 chField = ch0->m_slotId;
    CString name = ((GruntzPlayer*)ch0)->GetName();
    i32 ok = RegisterChannelFrom(name, chField, -1, m_hostIndex);
    return ok != 0 ? enumResult : 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnJoinConfirm  (__thiscall; ret 0x4, 1 arg; /GX EH frame).
// The join dialog's confirm handler. Reads the selected player from the dialog's
// list box (ReadPlayerSel on control 0x3fc), resolves the local player against it
// (EnumPlayersCb under the local name) and, on success, parses the peer's config
// blob (CMDDELAY/RESEND/dyn/LEVEL keys) into the command-timing fields, then builds
// and ships the 0x28-byte "player joined" packet (stat 0x3f9) carrying the local
// player id + name. Returns 1 on success, 0 on any bail.
// @early-stop
// /GX CString-temp + packet-build wall: the ReadPlayerSel/EnumPlayersCb resolve, the
// four config-key parses, the field latches and the stat-0x3f9 packet build + inline
// strcpy + SendStatFrom are reproduced, but retail's EH-state cookies over the scoped
// CString temps and its packet stack-slot packing aren't source-steerable. Final sweep.
RVA(0x000b8cf0, 0x23b)
i32 CMulti::OnJoinConfirm(void* hDlg) {
    if (hDlg == 0) {
        return 0;
    }

    g_groupEnumMgr->ReadPlayerSel(::GetDlgItem((HWND)hDlg, 0x3fc));
    void* sel = (void*)Peer()->m_playerSel;
    if (sel == 0) {
        return 0;
    }

    void* lp;
    {
        CString name = GetString5a0();
        lp = (void*)Peer()->EnumPlayersCb(sel, reinterpret_cast<i32>(static_cast<const char*>(name)), reinterpret_cast<i32>(g_emptyString), 0);
    }
    m_5bc = reinterpret_cast<i32>((CNetPlayerEntry*)lp);
    if (lp == 0) {
        ReportConnectFailed(0);
        return 0;
    }

    const char* cfgStr = *(const char**)((char*)sel + 0x34);
    char buf[0x28];
    if (Cfg_GetKey(buf, cfgStr, "CMDDELAY")) {
        m_5a4 = atoi(buf);
    }
    if (Cfg_GetKey(buf, cfgStr, "RESEND")) {
        m_drainReload = atoi(buf);
    }
    if (Cfg_GetKey(buf, cfgStr, "DynCmdDelay")) {
        ApplyDynSetting(CString(buf));
    }
    m_syncGate = 0;
    ResyncLParam() = 1;
    m_hostIndex = *(i32*)((char*)lp + 4);
    if (Cfg_GetKey(buf, cfgStr, "LEVEL")) {
        ResyncLParam() = atoi(buf);
    }

    char packet[0x28];
    memset(packet, 0, 0x28);
    packet[0] |= 0x80;
    *(i32*)(packet + 4) = 0x3f9;
    packet[8] = 1;
    packet[9] = 0;
    packet[0xa] = 1;
    packet[0xb] = 0;
    packet[0xc] = 0x63;
    packet[0xd] = 0xf;
    packet[0xe] = 0;
    *(i32*)(packet + 0x10) = m_hostIndex;
    CString name2 = GetString5a0();
    strcpy(packet + 0x14, name2);
    SendStatFrom((CNetStatPacket*)packet, 0x28, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::VerifyCustomLevel  (__thiscall; /GX EH frame).
// Confirms every player is on the same custom level before the match starts.
// No-op (0) on a null token pair or when the config isn't loaded (m_530 == 0; that
// path just pumps the receive queue). Builds the level rez path from the active
// config name (GetConfigNameB when a custom id m_5b0 is set, else GetConfigNameA),
// hands it to the active session's Poll, and reports: Poll failure -> re-disable +
// "unable to verify"; verified-but-mismatch (m_levelVerifyResult still 0) -> "not all players
// have the same level"; agreement -> 1. The by-value CString rez-path arg + the
// name temp run under the /GX frame.
// @early-stop
// /GX CString-by-value EH-frame-layout wall (7%): the instruction SEQUENCE is
// faithful (the arg1/arg2/m_530 guards, the GetConfigNameA/B selection, the
// BuildRezPath by-value CString copy-ctor, the multi-temp destruct bitmask, the
// g_connectRptMgr Poll dispatch and both ShowModal reports), but retail reserves two
// dedicated EH-state dwords (`sub esp,8`) and overlaps the CString temps onto the
// now-dead arg slots, while cl folds the EH state into the arg-overlap area and
// omits the sub - an 8-byte frame-size delta that cascades through every
// stack-relative offset. Same CString-EH residue family as the dialog sibling
// CMultiStartDlg::VerifyCustomLevel (0xc4c00, parked ~55%); not source-steerable.
// See docs/patterns/gx-scoped-local-eh-frame-size.md. Final sweep.
RVA(0x000b8fc0, 0x151)
i32 CMulti::VerifyCustomLevel(void* h, i32 playerTok) {
    if (h == 0) {
        return 0;
    }
    if (playerTok == 0) {
        return 0;
    }
    if (m_530 == 0) {
        PollSession();
        return 0;
    }

    i32 token;
    if (m_5b0 != 0) {
        CString b = GetConfigNameB();
        token = ((CGruntzMgr*)g_gameReg)->BuildLevelRezPath(0, m_5b0, 0, 0, b);
    } else {
        CString a = GetConfigNameA();
        token = ((CGruntzMgr*)g_gameReg)->BuildLevelRezPath(0, m_5b0, 0, 0, a);
    }

    g_connectRptMgr->m_levelVerifyResult = 0;
    if (g_connectRptMgr->Poll(token) == 0) {
        m_530 = 0;
        ((CGruntzMgr*)(void*)g_gameReg)
            ->EnterModalUI("Unable to verify custom level with other players");
        return 0;
    }
    if (g_connectRptMgr->m_levelVerifyResult == 0) {
        ((CGruntzMgr*)(void*)g_gameReg)
            ->EnterModalUI("Not all players have the (same) custom level.");
        m_530 = 0;
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::PollSessionGated  (__thiscall).
// With both args non-null: if the session done-latch (m_534) is already set report
// success; otherwise poll the session once (PollSession) and report whether the poll
// set the latch.
RVA(0x000b9180, 0x4a)
i32 CMulti::PollSessionGated(i32 a1, i32 a2) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    if (m_534 != 0) {
        return 1;
    }
    PollSession();
    return m_534 != 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatBuf  (__thiscall).
// Core stat sender: sets the packet's bit7 flag, then ships the 0x10-byte
// packet to the local player's peer group via the DirectPlay set-data wrapper
// (m_peer->SetGroupDataFrom(localPlayer, flag, pkt, 0x10)). Returns the
// success bool (hr == 0).
RVA(0x000b91f0, 0x31)
i32 CMulti::SendStatBuf(CNetStatPacket* pkt, i32 flag) {
    pkt->m_0 |= 0x80;
    i32 hr = Peer()->SetGroupDataFrom(LocalPlayer(), flag, reinterpret_cast<i32>(pkt), 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatFlag  (__thiscall).
// Builds the 0x10-byte stat header {id, localPlayer.id} on the stack and ships
// it through SendStatBuf with the caller's flag.
RVA(0x000b9240, 0x38)
void CMulti::SendStatFlag(i32 id, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = LocalPlayer()->m_4;
    SendStatBuf(&pkt, flag);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendNetStat  (__thiscall).
// Builds the 0x10-byte stat header {id, value} on the stack and ships it
// through SendStatBuf with the caller's flag.
RVA(0x000b9290, 0x32)
void CMulti::SendNetStat(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = value;
    SendStatBuf(&pkt, flag);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatFrom  (__thiscall).
// No-op on a null packet; otherwise ships the caller's packet to the local
// player's peer group via SetGroupDataFrom(localPlayer, c, pkt, b).
RVA(0x000b92e0, 0x34)
i32 CMulti::SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c) {
    if (pkt == 0) {
        return 0;
    }
    i32 hr = Peer()->SetGroupDataFrom(LocalPlayer(), c, reinterpret_cast<i32>(pkt), b);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatPair  (__thiscall).
// Null-recipient -> 0; otherwise sets the packet's bit7 flag and ships both
// through SetGroupData2(localPlayer, recipient, c, packet, 0x10).
RVA(0x000b9330, 0x41)
i32 CMulti::SendStatPair(CNetPlayerEntry* recipient, CNetStatPacket* pkt, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    pkt->m_0 |= 0x80;
    i32 hr = Peer()->SetGroupData2(LocalPlayer(), recipient, c, reinterpret_cast<i32>(pkt), 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatTo  (__thiscall).
// No-op on a null recipient; otherwise builds the 0x10-byte stat header
// {flag, id, localPlayer.id} on the stack and ships it to that one player via
// SendStatPair.
RVA(0x000b93a0, 0x47)
i32 CMulti::SendStatTo(CNetPlayerEntry* recipient, i32 id, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = LocalPlayer()->m_4;
    return SendStatPair(recipient, &pkt, c);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStat3  (__thiscall).
// The 3-arg stat sender OnDropPlayer fires: builds the 0x10-byte stat header
// {flag, value, localPlayer.id} on the stack and ships it through SetData
// (a=localPlayer.id, b=id, c=flag). Returns the success bool.
RVA(0x000b9410, 0x51)
i32 CMulti::SendStat3(i32 id, u32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = value;
    pkt.m_8 = LocalPlayer()->m_4;
    i32 hr = Peer()->SetData(LocalPlayer()->m_4, id, flag, reinterpret_cast<i32>(&pkt), 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendNetStatTo  (__thiscall).
// SendStatTo's explicit-value twin: null-recipient -> 0; otherwise builds the 0x10-byte
// stat header {id, value} on the stack and ships it to that one player via SendStatPair.
RVA(0x000b9490, 0x42)
i32 CMulti::SendNetStatTo(CNetPlayerEntry* recipient, i32 id, u32 value, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = id;
    pkt.m_8 = value;
    return SendStatPair(recipient, &pkt, c);
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatPairRaw  (__thiscall).
// Forwards a caller packet to one recipient via SetGroupData2 (no bit7 stamp,
// caller-supplied size): null-recipient or null-packet -> 0, else ships
// SetGroupData2(localPlayer, recipient, c, pkt, size). Returns the success bool.
RVA(0x000b9500, 0x46)
i32 CMulti::SendStatPairRaw(CNetPlayerEntry* recipient, void* pkt, i32 size, i32 c) {
    if (recipient == 0) {
        return 0;
    }
    if (pkt == 0) {
        return 0;
    }
    i32 hr = Peer()->SetGroupData2(LocalPlayer(), recipient, c, reinterpret_cast<i32>(pkt), size);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendStatValue  (__thiscall).
// Builds the 0x10-byte stat header {flag, statId, value} on the stack and ships
// it through SetData (a=localPlayer.id, b=id, c=flag). Returns the success bool.
RVA(0x000b9570, 0x53)
i32 CMulti::SendStatValue(i32 id, i32 statId, i32 value, i32 flag) {
    CNetStatPacket pkt;
    pkt.m_0 |= 0x80;
    pkt.m_4 = statId;
    pkt.m_8 = value;
    i32 hr = Peer()->SetData(LocalPlayer()->m_4, id, flag, reinterpret_cast<i32>(&pkt), 0x10);
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::PollSession  (__thiscall).
// Pumps the DirectPlay receive queue. Bails (returns 0) if there is no local
// player. Asks the peer's interface how many messages are pending for the local
// player (GetMessageCount, slot 0x44), then receives + dispatches up to that
// many (each Receive into the shared g_recvBuffer, slot 0x64): a nonzero HRESULT
// is reported (NetMgr.h:0x141) and breaks; a message not addressed from the
// local player is handed to the engine dispatcher (Stub_0b9750) and counted.
// Stops early if the abort latch (m_pollAbort) is set. Returns the dispatched
// count.
// @early-stop
// frame-size + regalloc + COM-slot-aliasing wall (~60%): logic, the null guard,
// the inlined GetMessageCount (slot 0x44) probe with the neg/sbb/not/and HRESULT
// mask, the receive loop (Receive slot 0x64), the ReportError on failure, and the
// per-message DispatchRecvMsg are all reproduced - but retail's frame is 0x10
// (mine 0xc), it pins this=esi / 0=edi / count=ebx across the function, and it
// OVERLAPS the receive {size,idFrom} stack slot (one local serves lpidFrom AND
// lpdwDataSize) which a clean C++ shape won't express. See
// docs/patterns/stack-buffer-size-drives-frame.md. Deferred to the final sweep.
RVA(0x000b95f0, 0x10f)
i32 CMulti::PollSession() {
    if (LocalPlayer() == 0) {
        return 0;
    }

    i32 count;
    if (LocalPlayer() == 0) {
        count = 0;
    } else {
        IDirectPlay4Z* dp = Peer()->m_directPlay;
        count = 0;
        i32 hr = dp->GetMessageCount(LocalPlayer()->m_4, &count);
        if (hr) {
            count = 0;
        }
    }
    if (count <= 0) {
        return 0;
    }

    i32 dispatched = 0;
    i32 sender = 0;
    while (count > 0) {
        if (m_pollAbort) {
            break;
        }

        i32 size = 0x800;
        i32 idTo = LocalPlayer()->m_4;
        IDirectPlay4Z* dp = Peer()->m_directPlay;
        i32 hr = dp->Receive(&size, &idTo, 1, (void*)g_recvBuffer, &size);
        if (hr) {
            CNetMgr::ReportError("c:\\proj\\incs\\netmgr.h", 0x141, hr, 0);
            if (hr) {
                break;
            }
        }
        count--;
        if (sender != LocalPlayer()->m_4) {
            DispatchRecvMsg(sender, g_recvBuffer, size);
            dispatched++;
        }
        if (hr) {
            break;
        }
    }
    return dispatched;
}

// ---------------------------------------------------------------------------
// CNetMgr::DispatchRecvMsg  (0xb9750, __thiscall; /GX EH frame).
// The per-received-message dispatcher PollSession hands each DirectPlay packet to.
// A null buffer -> 0. A zero `sender` is a local control record forwarded to
// HandleControlMsg. Otherwise it resolves the sender's command slot (clearing that
// slot's latched latency once a connection/rejoin is up), gates on the bit7 flag,
// then switches on the message id (msg->m_id, 0x3e8..0x423) - the 60-entry jump
// table the compiler emits (a byte index table + a distinct-target array). Every
// handled arm returns 1; the default (and the early null/flag guards) return 0.

// The channel in-use table accessors (ChannelSlots.cpp, __cdecl free functions).
i32 ChannelSlots_Get(i32 i);         // 0xdb2d0
i32 ChannelSlots_FindFree();         // 0xdb280
void ChannelSlots_Set(i32 i, i32 v); // 0xdb2b0

// (CNetColorHolder is gone - a DEAD comment-only view with zero uses, and the TENTH name
// pinned on 0xdb200. That rva is GruntzPlayer::SwapChannel; see the fold note at the top.)

// LeafCue::PlayIfElapsed (0x1f940, __thiscall): plays the positional sound cue when
// the kill-cue clock throttle has elapsed. Reached as a bare call - the caller's
// preceding null-check leaves the cue object in ecx, so no explicit `this` load is
// emitted; modeled as a flat __stdcall alias. Fires through the 0x25fe incremental-
// link thunk; external -> reloc-masked.
extern "C" void __stdcall PlayIfElapsed(i32 tag, i32 a, i32 b, i32 c); // 0x1f940

// The cached USER32 PostMessageA pointer (the game's own function-pointer global,
// distinct from the IAT import) + the modal chat-sink handle. DIR32 reloc-masked.
// (g_sharedFlag @0x648ce0 is declared+defined once, at the top of this TU)

// The received-message view: a bit7 flag byte, the message id, then a payload the
// arms read as a word / channel byte / chat text depending on the id.
SIZE_UNKNOWN(CNetMsg);
// CNetMsg (the DispatchRecvMsg wire packet) is a fully-known sibling of the send structs
// in <Net/NetPackets.h> (included above) - a wire struct has no business in a .cpp.

// @early-stop
// tail-merge + regalloc wall (~78%): the whole dispatcher is byte-faithful - the
// /GX prologue, the sender==0 HandleControlMsg forward, the command-slot latency
// clear, the 60-entry byte-index jump table (COMDAT emitted + case grouping exact),
// and every one of the 32 arms. The residual is MSVC's per-guard tail-merge coin
// flip (some guards `jne b9e80` share the trailing `mov eax,1`, others inline it -
// steered as far as source allows by break/return + the call-result-null inline
// idiom) plus register-choice/scheduling nits inside the channel-latency,
// running-ping-average (0x420) and record-ack (0x41c/0x421) arms (eax<->edx /
// esi<->edi recolor, store-order permutation). Not further source-steerable. Final sweep.
RVA(0x000b9750, 0x74e)
i32 CMulti::DispatchRecvMsg(i32 sender, char* buf, i32 size) {
    CNetMsg* msg = (CNetMsg*)buf;
    if (msg == 0) {
        return 0;
    }
    if (sender == 0) {
        return HandleControlMsg((CNetCtrlMsg*)msg, size);
    }

    CNetPlayerEntry* pd = (CNetPlayerEntry*)Peer()->GetPlayerData(sender);
    if (m_connected != 0 || m_pumpGuard != 0) {
        if (pd != 0) {
            CNetCmdSlot* slot = Session()->FindCmdSlot(pd->m_4);
            if (slot != 0) {
                slot->m_latency = 0;
            }
        }
    }

    if ((msg->m_0 & 0x80) == 0) {
        return 0;
    }

    switch (msg->m_4) {
        case 0x3e8:
            m_534 = 1;
            return 1;

        case 0x3fc:
            m_530 = 1;
            return 1;

        case 0x3ed:
            if (m_534 != 0) {
                break;
            }
            RecordDropPlayer2(reinterpret_cast<i32>(pd), sender);
            break;

        case 0x422: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            if (player->m_030 == 0) {
                player->m_030 = 1;
                g_activePlayerCount++;
            }
            OnMultiOptions();
            break;
        }

        case 0x423: {
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            if (player->m_030 == 0) {
                break;
            }
            player->m_030 = 0;
            g_activePlayerCount--;
            break;
        }

        case 0x3f0: {
            if (g_sharedFlag != 0) {
                ShowChatLine((void*)g_sharedFlag, msg->m_c);
                break;
            }
            if (m_connected == 0) {
                break;
            }
            GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            ((CFontConfig*)NetGameMgr()->m_5c)->AddItem(msg->m_c, 0x30, player->m_008);
            CSndHost* host = m_c->m_soundRegistry;
            if (host->m_emitGate != 0) {
                break;
            }
            void* e_ob = 0;
            host->m_10.Lookup("GAME_CHAT", e_ob);
            LeafCue* e = (LeafCue*)e_ob;
            if (e == 0) {
                break;
            }
            PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            break;
        }

        case 0x411:
            if (m_pollAbort != 0) {
                break;
            }
            ReportVersionMsg("You have been dropped from the game.", 0);
            ::PostMessageA(NetGameMgr()->m_wnd->m_hwnd, 0x111, 0x8023, 0);
            m_pollAbort = 1;
            break;

        case 0x410:
            AckDropPlayer(msg->m_8);
            break;

        case 0x3ea:
            OnPlayerLeft(sender);
            ResetPlayerCommands(sender);
            g_playerLeftFlag = 1;
            break;

        case 0x3f7:
            if (m_isHost == 0) {
                break;
            }
            BroadcastChannelTable(pd);
            break;

        case 0x3f8:
            if (m_isHost != 0) {
                break;
            }
            ParseChannelTable(msg);
            g_playerLeftFlag = 1;
            break;

        case 0x3f9:
            if (m_isHost == 0) {
                break;
            }
            if (m_connected != 0) {
                break;
            }
            if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
                break;
            }
            if (ChannelSlots_Get(((u8*)&msg->m_8)[1]) == 0) {
                ((u8*)&msg->m_8)[1] = static_cast<u8>(ChannelSlots_FindFree());
            }
            ChannelSlots_Set(((u8*)&msg->m_8)[1], 0);
            RegisterChannelRec(msg);
            BroadcastChannelTable(0);
            SaveConfig(pd);
            g_playerLeftFlag = 1;
            break;

        case 0x3fa: {
            if (m_isHost == 0) {
                break;
            }
            if (m_connected != 0) {
                break;
            }
            GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(msg->m_14);
            if (player == 0) {
                return 0;
            }
            if (player->SwapChannel(static_cast<u8>(msg->m_c[1])) == 0) {
                msg->m_c[1] = static_cast<char>(player->m_008);
                SendStatTo(pd, 0x419, 1);
            }
            ParseOneChannel(msg);
            BroadcastChannelTable(0);
            g_playerLeftFlag = 1;
            break;
        }

        case 0x3fb:
            if (m_isHost != 0) {
                break;
            }
            m_538 = 1;
            break;

        case 0x419:
            if (m_isHost != 0) {
                break;
            }
            m_568 = 1;
            break;

        case 0x3fd:
            if (m_isHost != 0) {
                break;
            }
            m_5ac = 1;
            break;

        case 0x3fe:
            if (m_isHost != 0) {
                break;
            }
            m_56c = 1;
            break;

        case 0x41f:
            SendStatValue(sender, 0x420, msg->m_8, 0);
            break;

        case 0x420: {
            i32 stamp = msg->m_8;
            u32 now = timeGetTime();
            i32 delta = now - stamp;
            GruntzPlayer* player = (GruntzPlayer*)((CGruntzMgr*)g_gameReg)->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            i32 num = player->m_22c * player->m_230 + delta;
            i32 np1 = player->m_230 + 1;
            player->m_230 = np1;
            player->m_22c = num / np1;
            break;
        }

        case 0x421: {
            if (m_isHost == 0) {
                break;
            }
            GruntzPlayer* player = (GruntzPlayer*)((CGruntzMgr*)g_gameReg)->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            m_channelLatency[player->m_playerIndex] = msg->m_8;
            break;
        }

        case 0x41d:
            m_verifyDone = 1;
            m_levelVerifyResult = 1;
            return 1;

        case 0x41e:
            m_levelVerifyResult = 0;
            m_verifyDone = 1;
            return 1;

        case 0x41c: {
            GruntzPlayer* player = (GruntzPlayer*)((CGruntzMgr*)g_gameReg)->FindOptionsSlot(sender);
            if (player == 0) {
                return 1;
            }
            m_recordAcked[player->m_playerIndex] = 1;
            m_recordToken[player->m_playerIndex] = msg->m_8;
            break;
        }

        case 0x402:
            m_lastSenderId = msg->m_8;
            m_584 = 1;
            return 1;

        case 0x403:
            if (m_isHost == 0) {
                break;
            }
            if (m_connected == 0) {
                break;
            }
            if (m_534 == 0) {
                break;
            }
            SendStatFlag(0x404, 1);
            OnOutOfSync();
            break;

        case 0x404:
            if (m_connected == 0) {
                break;
            }
            OnOutOfSync();
            break;

        case 0x407:
            if (m_connected == 0) {
                break;
            }
            OnMultiPause();
            break;

        case 0x415:
            if (m_isHost == 0) {
                break;
            }
            SaveConfig(pd);
            break;

        case 0x416:
            if (LoadConfig(msg) == 0) {
                break;
            }
            m_58c = 1;
            break;

        case 0x417:
            HandleVersionCheck((CNetVersionMsg*)msg);
            break;

        case 0x418: {
            CString result;
            if (pd != 0) {
                CString name = ((CNetMgr*)pd)->GetName();
                result.Format("*** %s has a different version of the game.", static_cast<const char*>(name));
            } else {
                result.Format("*** A player had a different version of the game.");
            }
            if (g_sharedFlag != 0) {
                ShowChatLine((void*)g_sharedFlag, result);
            } else {
                ((CFontConfig*)NetGameMgr()->m_5c)->AddItem(result, 0, 0x11);
            }
            break;
        }

        case 0x3f6:
            break;

        default:
            return 0;
    }
    return 1;
}

// CNetMgr::GetConfigNameA (0x000b6090) is now an inline member in the header.

// CNetMgr::GetConfigNameB (0x000b60d0) is now an inline member in the header.

// CNetMgr::GetName (0x000ba170) is now an inline member in the header.

// CNetPlayerEntry::GetName - identical +0x8 read; COMDAT-folds with CNetMgr::GetName @0xba170
// in retail (one address). Defined (not RVA-annotated: cannot dup the RVA).
CString CNetPlayerEntry::GetName() {
    return m_8;
}

// ---------------------------------------------------------------------------
// CNetMgr::HandleControlMsg  (__thiscall).
// Dispatches a network control message on its +0x0 code: 3 -> the sprite/menu
// handler; 5 -> the player-left path (when sub-code 1, report+teardown then set
// the shared player-left flag); 0x31 -> latch m_sessionTerminated; 0x101 ->
// latch m_useChannelLatency. Anything else (or a null/out-of-range code) -> 0;
// the matched cases return 1.
// @early-stop
// jump-table-data-overlap scoring artifact (objdiff 0%, CODE byte-exact: all 37
// dispatch+case bytes match retail, incl. the two-level byte-index + jump-ptr
// table). objdiff mis-scores the inline .rdata table region against the
// differently-named switchdataD_004ba2xx symbols. See
// docs/patterns/jumptable-data-overlap.md (topic:scoring-artifact). Logic correct.
RVA(0x000ba1a0, 0x83)
i32 CMulti::HandleControlMsg(CNetCtrlMsg* msg, i32 arg2) {
    if (msg == 0) {
        return 0;
    }

    switch (msg->m_0) {
        case 3:
            HandleSpriteMsg(msg);
            return 1;
        case 5:
            if (msg->m_4 != 1) {
                return 1;
            }
            OnPlayerLeft(msg->m_8);
            g_playerLeftFlag = 1;
            return 1;
        case 0x31:
            m_isHost = 1;
            return 1;
        case 0x101:
            m_sessionTerminated = 1;
            return 1;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnPlayerLeft  (__thiscall; /GX EH frame).
// Tears down a leaving player and announces it. Looks the player's data blob up
// in the peer (GetPlayerData); ignores the local player. Resolves the player's
// slot (m_4->FindPlayer); requires its +0x20 / +0x14 gates set. Releases the
// slot's global flag (g_netSlotTable[slot->m_008] via SetNetSlot, decrement
// the active-player refcount g_activePlayerCount if armed), clears its list link, builds "<name> has left the
// game." and appends it to the chat log (m_4->m_5c->AddItem, type 0x20 data
// 0x11), and unlinks the blob (RemovePlayerObj). If the channel selector is set
// and not yet connected, fires the rejoin finalizer and sets g_playerLeftFlag.
// The two CString temps' dtors run under the /GX frame.
RVA(0x000ba3b0, 0x17f)
i32 CMulti::OnPlayerLeft(i32 playerId) {
    CNetPlayerObj* blob = (CNetPlayerObj*)Peer()->GetPlayerData(playerId);
    if (blob == (CNetPlayerObj*)LocalPlayer()) {
        return 0;
    }

    GruntzPlayer* slot = (GruntzPlayer*)Mgr()->FindOptionsSlot(playerId);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_liveGate == 0) {
        return 0;
    }
    if (slot->m_014 == 0) {
        return 0;
    }

    if (slot->m_030 != 0) {
        slot->m_030 = 0;
        g_activePlayerCount--;
    }
    slot->m_liveGate = 0;
    ChannelSlots_Set(slot->m_008, 1);

    CString line = ((CNetMgr*)slot)->GetName() + " has left the game.";
    ((CFontConfig*)NetGameMgr()->m_5c)->AddItem(const_cast<char*>(static_cast<const char*>(line)), 0x20, 0x11);

    if (blob != 0) {
        Peer()->RemovePlayerObj(blob);
    }
    if (m_isHost != 0 && m_connected == 0) {
        RejoinIfNeeded(0);
        g_playerLeftFlag = 1;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::AckDropPlayer  (__thiscall).
// Finalizes a dropped player. When the host-mode flag (m_534) is clear it records
// the drop (RecordDropPlayer), then looks the player's command slot up
// (Session()->FindCmdSlot) and, if found, latches+resets it (Touch + FullReset)
// and arms both the slot and its command-list head (slot->m_state = 1,
// slot->m_cmdHead[+0x2c] = 1). In host mode it instead tears the player down directly
// (OnPlayerLeft) and flushes their resend buffers (ResetPlayerCommands).
RVA(0x000ba590, 0x63)
void CMulti::AckDropPlayer(i32 id) {
    if (m_534 == 0) {
        RecordDropPlayer2(0, id);
        CNetCmdSlot* slot = Session()->FindCmdSlot(id);
        if (slot != 0) {
            slot->Touch();
            slot->FullReset();
            slot->m_state = 1;
            slot->m_cmdHead[0xb] = 1;
        }
        return;
    }

    OnPlayerLeft(id);
    ResetPlayerCommands(id);
}

// --- CNetMgr::LoadMenuSelectSprite (0xba620, __thiscall) -----------------------
// On an armed (ev->m_armed==1) menu-select event: resolve/create the player session
// node, then (unless a paused/over gate m_530/m_connected is set -> a 0x3fd stat)
// either bail with a 0x3fe stat when the ready-options count is full (>=4), or
// announce the version and play the cooldown-gated "GAME_MENUS_SELECT" cue.
//
// Uses the shared CNetMgr (NetMgr.h): the +0x524 player sub-object (its own 1-arg
// GetPlayerData / 4-arg AddSessionNode) is that TU's PlayerMgr, reached by casting
// m_peer each use (reproducing the retail double-load of [this+0x524]); the +0x4
// options host and the +0xc sound sub-mgr are cast from their shared-class slots.
// @confidence: med
// @source: decomp-xref
// RELOC-Multi: the cue lookup at +0xe0 is a DIRECT retail call to 0x1b8438, which IS the
// MFC library `CMapStringToOb::Lookup` (?Lookup@CMapStringToOb@@QBEHPBDAAPAVCObject@@@Z,
// NAFXCW - disasm-proven: it tail-calls CMapStringToOb::GetAssocAt@0x1b83de and reuses the
// `key` arg slot as the out-nHash local). `CSndFinder::Lookup` was a fake-view alias of it,
// so the rel32 bound to nothing. The finder embedded at CSndHost+0x10 IS a CMapStringToOb
// (its 0x1c bytes exactly fill +0x10..+0x2c), so the call is made through the real MFC
// class here and now links against the library symbol. (The feared C1189 wall on typing
// CSndHost::m_10 was measured and was only TWO Win32-umbrella TUs deep, not ~60: both were
// switched to <Mfc.h> and the member is now the real CMapStringToOb.) The
// out-value is CObject* because that is what the MFC container's own API types it.
// (RELOC-Multi follow-up: CSndHost::m_10 is now TYPED CMapStringToOb in <Gruntz/SoundCue.h>
RVA(0x000ba620, 0x14a)
i32 CMulti::LoadMenuSelectSprite(void* evp) {
    MenuSelectEvent* ev = (MenuSelectEvent*)evp;
    if (ev == 0) {
        return 0;
    }
    if (ev->m_armed != 1) {
        return 0;
    }
    void* node = Peer()->GetPlayerData(ev->m_id);
    if (node == 0) {
        node = (void*)Peer()->AddSessionNode(ev->m_id, ev->m_nameA, ev->m_nameB, reinterpret_cast<i32>(node));
        if (node == 0) {
            return 0;
        }
    }
    if (m_530 == 0 && m_connected == 0) {
        if (m_isHost != 0) {
            if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
                SendStat3(ev->m_id, 0x3fe, 1);
                return 0;
            }
            if (m_isHost != 0) {
                AnnounceVersion(reinterpret_cast<i32>(node));
            }
        }
        CSndHost* host = m_c->m_soundRegistry;
        if (host->m_emitGate == 0) {
            void* out = 0;
            host->m_10.Lookup("GAME_MENUS_SELECT", out);
            LeafCue* e = (LeafCue*)out;
            if (e != 0) {
                i32 enabled = g_sndEnabled;
                i32 tag = g_sndCueTag;
                if (enabled != 0) {
                    u32 now = g_killCueClock;
                    if (static_cast<u32>((now - e->m_14)) >= e->m_18) {
                        e->m_14 = now;
                        e->m_10->ConfigureItem(tag, 0, 0, 0);
                    }
                }
            }
        }
        return 1;
    }
    SendStat3(ev->m_id, 0x3fd, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ResolveLocalPlayer  (__thiscall).
// Resolves the local player descriptor: bails (0) with no peer; otherwise looks
// the local player id (m_localPlayerId) up in the peer's player list and latches the
// result into m_5bc, returning whether one was found.
RVA(0x000ba7d0, 0x2e)
i32 CMulti::ResolveLocalPlayer() {
    if (Peer() == 0) {
        return 0;
    }
    m_5bc = reinterpret_cast<i32>(Peer()->FindPlayerById(m_hostIndex));
    return LocalPlayer() != 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastChannelTable  (__thiscall).
// Serializes the whole four-channel ack/state table (m_4 + 0x150, stride 0x238)
// into a 0x88-byte stat packet (stat id 0x3f8) - one 0x20-byte record per channel
// (each carries the channel's gate/id bytes plus its name copied in via the
// engine GetName CString + inline strcpy) - then ships it: to one recipient via
// SendStatPairRaw when given, else to the local player's group via SendStatFrom.
// @early-stop
// regalloc + load-width wall (~86%): the logic, the 0x88 packet memset, the
// per-channel record fill, the GetName CString + inline strcpy and the
// recipient/group send branch are all reproduced - but retail anchors the record
// ptr (ebx) one byte lower, dword-loads each channel field before the byte store
// (movl;movb vs my movb;movb), and orders the slot-address lea operands
// (eax,ebp vs ebp,eax) differently; none move under source restructuring (the
// inverse parse ParseChannelTable is 99.9%). Deferred to the final sweep.
RVA(0x000ba810, 0x11c)
i32 CMulti::BroadcastChannelTable(CNetPlayerEntry* recipient) {
    char packet[0x88];
    memset(packet, 0, 0x88);
    packet[0] |= 0x80;
    *(i32*)(packet + 4) = STAT_CHANNEL_TABLE;

    char* rec = packet + 9;
    for (i32 i = 0; i < 4; i++) {
        CNetChannel* ch = &NetGameMgr()->m_channels[i];
        if (ch != 0) {
            rec[-1] = static_cast<char>(ch->m_active);
            rec[0] = static_cast<char>(ch->m_slotId);
            rec[1] = static_cast<char>(ch->m_14);
            rec[2] = static_cast<char>(ch->m_10);
            rec[5] = static_cast<char>(ch->m_flag);
            rec[4] = static_cast<char>(ch->m_228);
            *(i32*)(rec + 7) = ch->m_playerId;
            CString name = ((GruntzPlayer*)ch)->GetName();
            strcpy(rec + 0xb, static_cast<const char*>(name));
        }
        rec += 0x20;
    }

    if (recipient != 0) {
        return SendStatPairRaw(recipient, packet, 0x88, 1);
    }
    return SendStatFrom((CNetStatPacket*)packet, 0x88, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::ParseChannelTable  (__thiscall).
// The inverse of BroadcastChannelTable: parses a received 0x88 packet back into
// the four-channel table. Bails (0) on a null packet; (re)initializes the global
// net-slot table when not channel-latency mode, then for each channel copies the
// record bytes back, restores its name CString, and - in non-channel mode for a
// newly active channel - frees its net slot (ChannelSlots_Set(id, 0)).
// @early-stop
// regalloc SIB-base wall (~99.9%): the whole body is byte-exact, the single
// residual is the slot-address `lea 0x150(%eax,%ebp)` vs my `lea 0x150(%ebp,%eax)`
// (SIB base/index swap of m_4 vs the loop counter); not steerable from source
// (m_4 is reloaded each iteration so a running pointer diverges). Final sweep.
RVA(0x000ba980, 0xca)
i32 CMulti::ParseChannelTable(void* packet) {
    if (packet == 0) {
        return 0;
    }
    if (m_isHost == 0) {
        ResetNetSlots();
    }

    char* rec = (char*)packet + 9;
    for (i32 i = 0; i < 4; i++) {
        CNetChannel* ch = &NetGameMgr()->m_channels[i];
        if (ch != 0) {
            ch->m_active = static_cast<u8>(rec[-1]);
            ch->m_slotId = static_cast<u8>(rec[0]);
            ch->m_14 = static_cast<u8>(rec[1]);
            ch->m_10 = static_cast<u8>(rec[2]);
            if (rec[5] != 0) {
                ch->m_flag = 1;
            } else {
                ch->m_flag = 0;
            }
            ch->m_228 = static_cast<u8>(rec[4]);
            ch->m_name = rec + 0xb;
            ch->m_playerId = *(i32*)(rec + 7);
            if (m_isHost == 0 && ch->m_active != 0) {
                ChannelSlots_Set(ch->m_slotId, 0);
            }
        }
        rec += 0x20;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannelFrom  (__thiscall).
// Thin forwarder: fixes the c=1 / idx=0 register arguments and tail-calls
// RegisterChannel with the caller's four fields (name, id, e, f).
RVA(0x000baa90, 0x20)
i32 CMulti::RegisterChannelFrom(const char* name, i32 b, i32 e, i32 f) {
    return RegisterChannel(name, b, 1, 0, e, f);
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannel  (__thiscall; /GX EH frame).
// Creates or refreshes one channel slot. Bails (0) if the table is full
// (m_4->CountActiveChannels >= 4). Tries the requested index (idx in [0,4]) when
// its slot is free, else linear-scans for the first inactive slot; bails if none.
// Stores the supplied fields into the slot (name CString into +0x4, id/flags) and
// marks it active. The CString temp's dtor runs under the /GX frame.
// @early-stop
// /GX EH-state cookie wall (~93%): the whole body is byte-exact (the full-table
// guard, the requested/scan slot selection, the SetNetSlot + scoped CString temp,
// every field store), but retail's __except prologue pushes the scope cookie 0x8
// where our cl pushes 0x0 and references its own funclet, the residual being the
// TU-wide EH-state numbering. See docs/patterns/gx-scoped-local-eh-frame-size.md +
// eh-state-numbering-base.md. Deferred to the final sweep.
RVA(0x000baac0, 0x12e)
i32 CMulti::RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e) {
    if (Mgr()->CountReadyOptionsSlots(1) >= 4) {
        return 0;
    }

    CNetChannel* ch = 0;
    if (idx >= 0 && idx <= 4) {
        ch = &NetGameMgr()->m_channels[idx];
        if (ch != 0 && ch->m_active != 0) {
            ch = 0;
        }
    }
    if (ch == 0) {
        CNetChannel* p = NetGameMgr()->m_channels;
        for (i32 i = 0; i < 4; i++) {
            ch = p;
            if (p != 0 && p->m_active == 0) {
                break;
            }
            ch = 0;
            p++;
        }
        if (ch == 0) {
            return 0;
        }
    }

    ChannelSlots_Set(id, 0);
    {
        CString temp(name);
        ch->m_name = temp;
    }
    ch->m_slotId = id;
    ch->m_14 = c;
    ch->m_10 = d;
    ch->m_flag = 0;
    ch->m_playerId = e;
    ch->m_active = 1;
    ch->m_latency = 0;
    ch->m_230 = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RegisterChannelRec  (__thiscall).
// Unpacks a received register record (a CNetCtrlMsg-shaped blob): no-op (returns
// 1) unless its +0x8 active byte is set; otherwise pulls the name pointer (+0x14)
// and the four header bytes (+0x9..+0xc) plus the id dword (+0x10) and registers
// the channel.
RVA(0x000bac40, 0x38)
i32 CMulti::RegisterChannelRec(void* rec) {
    u8* r = (u8*)rec;
    if (r[8] != 0) {
        RegisterChannel(reinterpret_cast<const char*>((r + 0x14)), r[9], r[0xa], r[0xb], r[0xc], *(i32*)(r + 0x10));
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RemoveChannel  (__thiscall).
// Tears down the channel slot at the given index: no-op on a null slot; returns 0
// if it was already inactive; otherwise clears its active gate and frees its net
// slot (ChannelSlots_Set(id, 1)). Returns 1 when a slot was removed.
RVA(0x000bac90, 0x46)
i32 CMulti::RemoveChannel(i32 idx) {
    CNetChannel* ch = &NetGameMgr()->m_channels[idx];
    if (ch == 0) {
        return 0;
    }
    if (ch->m_active == 0) {
        return 0;
    }
    ch->m_active = 0;
    ChannelSlots_Set(ch->m_slotId, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnPauseChannel  (__thiscall).
// No-op (returns 0) unless connected (m_580); otherwise announces the pause
// (SendStatFlag(0x407, 1)) and runs the multiplayer pause handler (OnMultiPause).
RVA(0x000bad00, 0x2d)
i32 CMulti::OnPauseChannel() {
    if (m_connected == 0) {
        return 0;
    }
    SendStatFlag(STAT_PAUSE, 1);
    OnMultiPause();
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiPause
// Reentrancy-guarded fire of MULTI_PAUSE. When the dispatch returns 0x4cc,
// forwards WM_COMMAND(0x80d7, m_1c) to the engine window.
RVA(0x000bad40, 0x6c)
void CMulti::OnMultiPause() {
    if (g_pauseGuard) {
        return;
    }

    m_584 = 0;
    g_pauseGuard = 1;
    i32 r = RunErrorDialog("MULTI_PAUSE", (void*)&MultiPauseCallback, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == DISPATCH_RESYNC) {
        HWND hwnd = NetGameMgr()->m_wnd->m_hwnd;
        ::PostMessageA(hwnd, WM_COMMAND, 0x80d7, ResyncLParam());
    }
}

// (The g_gameReg +0x38 config store is the SAME Utils::RegistryHelper the CNetGameMgr
// exposes as m_configStore - GetString lives on that one class now.)

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiOptions
// Reentrancy-guarded fire of the MULTI_OPTIONZ command. Clears m_584, dispatches
// (return value ignored), then clears the shared flag.
RVA(0x000badd0, 0x43)
void CMulti::OnMultiOptions() {
    if (g_optionzGuard) {
        return;
    }

    m_584 = 0;
    g_optionzGuard = 1;
    RunErrorDialog("MULTI_OPTIONZ", (void*)&MultiOptionzCallback, 0);
    g_optionzGuard = 0;
    g_sharedFlag = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnOutOfSync
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
RVA(0x000bae40, 0x84)
void CMulti::OnOutOfSync() {
    if (m_574) {
        return;
    }

    m_574 = 1;
    m_584 = 0;
    i32 r = RunErrorDialog("MULTI_OUTOFSYNC", (void*)&MultiOutOfSyncCallback, 0);
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESYNC: {
            HWND hwnd = NetGameMgr()->m_wnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x80d7, ResyncLParam());
            break;
        }
        case DISPATCH_RESET:
            break;
        default: {
            HWND hwnd = NetGameMgr()->m_wnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastOneChannel  (__thiscall).
// Serializes one channel descriptor into a 0x2c-byte stat packet (stat id 0x3fa,
// the same record byte layout as a single BroadcastChannelTable slot plus the
// channel name strcpy'd in) and ships it to the local player's group via
// SendStatFrom.
// @early-stop
// load-width wall (~87%): the whole shape matches retail - the 0x2c packet build,
// the frameless CString name temp (scoped to elide the /GX frame), the inline
// strcpy and the SendStatFrom send are byte-aligned - but retail dword-loads each
// i32 channel field before the byte store (movl;movb) where our cl byte-loads it
// (movb;movb), shuffling the field-store order. Same wall as
// BroadcastChannelTable; not steerable from source. Deferred to the final sweep.
RVA(0x000baf00, 0xb2)
i32 CMulti::BroadcastOneChannel(i32 chan) {
    CNetChannel* ch = (CNetChannel*)chan;
    char packet[0x2c];
    memset(packet, 0, 0x2c);
    packet[0] |= 0x80;
    *(i32*)(packet + 4) = STAT_CHANNEL_ONE;
    *(i32*)(packet + 8) = ch->m_id;

    packet[0xd] = ch->m_slotId;
    packet[0xe] = ch->m_14;
    packet[0xf] = ch->m_10;
    packet[0x12] = ch->m_flag;
    packet[0xc] = 1;
    packet[0x11] = ch->m_228;
    {
        i32 id = ch->m_playerId;
        CString name = ((GruntzPlayer*)ch)->GetName();
        *(i32*)(packet + 0x18) = id;
        strcpy(packet + 0x18, static_cast<const char*>(name));
    }

    return SendStatFrom((CNetStatPacket*)packet, 0x2c, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::ParseOneChannel  (__thiscall).
// The inverse of BroadcastOneChannel: parses a single-channel record into the
// channel slot named by the record's index (rec+0x8, must be in [0,4]). Bails (0)
// on a null record / out-of-range index / null slot. Restores the channel name
// CString (+0x4) and the header bytes, then marks the slot active.
RVA(0x000baff0, 0x88)
i32 CMulti::ParseOneChannel(void* rec) {
    if (rec == 0) {
        return 0;
    }
    u8* r = (u8*)rec;
    i32 idx = *(i32*)(r + 8);
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    CNetChannel* ch = &NetGameMgr()->m_channels[idx];
    if (ch == 0) {
        return 0;
    }

    ch->m_name = reinterpret_cast<char*>((r + 0x18));
    ch->m_slotId = r[0xd];
    ch->m_10 = r[0xf];
    if (r[0x12] != 0) {
        ch->m_flag = 1;
    } else {
        ch->m_flag = 0;
    }
    ch->m_228 = r[0x11];
    ch->m_14 = r[0xe];
    ch->m_playerId = *(i32*)(r + 0x14);
    ch->m_active = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendChannelStat422  (__thiscall).
// Stamps the static stat-0x422 packet (flag bit7, value 0) and ships it to the
// local player's group via SetGroupDataFrom.
RVA(0x000bb0b0, 0x44)
i32 CMulti::SendChannelStat422() {
    g_chanStat422_id = 0x422;
    g_chanStat422_flag |= 0x80;
    g_chanStat422_val = 0;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, reinterpret_cast<i32>(&g_chanStat422_flag), 0xc);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::SendChannelStat423  (__thiscall).
// As SendChannelStat422 but for the static stat-0x423 packet.
RVA(0x000bb120, 0x44)
i32 CMulti::SendChannelStat423() {
    g_chanStat423_id = 0x423;
    g_chanStat423_flag |= 0x80;
    g_chanStat423_val = 0;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, reinterpret_cast<i32>(&g_chanStat423_flag), 0xc);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::BroadcastChatLine  (__thiscall).
// Assembles a chat line and broadcasts it (stat 0x3f0). Bails (0) on a null/empty
// text. Caps the text at 0x80 chars and trims up to two trailing control chars.
// When toChat is set it prefixes the local player's name ("<name>: <text>") via
// sprintf; otherwise the raw text is used. When showWnd is set the line is either
// posted to a Win32 chat control (ShowChatLine) or appended to the in-game chat
// log (m_4->m_5c->AddItem). Finally the line is stamped into the static chat
// packet and shipped through SetGroupDataFrom.
// @early-stop
// scheduling wall (~75%): the full logic is reproduced - the null/empty guard,
// the 0x80 cap, the 2-iteration trailing-control-char trim, the toChat "name: msg"
// sprintf (frameless GetName temp), the showWnd ShowChatLine/AddItem branch, and
// the static-packet (per-field globals) build + SetGroupDataFrom send - and the
// /GX frame is correctly elided (line modeled as a stack char buffer, not a
// CString). The residual is instruction-selection that desyncs the tail: retail
// hoists the 0x20 trim constant + `cmpb %al,mem` (vs my `movb mem,%dl;cmpb`), and
// folds the send-size strlen differently (`dec;add 0xd` vs `add 0xc`), reordering
// the static stores. Big function, scheduling-class; deferred to the final sweep.
RVA(0x000bb190, 0x1c5)
i32 CMulti::BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd) {
    if (text == 0) {
        return 0;
    }
    if (text[0] == 0) {
        return 0;
    }

    i32 len = strlen(text);
    if (len > 0x80) {
        text[0x80] = 0;
        len = 0x80;
    }
    if (len > 0 && text[len - 1] < 0x20) {
        text[len - 1] = 0;
        len--;
        if (len > 0 && text[len - 1] < 0x20) {
            text[len - 1] = 0;
        }
    }

    char line[0x12c];
    if (toChat != 0) {
        GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(LocalPlayer()->m_4);
        CString name = player->GetName();
        sprintf(line, "%s: %s", static_cast<const char*>(name), text);
    } else {
        strcpy(line, text);
    }

    if (showWnd != 0) {
        if (hWnd != 0) {
            ShowChatLine(hWnd, line);
        } else {
            GruntzPlayer* player = (GruntzPlayer*)Mgr()->FindOptionsSlot(m_hostIndex);
            if (player != 0) {
                ((CFontConfig*)NetGameMgr()->m_5c)->AddItem(line, 0x30, player->m_008);
            }
        }
    }

    g_chatPacket_id = STAT_CHAT;
    g_chatPacket_val = 0;
    strcpy(&g_chatPacket_buf, line);
    g_chatPacket_flag |= 0x80;
    Peer()->SetGroupDataFrom(LocalPlayer(), 1, reinterpret_cast<i32>(&g_chatPacket_flag), strlen(line) + 0xd);
    return 1;
}

namespace NetLobby {
    // __stdcall(edit, str): append `str` to an edit control, prefixing a CRLF when
    // the control is non-empty, then scroll to keep the caret in view.
    RVA(0x000bb3e0, 0xe5)
    void __stdcall AppendEditLine(HWND edit, char* str) {
        if (!edit || !str || !str[0]) {
            return;
        }
        i32 len = ::GetWindowTextLengthA(edit);
        if (len == 0) {
            ::SendMessageA(edit, 0xb1, len, -1);
        } else {
            ::SendMessageA(edit, 0xb1, len, len);
        }
        char buf[0x80];
        buf[0] = 0;
        if (len > 0) {
            strcat(buf, "\r\n");
        }
        strcat(buf, str);
        ::SendMessageA(edit, 0xc2, 0, reinterpret_cast<LPARAM>(buf));
        ::SendMessageA(edit, 0xb6, 0, 0x270f);
    }
} // namespace NetLobby

// ---------------------------------------------------------------------------
// CNetMgr::DropChannelPlayer  (__thiscall).
// Drops the player owning channel[idx]. Bails (0) on an out-of-range index
// ([0,4)), no peer (m_528), or a null channel. Looks the channel player's data
// up in the peer (GetPlayerData by the channel's +0x18 id); when the channel's
// +0x14 "active" gate is set it reports the player-left to the rest (stat 0x3fb
// via SendStatPair when the data is present, else just clears below). Removes the
// channel (RemoveChannel(idx)) and, on success, fires the rejoin finalizer
// (RejoinIfNeeded(0)) and latches the player-left flag. Returns 1 when a channel
// was dropped, else 0.
// @early-stop
// regalloc wall (~98%): the whole body is byte-aligned (index guard, m_528 gate,
// channel-record lea, GetPlayerData probe, m_14-gated SendStatTo, RemoveChannel +
// RejoinIfNeeded + g_playerLeftFlag tail) but retail pins the "active" flag
// (ch->m_14) in edi (callee-saved across the calls) where cl keeps it in ecx, and
// shares the failure epilogue one instruction tighter. Final sweep.
RVA(0x000bb510, 0x9d)
i32 CMulti::DropChannelPlayer(i32 idx) {
    if (idx < 0 || idx >= 4) {
        return 0;
    }
    if (m_isHost == 0) {
        return 0;
    }

    CNetChannel* ch = &NetGameMgr()->m_channels[idx];
    if (ch == 0) {
        return 0;
    }

    void* data = Peer()->GetPlayerData(ch->m_playerId);
    i32 active = ch->m_14;
    if (data == 0) {
        if (active != 0) {
            return 0;
        }
    } else if (active != 0) {
        SendStatTo((CNetPlayerEntry*)data, STAT_CHANNEL_LEFT, 1);
    }

    if (RemoveChannel(idx) == 0) {
        return 0;
    }
    RejoinIfNeeded(0);
    g_playerLeftFlag = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::RecordDropPlayer2  (__thiscall).
// Records a pending player-drop into the m_dropIds id array. No-op once the host
// latch m_534 is set, or for the local player (m_localPlayerId). Skips a player already
// recorded; otherwise fills the first empty m_dropIds slot, bailing if the array is
// full. Then, once the number of recorded drops reaches the number of
// command slots in state 3 (the m_520 sub-object, stride 0x64), it announces the
// drop twice (stat 0x3e8) and latches m_534.
// @early-stop
// regalloc wall (~93%): every instruction matches in the multiset (the m_534/m_localPlayerId
// guards, the three m_dropIds scans, the state-3 slot count, the double SendStatFlag and
// the m_534 latch) but retail pins this->esi / id->edi where cl assigns this->edi /
// id->esi; the register choice is not steerable from source. Final sweep.
RVA(0x000bb5e0, 0xd9)
void CMulti::RecordDropPlayer2(i32 a, i32 id) {
    if (m_534 != 0) {
        return;
    }
    if (id == m_hostIndex) {
        return;
    }

    i32 count = m_604.GetSize();
    i32 i;
    for (i = 0; i < count; i++) {
        if (static_cast<i32>(m_604[i]) == id) {
            return;
        }
    }

    i32 slot = 0;
    while (slot < count) {
        if (m_604[slot] == 0) {
            break;
        }
        slot++;
    }
    if (slot >= count) {
        return;
    }
    m_604[slot] = id;

    i32 stateThree = 0;
    CNetCmdSlot* p = m_session->m_slots;
    for (i = 0; i < 4; i++) {
        if (p != 0 && p->m_state == 3) {
            stateThree++;
        }
        p++;
    }

    i32 recorded = 0;
    for (i = 0; i < count; i++) {
        if (m_604[i] != 0) {
            recorded++;
        }
    }
    if (recorded < stateThree) {
        return;
    }

    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    SendStatFlag(STAT_DROP_ANNOUNCE, 1);
    m_534 = 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::WaitForOtherPlayers (0xbb700, /GX) - after clearing the per-slot vote
// scratch, if the peer is ready (or no slot is still in state 3) latch m_534 and
// return 1. Otherwise announce the wait (stat 0x3ed), draw the "Waiting for other
// playerz..." status string, and spin (Sleep(50) + PollSession) with a 5s resend
// timer (AckJoinFailure + re-announce) and a 120s abort timer (DropTimeout), while
// accumulating elapsed time onto each state-3 slot, until m_534 latches or ESC is
// pressed. On exit republish the frame clock and, if ambient sound is enabled, play
// the "AMBIENT%d" cue. The "Waiting..." CString is the /GX frame's destructible.
// @early-stop
// Complete, structurally-faithful reconstruction (~75%); parks below 100% on three
// compounding codegen walls, NOT reloc artifacts (verified base-vs-target with
// llvm-objdump -dr - every REL32 callee / DIR32 data referent is named/masked):
//   (1) tail-merge/block-layout: retail folds the two `m_534=1; return 1` early
//       exits (peer-ready and no-state-3-slot) into one shared epilogue block both
//       sites `je`; MSVC5 duplicates the epilogue inline at each site. Not steerable
//       (same family as the sibling Poll 0xbba10 @early-stop epilogue tail-dup wall).
//   (2) status-text SetRect/EngStr_DrawText arg block: the GruntInfoText register-
//       rotation wall (docs/patterns/select-zero-mask-dest-register.md family) -
//       retail threads modeW/modeH/rect through a register rotation cl won't
//       reproduce and spills a second modeW/modeH pair, shifting the /GX frame by 4
//       bytes (sub 0x5c vs 0x58) so every esp-relative slot below diverges.
//   (3) the resend/abort timers + the reused 0-constant are register-pinned across
//       the wait loop (edi/ebx/ebp) differently than cl colours them - the same
//       zero-register/regalloc wall as Poll. Logic + control flow are byte-faithful.
RVA(0x000bb700, 0x265)
i32 CMulti::WaitForOtherPlayers() {
    CDWordArray* votes = &m_604;
    votes->SetSize(0, -1);
    for (i32 k = 3; k != 0; k--) {
        votes->SetAtGrow(votes->GetSize(), 0);
    }
    if (Peer()->m_sessions.GetCount() == 1) {
        m_534 = 1;
        return 1;
    }
    i32 count = 0;
    CNetCmdSlot* slot = m_session->m_slots;
    for (i32 j = 4; j != 0; j--) {
        if (slot != 0 && slot->m_state == 3) {
            count++;
        }
        slot++;
    }
    if (count == 0) {
        m_534 = 1;
        return 1;
    }

    SendStatFlag(0x3ed, 1);
    CString waitStr("Waiting for other playerz...");
    CGameRegistry* g = g_gameReg;
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = g->m_modeW;
    rc.bottom = g->m_modeH;
    EngStr_DrawText(
        (EngStrRenderObj*)g->m_world,
        reinterpret_cast<i32>(&waitStr),
        reinterpret_cast<i32>(&rc),
        0x82,
        1,
        0xff,
        0xff,
        0,
        1
    );

    i32 resend = 0x1388;
    i32 abort = 0x1d4c0;
    while (m_534 == 0) {
        u32 start = timeGetTime();
        Sleep(0x32);
        PollSession();
        if (GetAsyncKeyState(0x1b) & 0x80000000) {
            return 0;
        }
        u32 elapsed = timeGetTime() - start;
        if (elapsed >= static_cast<u32>(resend)) {
            resend = 0;
        } else {
            resend -= elapsed;
        }
        if (elapsed >= static_cast<u32>(abort)) {
            abort = 0;
        } else {
            abort -= elapsed;
        }
        for (i32 i = 0; i < 4; i++) {
            CNetCmdSlot* s = &m_session->m_slots[i];
            if (s->m_state == 3) {
                s->m_latency += elapsed;
            }
        }
        if (abort == 0) {
            DropTimeout();
            abort = 0x1d4c0;
        }
        if (resend == 0) {
            resend = 0x1388;
            AckJoinFailure();
            SendStatFlag(0x3ed, 1);
        }
    }

    g_scoreTimeBase = timeGetTime();
    if (g->m_14 != 0) {
        char buf[0x40];
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        NetGameMgr()->m_sound->PlayByName(buf, 1);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::Poll (0xbba10, __thiscall) - block (pumping the session) until the
// custom-level verify vote resolves (m_verifyDone latches) or the timer runs out.
//
// Two modes on the is-host latch m_528:
//   guest (m_528==0): ship the verify request (stat 0x41c), spin with a 5s resend
//     timer and a 15s abort timer; on each 5s lapse re-arm (AckJoinFailure) and
//     re-send. Exits 1 once PollSession latches m_verifyDone, 0 on the 15s timeout.
//   host  (m_528!=0): clear the per-record ack/vote latches, spin with a 15s abort
//     timer, and each pass scan the four active session records - if every present
//     record has acked (m_recordAcked[i]) then vote agree/disagree by whether every ack
//     token (m_recordToken[i]) matches ours, push the result stat (0x41d agree / 0x41e
//     disagree), record it (m_levelVerifyResult) and latch m_verifyDone. Exits 1 on resolve, 0 on timeout.
//
// @early-stop
// Real codegen diff (~95.5%, NOT a reloc artifact): the body is byte-exact - both
// timer paths, the array-zero loop, the 4-record scan (0x238 stride, [eax-8]/[eax]/
// [eax-0xc] gates, the m_recordAcked/m_recordToken latch + token vote). objdiff MASKS REL32 call/branch
// reloc target-names (measured: renaming the ILT-thunk callees SendNetStat/PollSession/
// AckJoinFailure/SendStatFlag to the real ?...@CNetMgr@@ symbols moved the score 0.0%),
// so the thunk-routed callees are NOT the cap. The residual is an epilogue
// tail-duplication difference: our base shares one return epilogue (jne/jmp) where
// retail tail-duplicates it (je; mov eax,1; ...; ret 4) - a regalloc/block-layout
// wall, not steerable here. See docs/wall-instructions.md.
RVA(0x000bba10, 0x1fb)
i32 CMulti::Poll(i32 token) {
    if (m_isHost == 0) {
        SendNetStat(STAT_VERIFY_REQUEST, token, 1);
        i32 resend = 0x1388;
        i32 abort = 0x3a98;
        m_verifyDone = 0;
        do {
            u32 start = timeGetTime();
            Sleep(0x32);
            PollSession();
            u32 elapsed = timeGetTime() - start;
            if (elapsed >= static_cast<u32>(resend)) {
                resend = 0;
            } else {
                resend -= elapsed;
            }
            if (elapsed >= static_cast<u32>(abort)) {
                abort = 0;
            } else {
                abort -= elapsed;
            }
            if (abort == 0) {
                return 0;
            }
            if (resend == 0) {
                resend = 0x1388;
                AckJoinFailure();
                SendNetStat(STAT_VERIFY_REQUEST, token, 1);
            }
        } while (m_verifyDone == 0);
        return 1;
    }

    i32 abort = 0x3a98;
    m_verifyDone = 0;
    for (i32 i = 0; i < 4; i++) {
        m_recordAcked[i] = 0;
        m_recordToken[i] = 0;
    }
    while (m_verifyDone == 0) {
        u32 start = timeGetTime();
        Sleep(0x32);
        PollSession();
        u32 elapsed = timeGetTime() - start;
        if (elapsed >= static_cast<u32>(abort)) {
            abort = 0;
        } else {
            abort -= elapsed;
        }
        if (abort == 0) {
            return 0;
        }

        i32 allAcked = 1;
        i32 allAgree = 1;
        // g_gameReg is the game-manager singleton; its +0x150 channel table is
        // the same CNetGameMgr::m_channels[4] the net mgr drives (retail walks it
        // from +0x170 == channel.m_20, reading back to m_18/m_14).
        CNetGameMgr* mgr = (CNetGameMgr*)g_gameReg;
        for (i32 i = 0; i < 4; i++) {
            CNetChannel* ch = &mgr->m_channels[i];
            if (ch->m_playerId != m_hostIndex && ch->m_active != 0 && ch->m_14 != 0) {
                if (m_recordAcked[i] == 0) {
                    allAcked = 0;
                } else if (!(m_recordToken[i] == token && token != 0)) {
                    allAgree = 0;
                }
            }
        }
        if (allAcked != 0) {
            if (allAgree != 0) {
                SendStatFlag(STAT_VERIFY_AGREE, 1);
                m_levelVerifyResult = 1;
                m_verifyDone = 1;
            } else {
                SendStatFlag(STAT_VERIFY_DISAGREE, 1);
                m_levelVerifyResult = 0;
                m_verifyDone = 1;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::CreateSession  (__thiscall; /GX EH frame).
// Stands up the DirectPlay command session: enumerate the host's group (via the
// peer's EnumGroupsRange over the create-context record), resolve the local
// player, then allocate + construct the 0x20bb0-byte CNetSession (operator new +
// the 4-slot vector-ctor + ResetAll), wire it (Init with the game sub-object,
// this manager and the peer), latch the local player and derive the resync tick
// byte (m_resyncTick), and finally seed one command slot per channel with a per-channel
// owner code (1 inactive, 2 local, 3 remote). The new'd session is the /GX-tracked
// object. Returns 1 once every slot is created, 0 on any failure.
// @early-stop
// reloc-masked + EH-cookie plateau (95.4%): the instruction stream is byte-identical
// to retail (RezAlloc alloc, the 4-slot vector-ctor + ResetAll, Init, the tick-byte
// derivation, the per-channel CreateSlot loop). The residual is non-steerable: the
// /GX unwind-table cookie immediate (push 0x16 vs 0x0; gx-scoped-local-eh-frame-
// size.md), the MSVC-internal vector-ctor/ctor/dtor helper symbols (delinker named
// them Boundary_/CGruntWingzTimeSprite - unalignable reloc names), and a 4-insn
// register shuffle around the m_session->m_c store. Final sweep.
RVA(0x000bbc90, 0x1b8)
i32 CMulti::CreateSession() {
    void* rec = g_netCreateCtx->m_74;
    if (rec == 0) {
        return 0;
    }
    Peer()->EnumGroupsRange(rec, 0);
    if (ResolveLocalPlayer() == 0) {
        return 0;
    }

    CNetSession* session = new CNetSession();
    m_session = session;
    if (session == 0) {
        return 0;
    }
    if (session->Init(NetGameMgr(), this, Peer()) == 0) {
        return 0;
    }

    Session()->m_localDesc = (SlotInfo*)LocalPlayer();
    i32 raw10 = m_session->m_tick;
    u8 b = static_cast<u8>(raw10);
    if (b == 0) {
        b = 0x7f;
    } else {
        b = b - 1;
    }
    m_curSlotId = b;

    for (i32 i = 0; i < 4; i++) {
        CNetChannel* ch = &NetGameMgr()->m_channels[i];
        i32 code = 1;
        if (ch->m_active != 0 && ch->m_14 != 0) {
            code = (ch->m_playerId == m_hostIndex) ? 2 : 3;
        }
        if (Session()->CreateSlot(i, code) == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::CNetCmdSlot  (__thiscall; /GX EH frame).
// Constructs the queued-command list (CPtrList m_cmds, default nBlockSize 10),
// then resets the slot to its empty state: zero the scalar header, drain the
// queue (ClearCmds), zero the command fields and splat both command ranges.
// The CObList member's dtor pulls in the /GX EH frame.
// @early-stop
// zero-register-pinning wall (78.8%): code bytes byte-faithful (EH frame, CObList
// ctor, every field store, ClearCmds + both ResetTriple calls all match retail).
// Retail re-materializes the splat 0 in eax (caller-saved, `xor eax,eax` after the
// CObList ctor AND after ClearCmds) and pushes only esi; cl pins 0 in callee-saved
// edi (one xor, an extra push/pop edi). Identical coin-flip to the sibling
// CNetCmdSlot::ResetAll/Init in netcmdslot; not source-steerable. See
// docs/patterns/zero-register-pinning.md. Final sweep.
RVA(0x000bbec0, 0x81)
CNetCmdSlot::CNetCmdSlot() {
    m_state = 0;
    m_resetGuard = 0;
    m_latchedSeq = 0;
    m_cmdHead = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    m_owner = 0;
    ClearCmds();
    m_ackFlags[0] = 0;
    m_ackFlags[1] = 0;
    m_ackFlags[2] = 0;
    m_ackFlags[3] = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// CNetCmdSlot helper reached only here (0xc0bb0, __thiscall, external).

// ---------------------------------------------------------------------------
// CNetSession::ResetAll (0xbbf80, __thiscall) - full session reset: zero the
// scalar header (m_1c latched to 1), reset every one of the four inline command
// slots (the same field-wipe + ClearCmds + ResetTriple sequence as
// CNetCmdSlot::ResetAll, inlined here), clear the 0x200-byte resync scratch
// block, then zero all 0x80 resync entries.
// ---------------------------------------------------------------------------
// @early-stop
// loop induction-variable / regalloc wall (71.5%): every operation is byte-faithful
// (header zero, the inlined per-slot wipe + ClearCmds + both ResetTriple calls, the
// rep stos over m_1b0, the 0x80-entry zero loop). Retail strength-reduces the slot
// loop into TWO induction vars (edi=slot passed as `this`, esi=slot+8 for the field
// stores) and SPILLS the down-counter to a stack slot (the leading `push ecx`),
// also basing the entry loop at entry+8; cl uses one IV (esi=slot) and keeps the
// counter in edi. A pure induction-var-selection coin-flip (same family as
// CNetSyncCheck::AllSlotsReady ~79%); not source-steerable. Final sweep.
RVA(0x000bbf80, 0xb7)
void CNetSession::ResetAll() {
    m_0 = 0;
    m_session = 0;
    m_netMgr = 0;
    m_localDesc = 0;
    m_tick = 0;
    m_snapshotDone = 0;
    m_seq = 0;
    m_period = 1;

    i32 i;
    CNetCmdSlot* slot = m_slots;
    for (i = 4; i != 0; i--) {
        slot->m_state = 0;
        slot->m_resetGuard = 0;
        slot->m_latchedSeq = 0;
        slot->m_cmdHead = 0;
        slot->m_latency = 0;
        slot->m_baseSeq = 0;
        slot->m_maxSeq = 0;
        slot->m_owner = 0;
        slot->ClearCmds();
        slot->m_ackFlags[0] = 0;
        slot->m_ackFlags[1] = 0;
        slot->m_ackFlags[2] = 0;
        slot->m_ackFlags[3] = 0;
        slot->ResetTriple(slot->m_rangeA);
        slot->ResetTriple(slot->m_rangeB);
        slot++;
    }

    memset(m_idMap, 0, sizeof(m_idMap));

    CNetResyncEntry* e = m_entries;
    for (i = 0x80; i != 0; i--) {
        e->m_0 = 0;
        e->m_8 = 0;
        e->m_c = 0;
        e->m_4 = 0;
        e++;
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::FrameSyncWait  (__thiscall).
// Paces the network frame: samples timeGetTime, records the delta since the
// last call (m_lastFrameDelta) and the new stamp (m_lastFrameTime). If the frame came in under 0x1f
// ms it busy-waits the remainder (ActiveWait) and re-stamps; otherwise, if the
// frame ran long (> 0x28 ms) and the sync gate m_syncGate is set, it flips the
// global low-bit sync toggle and returns it.
// @early-stop
// regalloc + schedule wall (~71%): logic byte-faithful (timeGetTime, the delta/stamp
// stores, the <=0x1e ActiveWait re-stamp, the >0x28 sync-toggle). Retail pins this->esi
// and now->edi and orders `m_lastFrameDelta` store before `m_lastFrameTime`; cl swaps
// the callee-saved pins (this->edi) and reorders the two stores. Not steerable. Final sweep.
RVA(0x000bc070, 0x73)
u32 CMulti::FrameSyncWait() {
    u32 now = timeGetTime();
    u32 delta = now - m_5e4;
    u32 ret = 0;
    m_accumTime = delta;
    m_5e4 = now;

    if (delta <= 0x1e) {
        ActiveWait(0x1f - delta);
        m_5e4 = (now - m_accumTime) + 0x1f;
        return 0;
    }
    if (delta > 0x28 && m_syncGate) {
        ret = g_syncToggle ^ 1;
        g_syncToggle = ret;
    }
    return ret;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnDropPlayer  (__thiscall).
// Reentrancy-guarded fire of the MULTI_DROPPLAYER command. Clears m_584,
// dispatches, then switches on the result: 0x4cd just resets the command
// buffers; 0x4ce resets and posts WM_COMMAND(0x8023) to the engine window;
// 0x4ea reports the leaving player (stat 0x411 if still present), broadcasts
// the drop (stat 0x410), acks it, and resets the buffers.
RVA(0x000bc110, 0xf6)
void CMulti::OnDropPlayer() {
    if (g_dropGuard) {
        return;
    }

    m_584 = 0;
    g_dropGuard = 1;
    i32 r = RunErrorDialog("MULTI_DROPPLAYER", (void*)&MultiDropPlayerCallback, 0);
    g_dropGuard = 0;
    g_sharedFlag = 0;

    switch (r) {
        case DISPATCH_RESET:
            Session()->ResetCmdBuffers();
            break;
        case DISPATCH_ABORT: {
            Session()->ResetCmdBuffers();
            HWND hwnd = NetGameMgr()->m_wnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
            break;
        }
        case DISPATCH_PLAYERLEFT:
            if (g_dropPlayerId != -999) {
                if (Peer()->FindPlayerById(g_dropPlayerId)) {
                    SendStat3(g_dropPlayerId, STAT_PLAYERLEFT_LOCAL, 1);
                }
            }
            SendNetStat(STAT_PLAYERLEFT, g_dropPlayerId, 1);
            AckDropPlayer(g_dropPlayerId);
            Session()->ResetCmdBuffers();
            break;
    }
}

// ===========================================================================
// CMulti::RunErrorDialog  @ 0x0bc250  - run the modal lobby dialog on the logic
// object (m_logic): pre-hook m_logic->m_60, run the 3-arg dialog, restore focus to
// m_logic->m_4->m_4, then ack the join failure. Returns the dialog result.
// ===========================================================================
RVA(0x000bc250, 0x55)
i32 CMulti::RunErrorDialog(char* tmpl, void* handler, i32 lparam) {
    if (!Mgr()) {
        return 2;
    }
    Mgr()->m_timer->DtorBody();
    i32 r = Mgr()->RunModalDialog(tmpl, handler, lparam);
    SetActiveAndFocus(Mgr()->m_gameWnd->m_hwnd);
    AckJoinFailure();
    return r;
}

// ===========================================================================
// CMulti::DropTimeout  @ 0x0bc2d0  - /GX: if a player has been silent past the
// throttle deadline, run the join-failure ack (rate-limited via g_ackThrottleDeadline), then
// look up the long-timeout slot, copy its host name into the session-name global
// (g_sessionName), and push the drop stat + OnDropPlayer.
// ===========================================================================
// @early-stop
// /GX EH + regalloc wall: the body is the complete, correct reconstruction (the
// throttle gate off g_ackThrottleDeadline/timeGetTime, the two FindSlot lookups, the slot host
// name copied into g_sessionName via the CString temp, then SendNetStat + OnDropPlayer).
// Retail keeps the 2nd FindSlot result live in eax across the CString-temp
// construction while our /O2 spills it to edi, and the EH-state funclet store order
// differs - structure + the call/branch chain match, register/EH scheduling does
// not. Deferred to the final sweep.
RVA(0x000bc2d0, 0xd2)
void CMulti::DropTimeout() {
    if (m_session->FindSlot(0x1388) == 0) {
        return;
    }
    if (g_ackThrottleDeadline < static_cast<u32>(timeGetTime())) {
        AckJoinFailure();
        g_ackThrottleDeadline = timeGetTime() + 0x3e8;
    }
    CNetCmdSlot* slot = m_session->FindSlot(0x2710);
    if (slot == 0) {
        return;
    }
    g_dropPlayerId = *(i32*)((char*)slot->m_desc + 0x18);
    CString nm;
    g_sessionName = *slot->BuildHostName(&nm); // slot->FUN_004bc3f0(&nm) -> &nm; g_sessionName = nm
    SendNetStat(0x40c, g_dropPlayerId, 1);
    OnDropPlayer();
}

// ---------------------------------------------------------------------------
// 0x0bc3f0: fill `out` with the slot's host name (delegated to the +0xc manager's
// 0x1f450 getter) and return it.
// @early-stop
// by-value-CString-return NRVO-temp wall: retail (and the identical-shape 0x1f450
// getter it forwards to) reserves + zeroes a stack temp (`push ecx; mov [esp+4],0`)
// from the CString-by-value return path; our signature is locked to `CString*` by the
// matched CMulti.cpp call site (`*slot->BuildHostName(&nm)`), so that temp can't be
// reproduced. Delegation + return-the-arg are otherwise exact. ~67%.
RVA(0x000bc3f0, 0x1e)
CString* CNetCmdSlot::BuildHostName(CString* out) {
    *out = (reinterpret_cast<GruntzPlayer*>(m_desc))->GetName();
    return out;
}

// ===========================================================================
// CMulti::AckJoinFailure  @ 0x0bc420  - if the join gate is armed
// (m_netGate && m_5bc && m_connected), push the join-failure stat flag.
// ===========================================================================
RVA(0x000bc420, 0x2b)
void CMulti::AckJoinFailure() {
    if (m_netGate && m_5bc && m_connected) {
        SendStatFlag(0x3f6, 1);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::SetupTcpIpConfig  (__thiscall; ret 0 args; /GX EH frame).
// The TcpIp-specific connection setup: fixes the config section to "TcpIp" with its
// timing defaults (m_cmdDelay=5/m_resend=0x3c), overrides them from the config
// store's "TcpIp_CmdDelay/_Resend" keys, latches the local player name into the
// channel table, then creates the local player (m_peer->CreatePlayer) and registers
// the local channel (RegisterChannelFrom). Returns whether the channel registered.
// @early-stop
// /GX CString-temp cluster wall: the config-key builds + GetInt reads, the
// channel-name latch, the CreatePlayer + RegisterChannelFrom tail and the local
// player latch are reproduced, but retail's EH-state cookie sequence over the
// scoped CString temps + their stack packing is not source-steerable. Same family
// as DetectConnectionConfig. Final sweep.
RVA(0x000bc460, 0x24e)
i32 CMulti::SetupTcpIpConfig() {
    m_598 = "TcpIp";
    m_5ac = 0;
    m_5a4 = 5;
    m_drainReload = 0x3c;

    Utils::RegistryHelper* cfg = NetGameMgr()->m_configStore;
    CString kDelay = m_598 + "_CmdDelay";
    CString kResend = m_598 + "_Resend";
    CString kDyn = m_598 + "_DynCmdDelay";
    i32 cd = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kDelay))), -1);
    i32 rs = cfg->GetValueDword(const_cast<char*>(static_cast<const char*>((kResend))), -1);
    if (cd != -1 && rs != -1) {
        m_5a4 = cd;
        m_drainReload = rs;
    }

    CNetChannel* ch0 = NetGameMgr()->m_channels;
    {
        CString name = GetString5a0();
        ch0->m_name = name;
    }
    ch0->m_slotId = 0;

    void* lp;
    {
        CString cn = ((GruntzPlayer*)ch0)->GetName();
        lp = (void*)Peer()->CreatePlayer((void*)static_cast<const char*>(cn), reinterpret_cast<i32>(g_emptyString), 0);
    }
    m_5bc = reinterpret_cast<i32>((CNetPlayerEntry*)lp);
    if (lp == 0) {
        ReportConnectFailed(0);
        return 0;
    }

    m_hostIndex = *(i32*)((char*)lp + 4);
    i32 chField = ch0->m_slotId;
    CString cn2 = ((GruntzPlayer*)ch0)->GetName();
    i32 ok = RegisterChannelFrom(cn2, chField, -1, m_hostIndex);
    return ok != 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::CreateLocalPlayer  (__thiscall; /GX EH frame).
// Registers the local player with the peer manager under the local name, latches
// its DirectPlay id (m_localPlayerId), blocks until the host admits it, and announces the
// join. Bails (reports + 0) when the peer rejects the player or the connect wait
// times out. The two name CString temps run under the /GX frame; the join packet's
// name field is filled with an inline strcpy.
// @early-stop
// reloc-masked + scheduling plateau (94.5%): the instruction stream is byte-faithful
// (GetString5a0 + CreatePlayer, the id latch, WaitForConnect, the full join-packet
// build, the inline strlen/rep-movs strcpy, SendStatFrom). The residual is non-
// steerable: the /GX unwind-cookie immediate (push 0x8 vs 0x0), a CString-buffer
// read kept in the return reg vs re-read from the temp slot, and the order MSVC
// schedules the adjacent packet byte-stores (0x63/0xf and the m_localPlayerId load). Final sweep.
RVA(0x000bc750, 0x151)
i32 CMulti::CreateLocalPlayer() {
    {
        CString name = GetString5a0();
        m_5bc = reinterpret_cast<i32>((CNetPlayerEntry*)Peer()
                    ->CreatePlayer((void*)static_cast<const char*>(name), reinterpret_cast<i32>(g_emptyString), 0));
    }
    if (LocalPlayer() == 0) {
        ReportConnectFailed(0);
        return 0;
    }

    m_hostIndex = LocalPlayer()->m_4;
    if (WaitForConnect() == 0) {
        return 0;
    }

    CNetJoinPacket pkt;
    memset(&pkt, 0, 0x28);
    pkt.m_0 = 0x80;
    pkt.m_4 = STAT_PLAYER_JOINED;
    pkt.m_8 = 1;
    pkt.m_9 = 0;
    pkt.m_a = 1;
    pkt.m_b = 0;
    pkt.m_c = 0x63;
    pkt.m_d = 0xf;
    pkt.m_e = 0;
    pkt.m_10 = m_hostIndex;
    {
        CString name = GetString5a0();
        strcpy(pkt.m_14, static_cast<const char*>(name));
    }
    SendStatFrom((CNetStatPacket*)&pkt, 0x28, 1);
    return 1;
}

// The shared MFC empty-string literal (0x6293f4); the empty group name handed to
// CreatePlayer. Home elsewhere; extern-only pin.

// ===========================================================================
// CMulti::OpenHostChannel  @ 0x0bc910  - /GX: latch the session params (m_5a4 /
// m_drainReload / m_levelIndex=1 / m_rngSeed=timeGetTime), create the session player
// from the host name (m_hostName via GetString5a0) through the +0x524 net gate, and -
// on success - register the channel from the resolved host record (m_hostIndex =
// player+0x4). A failed create reports the net error and returns 0. Returns whether
// the channel registered.
// ===========================================================================
RVA(0x000bc910, 0xf6)
i32 CMulti::OpenHostChannel(void* a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    if (a0 == 0) {
        return 0;
    }
    m_5a4 = a3;
    m_drainReload = a4;
    m_levelIndex = 1;
    m_rngSeed = timeGetTime();
    m_5bc = Peer()->CreatePlayer((void*)static_cast<const char*>(GetString5a0()), reinterpret_cast<i32>(g_emptyString), 0);
    if (m_5bc == 0) {
        ReportNetError(m_5bc);
        return 0;
    }
    m_hostIndex = (reinterpret_cast<i32*>(m_5bc))[1];
    return RegisterChannelFrom(reinterpret_cast<const char*>(a1), a2, -1, m_hostIndex) != 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::WaitForConnect  (__thiscall).
// Blocks (pumping the session) until the local player is admitted to the host's
// game or the attempt fails. Bails immediately if there is no DirectPlay
// interface (m_peer) or local player descriptor (m_localPlayer). Announces "connecting"
// (stat 0x415), clears the admit flag m_admitted, then loops: each pass times out at
// 60s or on Esc (-> status 0x8022, fail), pumps the receive queue, and reports +
// fails on any of the session-state flags (terminated / removed / closed / full
// / version-mismatch). Returns 1 once m_admitted latches (admitted), 0 on any failure.
// @early-stop
// tail-merge + regalloc wall (~74%): logic byte-faithful. Retail inlines the shared
// `xor eax,eax; pop..; ret` early-out epilogue at each guard site + holds the timeGetTime
// import ptr in ebp; cl tail-merges the identical zero-return epilogues and uses ebx.
// See identical-return-epilogue-tailmerge.md (topic:wall). Final sweep.
RVA(0x000bca50, 0x155)
i32 CMulti::WaitForConnect() {
    if (Peer() == 0) {
        return 0;
    }
    if (LocalPlayer() == 0) {
        return 0;
    }

    SendStatFlag(STAT_CONNECTING, 1);
    m_58c = 0;
    u32 start = timeGetTime();
    if (m_58c != 0) {
        return 1;
    }

    do {
        u32 now = timeGetTime();
        if (now > start + 60000 || (static_cast<i32>(GetAsyncKeyState(VK_ESCAPE)) & 0x80000000)) {
            ReportStatusId(0x8022, 0);
            return 0;
        }
        PollSession();
        if (m_sessionTerminated) {
            ReportVersionMsg("The game session has been terminated.", 0);
            return 0;
        }
        if (m_538) {
            ReportVersionMsg("You have been removed from the game by the host.", 0);
            return 0;
        }
        if (m_5ac) {
            ReportVersionMsg("This game is closed.", 0);
            return 0;
        }
        if (m_56c) {
            ReportVersionMsg("This game is already full.", 0);
            return 0;
        }
        if (m_570) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            return 0;
        }
    } while (m_58c == 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::AutoTuneCmdDelay  (__thiscall).
// Derives the command-timing config (m_cmdDelay/m_resend) from the measured peer
// ping. No-op (returns) once the config-loaded gate m_530 is set. Samples the
// ping (MeasurePing), divides it by 9 (the 0x88888889 reciprocal-multiply) and
// adds 2 -> a base command delay clamped to a minimum of 3; bumps it by 1 plus a
// further 1 when a secondary probe (ProbeLatency(0)) exceeds 2, stores it as
// m_cmdDelay, and picks a resend window (10 for <=5, else 30 or 20 for >8) before
// persisting the pair (ApplyCmdDelayDefaults via the 0-arg overload).
// @early-stop
// regalloc + integer-division-idiom wall (~68%): the logic is reproduced (the m_530
// gate, the /9 + 2 clamp(min 3), the ProbeLatency>2 bump, the m_5a4 store, the
// <=5/>8 resend selection, the two WriteCmdDelay persists) but retail keeps this in
// edi, folds the divide with a different /9 magic (0x38e38e39;shr1 vs cl's
// 0x88888889;shr4) and folds the (>2?1:0)+1 bump into a single lea - none steerable
// from C source. Final sweep (an optimizer-math idiom).
RVA(0x000bcc10, 0x8e)
void CMulti::AutoTuneCmdDelay() {
    if (m_530 != 0) {
        return;
    }

    u32 ping = static_cast<u32>(MeasurePing());
    i32 base = static_cast<i32>((ping / 9)) + 2;
    if (base < 3) {
        base = 3;
    }

    i32 probe = ProbeLatency(0);
    base += (probe > 2 ? 1 : 0) + 1;
    m_5a4 = base;
    if (base <= 5) {
        m_drainReload = 0xa;
        WriteCmdDelay(0);
        return;
    }
    m_drainReload = (base > 8 ? 0x14 : 0x1e);
    WriteCmdDelay(0);
}

// The game-settings singleton (_g_mgrSettings @0x64556c) - the same *0x64556c object
// modeled as CNetGameMgr (BuildRezPath/ShowModal/FindPlayer). External;
// the `call rel32` reloc-masks.

// The active net session the verify path polls (DAT_00648cf8, a CNetMgr*).
extern "C" CMulti* g_connectRptMgr; // 0x648cf8

// The shared empty-string literal CreateLocalPlayer hands to the peer's player
// factory (0x6293f4; DIR32 reloc-masked).

// The 0x11c-byte command-timing config blob SaveConfig builds and ships as stat
// 0x416 (the inverse of LoadConfig): a flag byte, the stat id, the config word,
// the two config-name strings (wsprintf'd in), then the four timing dwords.
// (CNetConfigBlob moved to <Net/NetPackets.h>.)

// ---------------------------------------------------------------------------
// CNetMgr::SaveConfig  (__thiscall; ret 4; /GX EH frame).
// Serializes the command-timing config into a 0x11c-byte stat-0x416 blob and
// ships it: the config word (m_5b0), the two config names formatted in with
// wsprintfA, and the four timing dwords (m_cmdDelay/m_resend/m_600/m_2d8). When a
// recipient is given it goes point-to-point (SendStatPairRaw), else it broadcasts
// (SendStatFrom). The two config-name CString temps run under the /GX frame.
// @early-stop
// reloc-masked plateau (96.4%): the instruction stream is byte-faithful (the
// memset, the |0x80 flag, every blob field store, both GetConfigName + cached
// wsprintfA-through-IAT formats, the four timing dwords, the recipient-vs-broadcast
// send). The residual is non-steerable: the /GX unwind cookie immediate (push 0xb
// vs 0x0), the wsprintfA IAT pointer symbol (__imp vs raw 0x6c44c0; reloc-masked),
// the CString-buffer read kept in the return reg vs re-read from the temp slot, and
// a tail `mov eax,1` materialization. Final sweep.
RVA(0x000bccd0, 0x141)
i32 CMulti::SaveConfig(CNetPlayerEntry* recipient) {
    CNetConfigBlob blob;
    memset(&blob, 0, sizeof(blob));
    blob.m_0 |= 0x80;
    blob.m_4 = STAT_CONFIG;
    blob.m_8 = m_5b0;
    {
        CString a = GetConfigNameA();
        wsprintfA(blob.m_nameA, static_cast<const char*>(a));
    }
    {
        CString b = GetConfigNameB();
        wsprintfA(blob.m_nameB, static_cast<const char*>(b));
    }
    blob.m_10c = m_5a4;
    blob.m_110 = m_drainReload;
    blob.m_114 = m_600;
    blob.m_118 = m_rngSeed;

    if (recipient != 0) {
        return SendStatPairRaw(recipient, &blob, 0x11c, 1);
    }
    return SendStatFrom((CNetStatPacket*)&blob, 0x11c, 1);
}

// ---------------------------------------------------------------------------
// CNetMgr::LoadConfig  (__thiscall).
// Copies the command-timing config out of a caller config blob into the manager:
// a config word (cfg+0x8 -> m_5b0), two name CStrings (cfg+0xc -> m_5b4,
// cfg+0x8c -> m_5b8) and four dwords (cfg+0x10c -> m_cmdDelay, +0x110 ->
// m_resend, +0x114 -> m_600, +0x118 -> m_2d8). No-op (0) on a null blob; else 1.
RVA(0x000bce80, 0x77)
i32 CMulti::LoadConfig(void* cfg) {
    if (cfg == 0) {
        return 0;
    }

    char* c = static_cast<char*>(cfg);
    m_5b0 = *(i32*)(c + 8);
    m_5b4 = static_cast<const char*>((c + 0xc));
    m_5b8 = static_cast<const char*>((c + 0x8c));
    m_5a4 = *(i32*)(c + 0x10c);
    m_drainReload = *(i32*)(c + 0x110);
    m_600 = *(i32*)(c + 0x114);
    m_rngSeed = *(i32*)(c + 0x118);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ResetPlayerCommands  (__thiscall).
// Flushes the resend buffers for one player's command slot. No-op unless
// connected (m_connected). Looks the player's slot up in the session (m_session); if found
// and not already reset (slot->m_resetGuard == 0), latches it, then for each command
// sequence number in the slot's window ([(seq0+1)..(seq0+1)+3] scaled by the
// per-command delay m_cmdDelay) re-dispatches the command through m_4's queue and
// drops it from the slot. Finally clears the slot's two command ranges.
// @early-stop
// schedule wall (92.8%): logic byte-faithful; retail reads m_4->m_6c later (into eax
// then ecx) and picks ecx/edx for the two ClearRange lea'd args where cl reads it
// earlier (into ecx) and picks edx/eax. Instruction-schedule permutation. Final sweep.
RVA(0x000bcf20, 0xaf)
i32 CMulti::ResetPlayerCommands(i32 id) {
    if (m_connected == 0) {
        return 0;
    }

    CNetCmdSlot* slot = Session()->FindCmdSlot(id);
    if (slot == 0) {
        return 0;
    }
    if (slot->m_resetGuard != 0) {
        return 0;
    }

    slot->Touch();
    i32 seq = (slot->m_baseSeq + 1) * static_cast<i32>(m_5a4);
    i32 end = seq + static_cast<i32>(m_5a4) * 3;
    for (; seq < end; seq++) {
        NetGameMgr()->m_6c->Dispatch(*slot->m_cmdHead, seq);
        slot->RemoveCmd(seq / static_cast<i32>(m_5a4));
    }
    slot->ResetTriple(slot->m_rangeA);
    slot->ResetTriple(slot->m_rangeB);
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::ReportAckLatency  (__thiscall).
// Thin wrapper: samples the current worst ack latency and ships it to the
// engine command dispatcher as stat 0x421.
RVA(0x000bd000, 0x19)
void CMulti::ReportAckLatency() {
    u32 latency = GetMaxAckLatency();
    SendNetStat(STAT_ACKLATENCY, latency, 0);
}

// ---------------------------------------------------------------------------
// CNetMgr::GetMaxAckLatency  (pure leaf; __thiscall).
// Returns the largest latency value across the four network slots. When the
// branch selector m_useChannelLatency is set the values come from the inline m_channelLatency[4] channel
// array (every entry counted); otherwise they come from the four per-player
// slots hanging off m_4 (stride 0x238), each counted only when BOTH of its
// "slot active" gate flags (m_164, m_170) are nonzero.
RVA(0x000bd030, 0x5d)
u32 CMulti::GetMaxAckLatency() {
    u32 max = 0;

    if (m_isHost != 0) {
        for (i32 i = 0; i < 4; i++) {
            if (m_channelLatency[i] > max) {
                max = m_channelLatency[i];
            }
        }
    } else {
        // The retail leaf addresses each slot NetGameMgr()-relative (base NetGameMgr(), disp +0x164/
        // +0x170/+0x37c), NOT via the +0x150 channel base - a distinct authentic
        // encoding, so the CNetPlayerSlot view is kept (removing it shifts base/disp).
        CNetPlayerSlot* slot = (CNetPlayerSlot*)NetGameMgr();
        for (i32 i = 0; i < 4; i++) {
            if (slot->m_164 && slot->m_170) {
                if (slot->m_37c > max) {
                    max = slot->m_37c;
                }
            }
            slot = (CNetPlayerSlot*)((char*)slot + 0x238);
        }
    }
    return max;
}

// ---------------------------------------------------------------------------
// CNetMgr::HandleVersionCheck  (__thiscall).
// Inspects a host packet's version pair (+0x18/+0x1c) against the two engine
// version locals. On any mismatch it latches m_versionMismatch, and - if a connection was
// already up (m_connected) - reports the canned "version mismatch" diagnostic and
// posts WM_COMMAND(0x8023) to the engine window; then fires stat 0x418 and
// sleeps 250ms before returning.
RVA(0x000bd0b0, 0x9a)
void CMulti::HandleVersionCheck(CNetVersionMsg* msg) {
    if (msg == 0) {
        return;
    }

    i32 mismatch = 0;
    if (g_localVersion != msg->m_1c) {
        mismatch = 1;
    }
    if (g_remoteVersion != msg->m_18) {
        mismatch = 1;
    }

    if (mismatch) {
        i32 wasConnected = m_connected;
        m_570 = 1;
        if (wasConnected) {
            ReportVersionMsg(
                "This version is not the same as the host computer's version of the game.",
                0
            );
            HWND hwnd = NetGameMgr()->m_wnd->m_hwnd;
            ::PostMessageA(hwnd, WM_COMMAND, 0x8023, 0);
        }
    }
    if (mismatch) {
        SendStatFlag(STAT_VERSIONMISMATCH, 1);
        Sleep(0xfa);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::AnnounceVersion  (__thiscall).
// Builds a 0x20-byte version-announce packet on the stack (flag byte, the
// CButeMgr config word, g_cfgWord, stat id 0x417, and the local/remote version
// pair) and ships it through the engine stat dispatcher as stat 0x417.
// @early-stop
// store-schedule wall (90%): logic byte-faithful (the 0x20 memset, the |0x80 flag,
// g_cfgWord/g_remoteVersion/param fields, the 0x417 send). Retail interleaves the
// packet field stores and the stack-arg-block setup at a different anchor than cl;
// an instruction-schedule permutation of the same store multiset. Final sweep.
RVA(0x000bd180, 0x66)
void CMulti::AnnounceVersion(i32 param) {
    CNetVersionPacket packet;
    memset(&packet, 0, sizeof(packet));

    packet.m_0 |= 0x80;
    packet.m_18 = g_remoteVersion;
    packet.m_c = g_cfgWord;
    packet.m_8 = g_buteMgrField4;
    packet.m_1c = g_localVersion;
    packet.m_10 = STAT_VERSIONPACKET;

    SendStatPacket(param, &packet, 0x20, 1);
}

// ===========================================================================
// CMulti::Vslot0b  @ 0x0bd210 (slot 11)  - /GX: the chat-input key handler. With the
// chat box up (m_hitTest->m_10) and connected, feed the key to the font-config input
// line (TypeChar); on a completed line longer than the 9-char command prefix, strip the
// prefix (Right(len-9)), broadcast the remainder as a chat line, and clear the input.
// With no chat box, forward to the base CPlay key handler (OnKeyCommand). Returns 1.
// ===========================================================================
RVA(0x000bd210, 0x14d)
i32 CMulti::Vslot0b(i32 arg0, i32 arg1) {
    if (m_hitTest && m_hitTest->m_10) {
        if (m_connected) {
            if (Mgr()->m_chatLog->TypeChar(arg0, arg1)) {
                CString line = Mgr()->m_chatLog->GetInputText();
                i32 n = line.GetLength();
                if (n > 9) {
                    CString text = line.Right(n - 9);
                    char buf[0x100];
                    strcpy(buf, text);
                    BroadcastChatLine(buf, 1, 1, 0);
                    Mgr()->m_chatLog->m_inputText.Empty();
                }
            }
        }
        return 1;
    }
    return CPlay::Vslot0b(arg0, arg1); // qualified: the BASE leg, not this override
}
