// Multi.cpp - the multiplayer / lobby game-state (C:\Proj\Gruntz). Recovered
// from RTTI (.?AVCMulti@@, vtable 0x5e9fe4) after the dynamic-this tracer
// mis-attributed the 0x08d270 / 0x0b6110-0x0bc420 cluster to CPlay: every method
// touches the networking/lobby sub-object block at member offsets > +0x510
// (m_netGate / m_connected / m_groupName / m_hostName / m_5bc / m_604) and the MULTI_JOIN /
// "Error: %s - %i" / "%s (%i)" lobby strings, which CPlay (layout tops at +0x510)
// has not got. CMulti : public CPlay, public CState (CHD numBaseClasses=3); the
// most-derived dtor walks down stamping CMulti -> CPlay -> CState vtables.
//
// Reconstructed in ascending retail-RVA order. Field names are placeholders;
// only the OFFSETS + the per-method call/branch structure are load-bearing
// (campaign carcass doctrine). Engine callees (SendNetStat / SendStatFlag / the
// m_logic logic object / the heap deleters / the MFC CString/CByteArray dtors) are
// external no-body fns -> their `call rel32` are reloc-masked.
#include <rva.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/SBI_RectOnly.h>
#include <Gruntz/FontConfig.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Dsndmgr/GruntzSoundZ.h>
#include <Gruntz/WorldSoundSet.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/Multi.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegistry.h> // g_64556c singleton (0x24556c) canonical view
#include <stdio.h>               // engine sprintf (reloc-masked)
#include <stdlib.h>              // srand (reloc-masked)
#include <Globals.h>

// ---------------------------------------------------------------------------
// Engine globals the session loop touches (re-declared TU-local with their
// retail .data addresses so the DIR32 operands reloc-mask).
// ---------------------------------------------------------------------------
DATA(0x002455fc)
extern "C" i32 g_6455fc; // 0x6455fc  cleared at session start
DATA(0x00244c54)
extern "C" i32 g_644c54; // 0x644c54  default cue wParam (= *host)
DATA(0x00245580)
extern "C" u32 g_645580; // 0x645580  draw clock
DATA(0x00245584)
extern "C" u32 g_645584; // 0x645584  delta cap
DATA(0x00245588)
extern "C" u32 g_645588; // 0x645588  accum clock

// The game-manager singleton + a divisor for the TITLE%d index.
DATA(0x0024556c)
extern "C" CGameRegistry* g_64556c; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
DATA(0x00245534)
extern "C" i32 g_645534; // 0x645534  title-index modulus

// The DirectPlay session-name CString global (assigned in StartTitle).
DATA(0x002473d8)
extern CString g_6473d8; // 0x6473d8

// ShowCursor Win32 import slot (PTR_ShowCursor_006c44c4) - typed pointer so the
// indirect `call [0x6c44c4]` reloc-masks.
typedef i32(WINAPI* ShowCursorFn)(i32);
DATA(0x002c44c4)
extern ShowCursorFn g_ShowCursor;

// The string registry lookup on CMulti::m_stateReg (returns a state pointer or 0). 0x0053c030
extern "C" void* RegistryFind(void* reg, char* key); // FUN_0053c030 (__cdecl-ish, see body)

// The player record OpenPlayer returns (stashed in m_netGate->m_player); its
// group-name accessor is read in StartTitle. 0x004b76a0. (The net-bind entry
// points Bind/Activate/OpenPlayer are now methods of CMultiReportGate in CMulti.h.)
class CMultiPlayer {
public:
    char* GroupName(); // 0x004b76a0
};

// The sub-window object at m_view->m_20: two thiscall ticks in Tick's present tail.
class CMultiTickWin {
public:
    void TickWinA(i32 now); // winapi_136e20
    void TickWinB(i32 now); // winapi_137ac0
};
// Tick's present-finish wait (cdecl free fn). 0x0013dfe0
extern void ActiveWait(i32 phase);

// The render-sub object reached via m_view->m_24->m_5c (thiscall). FUN_00563300.
class CMultiSubTick {
public:
    void SubTick(); // 0x00563300
};

// ---------------------------------------------------------------------------
// The CState sub-object dtor-view the most-derived ~CMulti walks: a virtual dtor
// whose auto vptr-restore stamps ??_7CState@@6B@ (0x5ea21c, name-matching
// config/vtable_names.csv) then runs the CState base teardown (BaseCleanup). ~CMulti
// keeps the explicit member teardown in retail order. (The CPlay sub-object teardown
// is CMulti's own CPlayDtorBody() - the ONE canonical CPlay <Gruntz/Play.h> can't be
// used as a sub-object dtor-view here because its ~CPlay would double-tear the CPlay
// members ~CMulti already unwinds; giving ~CMulti the ??_7CPlay restamp would require
// modeling CMulti : public CPlay for real, out of this CPlay-consolidation's scope.)
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
// in Multi.h; the old free-__stdcall extern here was a byte-shape placeholder. The
// Teardown call site relies on entry ecx still holding `this` - retail emits no
// mov ecx before the call - which the member spelling reproduces.)

// The engine heap free (CLobbyObjA/B teardown above pairs with it).
extern "C" void RezFree(void* p); // 0x001b9b82 (_RezFree, __cdecl)

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
    // cl's implicit vptr store (??_7CMulti) stamps here at dtor entry (CMulti's own
    // virtual dtor); no manual stamp is needed at the most-derived level.
    Teardown();
    // CMulti sub-object teardown (high block).
    m_604.~CByteArray();
    m_5b8.~CString();
    m_5b4.~CString();
    m_hostName.~CString();
    m_groupName.~CString();
    m_598.~CString();
    // CPlay/CState sub-objects (m_488, m_cueText, m_3a4[], m_startMarkers, m_1b4, ...)
    // are torn down by the compiler-chained ~CPlay -> ~CState base destructors now that
    // CMulti : CPlay. No manual base teardown here (that was the old flat-model dtor).
}

// ===========================================================================
// CMulti::Teardown  @ 0x0b6110  - drains the lobby state on teardown: if the
// join gate is fully armed (m_netGate && m_5bc && m_session && m_connected) push the two final
// stat updates, then free the two heap lobby sub-objects (m_session, m_attractOverlay), release
// the report gate object (m_netGate, via its vtable dtor), and hand m_590 back to the
// logic object (m_logic->m_110).
// ===========================================================================
RVA(0x000b6110, 0xc7)
void CMulti::Teardown() {
    if (m_netGate && m_5bc && m_session && m_connected) {
        SendNetStat(0x402, 0x4d2, 1);
        SendStatFlag(0x3ea, 1);
    }
    CNetSession2* p520 = m_session;
    if (p520) {
        p520->Teardown();
        RezFree(p520);
        m_session = 0;
    }
    if (m_netGate) {
        delete m_netGate;
        m_netGate = 0;
    }
    CLobbyObjA* p320 = (CLobbyObjA*)m_overlayActive;
    if (p320) {
        p320->Teardown();
        RezFree(p320);
        m_overlayActive = 0;
    }
    Mgr()->m_110 = m_590;
    CPlayDtorBody();
}

// FUN_00021bd0 is invoked both on `this` (CMulti) and on m_logic->m_5c; one symbol,
// so it is modeled on a neutral helper and reached by cast at both sites.

// ===========================================================================
// CMulti::StartSession  @ 0x0b6580  - resolve the chosen host, reseed the RNG +
// the frame timers, prime the per-slot config table (m_logic->m_150[0..3]), load the
// level, then re-arm everything for the live session. Returns 1 on success.
// ===========================================================================
RVA(0x000b6580, 0x1eb)
i32 CMulti::StartSession(i32 mode, i32 unused) {
    g_6455fc = 0;
    i32* host = Mgr()->ResolveHost(m_hostIndex);
    if (!host) {
        return 0;
    }
    g_644c54 = *host;
    srand(m_rngSeed);
    g_648cec = 0;
    g_645584 = 0;
    g_645580 = 0;
    g_645588 = 0;
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
    m_curSlotId = m_session->m_10 - 1;
    if (LoadLevelByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        CMultiMgrOptions* e = &Mgr()->m_150[i];
        if (e == 0) {
            return 0;
        }
        ((CBattlezMapConfig*)&e->m_inner)->FreeArrays();
        if (((CBattlezMapConfig*)&e->m_inner)
                ->LoadConfig((CLevelInfo*)Mgr(), i, e->m_10)
            == 0) {
            return 0;
        }
        if (e->m_14 && e->m_20) {
            ((CBattlezMapConfig*)&e->m_inner)->Clear_02ade0();
        }
    }
    this->RefreshSlotTable();
    srand(m_rngSeed);
    g_645584 = 0;
    g_645580 = 0;
    g_645588 = 0;
    m_savedClock = 0;
    m_5d0 = 0;
    m_drainTimer = 0;
    m_lastTime = timeGetTime();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = timeGetTime();
    m_curSlotId = m_session->m_10 - 1;
    m_574 = 0;
    Mgr()->m_5c->FreeNodes();
    m_session->StartTick();
    Mgr()->m_60->StartTitleHook();
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
    if (Mgr()->ProbeSession(mode, 0, 0) == 0) {
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
// CMulti::Tick  @ 0x0b6890  - the per-frame lobby pump: redraw, advance the
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
i32 CMulti::Tick() {
    m_drewThisFrame = 0;
    vtbl()->Redraw(this, 0, m_cursorX, m_cursorY);
    i32 oldT = m_lastTime;
    i32 t = timeGetTime();
    m_lastTime = t;
    m_frameDelta = t - oldT;
    m_accumTime += (t - oldT);
    i32 newId = m_session->m_10;
    if (m_curSlotId != newId) {
        m_curSlotId = newId;
        CMultiLogicList* lst = Mgr()->m_6c;
        CMultiLogicNode* node;
        if (lst->m_28 == 0) {
            node = 0;
        } else {
            node = lst->RemoveHead();
        }
        if (node) {
            node->m_c = 1;
            i32 v = m_curSlotId + (i32)m_5a4 * 2;
            i32 s = v < 0 ? -v : v;
            s &= 0x7f;
            node->m_6 = (u8)(v < 0 ? -s : s);
        }
        m_session->ArmSlot(node, (i32)(u8)((u8)m_5a4 << 1));
    }
    i32 dt = m_frameDelta;
    if ((u32)dt >= g_645584) {
        dt = (i32)g_645584;
    }
    m_packetsRcvd = m_session->Step(dt);
    m_packetsSent = 0;
    if ((u32)m_frameDelta < (u32)m_drainTimer) {
        m_drainTimer = m_drainTimer - m_frameDelta;
    } else {
        m_drainTimer = 0;
    }
    if (m_drainTimer == 0) {
        m_packetsSent = m_session->Drain();
        m_drainTimer = m_drainReload;
    }
    i32 fin = 0;
    if (m_session->IsBusy() && m_pollAbort == 0) {
        fin = 1;
    }
    vtbl()->PostRedraw(this);
    void* sub = *(void**)((char*)*(void**)((char*)m_c + 0x24) + 0x5c);
    if (sub) {
        ((CMultiSubTick*)sub)->SubTick(); // FUN_00563300 (thiscall on m_c->m_24->m_5c)
    }
    if (fin == 0) {
        if (m_session->IsStalled() == 0 && m_574 == 0) {
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
    CMultiTickWin* win = (CMultiTickWin*)*(void**)((char*)m_c + 0x20);
    if (win) {
        i32 now = timeGetTime();
        win->TickWinA(now); // winapi_136e20 (thiscall on m_c->m_20, arg now)
        win->TickWinB(now); // winapi_137ac0 (thiscall, arg now)
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
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc; // 0x6bf3bc  (= delta cap mirror)
DATA(0x0024558c)
extern "C" i32 g_64558c; // 0x64558c  ambient frame counter
DATA(0x00245590)
extern "C" u32 g_645590;       // 0x645590  stat timer 1
extern "C" i32 g_645594;       // 0x645594  (?g_645594@@3HA)
extern "C" i32 g_strikeThresh; // 0x645598 (?g_strikeThresh@@3HA)
DATA(0x0024559c)
extern "C" u32 g_64559c; // 0x64559c  stat timer 4
DATA(0x002455a0)
extern "C" u32 g_6455a0; // 0x6455a0  stat timer 5

// The FOREIGN redraw vfn host at (CMulti::m_view)->m_8: two thiscall slots pumped
// each frame; the rest are unreconstructed engine code. Honest model = a manual vptr
// into a typed vtable struct naming ONLY the two dispatched slots as 4-byte thiscall
// PMFs + char pad[], NO fake virtuals; each dispatch is ecx=this, no push (same as
// the pure-virtual form).
// Real polymorphic view: the two dispatched slots are real virtuals at their retail
// offsets (+0x24 = slot 9, +0x40 = slot 16); declared-only, so no vtable is emitted.
struct McObj {
    virtual void Slot00();
    virtual void Slot01();
    virtual void Slot02();
    virtual void Slot03();
    virtual void Slot04();
    virtual void Slot05();
    virtual void Slot06();
    virtual void Slot07();
    virtual void Slot08();
    virtual void Slot24(); // +0x24 (slot 9)
    virtual void Slot10();
    virtual void Slot11();
    virtual void Slot12();
    virtual void Slot13();
    virtual void Slot14();
    virtual void Slot15();
    virtual void Slot40(); // +0x40 (slot 16)
    void CallSlot24() {
        Slot24();
    }
    void CallSlot40() {
        Slot40();
    }
};
struct McHost { // CMulti::m_view
    char m_pad0[8];
    McObj* m_8; // +0x08
};

// Per-frame receivers (thiscall, out-of-line -> reloc-masked).
// CMultiMgr::m_48 IS the real CGruntzSoundZ (<Dsndmgr/GruntzSoundZ.h>): the former
// CMultiSoundZ view is dissolved - PlayByName (0x138840) / FindBank (0x138730) and
// the +0x1c inner (m_pCurrent) are CGruntzSoundZ's own members.
//
// CMultiSub68 (m_68): the single per-frame FX-driver object, merged with the former
// PBSub68 view (Step3017 poke + the +0x230 armed gate / Fire1398 / Reset2b85).
class CMultiSub68 { // CMultiMgr::m_68
public:
    void Step3017(i32 dt); // 0x3017
    char m_pad00_230[0x230];
    i32 m_armed;      // +0x230  armed gate
    void Fire1398();  // 0x00001398
    void Reset2b85(); // 0x00002b85
};
class CMultiSubDC { // CMulti::m_fxOverlay (the primary FX overlay)
public:
    i32 m_0; // +0x00  state
    char m_pad04_10c[0x10c - 0x04];
    i32 m_mode; // +0x10c mode
};

// @early-stop
// large-body regalloc/scheduling wall (~88%). Prologue, the m_594/m_logic->m_c/ready
// early-out, the shared-clock advance and frame size (sub esp,0x44) are byte-exact
// (llvm-objdump -dr); the residual is MSVC5's register/branch choices across the
// five stat-timer decay blocks + the m_view->m_8 vfn-host dispatch on this 670-byte
// body (0-in-ebp reuse, g_645584 single-load hoisting), not steerable from source.
RVA(0x000b6b40, 0x29e)
i32 CMulti::PumpA() {
    i32 ready = PumpAReady();
    if (m_594 == 0 && Mgr()->m_c != 0 && ready == 0) {
        PumpAReset();
        return 1;
    }
    g_645580 += 0x21;
    g_645588 += 0x21;
    g_645584 = 0x21;
    g_killCueClock = g_645580;
    g_6bf3bc = 0x21;
    if (m_ambientInitDone == 0) {
        if ((i64)(u32)g_645588 - *(i64*)&m_ambientTimerLo >= *(i64*)&m_ambientInterval) {
            char name[0x40];
            wsprintfA(name, "AMBIENT%d", PumpAIndex());
            if (g_64556c->m_14 != 0) {
                Mgr()->m_48->PlayByName(name, 1);
            } else {
                CGruntzSoundInnerZ* p = Mgr()->m_48->FindBank(name);
                if (p) {
                    Mgr()->m_48->m_pCurrent = p;
                }
                if (Mgr()->m_48->m_pCurrent) {
                    Mgr()->m_48->m_pCurrent->SetLoop(1);
                }
            }
            m_ambientInitDone = 1;
        }
    }
    Mgr()->m_6c->Step20b3(m_curSlotId % 128);
    m_session->Step2437();
    g_64558c++;
    u32 t1 = g_645590 ? g_645590 : 0x32;
    if (g_645584 < t1) {
        g_645590 = t1 - g_645584;
    } else {
        g_645590 = 0;
    }
    u32 t2 = g_645594 ? g_645594 : 0x64;
    if (g_645584 < t2) {
        g_645594 = t2 - g_645584;
    } else {
        g_645594 = 0;
    }
    u32 t3 = g_strikeThresh ? g_strikeThresh : 0xc8;
    if (g_645584 < t3) {
        g_strikeThresh = t3 - g_645584;
    } else {
        g_strikeThresh = 0;
    }
    u32 t4 = g_64559c ? g_64559c : 0x190;
    if (g_645584 < t4) {
        g_64559c = t4 - g_645584;
    } else {
        g_64559c = 0;
    }
    u32 t5 = g_6455a0 ? g_6455a0 : 0x1f4;
    if (g_645584 < t5) {
        g_6455a0 = t5 - g_645584;
    } else {
        g_6455a0 = 0;
    }
    ((McHost*)m_c)->m_8->CallSlot24();
    ((McHost*)m_c)->m_8->CallSlot40();
    Mgr()->m_68->Step3017(g_645584);
    ((CSBI_RectOnly*)((CMultiSubDC*)m_guts))->LoadDestructButtonSprite(g_645584);
    CMultiTickWin* win = (CMultiTickWin*)*(void**)((char*)m_c + 0x20);
    if (win) {
        i32 now = timeGetTime();
        win->TickWinA(now);
        win->TickWinB(now);
    }
    ((CTileTriggerContainer*)m_beginMarker)->FilterList2((void*)g_645584);
    Mgr()->m_70->UpdateDiagonals((i32)Mgr());
    if (ready == 0) {
        PumpAReset();
    }
    Mgr()->Step2d33();
    return 1;
}

// ===========================================================================
// CMulti::PumpB  @ 0x0b6e90  - the lobby/attract render pump. A light path when
// the game is idle (m_594==0 && m_logic->m_c!=0) drives just the compositor + the
// primary pane; otherwise the full frame runs: two deadline-gated FX, the
// m_view manager sub-tree (panes m_10/m_14, the +0xc vfn host, the +0x24 chain),
// the ambient overlays (m_fxOverlay/m_2e0/m_attractOverlay) and two int64 clock gates against
// g_645588. All callees are out-of-line (reloc-masked); PumpB's members not in
// CMulti.h are reached through dedicated view structs / documented offsets.
// ---------------------------------------------------------------------------

// The per-pane render target hung off m_view->m_4->{m_10,m_14}->m_2c (thiscall).
// A render pane (m_view->m_4->m_10 / ->m_14). m_14 also owns the palette blit.
// The +0xc vfn host: dispatched through vtable slot +0x34 (index 13).
class PBVfnHost {
public:
    virtual void s00();
    virtual void s01();
    virtual void s02();
    virtual void s03();
    virtual void s04();
    virtual void s05();
    virtual void s06();
    virtual void s07();
    virtual void s08();
    virtual void s09();
    virtual void s10();
    virtual void s11();
    virtual void s12();
    virtual void Blit34(void* a, void* b); // +0x34
};
// The m_view->m_24 chain and its +0x5c compositor target.
// The m_view manager sub-object tree.
class PBSub4 { // m_view->m_4
public:
    char m_pad00_10[0x10];
    CDDrawSurfacePair* m_10; // +0x10
    CDDrawSurfacePair* m_14; // +0x14
    void* m_18;              // +0x18
};
class PBMgr { // CMulti::m_view
public:
    void* m_0;
    PBSub4* m_4;            // +0x04
    CGameObjChain* m_8;     // +0x08  world object chain (VisitVisible arg)
    PBVfnHost* m_c;         // +0x0c
    char m_pad10_24[0x24 - 0x10];
    CGameLevel* m_24; // +0x24
};
// The output sink hung off CMultiMgr::m_54 (thiscall 2-arg blit).
// (CMultiMgr::m_68's FX-driver view PBSub68 is folded into CMultiSub68 above.)
class PBSub320 { // CMulti::m_attractOverlay (attract-mode overlay)
public:
    void Tick1fa0(u32 clock, i32 flag);    // 0x00001fa0
    void Render14dd(void* pane, RECT* rc); // 0x000014dd
};
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
    PBMgr* mgr = (PBMgr*)m_c;
    if (m_594 == 0 && Mgr()->m_c != 0) {
        StepInputA();
        mgr->m_24->VisitVisible(mgr->m_4->m_14, mgr->m_8);
        mgr->m_c->Blit34(mgr->m_4->m_14, mgr->m_4->m_18);
        ((CSBI_RectOnly*)((CMultiSubDC*)m_guts))->LoadMainStatusBarSprite();
        CDDrawSurfacePair* h = mgr->m_4->m_14;
        if (h == 0) {
            return;
        }
        StepGridWalk(g_645584);
        CopyRect(h);
        mgr->m_4->m_10->m_surface->Flip(0);
        return;
    }
    StepInputA();
    StepC();
    if (m_region0Gate != 0) {
        mgr->m_4->m_14->m_surface->Fill(0);
        ((CSBI_RectOnly*)((CMultiSubDC*)m_guts))->Deactivate();
    }
    if (m_worldReady == 0) {
        if (Mgr()->m_68->m_armed != 0) {
            Mgr()->m_68->Fire1398();
        } else {
            LoadScrollSpeedOptions();
        }
    }
    StepScroll();
    Mgr()
        ->m_54->Retune(
            ((CPlaneRender*)mgr->m_24->m_mainPlane)->m_84,
            ((CPlaneRender*)mgr->m_24->m_mainPlane)->m_88
        );
    if (m_region1Gate != 0) {
        NotifyVisibleEntities();
    } else {
        mgr->m_24->VisitVisible(mgr->m_4->m_14, mgr->m_8);
        mgr->m_c->Blit34(mgr->m_4->m_14, mgr->m_4->m_18);
    }
    ((CSBI_RectOnly*)((CMultiSubDC*)m_guts))->LoadMainStatusBarSprite();
    if (m_overlayActive != 0) {
        CMultiSubDC* fx = ((CMultiSubDC*)m_guts);
        if (fx->m_0 != 2 && fx->m_mode != 5) {
            RECT rc;
            if (fx->m_0 == 1) {
                SetRect(&rc, 20, 5, 140, 125);
            } else {
                i32 cx = g_64556c->m_modeH;
                i32 cy = g_64556c->m_modeW;
                rc.top = cx;
                SetRect(&rc, cy - 140, 5, cy - 20, 125);
            }
            PBSub320* ov = (PBSub320*)m_overlayActive;
            ov->Tick1fa0(g_645584, 0);
            ov->Render14dd(mgr->m_4->m_14, &rc);
        }
    }
    Mgr()->m_5c->Scroll(g_645584);
    CDDrawSurfacePair* h = mgr->m_4->m_14;
    if (h == 0) {
        return;
    }
    m_hitTest->LoadChatBoxSprite((i32)h);
    DrawDebugStats();
    Mgr()->m_68->Reset2b85();
    StepGridWalk(g_645584);
    CopyRect(h);
    if (m_worldReady != 0) {
        h->DrawBox((i32*)&m_hudRect, 0xff);
    }
    mgr->m_4->m_10->m_surface->Flip(0);
    PumpBRefresh2356(g_64556c, ((CMultiSubDC*)m_guts), m_region0Gate);
    if (mgr->m_24->m_mainPlane != 0) {
        ((CPlaneRender*)mgr->m_24->m_mainPlane)->CenterScrollB();
    }
    if (m_region0Gate != 0) {
        if ((i64)g_645588 - *(i64*)&m_region0TimerLo >= *(i64*)&m_region0Interval) {
            OnRegion2(0);
        }
    }
    if (m_region1Gate != 0) {
        if ((i64)g_645588 - *(i64*)&m_region1TimerLo >= *(i64*)&m_region1Interval) {
            OnRegion1(0);
        }
    }
}

// The m_view->m_4 view-reset target (thiscall). FUN_00558dc0.
class CMultiViewReset {
public:
    void Reset(); // 0x00558dc0
};

// ===========================================================================
// CMulti::StartTitle  @ 0x0b72c0  - /GX: pick a randomized "TITLE%d" backdrop,
// load it, reset the view + cursor, then bind the DirectPlay host (m_netGate) to the
// resolved descriptor, open the local player, and stash the host/group strings.
// Returns 1 on a fully-bound session.
// ===========================================================================
// @early-stop
// MFC CString temp + /GX EH wall: the body is the complete, correct
// reconstruction (the TITLE%d Format, the LoadTitleScreen gate, the view/cursor
// reset, the m_netGate Bind/Activate/OpenPlayer net chain, and the two CString stash
// helpers). Retail interleaves the EH-state stores (the [esp+...]=-1 funclet
// indices) and the inline strlen+rep-movs CString constructions with a register
// allocation our MSVC5 /O2 lowering reorders, and the empty-CString Init differs
// (the documented cstring-empty-init-version-divergence wall). >512 B; logic is
// correct, byte-match deferred to the final sweep.
RVA(0x000b72c0, 0x30b)
i32 CMulti::StartTitle() {
    Mgr()->m_9c = 0;
    m_588 = 1;
    if (!m_netGate) {
        return 0;
    }
    CResSource* saved = m_2c;
    CResSource* st = (CResSource*)RegistryFind(m_8, "STATEZ_ATTRACT");
    m_2c = st;
    if (!st) {
        return 0;
    }
    i32 idx = g_64556c->m_numRuns % g_645534 + 1;
    CString title;
    title.Format("TITLE%d", idx);
    if (LoadTitleScreen(title, 0, 0, 1, 0) == 0) {
        m_2c = saved;
        return 0;
    }
    ((CMultiViewReset*)((char*)m_c + 4))->Reset(); // (m_c->m_4)->Reset()
    void* vobj = *(void**)(*(void**)((char*)m_c + 0x1c));
    (*(void(__stdcall**)(void*))((char*)*(void**)vobj + 0x28))(vobj); // vfn +0x28(vobj)
    m_2c = saved;
    while (g_ShowCursor(1) < 0) {
    }
    if (!Mgr()->m_c0) {
        return 0;
    }
    CMultiLogicDesc* desc = Mgr()->m_c4;
    if (!desc) {
        return 0;
    }
    m_isHost = (desc->m_flags & 2) ? 1 : 0;
    i32 tmpl[4];
    tmpl[0] = g_60fab8[0];
    tmpl[1] = g_60fab8[1];
    tmpl[2] = g_60fab8[2];
    tmpl[3] = g_60fab8[3];
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
    i32 r = m_isHost ? RebindHostAlt() : RebindHost();
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

// CMulti::GetString59c (0x000b7a90) is now an inline member in the header.


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
            Mgr()->LogLine(buf);
        } else {
            Mgr()->LogLine(msg);
        }
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
    Mgr()->m_60->PreDialog();
    i32 r = Mgr()->RunDialog(tmpl, handler, lparam);
    MultiRestoreFocus(Mgr()->m_4->m_4);
    AckJoinFailure();
    return r;
}

// ===========================================================================
// CMulti::DropTimeout  @ 0x0bc2d0  - /GX: if a player has been silent past the
// throttle deadline, run the join-failure ack (rate-limited via g_648d14), then
// look up the long-timeout slot, copy its host name into the session-name global
// (g_6473d8), and push the drop stat + OnDropPlayer.
// ===========================================================================
// @early-stop
// /GX EH + regalloc wall: the body is the complete, correct reconstruction (the
// throttle gate off g_648d14/timeGetTime, the two FindSlot lookups, the slot host
// name copied into g_6473d8 via the CString temp, then SendNetStat + OnDropPlayer).
// Retail keeps the 2nd FindSlot result live in eax across the CString-temp
// construction while our /O2 spills it to edi, and the EH-state funclet store order
// differs - structure + the call/branch chain match, register/EH scheduling does
// not. Deferred to the final sweep.
RVA(0x000bc2d0, 0xd2)
void CMulti::DropTimeout() {
    if (m_session->FindSlot(0x1388) == 0) {
        return;
    }
    if (g_648d14 < (u32)timeGetTime()) {
        AckJoinFailure();
        g_648d14 = timeGetTime() + 0x3e8;
    }
    CLobbySlot* slot = m_session->FindSlot(0x2710);
    if (slot == 0) {
        return;
    }
    g_611d88 = *(i32*)((char*)slot->m_c + 0x18);
    CString nm;
    g_6473d8 = *slot->BuildHostName(&nm); // slot->FUN_004bc3f0(&nm) -> &nm; g_6473d8 = nm
    SendNetStat(0x40c, g_611d88, 1);
    OnDropPlayer();
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

// The shared MFC empty-string literal (0x6293f4); the empty group name handed to
// CreatePlayer. Home elsewhere; extern-only pin.
extern "C" char g_emptyString[];

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
    m_5bc = m_netGate->CreatePlayer((void*)(const char*)GetString5a0(), (i32)g_emptyString, 0);
    if (m_5bc == 0) {
        ReportNetError(m_5bc);
        return 0;
    }
    m_hostIndex = ((i32*)m_5bc)[1];
    return RegisterChannelFrom(a1, a2, -1, m_hostIndex) != 0;
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
            if (Mgr()->m_5c->TypeChar(arg0, arg1)) {
                CString line = Mgr()->m_5c->GetInputText();
                i32 n = line.GetLength();
                if (n > 9) {
                    CString text = line.Right(n - 9);
                    char buf[0x100];
                    strcpy(buf, text);
                    BroadcastChatLine(buf, 1, 1, 0);
                    Mgr()->m_5c->m_inputText.Empty();
                }
            }
        }
        return 1;
    }
    return OnKeyCommand(arg0, arg1);
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CLobbyObjA);
SIZE_UNKNOWN(CMulti);
SIZE_UNKNOWN(CMultiDialogHook);
SIZE_UNKNOWN(CState); // local dtor-view (stamps ??_7CState in ~CMulti)
SIZE_UNKNOWN(CMultiMgr);
SIZE_UNKNOWN(CMultiLogicDesc);
SIZE_UNKNOWN(CMultiMgrOptions);
SIZE_UNKNOWN(CSlotConfig);
SIZE_UNKNOWN(CMultiLogicList);
SIZE_UNKNOWN(CMultiLogicNode);
SIZE_UNKNOWN(CMultiPlayer);
SIZE_UNKNOWN(CMultiReportGate);
SIZE_UNKNOWN(CMultiStateBase);
SIZE_UNKNOWN(CMultiSub68);
SIZE_UNKNOWN(CMultiSubDC);
SIZE_UNKNOWN(CMultiSubTick);
SIZE_UNKNOWN(CMultiTickWin);
SIZE_UNKNOWN(CMultiViewReset);
SIZE_UNKNOWN(CMultiSlotView);
SIZE_UNKNOWN(CRefresh21bd0);
SIZE_UNKNOWN(McHost);
SIZE_UNKNOWN(McObj);
SIZE_UNKNOWN(PBListSink);
SIZE_UNKNOWN(PBMgr);
SIZE_UNKNOWN(PBSub320);
SIZE_UNKNOWN(PBSub4);
SIZE_UNKNOWN(PBVfnHost);

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
