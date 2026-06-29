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
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <stdlib.h> // srand (reloc-masked)

// ---------------------------------------------------------------------------
// Engine globals the session loop touches (re-declared TU-local with their
// retail .data addresses so the DIR32 operands reloc-mask).
// ---------------------------------------------------------------------------
DATA(0x002455fc)
extern "C" i32 g_6455fc; // 0x6455fc  cleared at session start
DATA(0x00244c54)
extern "C" i32 g_644c54; // 0x644c54  default cue wParam (= *host)
extern "C" i32 g_648cec; // 0x648cec
DATA(0x00245580)
extern "C" u32 g_645580; // 0x645580  draw clock
DATA(0x00245584)
extern "C" u32 g_645584; // 0x645584  delta cap
DATA(0x00245588)
extern "C" u32 g_645588;    // 0x645588  accum clock
extern "C" u32 g_648d14;    // 0x648d14  drop-throttle deadline
extern "C" i32 g_611d88;    // 0x611d88  saved dropped-player id
extern "C" i32 g_60fab8[4]; // 0x60fab8  the 16-byte net-bind template

// The game-manager singleton + a divisor for the TITLE%d index.
DATA(0x0024556c)
extern "C" void* g_64556c; // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c
DATA(0x00245534)
extern "C" i32 g_645534; // 0x645534  title-index modulus

// The DirectPlay session-name CString global (assigned in StartTitle).
DATA(0x002473d8)
extern CString g_6473d8; // 0x6473d8

// ShowCursor Win32 import slot (PTR_ShowCursor_006c44c4) - typed pointer so the
// indirect `call [0x6c44c4]` reloc-masks.
typedef i32(__stdcall* ShowCursorFn)(i32);
DATA(0x002c44c4)
extern ShowCursorFn g_ShowCursor;

// The string registry lookup on CMulti::m_8 (returns a state pointer or 0). 0x0053c030
extern "C" void* RegistryFind(void* reg, char* key); // FUN_0053c030 (__cdecl-ish, see body)

// The CNetMgr report-gate (m_524) net-bind entry points (reloc-masked thiscall).
class CMultiNetGate {
public:
    i32 Bind(i32* tmpl);        // 0x00578170  bind to the host template -> nonzero ok
    void Activate();            // 0x00578750
    i32 OpenPlayer(char* name); // 0x005786d0 -> player id (0 fail)
};

// The player record OpenPlayer returns (stashed in m_524->m_74); its group-name
// accessor is read in StartTitle. 0x004b76a0.
class CMultiPlayer {
public:
    char* GroupName(); // 0x004b76a0
};

// The sub-window object at m_c->m_20: two thiscall ticks in Tick's present tail.
class CMultiTickWin {
public:
    void TickWinA(i32 now); // winapi_136e20
    void TickWinB(i32 now); // winapi_137ac0
};
// Tick's present-finish wait (cdecl free fn). 0x0013dfe0
extern void ActiveWait(i32 phase);

// The render-sub object reached via m_c->m_24->m_5c (thiscall). FUN_00563300.
class CMultiSubTick {
public:
    void SubTick(); // 0x00563300
};

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
extern "C" i32 g_code;      // 0x6bf6fc  hr & 0xffff (the (%i) arg)
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

// External CMulti method: load the level for the chosen mode (thiscall, 2 args).
class CMultiLevelLoader {
public:
    i32 LoadLevelByMode(i32 mode, i32 flags); // 0x000ca200
};
// FUN_00021bd0 is invoked both on `this` (CMulti) and on m_4->m_5c; one symbol,
// so it is modeled on a neutral helper and reached by cast at both sites.
class CRefresh21bd0 {
public:
    void Refresh(); // 0x00021bd0
};

// ===========================================================================
// CMulti::StartSession  @ 0x0b6580  - resolve the chosen host, reseed the RNG +
// the frame timers, prime the per-slot config table (m_4->m_150[0..3]), load the
// level, then re-arm everything for the live session. Returns 1 on success.
// ===========================================================================
RVA(0x000b6580, 0x1eb)
i32 CMulti::StartSession(i32 mode, i32 unused) {
    g_6455fc = 0;
    i32* host = m_4->ResolveHost(m_5c0);
    if (!host) {
        return 0;
    }
    g_644c54 = *host;
    srand(m_2d8);
    g_648cec = 0;
    g_645584 = 0;
    g_645580 = 0;
    g_645588 = 0;
    m_1cc = 0;
    m_5d0 = 0;
    m_5d4 = 0;
    m_5dc = timeGetTime();
    m_5d8 = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_5e0 = 0;
    m_5e4 = timeGetTime();
    m_574 = 0;
    m_5cc = m_520->m_10 - 1;
    if (((CMultiLevelLoader*)this)->LoadLevelByMode(mode, 0) == 0) {
        return 0;
    }
    for (i32 i = 0; i < 4; ++i) {
        CMultiLogicEntry* e = &m_4->m_150[i];
        if (e == 0) {
            return 0;
        }
        ((CMulti*)((char*)e + 0x38))->FreeSlotInner();
        if (((CMulti*)((char*)e + 0x38))->LoadSlotConfig(m_4, i, e->m_10) == 0) {
            return 0;
        }
        if (e->m_14 && e->m_20) {
            ((CMulti*)((char*)e + 0x38))->ArmSlotInner();
        }
    }
    this->RefreshSlotTable();
    srand(m_2d8);
    g_645584 = 0;
    g_645580 = 0;
    g_645588 = 0;
    m_1cc = 0;
    m_5d0 = 0;
    m_5d4 = 0;
    m_5dc = timeGetTime();
    m_5d8 = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_5e0 = 0;
    m_5e4 = timeGetTime();
    m_5cc = m_520->m_10 - 1;
    m_574 = 0;
    ((CRefresh21bd0*)m_4->m_5c)->Refresh();
    m_520->StartTick();
    m_4->m_60->StartTitleHook();
    return 1;
}

// ===========================================================================
// CMulti::Connect  @ 0x0b67f0  - probe the chosen session on m_4; on failure
// report the netbind error, else run the connect-wait pump (m_57c reentrancy
// guard) and mark m_580 on success.
// ===========================================================================
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the body
// is the complete, correct reconstruction. Retail pins edi=0 once (xor edi,edi)
// and reuses it for the two arg pushes AND the m_580/m_534 stores, while our /O2
// emits immediate `push $0` + `mov [esi+N],$0`. Structure + offsets are
// byte-exact; only the constant-0 materialization differs, and no source lever
// (`int z=0;`, reorder) forces the pinning under /O2. Deferred to the final sweep.
RVA(0x000b67f0, 0x74)
i32 CMulti::Connect(i32 mode) {
    m_580 = 0;
    m_534 = 0;
    if (m_4->ProbeSession(mode, 0, 0) == 0) {
        m_4->ReportError(0x8005, 0x446);
        return 0;
    }
    m_57c = 1;
    i32 r = PumpA();
    m_57c = 0;
    if (r == 0) {
        return 0;
    }
    m_580 = 1;
    return 1;
}

// ===========================================================================
// CMulti::Tick  @ 0x0b6890  - the per-frame lobby pump: redraw, advance the
// frame clock off timeGetTime, step the lobby object, arm the next slot, then
// either keep ticking, flag out-of-sync, or finish + present.
// ===========================================================================
// @early-stop
// regalloc / scheduling wall: the body is the complete, correct reconstruction
// (the redraw vfn call, the timeGetTime delta math into m_5d8/m_5e0, the slot
// arm via m_520->ArmSlot, the Step/Drain clamp, then the busy/stall branch and
// the present tail). MSVC pins ebp=this and threads the timeGetTime import ptr
// through a callee-saved reg across the whole body; our /O2 lowering picks a
// different this/scratch allocation and reorders the m_5dx stores, so the
// instruction stream diverges despite identical logic. >512 B; deferred to the
// final sweep (no NEW idea closes it here).
RVA(0x000b6890, 0x21b)
i32 CMulti::Tick() {
    m_414 = 0;
    m_vtbl->Redraw(this, 0, *(i32*)((char*)this + 0x150), *(i32*)((char*)this + 0x154));
    i32 oldT = m_5dc;
    i32 t = timeGetTime();
    m_5dc = t;
    m_5d8 = t - oldT;
    m_5e0 += (t - oldT);
    i32 newId = m_520->m_10;
    if (m_5cc != newId) {
        m_5cc = newId;
        CMultiLogicList* lst = m_4->m_6c;
        CMultiLogicNode* node;
        if (lst->m_28 == 0) {
            node = 0;
        } else {
            node = lst->RemoveHead();
        }
        if (node) {
            node->m_c = 1;
            i32 v = m_5cc + (i32)m_5a4 * 2;
            i32 s = v < 0 ? -v : v;
            s &= 0x7f;
            node->m_6 = (u8)(v < 0 ? -s : s);
        }
        m_520->ArmSlot(node, (i32)(u8)((u8)m_5a4 << 1));
    }
    i32 dt = m_5d8;
    if ((u32)dt >= g_645584) {
        dt = (i32)g_645584;
    }
    m_2d0 = m_520->Step(dt);
    m_2d4 = 0;
    if ((u32)m_5d8 < (u32)m_5d4) {
        m_5d4 = m_5d4 - m_5d8;
    } else {
        m_5d4 = 0;
    }
    if (m_5d4 == 0) {
        m_2d4 = m_520->Drain();
        m_5d4 = m_5a8;
    }
    i32 fin = 0;
    if (m_520->IsBusy() && m_564 == 0) {
        fin = 1;
    }
    m_vtbl->PostRedraw(this);
    void* sub = *(void**)((char*)*(void**)((char*)m_c + 0x24) + 0x5c);
    if (sub) {
        ((CMultiSubTick*)sub)->SubTick(); // FUN_00563300 (thiscall on m_c->m_24->m_5c)
    }
    if (fin == 0) {
        if (m_520->IsStalled() == 0 && m_574 == 0) {
            if (m_528 != 0) {
                SendStatFlag(0x404, 1);
                OnOutOfSync();
                PumpA();
                m_5d4 = 0;
                return 1;
            }
            SendStatFlag(0x403, 1);
        }
        PumpA();
        m_5d4 = 0;
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

// External CMulti method: load the named title screen (5-arg thiscall). 0x004fa350
class CMultiTitleLoader {
public:
    i32 LoadTitleScreen(char* name, i32 a, i32 b, i32 c, i32 d); // 0x004fa350
};
// The m_c->m_4 view-reset target (thiscall). FUN_00558dc0.
class CMultiViewReset {
public:
    void Reset(); // 0x00558dc0
};

// ===========================================================================
// CMulti::StartTitle  @ 0x0b72c0  - /GX: pick a randomized "TITLE%d" backdrop,
// load it, reset the view + cursor, then bind the DirectPlay host (m_524) to the
// resolved descriptor, open the local player, and stash the host/group strings.
// Returns 1 on a fully-bound session.
// ===========================================================================
// @early-stop
// MFC CString temp + /GX EH wall: the body is the complete, correct
// reconstruction (the TITLE%d Format, the LoadTitleScreen gate, the view/cursor
// reset, the m_524 Bind/Activate/OpenPlayer net chain, and the two CString stash
// helpers). Retail interleaves the EH-state stores (the [esp+...]=-1 funclet
// indices) and the inline strlen+rep-movs CString constructions with a register
// allocation our MSVC5 /O2 lowering reorders, and the empty-CString Init differs
// (the documented cstring-empty-init-version-divergence wall). >512 B; logic is
// correct, byte-match deferred to the final sweep.
RVA(0x000b72c0, 0x30b)
i32 CMulti::StartTitle() {
    m_4->m_9c = 0;
    m_588 = 1;
    if (!m_524) {
        return 0;
    }
    i32 saved = m_2c;
    void* st = RegistryFind(m_8, "STATEZ_ATTRACT");
    m_2c = (i32)st;
    if (!st) {
        return 0;
    }
    i32 idx = *(i32*)((char*)g_64556c + 0x80) % g_645534 + 1;
    CString title;
    title.Format("TITLE%d", idx);
    if (((CMultiTitleLoader*)this)->LoadTitleScreen((char*)(const char*)title, 0, 0, 1, 0) == 0) {
        m_2c = saved;
        return 0;
    }
    ((CMultiViewReset*)((char*)m_c + 4))->Reset(); // (m_c->m_4)->Reset()
    void* vobj = *(void**)(*(void**)((char*)m_c + 0x1c));
    (*(void(__stdcall**)(void*))((char*)*(void**)vobj + 0x28))(vobj); // vfn +0x28(vobj)
    m_2c = saved;
    while (g_ShowCursor(1) < 0) {
    }
    if (!m_4->m_c0) {
        return 0;
    }
    CMultiLogicDesc* desc = m_4->m_c4;
    if (!desc) {
        return 0;
    }
    m_528 = (desc->m_flags & 2) ? 1 : 0;
    i32 tmpl[4];
    tmpl[0] = g_60fab8[0];
    tmpl[1] = g_60fab8[1];
    tmpl[2] = g_60fab8[2];
    tmpl[3] = g_60fab8[3];
    if (((CMultiNetGate*)m_524)->Bind(tmpl) == 0) {
        return 0;
    }
    ((CMultiNetGate*)m_524)->Activate();
    i32 id = ((CMultiNetGate*)m_524)->OpenPlayer(desc->m_8);
    if (id == 0) {
        return 0;
    }
    *(i32*)((char*)m_524 + 0x74) = id;
    CString hostName(*(char**)((char*)desc->m_c + 8)); // desc->m_c->m_8
    ClearString5a0(hostName);                          // clear m_5a0, return the temp
    char* grp = ((CMultiPlayer*)(void*)id)->GroupName();
    CString grpName(grp);
    ClearString59c(grpName); // clear m_59c, return the temp
    i32 r = m_528 ? RebindHostAlt() : RebindHost();
    return r ? 1 : 0;
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
// CMulti::DropTimeout  @ 0x0bc2d0  - /GX: if a player has been silent past the
// throttle deadline, run the join-failure ack (rate-limited via g_648d14), then
// look up the long-timeout slot, copy its host name into the session-name global
// (g_6473d8), and push the drop stat + OnDropPlayer.
// ===========================================================================
// @early-stop
// /GX EH + regalloc wall: the body is the complete, correct reconstruction (the
// throttle gate off g_648d14/timeGetTime, the two FindSlot lookups, the slot host
// name copied into g_6473d8 via the CString temp, then SendNetStat3 + OnDropPlayer).
// Retail keeps the 2nd FindSlot result live in eax across the CString-temp
// construction while our /O2 spills it to edi, and the EH-state funclet store order
// differs - structure + the call/branch chain match, register/EH scheduling does
// not. Deferred to the final sweep.
RVA(0x000bc2d0, 0xd2)
void CMulti::DropTimeout() {
    if (m_520->FindSlot(0x1388) == 0) {
        return;
    }
    if (g_648d14 < (u32)timeGetTime()) {
        AckJoinFailure();
        g_648d14 = timeGetTime() + 0x3e8;
    }
    CLobbySlot* slot = (CLobbySlot*)m_520->FindSlot(0x2710);
    if (slot == 0) {
        return;
    }
    g_611d88 = *(i32*)((char*)slot->m_c + 0x18);
    CString nm;
    g_6473d8 = *slot->BuildHostName(&nm); // slot->FUN_004bc3f0(&nm) -> &nm; g_6473d8 = nm
    SendNetStat3(0x40c, g_611d88, 1);
    OnDropPlayer();
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
