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
        i32 winapi_0e6020_SetRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32);
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

    // @confidence: low
    // @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
    // @early-stop
    // Battlez multiplayer-setup dialog init (GAME code, 2664 B). Reads config via
    // g_buteMgr ("Battlez_Setup" section: LastMaxGruntz%d / LastDiff%d / LastColour%d,
    // DefaultMaxGruntz) + g_mgrSettings, populates the dialog controls (the
    // "Computer (easy/normal/difficult)", "Human", "Player", "Serra", "Jebediah"
    // combo/list strings) and drives them via the g_pSendMessageA / PTR_GetWindow /
    // PTR_GetWindowLongA / PTR_SetWindowLongA function-pointer trampolines. Deferred
    // to the leaf-first final sweep: a >512B body over ~20 CButeMgr/CString/CGameReg
    // callees + a subclass window trampoline that must be modeled first; a partial
    // under-counts AND diverges its regalloc, so the return-0 normalization artifact
    // is kept per the >512B REVERT rule.
    RVA(0x00014d00, 0xa68)
    i32 __stdcall winapi_014d00_GetWindow_GetWindowLongA_SetWindowLongA(i32) {
        return 0;
    }

    // A CWnd-ish object whose HWND lives at +0x1c (returned by the dialog-item
    // resolver thunks that several wrappers below call).
    struct WndItem {
        char m_pad0[0x1c];
        HWND m_hwnd; // +0x1c
    };
    // (0x15cc0/d00/d30/d70 listbox helpers re-homed to CBattlezDlg in Dialogs.cpp.)

    // A dialog-host class whose GetItem(id) (RVA 0x1be27d) returns a CWnd-ish
    // whose HWND lives at +0x1c.
    struct DlgHost {
        WndItem* GetItem(i32 id); // thiscall, RVA 0x1be27d
        void OnPick();            // thiscall, RVA 0x1bacc3
        void Pick0183f0();        // thiscall, RVA 0x183f0
    };
    // __thiscall(): send 0x188 to item 0x516; if it returned != -1, run OnPick().
    // @orphan: only inbound edge is a fn-ptr-table slot (~g_5e8e98+0x1c, via thunk
    // 0x3d5f) - no class vtable / new-site trace, so the owning dialog class is unrecovered.
    RVA(0x000183f0, 0x2e)
    void DlgHost::Pick0183f0() {
        HWND h = GetItem(0x516)->m_hwnd;
        if (SendMessageA(h, 0x188, 0, 0) != -1) {
            OnPick();
        }
    }

    // (0x19f50 RNG helper re-homed to Rng::RangeStd in src/Gruntz/Random.cpp.)

    // @confidence: low
    // @source: winapi:CopyRect
    // @early-stop
    // Level-message HUD + explosion eye-candy driver (GAME code, 1718 B). Walks the
    // g_levelMsgStrings (CString[]) / g_levelMsgRectsA / g_levelMsgRectsB (RECT[])
    // parallel arrays, copies rects via the g_pCopyRect function pointer, spawns a
    // "GAME_EXPLOSION1" sprite gated on g_mgrSettings, and fires a rate-limited sound
    // cue (g_sndCueTag / g_sndEnabled / g_killCueClock). Deferred to the leaf-first
    // final sweep: a >512B body over the CString array + sprite-create + sound callee
    // set; a partial under-counts AND diverges its regalloc, so the return-0
    // normalization artifact is kept per the >512B REVERT rule.
    RVA(0x0001a700, 0x6b6)
    i32 winapi_01a700_CopyRect() {
        return 0;
    }

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

    // __thiscall(x, _, y): if (x,y) is in the 0..0x64 box, run the click handler
    // (0x3d41); otherwise post a 0x111 command (0x8023/0x8027 by mode). Always ret 1.
    // NOT re-homed: the worklist attributed this to CGruntzMgr on a thunk-band
    // mis-read - the thunk that UpdateScoreHud/AccrueScoreTime actually call (0x1884
    // "SetCount") jmps to 0xfcad0 (the real ScoreHud::Refresh, 1-arg), NOT here. This
    // 3-arg (ret 0xc) hit-test is reached only via the adjacent 0x1889 thunk, which
    // has no indexed rel32 caller, so its owning class is unrecovered. Left put.
    // @early-stop
    // owner unresolved (thunk-band mis-chase, see above) + RECT/POINT stack-frame
    // codegen plateau; logic (PtInRect hit-test -> Activate else WM_COMMAND) is exact.
    struct ClickWnd_0394b0 {
        char m_pad0[4];
        ClickWnd_0394b0* m_4; // +0x04 -> m_4 -> m_4 = HWND
    };
    struct ClickHost_0394b0 {
        char m_pad0[4];
        ClickWnd_0394b0* m_4; // +0x04
        char m_pad8[0x24 - 8];
        i32 m_24; // +0x24
        i32 OnClick(i32 x, i32 unused, i32 y);
        void Activate(); // RVA 0x3d41
    };
    RVA(0x000394b0, 0x86)
    i32 ClickHost_0394b0::OnClick(i32 x, i32 unused, i32 y) {
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = 0x64;
        rc.bottom = 0x64;
        POINT pt;
        pt.x = x;
        pt.y = y;
        if (PtInRect(&rc, pt)) {
            Activate();
            return 1;
        }
        i32 cmd;
        if (m_24 == 5) {
            cmd = 0x8023;
        } else {
            cmd = 0x8027;
        }
        PostMessageA((HWND)m_4->m_4->m_4, 0x111, cmd, 0);
        return 1;
    }

    // A spatial object: its tile coords are m_5c/m_60 in 1/32-pixel units (>>5).
    struct Spatial_77df0 {
        char m_pad0[0x5c];
        i32 m_5c; // +0x5c x
        i32 m_60; // +0x60 y
    };
    struct Cell_77df0 {
        char m_pad0[0x10];
        Spatial_77df0* m_10; // +0x10
        char m_pad14[0x1fc - 0x14];
        i32 m_1fc; // +0x1fc live flag
        char m_pad200[0x258 - 0x200];
        i32 m_258; // +0x258 kind
    };
    struct World_77df0 {
        char m_pad0[0x10];
        Spatial_77df0* m_10; // +0x10 reference object
        char m_pad14[0x17c - 0x14];
        i32 m_17c; // +0x17c reference x
        i32 m_180; // +0x180 reference y
        char m_pad184[0x1ec - 0x184];
        i32 m_1ec; // +0x1ec row to skip
        char m_pad1f0[0x298 - 0x1f0];
        i32 m_298; // +0x298 radius part
        char m_pad29c[0x2dc - 0x29c];
        i32 m_2dc; // +0x2dc radius part
    };
    // A 4x15 grid of cell slots starting at +0x1c.
    struct Grid_77df0 {
        char m_pad0[0x1c];
        Cell_77df0* m_cells[4][15]; // +0x1c (row stride 0x3c)
        Cell_77df0* FindNearest(World_77df0* w);
    };
    // __thiscall(w): of the live, non-kind-0x36 cells in the grid (skipping row
    // w->m_1ec), pick the one nearest the reference tile; null it unless it lands
    // inside the reference object's +/-(m_298+m_2dc+1) tile box.
    // @early-stop
    // regalloc wall: logic + the distance/rect math are byte-exact, but MSVC spills
    // colPtr/rowPtr to the stack where retail keeps them in edi/ecx (it instead
    // reloads `w` per outer iter). A spill-weight choice; the loop body matches.
    RVA(0x00077df0, 0x13d)
    Cell_77df0* Grid_77df0::FindNearest(World_77df0* w) {
        Cell_77df0* best = 0;
        i32 bestDist = 0x7fffffff;
        i32 tileX = w->m_17c >> 5;
        i32 tileY = w->m_180 >> 5;
        Cell_77df0** rowPtr = &m_cells[0][0];
        for (i32 i = 0; i < 4; i++) {
            if (i != w->m_1ec) {
                Cell_77df0** colPtr = rowPtr;
                i32 j = 15;
                do {
                    Cell_77df0* cell = *colPtr;
                    if (cell && cell->m_1fc != 0 && cell->m_258 != 0x36) {
                        i32 dx = (cell->m_10->m_5c >> 5) - tileX;
                        i32 dy = (cell->m_10->m_60 >> 5) - tileY;
                        i32 dist = dx * dx + dy * dy;
                        if (dist < bestDist) {
                            best = cell;
                            bestDist = dist;
                        }
                    }
                    colPtr++;
                } while (--j != 0);
            }
            rowPtr += 15;
        }
        i32 k = w->m_298 + w->m_2dc + 1;
        i32 px = w->m_10->m_5c >> 5;
        i32 py = w->m_10->m_60 >> 5;
        RECT rc;
        rc.left = px - k;
        rc.top = py - k;
        rc.right = px + k + 1;
        rc.bottom = py + k + 1;
        if (best) {
            POINT pt;
            pt.x = best->m_10->m_5c >> 5;
            pt.y = best->m_10->m_60 >> 5;
            if (!PtInRect(&rc, pt)) {
                best = 0;
            }
        }
        return best;
    }

    // (0x8c380 RectHost::Set re-homed to ApiMisc in src/Gruntz/ApiMiscHelpers.cpp.)

    // __thiscall(out): default to the full 0x280x0x1e0 screen rect, or the active
    // viewport's rect (m_30->m_24 + 0x10) when one is set; write it to *out.
    struct ViewObj_08e3a0 {
        char m_pad0[0x24];
        char* m_24; // +0x24 (its +0x10 is a RECT)
    };
    struct RectQuery_08e3a0 {
        char m_pad0[0x30];
        ViewObj_08e3a0* m_30; // +0x30
        void GetRect(RECT* out);
    };
    // @orphan: a shared viewport-rect helper called on divergent `this` from many
    // owners (CSingleFrameMessage ctor, CPlay::Render/DrawDebugStats/Profile*) - no
    // single class owns it; RVA-adjacent .obj not pinned. Left carved.
    RVA(0x0008e3a0, 0x94)
    void RectQuery_08e3a0::GetRect(RECT* out) {
        RECT local;
        SetRect(&local, 0, 0, 0x27f, 0x1df);
        if (!m_30) {
            *out = local;
            return;
        }
        local = *(RECT*)(m_30->m_24 + 0x10);
        *out = local;
    }

    // The shared caption buffer (DAT_0060aac8) passed as the MessageBoxA title.
    DATA(0x0020aac8)
    extern char g_msgCaption[];
    struct Poly08 { // real polymorphic; Slot28 is slot 10 (+0x28)
        virtual void Slot00();
        virtual void Slot01();
        virtual void Slot02();
        virtual void Slot03();
        virtual void Slot04();
        virtual void Slot05();
        virtual void Slot06();
        virtual void Slot07();
        virtual void Slot08();
        virtual void Slot09();
        virtual void __stdcall Slot28(); // slot 10 (+0x28)
    };
    struct AudioSub_08ee70 {
        char m_pad0[0x14];
        i32 m_14; // +0x14 = audio handle
    };
    // An audio-ish sub-object: +0x4 -> sub whose [+0x14] is a handle for Stop_158c70,
    // +0x1c -> a Poly08* (the actual object pointer) whose vtable slot +0x28 runs.
    struct AudioObj_08ee70 {
        char m_pad0[4];
        AudioSub_08ee70* m_4; // +0x04 (its [+0x14] is the audio handle)
        char m_pad8[0x1c - 8];
        Poly08** m_1c; // +0x1c
    };
    struct MsgWnd_08ee70 {
        char m_pad0[4];
        HWND m_4; // +0x04 -> HWND
    };
    struct MsgHost_08ee70 {
        char m_pad0[4];
        MsgWnd_08ee70* m_4; // +0x04
        char m_pad8[0x30 - 8];
        AudioObj_08ee70* m_30; // +0x30
        i32 Show(i32 text, i32 type);
    };
    // Pause audio (slot 0x28), force the cursor visible, MessageBoxA, then hide it.
    void __stdcall Stop_158c70(i32 handle); // RVA 0x158c70
    // @early-stop
    // regalloc free-list-pick wall (docs/patterns/select-zero-mask-dest-register.md):
    // body byte-exact, but every value-holding register is rotated vs retail - the
    // `m_30->m_4->m_14` chain (ecx/eax vs our eax/ecx), the `*m_30->m_1c; ->vtbl
    // ->Slot28(p)` dispatch (container ecx + vtbl edx vs our edx + ecx) and the
    // MessageBoxA arg setup all carry the same global re-colouring. Same
    // instructions, swapped registers; not source-steerable (~98.5%).
    RVA(0x0008ee70, 0x7c)
    i32 MsgHost_08ee70::Show(i32 text, i32 type) {
        if (m_30) {
            Stop_158c70(m_30->m_4->m_14);
            Poly08* p = *m_30->m_1c;
            p->Slot28();
        }
        i32 wasShown = ShowCursor(1);
        while (ShowCursor(1) < 0) {
        }
        i32 result = MessageBoxA(m_4->m_4, (LPCSTR)text, g_msgCaption, type);
        if (wasShown <= 0) {
            while (ShowCursor(0) >= 0) {
            }
        }
        return result;
    }

    // A polymorphic mode object whose slot 4 (+0x10) returns the current mode.
    struct ModeObj_08f480 {
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual i32 GetMode(); // slot 4 == vtable +0x10
    };
    struct WndChain_08f480 {
        char m_pad0[4];
        HWND m_4; // +0x04
    };
    struct Sub_08f480 {
        void Reset(); // thiscall, RVA 0x1b9c69 (on this+0xc8)
    };
    struct ModeHost_08f480 {
        char m_pad0[4];
        WndChain_08f480* m_4; // +0x04
        char m_pad8[0x2c - 8];
        ModeObj_08f480* m_2c; // +0x2c
        char m_pad30[0xc8 - 0x30];
        Sub_08f480 m_c8; // +0xc8
        i32 Notify();
    };
    // __thiscall(): if the mode is 2/3/5, reset and post a 0x8005 command. Returns 1.
    // @orphan: no direct rel32 caller (reached only as a data/command-table slot) -
    // owning class of the +0x2c mode object unrecovered.
    RVA(0x0008f480, 0x49)
    i32 ModeHost_08f480::Notify() {
        i32 mode = m_2c->GetMode();
        if (mode == 5 || mode == 2 || mode == 3) {
            m_c8.Reset();
            PostMessageA(m_4->m_4, 0x111, 0x8005, 0);
            return 1;
        }
        return 0;
    }

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

    DATA(0x0024c86c)
    extern i32 g_dlg64c86c; // DAT_0064c86c (the active dialog HWND)
    DATA(0x00213a9c)
    extern i32 g_dlgSel613a9c;                                    // DAT_00613a9c
    i32 __cdecl DlgFallback_1302(HWND hDlg, i32 wParam, i32 cur); // RVA 0x1302
    void __cdecl DlgInit_2e05(HWND hDlg, i32 v);                  // RVA 0x2e05
    // __stdcall DlgProc(hDlg, msg, wParam, lParam).
    RVA(0x000e35f0, 0x77)
    i32 CALLBACK winapi_0e35f0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (DlgFallback_1302(hDlg, wParam, g_dlg64c86c) != 0) {
                    return 1;
                }
                // falls through to the shared "return 0" default
            default:
                return 0;
            case 0x110: {
                i32 v = (i32)g_gameReg->m_saveSink;
                g_dlgSel613a9c = -1;
                g_dlg64c86c = v;
                DlgInit_2e05(hDlg, v);
                return 1;
            }
        }
    }

    // SetDlgItemTextA helper defined below (RVA 0xe4850, reached via thunk 0x103c).
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item);
    // The optional info-line text shown on WM_INITDIALOG (DAT_0064c864).
    DATA(0x0024c864)
    extern char* g_dlgInfoText;

    // The gameReg->m_58 dialog helper sub-object; its M1834/M2d97 thunks live in
    // this TU. (m_58 is reused elsewhere as an int/void* gate, so cast locally.)
    struct DlgSub58_0e3a40 {
        // M1834 IS CloseTempFile_e5550 free fn; call it.
        // M2d97 IS CSaveGame::Save; cast at the call.
    };
    // The SetDlgItemTextA helper (RVA 0xe4850) is reached here via thunk 0x103c.
    // __stdcall DialogProc: OK closes; Cancel runs the helper sub-object; init fills.
    RVA(0x000e3a40, 0xb0)
    i32 CALLBACK winapi_0e3a40_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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
                    CloseTempFile_e5550((SaveTempRec*)g_dlgInfoText);
                    ((CSaveGame*)g_gameReg->m_saveSink)->Save(0, 0x81a6);
                    EndDialog(hDlg, 1);
                    return 1;
                }
                break;
        }
        return 0;
    }

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

    // __cdecl: SetDlgItemTextA(hWnd, 0x40d, &item->text) when all ptrs non-null.
    RVA(0x000e4850, 0x29)
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item) {
        if (hWnd && gate && item) {
            SetDlgItemTextA(hWnd, 0x40d, item + 0x14);
        }
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @early-stop
    // stub artifact wins: the tiny stub's push/pop-4 + `ret 0x28` epilogue
    // coincidentally aligns with the target's fail-tail, so objdiff (base-length
    // normalized) scores it ~86%. A faithful full-body reconstruction (GameView
    // ::Init, __thiscall(a0..a9): geometry stash + mgr-alloc + 3 bounded map
    // lookups + SetRect) reaches only ~42%: target keeps 4 callee-saved regs and
    // reuses the dead incoming-arg slots as SetRect/lookup scratch, while cl
    // spills a fresh `sub esp,0x10` RECT frame + drops ebp - a uniform frame
    // shift that mismatches every [esp+X] operand. Frame/regalloc wall; the 86%
    // artifact stub is kept so as not to regress the headline.
    // proximity: CFileIO@-0x920 | CSBI_WellGoo@+0x360
    RVA(0x000e6020, 0x288)
    i32 ThisStubOwner::winapi_0e6020_SetRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) {
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

    // (0x115b30 RectHost::Copy, 0x118930 SetActiveWindow_SetFocus, 0x1192d0 IsIconic
    // re-homed to ApiMisc in src/Gruntz/ApiMiscHelpers.cpp.)

    // 0x11b3b0 + 0x11b7c0 (CGruntSpawnConfig weighted grunt-voice spawn drivers): both
    // re-homed to src/Gruntz/GruntSpawnConfig.cpp (SpawnVoiceDriver / SpawnVoiceDriverStd).

    // 0x1206b0 (CRT _heapchk/_heapwalk: SEH frame + HeapValidate/HeapWalk loop) and
    // 0x123d10 (CRT _threadstartex thread bootstrap: SEH + GetCurrentThreadId/
    // TlsSetValue) are FID-carved as library (config/library_labels.csv), SKIP per
    // matcher policy. HeapDiag.cpp keeps its own reloc-masked extern decl for the
    // former (called through a casted HeapWalkFn pointer).

    // (0x136a30 WaveHost::LoadWave + 0x136ce0 WaveHost2::LoadWave re-homed to
    // ResLoaders in src/Gruntz/ResourceLoaders.cpp.)

    // (0x137720 DSOUND play wrapper re-homed to PlaySoundDefaulted in
    // src/Dsndmgr/SoundStream.cpp (RVA-adjacent free helper in that .obj).)

    // (0x137e30 Throttle::Tick + 0x1380d0 Timer::Tick re-homed to ApiMisc in
    // src/Gruntz/ApiMiscHelpers.cpp.)

    // (The AIL MIDI/XMIDI sequence subsystem - 0x138490/0x1384f0 driver bring-up +
    // teardown, 0x138950/0x1389c0 XMIDI master volume, 0x138c20 sequence-record
    // init, and the 0x138dd0-0x139030 player methods - re-homed as real
    // CGruntzSoundZ / CGruntzSoundInnerZ methods in src/Dsndmgr/GruntzSoundZ.cpp.)

    // (0x13d4c0 was WndHolder_13d4c0::Destroy; recovered as CGameWnd::OnClose -
    // the WM_CLOSE handler, vtable slot 4 - and migrated to src/Wap32/GameWnd.cpp.
    // The "WndHolder_13d4c0" placeholder class is CGameWnd: m_4 (HWND) / m_c
    // (destroyed flag) are the CGameWnd ctor-zeroed fields.)

    // (0x144270 ResLoad::Load + 0x1479e0 PalLoad::Load (with g_resModule) and
    // 0x164380 DrawCount + 0x164420 DrawLabel re-homed to ResLoaders in
    // src/Gruntz/ResourceLoaders.cpp.)

    // (The four CriticalSection thunks (0x16c9c0/d0/e0/f0) are FID-carved as library
    // (config/library_labels.csv, LIBCIMT). They sit inside the LIBCIMT iostream text
    // region (0x16c800..0x16ca80: iostream/filebuf ctors+dtors) and their retail
    // callers are iostream internals (streambuf/setbuf/open/close via `sema xref`),
    // i.e. they are the C++ library's own critical-section locking wrappers, not game
    // code. Carved per the game-not-CRT matcher policy; game callers reach them
    // reloc-masked through the library.)

    // (0x1775f0 PalHost::Apply (with g_palModule_6bf6e0) re-homed to ResLoaders in
    // src/Gruntz/ResourceLoaders.cpp.)

    // (0x179e70 winapi_179e70_IntersectRect reconstructed as the real
    // FontRenderer::DrawGlyphRun and re-homed to src/Font/Font.cpp - the inner
    // 16bpp glyph-run alpha-blit DrawLineClipped triple-calls; arg2 proven to be
    // the CDDSurface* destination (Lock/m_width/m_height/m_pitch/m_8->Unlock).)

    // (0x17caa0 RenderFrame re-homed to src/Io/SmackerVideoWindow.cpp as the real
    // CSmackWin::Frame - class identity proven (CSmackWin::Pump calls it on this=
    // CSmackWin, which already declared Frame @0x17caa0). The m_24 field-typing conflict
    // is resolved: it is the real IDirectDrawSurface (open/close Release it @slot 2, this
    // method Lock/Restore/Unlock it @slots 25/27/32), so the former SmkBuf/manual-vtable
    // views folded into one <ddraw.h> interface.)

    // (0x17cd90 PalCache::Snapshot re-homed to ResLoaders in
    // src/Gruntz/ResourceLoaders.cpp.)

    // (0x182ab0 Region::Init re-homed to src/Gruntz/MenuStateAssets.cpp (TileRegion)
    // - the tile-region initializer CMenuState::LoadAssets drives.)

    // The 0x1ba677..0x1bbff4 cluster is statically-linked MFC (CDialog / CWnd
    // dialog-template + WndProc-subclass internals): they reference AfxDlgProc,
    // s_AfxOldWndProc, AfxRegisterClass and carry the /GX EH state var. NOT game
    // code - FID-carved as library (config/library_labels.csv), SKIP per matcher
    // policy (use the library, don't hand-reconstruct). The six carved RVAs:
    //   0x1ba677  CDialog::CreateIndirect  (CreateDialogIndirectParamA + AfxDlgProc)
    //   0x1ba819  dialog teardown          (DestroyWindow/GlobalFree/GlobalUnlock)
    //   0x1ba9d2  CDialog::DoModal         (dialog resource load + modal loop)
    //   0x1baaef  modal enable/activate    (EnableWindow/Get+SetActiveWindow)
    //   0x1bb31b  _AfxActivationWndProc    (GetProp AfxOldWndProc subclass chain)
    //   0x1bbff4  AfxRegisterClass         (GetClassInfoA/RegisterClassA)

    // (0x1bf577 LibHost::Run + 0x1c09de GlobalOwner::Free re-homed to ApiMisc in
    // src/Gruntz/ApiMiscHelpers.cpp.)

} // namespace ApiCallerStubs
