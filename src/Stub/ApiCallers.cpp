#include <Mfc.h> // real MFC (CObject/CString/CPtrList) + windows.h via afx.h (superset of Win32.h)

#include <DDrawMgr/DDSurface.h> // CDDSurface - FontRenderer::DrawGlyphRun's destination surface
#include <ddraw.h>              // real IDirectDrawSurface (surf->m_8->Unlock(0))
#include <Font/Font.h>          // FontRenderer/Font/Glyph/Rect/TextExtent (DrawGlyphRun 0x179e70)
#include <Gruntz/GruntzMgr.h>   // real CGruntzMgr (the 0x24556c game-manager singleton)
#include <Gruntz/Multi.h>       // real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/Play.h> // real CPlay (PickPlayOrPausedState's concrete return; m_stepCountdown@+0x510)
#include <Dsndmgr/GruntzSoundZ.h> // real CGruntzSoundZ (Dispatch-quiesce sound-bank flush: StopAndFlush)
#include <rva.h>
#include <smack.h> // the genuine RAD Smacker SDK (SMACKW32.DLL) - Smack handle + Smack* API
// smack.h pulls rad.h, which defines u8/u16/u32/u64/s8/s16/s32/s64 as object-like
// macros that shadow <Ints.h>'s typedefs; undo them so the rest of this TU keeps our
// aliases (matching-neutral: rad's u32==unsigned long is the same 4 bytes).
#undef u8
#undef u16
#undef u32
#undef u64
#undef s8
#undef s16
#undef s32
#undef s64
#include <string.h>
#include <stdio.h> // sprintf (0x11f890)
struct SaveTempRec;
i32 __stdcall CloseTempFile_e5550(SaveTempRec* r);
class CSaveGame {
public:
    i32 Save(i32 a, i32 b);
};

// Auto-generated API-caller stubs from docs/api-caller-name-plan.tsv.
// Greenfield only: tracked/already-tried and named-untracked library rows are intentionally excluded.
// One stub is emitted per RVA; rows with multiple API categories are merged.
//
// RESIDUAL TAXONOMY (this file is the drained backlog; ~74 of the original ~126
// stubs already re-homed to their real class TUs - see the inline "re-homed to ..."
// notes. The rest are the BLOCKED residual, each parked here for one of):
//   [CRT]      Win32 CRITICAL_SECTION wrappers 0x16c9c0/d0/e0/f0 - not game code;
//              STAY per the game-not-CRT matcher policy (no owning game class).
//   [SPINE]    the g_gameReg DlgProc cluster. Its receiver view was FOLDED onto the
//              canonical CGruntzMgr (<Gruntz/GruntzMgr.h>) - the DlgProcs stay here
//              (they are __stdcall CALLBACKs, not methods of a re-homeable class), but
//              they now deref the real singleton (m_gameWnd/m_saveSink/
//              PickPlayOrPausedState) instead of a placeholder CGameReg.
//   [ORPHAN]   real bodies on ANONYMOUS RVA-named placeholder hosts (CmdHost_*, Grid_*,
//              QlHost_*, EditAppendHost_*, Region_*, ...) whose true owning class is
//              not yet recovered; moving them would force-guess an owner - LEFT put.
//   [BIG]      >512B @early-stop bodies (0x14d00/0x1a700/0xe6020) kept at their
//              return-0/artifact plateau per the >512B REVERT rule (final sweep).
//              (0x179e70 reconstructed as FontRenderer::DrawGlyphRun and re-homed
//              to src/Font/Font.cpp.)
//   [ORPHAN-2] 0x0394b0 (ClickHost hit-test) - the worklist's CGruntzMgr attribution
//              was a thunk-band mis-chase; real owner unrecovered, so LEFT put (see it).

// The game-manager singleton at RVA 0x24556c is the canonical CGruntzMgr
// (<Gruntz/GruntzMgr.h>): the old per-TU CGameReg / GameWnd placeholder views were
// folded onto it. The DlgProcs below touch only its window (m_gameWnd->m_hwnd, the
// base CGameWnd), its +0x58 save-sink slot (m_saveSink), and PickPlayOrPausedState()
// @0x92990 (the FindStateById(3) accessor). Reloc-masked DATA symbol.
DATA(0x0024556c)
extern CGruntzMgr* g_gameReg;

// (The Miles Sound System (AIL) imports + g_ailMidiDriver/g_midiSeqCounter/
// g_ailDriver64, and the RNG/coin state g_rngSeeded/g_rngState/g_coinRolled/
// g_coinValue, moved with the MIDI sequence + LCG helpers to
// src/Dsndmgr/GruntzSoundZ.cpp and src/Gruntz/Random.cpp.)

// USER32 entry points reached through game-owned IAT-style function pointers
// (ff 15 [ptr]); g_pSendMessageA is the same global BattlezDlgRow.cpp binds.
DATA(0x002c4520)
extern HWND(WINAPI* g_pGetFocus)();
DATA(0x002c44a4)
extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM);
DATA(0x002c44f0)
extern BOOL(WINAPI* g_pInvalidateRect)(HWND, const RECT*, BOOL);

namespace ApiCallerStubs {
    // Fake placeholder host: these ApiCaller stubs are __thiscall (disasm shows
    // they take `this` in ecx) but their real owning class isn't recovered yet.
    // Membership surfaces the implicit `this` + __thiscall ABI; explicit args are
    // the N/4 from the callee's `ret N`.
    struct ThisStubOwner {
        i32 winapi_015fe0_SendMessageA(i32);
        i32 winapi_0c7ec0_timeGetTime(i32, i32, i32);
        i32 winapi_0ecc90_IntersectRect();
        i32 winapi_0ed9f0_PtInRect();
        i32 winapi_0f0e20_IntersectRect_PtInRect();
        i32 winapi_0f42f0_PtInRect();
        i32 winapi_0f60f0_IntersectRect();
        i32 winapi_136fe0_timeGetTime(i32, i32, i32, i32, i32, i32);
        i32 winapi_13f460_CopyRect(i32, i32);
        i32 winapi_1480a0_timeGetTime();
        i32 winapi_1485b0_CreateDCA_DeleteDC_GetSystemPaletteEntries();
        i32 winapi_153ff0_CopyRect(i32, i32);
        i32 winapi_154750_CopyRect(i32, i32);
        i32 winapi_168080_SetRect(i32, i32, i32, i32, i32, i32, i32, i32);
        i32 winapi_17c3f0_ShowCursor(
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32,
            i32
        );
        i32 winapi_17fe00_timeGetTime(i32);
        i32 winapi_1804a0_PtInRect(i32);
        i32 winapi_1d4a18_FreeLibrary();
        i32 thirdparty_138c20_AIL_allocate_sequence_handle_4_AIL_init_sequence_12_AIL_(
            i32,
            i32,
            i32
        );
        i32 thirdparty_17c8e0_SmackGoto_8_SmackWait_4(i32, i32);
        i32 thirdparty_17caa0_SmackDoFrame_4_SmackNextFrame_4_SmackToBuffer_28();
        void BootyState_OnActivate2_vfunc8();
        void LoadCreditzAssets2();
        void BuildWorldLevelKey(i32);
        void LoadBombGruntRunConfig2();
        void LoadFreezeSpellAssets();
        void LoadFinishLevelSprite(i32);
        void LoadMonologoSprite();
        void LoadStateImages_vfunc8();
        void LoadLevelPreviewScreen();
        void LoadGameAssetNamespaces(i32, i32, i32);
        void UpdateDestructButtonStatusBar2(i32);
        void DebugPrintf();
        void InitConnectRecord(i32);
        void WriteProfileValue(i32, i32, i32);
        void ReadProfileValue(i32, i32, i32, i32);
    };

    // ---- Proximity-attributed owners (HIGH, both-sides RVA bracket;
    // docs/tu-spatial-structure.md). These stubs were ThisStubOwner;
    // their real classes live in their own TUs - these are minimal placeholder
    // hosts so each stub files under its attributed class (matching-neutral). ----
    struct CGrunt {
        i32 winapi_057db0_IntersectRect();
        void LoadGruntCombatAnimations(i32, i32, i32, i32, i32, i32, i32, i32);
    };
    // (The old proximity-host `struct CMulti` here is gone: this TU now includes the
    // real <Gruntz/Multi.h>, so CMulti refers to the real multiplayer game-state class.)

    // (0xcd00/0xcd70/0x19f50/0x15cbe0 RNG helpers re-homed to Rng in
    // src/Gruntz/Random.cpp.)

    // (0x14720 CmdHost_014720::Key re-homed to CAttract::Vslot0c in Attract.cpp.)

    // (0x14d00 winapi_014d00 -> BattlezSetupDlgInit in src/Gruntz/Dialogs.cpp; 0x183f0
    // DlgHost::Pick0183f0 -> DlgHost_183f0::PickIfSelected in src/Gruntz/OrphanLeaves.cpp;
    // 0x1a700 winapi_01a700 -> LevelMsgHudDriver in src/Gruntz/GameMode.cpp - all
    // RVA-homed to their linker-layout TUs.)

    // (0x15cc0/d00/d30/d70 listbox helpers re-homed to CBattlezDlg in Dialogs.cpp.)

    // (0x19f50 RNG helper re-homed to Rng::RangeStd in src/Gruntz/Random.cpp.)

    // __thiscall(): if the cached key (m_1b8) is 0xc7, post a 0x8023 command. Returns 1.
    struct KeyHost_01f8a0 {
        char m_pad0[0x1b8];
        i32 m_1b8; // +0x1b8
        i32 Check();
    };
    // @orphan: only caller is the adjacent unrecovered fn @~0x1f8d0 (no named/attributable
    // caller, no vtable ref) - owning class of the +0x1b8 key cache unrecovered.
    RVA(0x0001f8a0, 0x30)
    i32 KeyHost_01f8a0::Check() {
        if (m_1b8 == 0xc7) {
            PostMessageA(g_gameReg->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
        }
        return 1;
    }

    // (0x2b340 ClipHost::Clip re-homed to ApiMisc in src/Gruntz/ApiMiscHelpers.cpp.)

    // (0x38150 find-and-select re-homed to CMultiSlotList::Method3396 in Dialogs.cpp.)

    // (0x38220 winapi_038220 -> GetSelItemData in src/Gruntz/MultiStartDlgRoster.cpp,
    // a CMultiStartDlg roster listbox helper.)

    // (0x39440 CmdHost_039440::Key re-homed to CCreditsState::Vslot0c in GameMode.cpp.)

    // (0x394b0 ClickHost_0394b0::OnClick -> src/Gruntz/GameMode.cpp; 0x77df0
    // Grid_77df0::FindNearest -> src/Gruntz/Brickz.cpp; 0x8e3a0 RectQuery_08e3a0::GetRect
    // -> src/Gruntz/GruntzMgr.cpp - all RVA-homed to their linker-layout TUs, with their
    // placeholder view structs carried alongside.)

    // (0x8c380 RectHost::Set re-homed to ApiMisc in src/Gruntz/ApiMiscHelpers.cpp.)


    // (0x90220 Post re-homed to src/Gruntz/GruntzMgr.cpp as the real CGruntzMgr::Post -
    // the WM_COMMAND 0x807f advance dispatch on the game-manager singleton (reached by
    // CPlay::Dispatch @0xcfbd0 as m_4->Post via the CState owner back-ptr). The old
    // CmdHost_090220/WndOwner_090220 views folded onto CGruntzMgr; the +0x48/0x54/0x60
    // sub-managers are CWorld::m_48/m_54/m_60 in <Gruntz/Play.h>.)

    // (0x92710 Quickload re-homed to src/Gruntz/GruntzMgr.cpp as the real
    // CGruntzMgr::Quickload - HandleCommand (0x862f0) calls it on `this`=CGruntzMgr;
    // the m_58/m_5c/m_60 subs are the canonical m_saveSink/m_chatLog/m_timer, and the
    // Fallback (0x29a0->0x92500) is CGruntzMgr::RunLoadGameDialog.)

    // (0x92ab0 numeric settings DlgProc + its 12 g_dlgVal_* field caches re-homed to
    // src/Gruntz/GruntzCommand.cpp - the command dialog CGruntzCommand::ApplyOne /
    // ApplyMask drive.)

    // (0x94bc0 VrHost::Validate re-homed to src/Gruntz/TerrainTileLoader.cpp
    // (ValidateHost) - the window-validate poll CTerrainTileLoader::Load drives.)

    // (0x953f0 CmdHost_0953f0::Key re-homed to CHelpState::Vslot0c in HelpState.cpp.)

    // (0x9dff0 GruntzLoadGameDlgProc + 0x9e390 LoadGameCommand re-homed to
    // src/Gruntz/LoadGameMenu.cpp - the in-game "GAME_LOAD" modal dialog its opener
    // CGruntzMgr::RunLoadGameDialog (@0x92500) drives; the load-side sibling of
    // SaveGameMenu.cpp. The DlgFallback_215d thunk decl folded into a direct
    // LoadGameCommand call, g_dlg645ca4 renamed g_dlgLoadSink (typed CSaveGame*).)

    // (0xc2980 winapi_0c2980 -> SetListCurSel in src/Gruntz/MultiStartDlgRoster.cpp,
    // a CMultiStartDlg roster listbox helper.)

    // (0xc2cb0 TimerDlg::OnInitDialog re-homed to AreaMgr.cpp (AreaTimerDlg): the
    // area/zone dialog CAreaMgr::Dispatch drives.)

    // (0xc2ce0 EditAppendHost::Append, 0xc3e30 OnReset, 0xc4230 UpdatePlayers,
    // 0xc4ee0/f30/f80/fd0 SelHost::Update0..3, 0xc50f0 RosterHost::Toggle re-homed to
    // the real CMultiStartDlg in src/Gruntz/MultiStartDlgRoster.cpp - the whole
    // 0xc2000-0xc5000 method band drives ONE multiplayer-start dialog; the per-fn
    // SelHost/RosterHost/BattlezDlg_c4230/EditAppendHost host views were placeholder
    // duplicates of it. See that file's header for the CNetGameDlg/CNetConnCoord
    // same-dialog dedup note.)

    // (0x0cfbd0 re-homed to src/Gruntz/Play.cpp as the real CPlay::Vslot15 - vtable
    // slot 21 (override of CState; ??_7CPlay@@6B@+0x54 -> ILT thunk 0x1c17). The
    // "unidentified owner" was CPlay: m_1c=CState::m_levelIndex, m_c=CState::m_c
    // (CSpriteFactoryHolder; ->m_28 CSndHost ->m_2c SoundStream::Stop @0x137a80),
    // m_4=CState::m_4 (CGruntzMgr) whose m_48/m_54/m_60 are the CWorld sound-bank/
    // draw/reset subs, and m_1bc/m_1c0 are carved CPlay members. Post = CGruntzMgr::
    // Post (re-homed above). PostMessageA goes via the cached g_pPostMessageA ptr.)

    // (0xd00a0 BlitHost::Show re-homed to src/Io/SaveGame.cpp (BlitHost) - the
    // save-flow "show" blit reached from the savegame temp-file path.)

    // (0xd7220 ActionHost::Begin re-homed to src/Gruntz/UserLogic.cpp
    // (ActionBeginHost) - the one-shot begin-action on a game-object action state
    // that CUserLogic::LoadGruntTypeTable drives.)

    // (0xda200 coin-flip re-homed to Rng::CoinFlip::Flip in src/Gruntz/Random.cpp.)

    // (0xde590 CmdHost::Cancel + its g_flag64c69c gate re-homed to
    // src/Gruntz/LevelPreview.cpp (PreviewCancelHost) - the preview-cancel
    // command host CPreviewState::Tick / LoadLevelPreviewScreen drive.)

    // SetDlgItemTextA helper defined below (RVA 0xe4850, reached via thunk 0x103c).
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item);
    // The optional info-line text shown on WM_INITDIALOG (DAT_0064c864).
    DATA(0x0024c864)
    extern char* g_dlgInfoText;

    // __stdcall DialogProc: OK/Cancel close the dialog; WM_INITDIALOG fills a line.
    RVA(0x000e3b20, 0x86)
    i32 CALLBACK winapi_0e3b20_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x110:
                if (g_dlgInfoText == 0) {
                    EndDialog(hDlg, (INT_PTR)g_dlgInfoText);
                    return 1;
                }
                winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_saveSink, g_dlgInfoText);
                return 1;
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (wParam == 1) {
                    EndDialog(hDlg, wParam);
                    return 1;
                }
                break;
        }
        return 0;
    }

    // __stdcall DlgProc(hDlg, msg, wParam, lParam): OK/Cancel end the dialog.
    RVA(0x000e3be0, 0x52)
    i32 CALLBACK winapi_0e3be0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x110:
                return 1;
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (wParam == 1) {
                    EndDialog(hDlg, 1);
                    return 1;
                }
                break;
        }
        return 0;
    }

    // (0xf8e20 soundfont-device DLL teardown re-homed to src/Gruntz/SoundFontPath.cpp.)

    // (0xfac70 PaintHost::Paint re-homed to src/Gruntz/Attract.cpp (StatePaintHost)
    // - a non-virtual shared base-class top-window paint poll that CAttract::Vslot07,
    // CMultiBootyState::ReadyAndPaint and CGuardedDispatch::Run each call on `this`.)

    // (0xfe460 Screen::Open + 0xfe600 RectWnd::Reset re-homed to src/Gruntz/GruntzMgr.cpp
    // as ONE class ScreenRegionMgr - the screen/video-region sub-object reached as
    // [owner+0x2dc]. They share the Prep_12fd/Sub194c/Sub3d55 helpers, proving one
    // class; the worklist's "0xfe600 -> play/sbi split" was a thunk-band mis-read, so
    // both are homed together in gruntzmgr. See that file for the disasm note.)

    // The blit target reached through the layer node (node->m_2c); Blit13ef90 paints
    // a rect-source into a destination rect with the given mode flags.
    struct LayerNode_115300 {
        char m_pad0[0x2c];
        CDDSurface* m_2c; // +0x2c  the blit target (BltFast @0x13ef90)
    };
    struct LayerSet_115300 {
        char m_pad0[0x10];
        LayerNode_115300* m_10; // +0x10
        LayerNode_115300* m_14; // +0x14
    };
    struct LayerHost_115300 {
        char m_pad0[4];
        LayerSet_115300* m_4; // +0x04
    };
    struct RectSrc_115300 {
        char m_pad0[0x10];
        i32 m_10; // +0x10 width
        i32 m_14; // +0x14 height
        i32 m_18; // +0x18 origin x
        i32 m_1c; // +0x1c origin y
        char m_pad20[0x2c - 0x20];
        void* m_2c; // +0x2c
    };
    // __cdecl(host, src, x, y, useFront, mode): blit src into the active layer node.
    // @orphan: a free __cdecl blit helper (no class receiver); callers are CPlay::
    // BuildHelpReveal + Gap_0d1650 but the helper's own .obj/owner is unrecovered.
    RVA(0x00115300, 0xf5)
    i32 winapi_115300_SetRect(
        LayerHost_115300* host,
        RectSrc_115300* src,
        i32 x,
        i32 y,
        i32 useFront,
        i32 mode
    ) {
        if (!host) {
            return 0;
        }
        if (!src) {
            return 0;
        }
        LayerNode_115300* node;
        if (useFront) {
            node = host->m_4->m_10;
            if (!node) {
                return 0;
            }
        } else {
            node = host->m_4->m_14;
            if (!node) {
                return 0;
            }
        }
        CDDSurface* dst = node->m_2c;
        if (!dst) {
            return 0;
        }
        void* srcHandle = src->m_2c;
        if (!srcHandle) {
            return 0;
        }
        i32 dx = x - src->m_18;
        i32 dy = y - src->m_1c;
        RECT rc;
        SetRect(&rc, 0, 0, src->m_10 - 1, src->m_14 - 1);
        RECT rc2 = rc;
        i32 flags = 0x10;
        if (mode) {
            flags = 0x11;
        }
        dst->BltFast(dx, dy, (CDDSurface*)srcHandle, &rc2, flags);
        return 1;
    }

} // namespace ApiCallerStubs
