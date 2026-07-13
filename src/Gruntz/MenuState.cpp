// MenuState.cpp - CMenuState, the front-end menu game-state (C:\Proj\Gruntz).
// Split out of the former GameMode.cpp god-TU (per-class TU cut): CMenuState now owns
// its full method set here; the CState base + the CGameModeBase cleanup pair stay in
// GameMode.cpp, the sibling states (CCreditsState/CBootyState/CMultiBootyState) in
// their own TUs. The ~CMenuState `??1` (with the CState ctor) is the class's vtable +
// inline-virtual (Update) emission anchor - it stays in this TU with the rest of
// CMenuState. Its MENU asset loader (LoadAssets @0x9fe50) lives in MenuStateAssets.cpp.
//
// Functions, ascending retail-RVA order:
//   CMenuState::FormatHudText @0x01af70 - the 960-B HUD-text formatter switch.
//   LoadGameOptionsToDialog   @0x036860 - free __cdecl options-dialog writer.
//   ReadMenuOptionsDialog     @0x036a30 - free __cdecl options-dialog reader.
//   OnToggle*Option           @0x036d00.. - per-checkbox WM_COMMAND handlers.
//   ~CMenuState / ReleaseResources / StartMusic / StopMusicChain / FrameSlot28 /
//   Render / Vslot0c / Vslot0e / Vslot10  @0x08ce60.. - teardown + per-frame draw.
//   CMenuState::ReadyGate     @0x0a0d40 - the &&-chained ready/transition probe.
//   CMenuState::BuildVersionString @0x0a0d80 - the on-screen version banner.
//
// CMenuState : CState (RTTI .?AVCMenuState@@); the class body lives in
// <Gruntz/GameMode.h>. Only offsets / control IDs / code bytes are load-bearing;
// names are placeholders for the recovered engine identities.
#include <Gruntz/GameMode.h>
#include <Gruntz/BattlezData.h>           // the REAL stats object (was the CHudStats view)
#include <Gruntz/GruntzMgr.h>             // CGruntzMgr (the game-manager singleton; one true shape)
#include <Gruntz/SoundCueMgr.h>           // CSoundCueMgr (StartMusic/StopMusicChain ConfigureItem)
#include <Gruntz/WwdGameReg.h>            // g_gameReg (StartMusic music gate)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // CDDrawSubMgrLeafScan (ReleaseResources leaf keys)
#include <DDrawMgr/DDrawSubMgrPages.h>    // CDDrawSubMgrPages (FrameSlot28 flush)
#include <DDrawMgr/DDrawWorkerRegistry.h> // canonical CDDrawWorkerRegistry (was a GameMode.cpp local view)
#include <DDrawMgr/DDSurface.h>           // CDDSurface Flip (FrameSlot28)

#include <rva.h>
#include <Win32.h> // IsDlgButtonChecked + HWND (real USER32 header)
#include <Globals.h>

// ---------------------------------------------------------------------------
// The game-manager singleton (*g_gameReg) - the one true CGruntzMgr shape lives in
// <Gruntz/GruntzMgr.h>. (The options-dialog reader/writer family that also taps it
// is homed in src/Gruntz/VideoConfig.cpp, the options-dialogs TU.)
extern "C" {
    DATA(0x0024556c)
    extern "C" CGruntzMgr* g_gameReg; // = g_gameReg (the CGruntzMgr singleton)
}

// The Render menu-entity list (aliases g_actorList @0x245574) and the version-string
// RECT source (aliases g_versionRectL @0x245cc8). Declared in GameMode.h; the DATA
// binding lives here (the .cpp) so Render's absolute loads reloc-bind to the real RVAs.
// DEFINED here (with storage), not merely declared: 0x645574 had NO definition anywhere
// under EITHER of its two former names (?g_actorList@@3PAUAttractActorList@@A from
// demo/helpstate/attractstate, _g_645574 from here/splashstate/creditsstate), so every
// reference to it was an unresolved external. One symbol, one definition; the shared decl
// is in <Gruntz/AttractActor.h>.
extern "C" {
    DATA(0x00245574)
    AttractActorList* g_actorList = 0;
}
DATA(0x00245cc8)
extern "C" {
CGMVerRect g_645cc8; // .bss - zero at load
}

// ===========================================================================
// CMenuState per-frame + teardown methods (moved from the former GameMode.cpp
// god-TU). The ~CMenuState `??1` anchors the CMenuState vtable/inline-virtual
// emission in this TU.
// ===========================================================================

// FormatHudText's stats source IS the real CBattlezData (g_gameReg->m_scoreHud, the
// +0x7c HUD/score accumulator). The CHudStats view that used to sit here is GONE: its
// 13 GetC10..GetC40 "getters" were placeholder names for CBattlezData's own
// SumGroupField* methods - PHANTOMS (declared-only, no body, no rva) that no obj and no
// .LIB could ever define. Each was resolved from the binary by following the ILT thunk
// FormatHudText actually calls to its target rva, every one of which is an
// already-reconstructed, rva-bound CBattlezData method in the `battlezdata` unit:
//     GetC10 -> SumGroupField08 (0xfd2e0)   GetC38 -> SumGroupField34 (0xfd0b0)
//     GetC1c -> SumGroupField14 (0xfd290)   GetC24 -> SumGroupField1c (0xfd060)
//     GetC20 -> SumGroupField18 (0xfd240)   GetC40 -> SumGroupField3c (0xfd1f0)
//     GetC34 -> SumGroupField30 (0xfd010)   GetC2c -> SumGroupField24 (0xfd1a0)
//     GetC18 -> SumGroupField10 (0xfcfc0)   GetC3c -> SumGroupField38 (0xfd150)
//     GetC30 -> SumGroupField2c (0xfcf70)   GetC28 -> SumGroupField20 (0xfd100)
//     GetC14 -> SumGroupField0c (0xfcf20)
// The view's cached fields were the same object's: its m_c gate is CBattlezData's
// m_allDone (+0x0c) and its m_10 is m_score (+0x10).
// The (CBattlezData*) cast that used to sit here is GONE: CGameRegistry::m_scoreHud is
// TYPED now. The apparent conflict with Wormhole.cpp (which cast the SAME member to a
// CTeleMgrSub*) was never a conflict - CTeleMgrSub was a one-field view of THIS object,
// its m_28 being CBattlezData::m_28 (+0x28), the teleporter counter this very function
// reads back as its case-7 stat. Both casts were pointing at the same class all along.
#define STATS (g_gameReg->m_scoreHud)
#define STAT(getter, field)                                                                        \
    ((m_initOnce != 0 && STATS->m_allDone != 0) ? STATS->getter() : STATS->field)

// CMenuState::FormatHudText(buf, sel) (0x1af70): the 960-byte HUD-text formatter - an
// 8-case switch that sprintf()s the game clock (MM:SS via the imul-by-0x10624dd3
// divide-by-1000 then /60), score, and "%d of %d" progress into `buf`. Every stat is
// read via STAT(getter, field). The default case writes "???".
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// 960-byte switch body is CODE-BYTE-EXACT (verified llvm-objdump -dr base vs retail:
// every stat sibling-guard block, the MM:SS unsigned /1000-then-/60 divide magic, the
// "%d of %d" clamp, the 13 stats-thiscall getters, and the sprintf pushes all match;
// the ~24 g_gameReg loads are the retail A1 moffs32 form). Residual ~2.5% is the
// inline .rdata jump table (8 case addresses) + the reloc-typed format-string DIR32
// operands, neither source-steerable. ~97.5%.
RVA(0x0001af70, 0x3c0)
void CBootyState::FormatHudText(CString* buf, i32 sel) {
    switch (sel) {
        case 0: {
            u32 secs = (u32)(STAT(SumGroupField08, m_score) / 1000);
            buf->Format("%d:%2.2d", secs / 60, secs % 60);
            return;
        }
        case 1:
            buf->Format("%d", STAT(SumGroupField14, m_1c));
            return;
        case 2:
            buf->Format("%d", STAT(SumGroupField18, m_20));
            return;
        case 3: {
            i32 total = STAT(SumGroupField30, m_34);
            i32 cap = STAT(SumGroupField30, m_34);
            i32 cur = STAT(SumGroupField10, m_18);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 4: {
            i32 total = STAT(SumGroupField2c, m_30);
            i32 cap = STAT(SumGroupField2c, m_30);
            i32 cur = STAT(SumGroupField0c, m_14);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 5: {
            i32 total = STAT(SumGroupField34, m_38);
            i32 cap = STAT(SumGroupField34, m_38);
            i32 cur = STAT(SumGroupField1c, m_24);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 6: {
            i32 total = STAT(SumGroupField3c, m_40);
            i32 cap = STAT(SumGroupField3c, m_40);
            i32 cur = STAT(SumGroupField24, m_2c);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        case 7: {
            i32 total = STAT(SumGroupField38, m_3c);
            i32 cap = STAT(SumGroupField38, m_3c);
            i32 cur = STAT(SumGroupField20, m_28);
            if (cur >= cap) {
                cur = cap;
            }
            buf->Format("%d of %d", cur, total);
            return;
        }
        default:
            *buf = "???";
            return;
    }
}

// The options-dialog helper family (LoadGameOptionsToDialog @0x36860,
// ReadMenuOptionsDialog @0x36a30, the OnToggle* handlers @0x36d00.., and the
// ApiCallerStubs scroll helpers @0x36e50/0x36ec0/0x371e0) lives in its home TU
// per the interval dossier (#10c): src/Gruntz/VideoConfig.cpp (the options-
// dialogs TU). The real CMenuState core below stays here.

// ---------------------------------------------------------------------------
// CMenuState teardown + per-frame draw (moved from GameMode.cpp). The ~CMenuState
// `??1` anchors the CMenuState vtable/inline-virtual emission in this TU.
// ---------------------------------------------------------------------------

// The owning game-manager (CState::m_4, a real CGruntzMgr*) reached through the
// gamemode-local CGMOwner reduced view (same helper the sibling state TUs use).
static inline CGMOwner* Owner(CState* s) {
    return (CGMOwner*)s->m_4;
}

// The scalar-deleting dtor's operator delete (declared so /GX tracks the EH state).
void operator delete(void*);

// The renderer's DisposeWorkers @0x163c60 IS CDDrawWorkerList::ClearWorkers; local decl
// (CDDrawWorkerList has no shared header - defined in src/DDrawMgr/DDrawWorkerList.cpp).
class CDDrawWorkerList {
public:
    void ClearWorkers();
};

// The menu music controller (CMenuState+0x1bc): a player @+0x10 (real DirectSoundMgr,
// IsPlaying 0x1353f0 / CloneAndPlay 0x135660) with a draw-clock gate (last @+0x14,
// interval @+0x18). ConfigureItem is dispatched via CSoundCueMgr.
class DirectSoundMgr;
SIZE_UNKNOWN(CMenuMusic);
struct CMenuMusic {
    char m_pad00[0x10];
    DirectSoundMgr* m_10; // +0x10  player (real DirectSoundMgr)
    i32 m_14;             // +0x14  last draw-clock
    i32 m_18;             // +0x18  interval
};

// The draw-clock mirror + the reentrancy gate the menu music poll save/restores.
extern "C" u32 g_killCueClock; // draw-clock mirror
extern i32 g_sndEnabled;       // DAT_0061ab20 reentrancy gate

// StartMusic reads the game registry through its WwdGameReg view (m_10 presence gate,
// m_11c configured item); same 0x24556c singleton as g_gameReg, typed WwdGameReg.

// CMenuState::~CMenuState() (`??1`, 0x8ce60): run the menu teardown then chain the base.
// ReleaseResources/the base ~CState are statically bound in the dtor.
RVA(0x0008ce60, 0x55)
CMenuState::~CMenuState() {
    ReleaseResources();
}

// CMenuState::ReleaseResources() (slot 2 / +0x8): release the MENU resource set
// (name registry + leaf registry), dispose the worker list, free the menu UI
// object, then chain BaseCleanup. Also reached directly from ~CMenuState.
RVA(0x000a02c0, 0x7d)
void CMenuState::ReleaseResources() {
    // m_c re-read for each access (retail does not cache it); the null-guarded
    // block tests m_c once and reuses it for both the Free and DisposeWorkers.
    ((CDDrawWorkerRegistry*)m_c->m_10)->RemoveKeysEqual_155360("MENU", "_");
    ((CDDrawSubMgrLeafScan*)m_c->m_28)->RemoveKeysEqual_157c70("MENU", "_");
    if (m_c) {
        // The test value of m_c is reused for the leaf-registry access; the
        // worker-list dispose re-reads m_c fresh (retail does not cache it).
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            ((SoundStream*)r)->Stop();
        }
        ((CDDrawWorkerList*)m_c->m_rendererB)->ClearWorkers();
    }
    // m_1b4 IS cached (retail holds it in edi across the pre-delete + delete).
    CChatBox* ui = m_1b4;
    if (ui) {
        ui->~CChatBox();
        operator delete(ui);
        m_1b4 = 0;
    }
    ((CGameModeBase*)this)->BaseCleanup();
}

// CMenuState::StartMusic() (0xa05a0): if the menu music + the registry gate are
// live, push the configured item into the player on the draw-clock window, under
// a save/restored reentrancy gate.
RVA(0x000a05a0, 0x74)
void CMenuState::StartMusic() {
    if (m_1bc == 0) {
        return;
    }
    if (((WwdGameReg*)g_gameReg)->m_10 == 0) {
        return;
    }
    i32 saved = g_sndEnabled;
    i32 flag = saved;
    if (!saved) {
        flag = 1;
        g_sndEnabled = 1;
    }
    i32 item = ((WwdGameReg*)g_gameReg)->m_11c;
    CMenuMusic* mus = m_1bc;
    if (flag) {
        u32 clk = g_killCueClock;
        if (clk - mus->m_14 >= (u32)mus->m_18) {
            mus->m_14 = clk;
            ((CSoundCueMgr*)mus->m_10)->ConfigureItem(item, 0, 0, 1);
        }
    }
    if (!saved) {
        g_sndEnabled = saved;
    }
}

// CMenuState::StopMusicChain() (0xa0640): if the menu music is playing, request
// a fade-out stop, then spin the cursor/anim tick until playback ends.
RVA(0x000a0640, 0x6a)
void CMenuState::StopMusicChain() {
    if (m_1bc == 0) {
        return;
    }
    CMenuMusic* mus = m_1bc;
    if (!((DirectSoundMgr*)mus->m_10)->IsPlaying()) {
        return;
    }
    m_1bc->m_10->CloneAndPlay(0, 0x1f4, 1);
    if (!m_1bc->m_10->IsPlaying()) {
        return;
    }
    do {
        CViewPooledRes* r = ((CSoundRegistry*)m_c->m_28)->m_2c;
        if (r) {
            ((SoundDevice*)r)->PurgeVoiceList(-1);
        }
    } while (m_1bc->m_10->IsPlaying());
}

// CMenuState::FrameSlot28(int) (slot 10 / +0x28, 0xa06d0): flush + flip the menu
// view, stamp the start clock, run the music-stop chain, then busy-wait m_1b8 ms.
RVA(0x000a06d0, 0x5f)
i32 CMenuState::FrameSlot28(i32) {
    ((CDDrawSubMgrPages*)m_c->m_drawTarget)->Method_158ee0();
    m_c->m_drawTarget->m_10->m_2c->Flip(0);
    u32 start = timeGetTime();
    StopMusicChain();
    while (timeGetTime() < start + m_1b8)
        ;
    return 1;
}

// CMenuState::Render(): the front-end per-frame menu draw.
//   1. ENTITY UPDATE LOOP: for each e in g_entityList: e->Update();
//   2. SIX entity-flag scans (masks 0x80000000/0x40000000/0x20000000/0x10000000/
//      0x3 (byte)/0x100): the FIRST scan that finds a flagged entity fires the
//      matching no-arg method on the UI object m_1b4 and short-circuits to the
//      tail; the 0x100 scan, if its handler returns 0, also posts a WM_COMMAND
//      0x8036 before the tail.
//   3. TAIL: m_1b4->Step(g_645584); m_1b4->Pre(); DrawVersion({g_645cc8..d4});
//      m_1b4->Post();   return 1;
RVA(0x000a0750, 0x1d0)
i32 CMenuState::Render() {
    CGMEntityList* L = g_actorList;

    // per-entity Update pass (re-reads count each iter, like the target)
    for (i32 i = 0; i < L->m_count; i++) {
        L->m_data[i]->Update();
    }

    // six prioritized entity-flag scans, each firing a distinct UI handler
    i32 c;
    L = g_actorList;
    i32 n = L->m_count;
    for (c = 0; c < n; c++) {
        if ((u32)L->m_data[c]->m_2ac & 0x80000000) {
            m_1b4->OnFlag80000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_data[c]->m_2ac & 0x40000000) {
            m_1b4->OnFlag40000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_data[c]->m_2ac & 0x20000000) {
            m_1b4->OnFlag20000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if ((u32)L->m_data[c]->m_2ac & 0x10000000) {
            m_1b4->OnFlag10000000();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_data[c]->m_2ac & 0x3) {
            m_1b4->OnFlag00000003();
            goto tail;
        }
    }
    for (c = 0; c < n; c++) {
        if (L->m_data[c]->m_2ac & 0x100) {
            if (!m_1b4->OnFlag00000100()) {
                PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8036, 0);
            }
            goto tail;
        }
    }
tail:
    m_1b4->Step(g_645584);
    m_1b4->Pre();
    BuildVersionString(g_645cc8); // 0xa0d80 (the fake DrawVersion alias folded away)
    m_1b4->Post();
    return 1;
}

// CMenuState::Vslot0c @0xa0b90 (slot 12) - the front-end keydown dispatcher: the
// arrow keys drive the menu UI object's HitTest nav, RETURN/SPACE its Activate, and
// ESCAPE its page Switch(1); when ESCAPE's Switch returns 0 (top page) it clears the
// fade duration and posts WM_COMMAND(0x8027) to the app window. Always returns 1.
RVA(0x000a0b90, 0xc7)
i32 CMenuState::Vslot0c(i32 key, i32 arg2) {
    if (key == 0x28) {
        m_1b4->HitTest2();
    } else if (key == 0x26) {
        m_1b4->HitTest1();
    } else if (key == 0x27) {
        m_1b4->HitTest4();
    } else if (key == 0x25) {
        m_1b4->HitTest3();
    } else if (key == 0xd || key == 0x20) {
        m_1b4->OnFlag00000003();
    } else if (key == 0x1b) {
        if (m_1b4->OnFlag00000100() == 0) {
            m_1b8 = 0;
            PostMessageA(Owner(this)->m_4->m_4, 0x111, 0x8027, 0);
        }
    }
    return 1;
}

// CMenuState slot 14 (+0x38) / slot 16 (+0x40): the two mouse hit-test forwarders -
// each passes (arg2, arg3) to the menu UI object's HitTest0 (== CChatBox::HitTest0),
// discards its result, and returns 1. arg1 (the message/event id) is unused.
RVA(0x000a0ca0, 0x21)
i32 CMenuState::Vslot0e(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}
RVA(0x000a0ce0, 0x21)
i32 CMenuState::Vslot10(i32 arg1, i32 arg2, i32 arg3) {
    if (m_1b4) {
        m_1b4->HitTest0(arg2, arg3);
    }
    return 1;
}

// CMenuState::ReadyGate @0x0a0d40 - the &&-chained ready/transition probe: poll
// the active/ready gate (Vfunc3, slot 3); if ready, attempt the pending state
// commit (CommitState, the 0x1136 thunk); if that succeeds, run the activation
// poll (Vslot06, slot 6). A short-circuit chain - each early bail returns the
// failing call's (zero) result.
RVA(0x000a0d40, 0x24)
i32 CMenuState::ReadyGate() {
    i32 r = Vfunc3();
    if (r == 0) {
        return r;
    }
    // The middle probe is the base CState::Vslot07 slot, invoked statically (direct
    // rel32 to 0xfac70, via the ILT thunk) - was the fake CMenuState::CommitState.
    r = CState::Vslot07();
    if (r == 0) {
        return r;
    }
    return Vslot06();
}

// CMenuState::BuildVersionString (0xa0d80): format the on-screen version banner
// into a transient CString, append " (SPAWN MODE)" when the CD prompt latched the
// spawn install, then hand it to the shared HUD-message sprite helper into the
// caller-supplied RECT (the 4 args form the RECT by value). The build/patch field
// g_65160c selects the two- vs three-number version format.
DATA(0x00251608)
extern "C" i32 g_651608 = 0;
DATA(0x0025160c)
extern "C" i32 g_65160c = 0;
DATA(0x00251610)
extern "C" i32 g_651610 = 0;
// The shared HUD message-sprite helper (0x1154b0, glyphstr): push a transient text
// sprite carrying a CString into a RECT. Canonical signature is
// ?ShowHudMessage@@YAXPAUHudMsgSink@@HHHHHHHH@Z (1 sink ptr + 8 int words) - the
// text/rect ptrs ride the trailing int args (cast at the call, as in BootyStateActivate).
struct HudMsgSink;
void ShowHudMessage(
    HudMsgSink* sink,
    i32 text,
    i32 rect,
    i32 dur,
    i32 a,
    i32 b,
    i32 c,
    i32 d,
    i32 e
); // 0x1154b0
RVA(0x000a0d80, 0xd7)
void CMenuState::BuildVersionString(CGMVerRect r) {
    CString str;
    if (g_65160c == 0) {
        str.Format("Gruntz v%d.%d", g_651608, g_651610);
    } else {
        str.Format("Gruntz v%d.%d%d", g_651608, g_65160c, g_651610);
    }
    if (g_cdPromptResult) {
        str += " (SPAWN MODE)";
    }
    ShowHudMessage((HudMsgSink*)m_c, (i32)&str, (i32)&r, 0x64, 1, 0xff, 0xff, 0, 0);
}
