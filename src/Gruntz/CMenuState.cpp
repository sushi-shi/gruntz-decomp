// CMenuState.cpp - the front-end menu state (C:\Proj\Gruntz). Trace-discovered
// routines, defined in ascending retail-RVA order:
//   LoadGameOptionsToDialog @0x036860 - the free __cdecl options-dialog writer that
//       snapshots g_gameReg into the g_opt_* staging globals and pushes them into the
//       checkboxes/sliders (inverse of ReadMenuOptionsDialog).
//   ReadMenuOptionsDialog @0x036a30 - the free __cdecl options-dialog reader that
//       commits the menu/audio/video checkbox + slider controls into g_gameReg.
//   CMenuState::ReadyGate  @0x0a0d40 - the &&-chained ready/transition probe.
//
// CMenuState : CState (RTTI .?AVCMenuState@@); the class body + the bulk of its
// methods live in <Gruntz/GameMode.h> / src/Gruntz/GameMode.cpp. This TU adds the
// two trace-discovered routines. Only offsets / control IDs / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
#include <Gruntz/GameMode.h>

#include <rva.h>
#include <Win32.h> // IsDlgButtonChecked + HWND (real USER32 header)

// ---------------------------------------------------------------------------
// The game-manager settings singleton (*g_64556c, the CGruntzMgr settings) the
// options reader commits into - the SAME object + helper set CPlay::ApplyGameOptions
// (0x036be0) writes; modeled identically so every reloc pairs. The setters reach
// the option/input/audio sub-systems through the manager (ApplyOpt 0x492340,
// StoreInputFlag 0x4919d0, StoreInputState 0x491a10, all __thiscall via their ILT
// thunks; the m_48 sound object's XMIDI master-volume push at 0x138950). All
// external/no-body so the call rel32 displacements reloc-mask.
struct CGameMgrSettings {
    void ApplyOpt(i32 v);        // 0x492340  (thiscall)
    void StoreInputFlag(i32 v);  // 0x4919d0  CGruntzMgr::StoreInputFlag
    void StoreInputState(i32 v); // 0x491a10  CGruntzMgr::StoreInputState
    void ApplySpeechOpt(i32 v);  // 0x4923b0  (thiscall, the speech-enable commit)

    struct CSound {
        char p0[0x28];
        i32 m_28;                   // +0x28  gate (skip the XMIDI push when 0)
        void SetXMidiVolume(i32 v); // 0x138950 (thiscall on m_48)
        i32 GetXMidiVolume();       // 0x1389c0 (thiscall on m_48; the read counterpart)
    };

    char p00[0x10];
    i32 m_10; // +0x10  music-block checkbox (0x46d)
    i32 m_14; // +0x14  speech-block checkbox (0x471)
    char p18[0x48 - 0x18];
    CSound* m_48; // +0x48  sound object
    char p4c[0x100 - 0x4c];
    i32 m_100; // +0x100 sound-enable checkbox (0x475)
    char p104[0x118 - 0x104];
    i32 m_118; // +0x118 master-enable checkbox (0x455)
    i32 m_11c; // +0x11c music-volume slider (0x470)
    i32 m_120; // +0x120 sound-volume slider (0x476)
    i32 m_124; // +0x124 master/quality slider (0x478)
};

extern "C" {
    DATA(0x0024556c)
    extern CGameMgrSettings* g_mgrSettings; // = g_64556c (the CGruntzMgr singleton)
    DATA(0x0020ccc4)
    extern i32 g_videoResolutionMode;
    // The audio-mode gate globals (.data ints) the reader branches on: the master
    // mute (0x6455b4), the music-block disable (0x6455bc), the speech-block disable
    // (0x6455c0) - the SAME gates CPlay::ApplyGameOptions tests.
    DATA(0x002455b4)
    extern i32 g_gate_2455b4;
    DATA(0x002455bc)
    extern i32 g_gate_2455bc;
    DATA(0x002455c0)
    extern i32 g_gate_2455c0;

    // The options-dialog staging globals (.data ints): LoadGameOptionsToDialog
    // (0x036860) snapshots g_gameReg into them then pushes them into the controls;
    // CPlay::ApplyGameOptions (0x036be0) reads them back. SAME g_opt_* set CPlay names.
    DATA(0x0022bd64)
    extern i32 g_opt_22bd64;
    DATA(0x0022bd68)
    extern i32 g_opt_22bd68;
    DATA(0x0022bd6c)
    extern i32 g_opt_22bd6c;
    DATA(0x0022bd70)
    extern i32 g_opt_22bd70;
    DATA(0x0022bd84)
    extern i32 g_opt_22bd84;
    DATA(0x0022bdc4)
    extern i32 g_opt_22bdc4;
    DATA(0x0022bdc8)
    extern i32 g_opt_22bdc8;
    DATA(0x0022bdcc)
    extern i32 g_opt_22bdcc;
    DATA(0x0022bdd0)
    extern i32 g_opt_22bdd0;
    DATA(0x0022bdd4)
    extern i32 g_opt_22bdd4;
}

// Screen-resolution detector (0x036f30-2 sibling at 0x0363a0): reads g_gameReg's
// screen width/height (+0x94/+0x98) and returns the mode (3=1024x768, 2=800x600,
// else 1). A free __cdecl no-arg fn (loads its own g_gameReg, ignores ecx).
i32 DetectScreenResMode(); // 0x0363a0

// The combo/slider dialog-population helpers (external/no-body so the call rel32
// reloc-masks). LoadVideoResolutionConfig seeds the resolution combo; the slider
// setter (winapi_0371e0, __cdecl(hDlg, id, pos, page)) fills a scroll/slider ctrl.
void LoadVideoResolutionConfig(void* hDlg, i32 nIDCombo, i32 nSel); // 0x036f30
namespace ApiCallerStubs {
    void winapi_0371e0_GetDlgItem_SetScrollInfo(void* hDlg, i32 id, i32 pos, i32 page); // 0x0371e0
}

// The dialog scroll-position getter every slider funnels through (the ILT thunk
// 0x402595 -> winapi_036ec0_GetDlgItem_GetScrollInfo, __cdecl(hDlg, id)). Declared
// in its real ApiCallerStubs namespace so the mangled call symbol matches the RVA.
namespace ApiCallerStubs {
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(void* hDlg, i32 id); // 0x036ec0
}

// LoadGameOptionsToDialog @0x036860 - the inverse of ReadMenuOptionsDialog: snapshot
// the live g_gameReg settings into the g_opt_* staging globals (so a later Cancel can
// restore / Apply can commit them), then push each value into the options dialog's
// checkboxes (CheckDlgButton via the cached IAT slot 0x6c44b4) and sliders
// (winapi_0371e0). The video-resolution combo + caption go through
// LoadVideoResolutionConfig. A free __cdecl function (no `this`); bails if no manager.
RVA(0x00036860, 0x16f)
void LoadGameOptionsToDialog(HWND hDlg) {
    if (g_mgrSettings == 0) {
        return;
    }
    g_opt_22bd70 = g_mgrSettings->m_118;
    g_opt_22bd6c = g_mgrSettings->m_11c;
    g_opt_22bd84 = g_mgrSettings->m_10;
    g_opt_22bdc4 = g_mgrSettings->m_120;
    g_opt_22bdd4 = g_mgrSettings->m_100;
    g_opt_22bdcc = g_mgrSettings->m_48->GetXMidiVolume();
    g_opt_22bdd0 = g_mgrSettings->m_14;
    g_opt_22bd68 = g_mgrSettings->m_124;
    g_opt_22bd64 = g_mgrSettings->m_14;
    g_opt_22bdc8 = DetectScreenResMode();
    g_videoResolutionMode = DetectScreenResMode();

    CheckDlgButton(hDlg, 0x455, g_mgrSettings->m_118);
    LoadVideoResolutionConfig(hDlg, 0x52c, g_videoResolutionMode);
    CheckDlgButton(hDlg, 0x46d, g_mgrSettings->m_10);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(hDlg, 0x470, g_mgrSettings->m_11c, 0x50);
    CheckDlgButton(hDlg, 0x475, g_mgrSettings->m_100);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(hDlg, 0x476, g_mgrSettings->m_120, 0x50);
    CheckDlgButton(hDlg, 0x471, g_mgrSettings->m_14);
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(
        hDlg,
        0x472,
        g_mgrSettings->m_48->GetXMidiVolume(),
        0x64
    );
    ApiCallerStubs::winapi_0371e0_GetDlgItem_SetScrollInfo(hDlg, 0x478, g_mgrSettings->m_124, 0x64);
}

// ReadMenuOptionsDialog @0x036a30 - commit the front-end options dialog's control
// state into the game-manager settings: the master-enable checkbox (0x455), the
// video-resolution slider (0x52c, clamped 0..100), and - unless globally muted -
// the music/sound block (0x46d/0x470/0x475/0x476) and, when the speech channel is
// present, the speech block (0x471/0x472). The trailing master/quality slider
// (0x478) is always committed. A free __cdecl function (no `this`). The SAME
// commit idiom as CPlay::ApplyGameOptions, with dialog reads as the value source.
//
// @early-stop
// entropy tail (~99.9%): every instruction byte matches retail except (a) the
// IsDlgButtonChecked IAT reference - retail names the cached IAT pointer by its
// fixed VA 0x6c44b0, our <Win32.h> import emits the identical `mov edi,ds:[__imp]`
// against __imp__IsDlgButtonChecked@8 (win32-import scoring artifact, same bytes),
// and (b) a single ecx-vs-edx coloring of the m_100 manager reload (regalloc
// coin-flip). No real code divergence; no source lever flips either.
RVA(0x00036a30, 0x14e)
void ReadMenuOptionsDialog(HWND hDlg) {
    if (g_mgrSettings == 0) {
        return;
    }
    g_mgrSettings->m_118 = IsDlgButtonChecked(hDlg, 0x455);
    i32 res = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x52c);
    if (res >= 0 && res <= 100) {
        g_videoResolutionMode = res;
    }
    if (g_gate_2455b4 == 0) {
        if (g_gate_2455bc == 0) {
            g_mgrSettings->ApplyOpt(IsDlgButtonChecked(hDlg, 0x46d));
            i32 mv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x470);
            if (mv >= 0 && mv <= 100) {
                g_mgrSettings->StoreInputFlag(mv);
            }
            g_mgrSettings->m_100 = IsDlgButtonChecked(hDlg, 0x475);
            i32 sv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x476);
            if (sv >= 0 && sv <= 100) {
                g_mgrSettings->StoreInputState(sv);
            }
        }
        if (g_gate_2455b4 == 0 && g_gate_2455c0 == 0 && g_mgrSettings->m_48->m_28 != 0) {
            g_mgrSettings->ApplySpeechOpt(IsDlgButtonChecked(hDlg, 0x471));
            i32 pv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x472);
            if (pv >= 0 && pv <= 100) {
                g_mgrSettings->m_48->SetXMidiVolume(pv);
            }
        }
    }
    i32 qv = ApiCallerStubs::winapi_036ec0_GetDlgItem_GetScrollInfo(hDlg, 0x478);
    if (qv >= 0 && qv <= 100) {
        g_mgrSettings->m_124 = qv;
    }
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
    r = CommitState();
    if (r == 0) {
        return r;
    }
    return Vslot06();
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CGameMgrSettings);
