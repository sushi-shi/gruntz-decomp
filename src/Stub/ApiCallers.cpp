#include <Mfc.h> // real MFC (CObject/CString/CPtrList) + windows.h via afx.h (superset of Win32.h)

#include <Gruntz/Multi.h> // real CMulti (the 0x64bd5c multiplayer game-state singleton)
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

// Auto-generated API-caller stubs from docs/api-caller-name-plan.tsv.
// Greenfield only: tracked/already-tried and named-untracked library rows are intentionally excluded.
// One stub is emitted per RVA; rows with multiple API categories are merged.
//
// RESIDUAL TAXONOMY (this file is the drained backlog; ~62 of the original ~126
// stubs already re-homed to their real class TUs - see the inline "re-homed to ..."
// notes. The 64 that remain are the BLOCKED residual, each parked here for one of):
//   [CRT]      Win32 CRITICAL_SECTION wrappers 0x16c9c0/d0/e0/f0 - not game code;
//              STAY per the game-not-CRT matcher policy (no owning game class).
//   [SPINE]    the g_gameReg / CGameReg spine + CGruntzMgr cluster (settings/menu
//              DlgProcs + SelHost/RosterHost/Dispatcher/ActionHost/Screen/RectWnd
//              /BattlezDlg). They deref a PLACEHOLDER CGameReg here; re-homing needs
//              the REAL spine (GruntzMgr.cpp/.h) typed first - out of this TU's scope.
//   [NET]      the NetMgr lobby/session DlgProcs (0xbdc00/0xbe0a0/0xbe2f0/0xbe400/
//              0xbe490/0xbe760) over a placeholder PeerSession_0be490 / SessionHost;
//              blocked on the NetMgr session class being typed first.
//   [ORPHAN]   real bodies on ANONYMOUS RVA-named placeholder hosts (CmdHost_*, Grid_*,
//              QlHost_*, EditAppendHost_*, Region_*, ...) whose true owning class is
//              not yet recovered; moving them would force-guess an owner - LEFT put.
//   [BIG]      >512B @early-stop bodies (0x14d00/0x1a700/0x179e70/0xe6020) kept at
//              their return-0/artifact plateau per the >512B REVERT rule (final sweep).
//   [DEFER]    MoviePlayer_17caa0::RenderFrame - CMoviePlayer/CSmackWin, deferred by a
//              documented m_10/m_24 field-typing conflict with its moved siblings.

// A window-host chain hung off the game registry: its m_4 is the top-level HWND.
SIZE_UNKNOWN(GameWnd);
struct GameWnd {
    char m_pad0[4];
    HWND m_4; // +0x04
};
// The CGameRegistry singleton (reloc-masked DATA symbol ?g_gameReg@@3PAUCGameReg@@A).
// Declared at global scope so it keeps the retail (un-namespaced) mangling.
struct CGameReg {
    char m_pad0[4];
    GameWnd* m_4; // +0x04 (m_4->m_4 is the top-level HWND)
    char m_pad8[0x2c - 8];
    void* m_curState; // +0x2c
    char m_pad30[0x54 - 0x30];
    void* m_54; // +0x54
    void* m_58; // +0x58
    char m_pad5c[0x100 - 0x5c];
    i32 m_isVoiceEnabled; // +0x100
    char m_pad104[0x118 - 0x104];
    i32 m_isEasyMode; // +0x118
    char m_pad11c[0x130 - 0x11c];
    i32 m_130; // +0x130
    i32 m_134; // +0x134
    char m_pad138[0x378 - 0x138];
    i32 m_378; // +0x378
    char m_pad37c[0x5b0 - 0x37c];
    i32 m_5b0; // +0x5b0
    char m_pad5b4[0x7e8 - 0x5b4];
    i32 m_7e8; // +0x7e8
    char m_pad7ec[0xa20 - 0x7ec];
    i32 m_a20;                          // +0xa20
    void Method92340(i32 state);        // __thiscall helper at RVA 0x92340
    void Method3df5(i32 state);         // __thiscall helper at RVA 0x3df5
    void ReportError(u32, i32);         // CGruntzMgr::ReportError, RVA 0x346d
    struct GameObj510* GetActive355d(); // __thiscall accessor, RVA 0x355d
};
SIZE_UNKNOWN(CGameReg); // (its own placeholder view; the SBI CGameReg facet was dissolved)
SIZE_UNKNOWN(GameObj510);
struct GameObj510 {
    char m_pad0[0x510];
    i32 m_510; // +0x510
};
DATA(0x0024556c)
extern CGameReg* g_gameReg;

// (The Miles Sound System (AIL) imports + g_ailMidiDriver/g_midiSeqCounter/
// g_ailDriver64, and the RNG/coin state g_rngSeeded/g_rngState/g_coinRolled/
// g_coinValue, moved with the MIDI sequence + LCG helpers to
// src/Dsndmgr/GruntzSoundZ.cpp and src/Gruntz/Random.cpp.)

// GetDlgItem(hWnd,0x4b6) cache (DAT_00648ce0), shared by several timer wrappers.
DATA(0x00248ce0)
extern HWND g_dlgItem_648ce0;

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
        void Stub_1c152f(i32);
        void Stub_1ccae7(i32, i32, i32);
        void Stub_1ccbfc(i32, i32, i32, i32);
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

    // The action/logic record hung off the object's m_7c; m_10 is its handler fn ptr
    // (compared against the default handler at LAB_00402d15 to set a flag bit).
    struct ActionRec_c840 {
        char m_pad0[0x10];
        void* m_10; // +0x10 handler
        char m_pad14[0x1c - 0x14];
        i32 m_1c; // +0x1c state
        char m_pad20[0x2c - 0x20];
        i32 m_2c; // +0x2c
        i32 m_30; // +0x30
        i32 m_34; // +0x34
        i32 m_38; // +0x38
    };
    // The placed sprite record returned by the emit helpers; its RECT is at +0x28.
    struct PlacedRec_c840 {
        char m_pad0[0x28];
        RECT m_28; // +0x28
    };
    struct Slot19c_c840 {
        char m_pad0[0x10];
        void* m_10; // +0x10
    };
    struct Obj_c840 {
        char m_pad0[8];
        i32 m_8; // +0x08 flags
        char m_pad0c[0x40 - 0xc];
        i32 m_40; // +0x40 flags
        char m_pad44[0x7c - 0x44];
        ActionRec_c840* m_7c; // +0x7c
        char m_pad80[0x120 - 0x80];
        i32 m_120; // +0x120
        char m_pad124[0x134 - 0x124];
        i32 m_134;  // +0x134
        i32 m_138;  // +0x138
        i32 m_13c;  // +0x13c
        i32 m_140;  // +0x140
        RECT m_144; // +0x144
        RECT m_154; // +0x154 (m_158 == m_154.top)
        char m_pad164[0x19c - 0x164];
        Slot19c_c840* m_19c; // +0x19c
    };
    // The default action handler, only its address matters (LAB_00402d15).
    extern "C" void DefaultActionHandler_2d15();
    // Emit helpers (__stdcall, ILT jmp-thunks) -> the placed sprite record.
    PlacedRec_c840* __stdcall EmitSpriteFull_3c97(
        void* tex,
        i32 z,
        RECT* rc,
        i32 a,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f
    ); // RVA 0x3c97
    PlacedRec_c840* __stdcall EmitSpriteSimple_2ad6(
        void* tex,
        i32 z,
        RECT* rc,
        i32 a,
        i32 b
    ); // RVA 0x2ad6
    // __cdecl(obj): commit the object's pending action into the active sprite layer.
    // @early-stop
    // arg-load scheduling wall (~94%): body byte-exact through the flag math and both
    // exits; the residual is MSVC's just-in-time vs pre-load interleaving of the Emit*
    // member-arg loads (same push order, same args) + the g_gameReg->m_54 test landing
    // in eax vs retail's ecx. Same instructions, different temp-register rotation.
    RVA(0x0000c840, 0x13d)
    i32 winapi_00c840_CopyRect_SetRect(Obj_c840* obj) {
        ActionRec_c840* rec = obj->m_7c;
        if (rec->m_1c == 0) {
            obj->m_8 |= 1;
            obj->m_40 |= 1;
            if (rec->m_10 == (void*)DefaultActionHandler_2d15) {
                obj->m_8 |= 2;
            } else {
                obj->m_8 &= ~2;
            }
            Slot19c_c840* slot = obj->m_19c;
            if (slot && g_gameReg) {
                RECT rc;
                CopyRect(&rc, &obj->m_144);
                if (rec->m_2c > 0 || rec->m_30 > 0) {
                    SetRect(&rc, rec->m_2c, rec->m_34, rec->m_30, rec->m_38);
                }
                if (g_gameReg->m_54) {
                    PlacedRec_c840* placed;
                    if (obj->m_138 > 0) {
                        placed = EmitSpriteFull_3c97(
                            slot->m_10,
                            0x64,
                            &rc,
                            obj->m_120,
                            obj->m_134,
                            obj->m_138,
                            obj->m_13c,
                            obj->m_140,
                            0
                        );
                    } else {
                        placed = EmitSpriteSimple_2ad6(slot->m_10, 0x64, &rc, obj->m_120, 0);
                    }
                    if (placed && obj->m_154.top > 0) {
                        placed->m_28 = obj->m_154;
                    }
                }
            }
            obj->m_8 |= 0x10000;
            rec->m_1c = 5;
        }
        return 1;
    }

    // (0xcd00/0xcd70/0x19f50/0x15cbe0 RNG helpers re-homed to Rng in
    // src/Gruntz/Random.cpp.)

    // __thiscall(code, _): on ESC/SPACE/ENTER post a 0x8023 command. Returns 1.
    struct CmdChain_014720 {
        i32 m_0;
        CmdChain_014720* m_4; // +0x04
    };
    struct CmdHost_014720 {
        i32 m_0;
        CmdChain_014720* m_4; // +0x04
        i32 Key(i32 code, i32 unused);
    };
    RVA(0x00014720, 0x37)
    i32 CmdHost_014720::Key(i32 code, i32 unused) {
        if (code == 0x20 || code == 0xd || code == 0x1b) {
            PostMessageA((HWND)(m_4->m_4->m_4), 0x111, 0x8023, 0);
        }
        return 1;
    }

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
    RVA(0x0001f8a0, 0x30)
    i32 KeyHost_01f8a0::Check() {
        if (m_1b8 == 0xc7) {
            PostMessageA(g_gameReg->m_4->m_4, 0x111, 0x8023, 0);
        }
        return 1;
    }

    // (0x2b340 ClipHost::Clip re-homed to ApiMisc in src/Gruntz/ApiMiscHelpers.cpp.)

    // (0x38150 find-and-select re-homed to CMultiSlotList::Method3396 in Dialogs.cpp.)

    // __stdcall(hDlg, id, *lo, *hi): split the selected listbox item's data into
    // two words. Returns 1 if a valid item is selected.
    RVA(0x00038220, 0x73)
    i32 __stdcall winapi_038220_GetDlgItem(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
        HWND list = GetDlgItem(hDlg, id);
        if (!list) {
            return 0;
        }
        i32 sel = SendMessageA(list, 0x147, 0, 0);
        if (sel == -1) {
            return 0;
        }
        i32 data = SendMessageA(list, 0x150, sel, 0);
        if (data == -1) {
            return 0;
        }
        *outLo = data & 0xffff;
        *outHi = (u32)data >> 0x10;
        return 1;
    }

    // __thiscall(code, _): on ESC/SPACE/ENTER post a 0x8023/0x8027 command (by m_24
    // mode). Always returns 1.
    struct CmdWnd_039440 {
        i32 m_0;
        CmdWnd_039440* m_4; // +0x04
    };
    struct CmdHost_039440 {
        i32 m_0;
        CmdWnd_039440* m_4; // +0x04
        char m_pad8[0x24 - 8];
        i32 m_24; // +0x24
        i32 Key(i32 code, i32 unused);
    };
    RVA(0x00039440, 0x46)
    i32 CmdHost_039440::Key(i32 code, i32 unused) {
        if (code == 0x1b || code == 0x20 || code == 0xd) {
            if (m_24 == 5) {
                PostMessageA((HWND)(m_4->m_4->m_4), 0x111, 0x8023, 0);
            } else {
                PostMessageA((HWND)(m_4->m_4->m_4), 0x111, 0x8027, 0);
            }
        }
        return 1;
    }

    // __thiscall(x, _, y): if (x,y) is in the 0..0x64 box, run the click handler;
    // otherwise post a 0x111 command (0x8023/0x8027 by mode). Always returns 1.
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
    struct Poly08 {
        struct PolyVtbl08* m_vptr;
    };
    struct PolyVtbl08 {
        void* s0[0xa];
        void(__stdcall* Slot28)(Poly08*); // +0x28
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
            p->m_vptr->Slot28(p);
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

    // __thiscall(int code): clamp code into (0,0x29] and post a 0x111 command.
    // This 0x90220 body is also what Dispatcher_0cfbd0::Dispatch (@0xcfbd0) reaches
    // as `m_4->Post(m_1c+1)` -- so the old DispOwner_0cfbd0 was a SECOND placeholder
    // view of this same class; merged here. The +0x48/0x54/0x60 sub-managers are the
    // extra fields Dispatch touches (leaf types fwd-declared just below).
    struct SoundBankRef_0cfbd0; // = CGruntzSoundZ (StopAndFlush @0x138530, gruntzsoundz)
    struct CmdHostSub54_0cfbd0; // unidentified sub-mgr (reset @0x40b660)
    struct CmdHostSub60_0cfbd0; // unidentified sub-mgr (reset @0x51af90)
    struct WndOwner_090220 {
        i32 m_0;
        HWND m_4; // +0x04 = HWND  (also the old DispWnd_0cfbd0: top-window holder)
    };
    struct CmdHost_090220 {
        i32 m_0;
        WndOwner_090220* m_4;      // +0x04
        char m_pad8[0x48 - 8];     // +0x08..0x47
        SoundBankRef_0cfbd0* m_48; // +0x48  audio bank (flushed on Dispatch quiesce)
        char m_pad4c[0x54 - 0x4c]; // +0x4c..0x53
        CmdHostSub54_0cfbd0* m_54; // +0x54
        char m_pad58[0x60 - 0x58]; // +0x58..0x5f
        CmdHostSub60_0cfbd0* m_60; // +0x60
        void Post(i32 code);
    };
    RVA(0x00090220, 0x2f)
    void CmdHost_090220::Post(i32 code) {
        if (code > 0 && code <= 0x29) {
            i32 v = (code == 0x29) ? 1 : code;
            PostMessageA(m_4->m_4, 0x111, 0x807f, v);
        }
    }

    struct QlWnd_092710 {
        char m_pad0[4];
        HWND m_4; // +0x04
    };
    struct Sub58_092710 {
        i32 Check(i32* save); // thiscall, RVA 0x12f3
    };
    struct Sub5c_092710 {
        void Notify(const char* msg, i32 a, i32 b); // thiscall, RVA 0x1483
    };
    struct Sub60_092710 {
        void Flush(); // thiscall, RVA 0x20a4
    };
    struct QlHost_092710 {
        char m_pad0[4];
        QlWnd_092710* m_4; // +0x04
        char m_pad8[0x58 - 8];
        Sub58_092710* m_58; // +0x58
        Sub5c_092710* m_5c; // +0x5c
        Sub60_092710* m_60; // +0x60
        char m_pad64[0xbc - 0x64];
        i32* m_bc; // +0xbc
        i32 Quickload();
        i32 Fallback(); // thiscall, RVA 0x29a0
    };
    RVA(0x00092710, 0x77)
    i32 QlHost_092710::Quickload() {
        if (!m_58) {
            return 0;
        }
        if (m_60) {
            m_60->Flush();
        }
        if (m_bc && (*m_bc & 1)) {
            if (m_58->Check(m_bc) == 0) {
                return 1;
            }
            PostMessageA(m_4->m_4, 0x111, 0x807e, 0);
            m_5c->Notify("Game Quickloaded successfully.", 0, 0x11);
            return 1;
        }
        return Fallback();
    }

    // The settings dialog's 12 numeric edit fields, cached as globals between the
    // WM_INITDIALOG load and the IDOK store.
    DATA(0x0024526c)
    extern i32 g_dlgVal_64526c;
    DATA(0x002452d0)
    extern i32 g_dlgVal_6452d0;
    DATA(0x00245268)
    extern i32 g_dlgVal_645268;
    DATA(0x00245568)
    extern i32 g_dlgVal_645568;
    DATA(0x00245538)
    extern i32 g_dlgVal_645538;
    DATA(0x002451a4)
    extern i32 g_dlgVal_6451a4;
    DATA(0x002452d4)
    extern i32 g_dlgVal_6452d4;
    DATA(0x002452a8)
    extern i32 g_dlgVal_6452a8;
    DATA(0x00245558)
    extern i32 g_dlgVal_645558;
    DATA(0x00245560)
    extern i32 g_dlgVal_645560;
    DATA(0x0024555c)
    extern i32 g_dlgVal_64555c;
    DATA(0x00245564)
    extern i32 g_dlgVal_645564;
    // __stdcall DlgProc(hDlg, msg, wParam, lParam): a numeric settings dialog.
    RVA(0x00092ab0, 0x20d)
    i32 CALLBACK winapi_092ab0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x110:
                SetDlgItemInt(hDlg, 0x4db, g_dlgVal_64526c, 0);
                SetDlgItemInt(hDlg, 0x4da, g_dlgVal_6452d0, 0);
                SetDlgItemInt(hDlg, 0x4dc, g_dlgVal_645268, 0);
                SetDlgItemInt(hDlg, 0x4dd, g_dlgVal_645568, 0);
                SetDlgItemInt(hDlg, 0x4de, g_dlgVal_645538, 0);
                SetDlgItemInt(hDlg, 0x4df, g_dlgVal_6451a4, 0);
                SetDlgItemInt(hDlg, 0x4e0, g_dlgVal_6452d4, 0);
                SetDlgItemInt(hDlg, 0x4e9, g_dlgVal_6452a8, 0);
                SetDlgItemInt(hDlg, 0x4e3, g_dlgVal_645558, 0);
                SetDlgItemInt(hDlg, 0x4e4, g_dlgVal_645560, 0);
                SetDlgItemInt(hDlg, 0x4e5, g_dlgVal_64555c, 0);
                SetDlgItemInt(hDlg, 0x4e6, g_dlgVal_645564, 0);
                return 1;
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (wParam == 1) {
                    g_dlgVal_64526c = GetDlgItemInt(hDlg, 0x4db, 0, 0);
                    g_dlgVal_6452d0 = GetDlgItemInt(hDlg, 0x4da, 0, 0);
                    g_dlgVal_645268 = GetDlgItemInt(hDlg, 0x4dc, 0, 0);
                    g_dlgVal_645568 = GetDlgItemInt(hDlg, 0x4dd, 0, 0);
                    g_dlgVal_645538 = GetDlgItemInt(hDlg, 0x4de, 0, 0);
                    g_dlgVal_6451a4 = GetDlgItemInt(hDlg, 0x4df, 0, 0);
                    g_dlgVal_6452d4 = GetDlgItemInt(hDlg, 0x4e0, 0, 0);
                    g_dlgVal_6452a8 = GetDlgItemInt(hDlg, 0x4e9, 0, 0);
                    g_dlgVal_645558 = GetDlgItemInt(hDlg, 0x4e3, 0, 0);
                    g_dlgVal_645560 = GetDlgItemInt(hDlg, 0x4e4, 0, 0);
                    g_dlgVal_64555c = GetDlgItemInt(hDlg, 0x4e5, 0, 0);
                    g_dlgVal_645564 = GetDlgItemInt(hDlg, 0x4e6, 0, 0);
                    EndDialog(hDlg, 1);
                    return 1;
                }
                break;
        }
        return 0;
    }

    // __thiscall(): if the sub-chain is live + ready, validate the whole window.
    struct VrSub2_094bc0 {
        i32 Ready(); // thiscall, RVA 0x2441
    };
    struct VrSub1_094bc0 {
        char m_pad0[8];
        VrSub2_094bc0* m_8; // +0x08
    };
    struct VrHost_094bc0 {
        char m_pad0[4];
        HWND m_4;           // +0x04
        VrSub1_094bc0* m_8; // +0x08
        i32 Validate();
    };
    RVA(0x00094bc0, 0x31)
    i32 VrHost_094bc0::Validate() {
        VrSub2_094bc0* sub = m_8->m_8;
        if (sub && sub->Ready()) {
            if (m_4) {
                ValidateRect(m_4, 0);
            }
            return 1;
        }
        return 0;
    }

    // __thiscall(int code, int): on ESC/SPACE/ENTER post a 0x111 command. Returns 1.
    struct WndChain_0953f0 {
        i32 m_0;
        WndChain_0953f0* m_4; // +0x04
    };
    struct CmdHost_0953f0 {
        i32 m_0;
        WndChain_0953f0* m_4; // +0x04
        i32 Key(i32 code, i32 unused);
    };
    RVA(0x000953f0, 0x37)
    i32 CmdHost_0953f0::Key(i32 code, i32 unused) {
        if (code == 0x1b || code == 0x20 || code == 0xd) {
            PostMessageA((HWND)(m_4->m_4->m_4), 0x111, 0x8036, 0);
        }
        return 1;
    }

    DATA(0x00245ca4)
    extern i32 g_dlg645ca4; // DAT_00645ca4 (the active dialog HWND)
    i32 __cdecl DlgFallback_215d(HWND hDlg, i32 wParam, i32 cur); // RVA 0x215d
    void __cdecl DlgInit_2ee6(HWND hDlg, i32 v);                  // RVA 0x2ee6
    // __stdcall DlgProc(hDlg, msg, wParam, lParam).
    RVA(0x0009dff0, 0x8c)
    i32 CALLBACK winapi_09dff0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case 0x111:
                if (wParam == 2 || wParam == 1) {
                    GameObj510* obj = g_gameReg->GetActive355d();
                    if (obj) {
                        obj->m_510 = 2;
                    }
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (DlgFallback_215d(hDlg, wParam, g_dlg645ca4) != 0) {
                    return 1;
                }
                // falls through to the shared "return 0" default
            default:
                return 0;
            case 0x110: {
                i32 v = (i32)g_gameReg->m_58;
                g_dlg645ca4 = v;
                DlgInit_2ee6(hDlg, v);
                return 1;
            }
        }
    }

    // The DirectPlay application GUID (DAT_0060fab8), passed by value to Connect.
    DATA(0x0020fab8)
    extern GUID g_dplayAppGuid;
    DATA(0x00248cf0)
    extern i32 g_isHost_648cf0; // DAT_00648cf0: nonzero when hosting
    struct DPlayConn_0b77a0 {
        i32 Connect(i32 sessionFlags, GUID guid); // thiscall, RVA 0x1780b0
    };
    struct DPlaySub_0b77a0 {
        void Init(); // thiscall, RVA 0x158dc0
    };
    struct DPlayHolder_0b77a0 {
        char m_pad0[4];
        DPlaySub_0b77a0* m_4; // +0x04
    };
    struct DPlayHost_0b77a0 {
        char m_pad0[0xc];
        DPlayHolder_0b77a0* m_c; // +0x0c
        char m_pad10[0x524 - 0x10];
        DPlayConn_0b77a0* m_524;                                // +0x524
        i32 m_528;                                              // +0x528
        void Configure(char* name, i32 a, i32 b, i32 c, i32 d); // RVA 0x3445
        i32 Build();                                            // RVA 0x3db9
        i32 HostStart();                                        // RVA 0x39bd
        i32 JoinStart();                                        // RVA 0x2487
        i32 Open();
    };
    // __thiscall(): configure the session, build it, connect via DirectPlay using
    // the app GUID, then host- or join-start depending on g_isHost_648cf0.
    RVA(0x000b77a0, 0xb5)
    i32 DPlayHost_0b77a0::Open() {
        if (!m_524) {
            return 0;
        }
        Configure("BACKGND", 0, 0, 1, 0);
        m_c->m_4->Init();
        i32 sessionFlags = Build();
        if (!sessionFlags) {
            return 0;
        }
        if (!m_524->Connect(sessionFlags, g_dplayAppGuid)) {
            return 0;
        }
        if (g_isHost_648cf0) {
            m_528 = 1;
            if (!HostStart()) {
                return 0;
            }
        } else {
            m_528 = 0;
            if (!JoinStart()) {
                return 0;
            }
        }
        return 1;
    }

    // App-instance chain: this->m_4->m_8->m_c is the HINSTANCE for LoadString.
    struct AppRes_0b7ec0 {
        char m_pad0[0xc];
        HINSTANCE m_c; // +0x0c
    };
    struct AppHolder_0b7ec0 {
        char m_pad0[8];
        AppRes_0b7ec0* m_8; // +0x08
    };
    struct StrHost_0b7ec0 {
        char m_pad0[4];
        AppHolder_0b7ec0* m_4;             // +0x04
        void SetText(char* text, i32 arg); // RVA 0xb7e30 (thunk 0x1af0)
        void Load(i32 id, i32 dest);
    };
    // __thiscall(id, dest): load string `id`, defaulting to "Error", then push it.
    RVA(0x000b7ec0, 0x7d)
    void StrHost_0b7ec0::Load(i32 id, i32 dest) {
        char buf[0x12a];
        if (m_4 && m_4->m_8->m_c) {
            if (!LoadStringA(m_4->m_8->m_c, id, buf, 0xfa)) {
                strcpy(buf, "Error");
            }
            SetText(buf, dest);
        }
    }

    // __stdcall(edit, str): append `str` to an edit control, prefixing a CRLF when
    // the control is non-empty, then scroll to keep the caret in view.
    RVA(0x000bb3e0, 0xe5)
    void __stdcall winapi_0bb3e0_GetWindowTextLengthA(HWND edit, char* str) {
        if (!edit || !str || !str[0]) {
            return;
        }
        i32 len = GetWindowTextLengthA(edit);
        if (len == 0) {
            SendMessageA(edit, 0xb1, len, -1);
        } else {
            SendMessageA(edit, 0xb1, len, len);
        }
        char buf[0x80];
        buf[0] = 0;
        if (len > 0) {
            strcat(buf, "\r\n");
        }
        strcat(buf, str);
        SendMessageA(edit, 0xc2, 0, (LPARAM)buf);
        SendMessageA(edit, 0xb6, 0, 0x270f);
    }

    void Init_42b4(HWND hWnd, void* ctx); // RVA 0x42b4 (__cdecl)
    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bda00, 0x3e)
    void winapi_0bda00_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_42b4(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void Init_1924(HWND hWnd, void* ctx); // RVA 0x1924 (__cdecl)
    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bdb90, 0x3e)
    void winapi_0bdb90_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_1924(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // DAT_0064557c: the active modeless dialog HWND, cached on entry/init.
    DATA(0x0024557c)
    extern HWND g_curDlg_64557c;
    // The chat/lobby PeerSession singleton at DAT_006496ac. Defined here (rather than
    // at its natural RVA-order point below) so the lobby DlgProcs earlier in RVA order
    // can dereference it. Field names are placeholders; offsets are load-bearing.
    struct TimerHost_148d {
        i32 Poll_148d(i32 elapsed); // __thiscall, RVA 0x148d (nonzero once the deadline passed)
    };
    struct PeerSession_0be490 {
        char m_pad0[0x520];
        TimerHost_148d* m_520; // +0x520
        char m_pad524[0x52c - 0x524];
        i32 m_52c; // +0x52c
        char m_pad530[0x564 - 0x530];
        i32 m_564;                                        // +0x564  abnormal-termination gate
        void Submit(char* text, i32 a, i32 b, HWND ctrl); // thiscall, RVA 0x2243
        void Notify_2955(i32 a, i32 wParam, i32 b);       // thiscall, RVA 0x2955
    };
    extern PeerSession_0be490* g_peerSession;
    // Lobby DlgProc helpers (cdecl, reached through ILT jmp-thunks).
    i32 PreHandleLobbyMsg_38c3(HWND, u32, u32, i32);    // RVA 0x38c3 -> 0x1192d0
    void OnLobbyInit_2c66(HWND, PeerSession_0be490*);   // RVA 0x2c66
    void OnLobbyInit_371f(HWND, PeerSession_0be490*);   // RVA 0x371f
    void OnLobbyTimerA_265d(HWND, PeerSession_0be490*); // RVA 0x265d
    void OnLobbyTimerB_154b(HWND, PeerSession_0be490*); // RVA 0x154b
    void OnLobbyTimerC_2185(HWND, PeerSession_0be490*); // RVA 0x2185 -> 0xbe3e0
    void OnLobbyCancel_2ae0(HWND, PeerSession_0be490*); // RVA 0x2ae0
    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the network-lobby dialog proc.
    RVA(0x000bdc00, 0x10c)
    i32 CALLBACK
    winapi_0bdc00_EndDialog_KillTimer(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_curState;
                OnLobbyInit_2c66(hWnd, g_peerSession);
                return 1;
            case 0x111:
                if (wParam == 0x4f7) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    OnLobbyCancel_2ae0(hWnd, g_peerSession);
                    return 1;
                }
                break;
            case 0x113:
                OnLobbyTimerA_265d(hWnd, g_peerSession);
                OnLobbyTimerB_154b(hWnd, g_peerSession);
                return 1;
        }
        return 0;
    }

    // Init helper at RVA 0xbddb0 (__cdecl(hWnd, ctx)).
    void Init_bddb0(HWND hWnd, void* ctx);
    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bdd60, 0x3e)
    void winapi_0bdd60_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bddb0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    void Init_2522(HWND hWnd, void* ctx); // RVA 0x2522 (__cdecl)
    // __cdecl(hWnd, ctx): init, arm a 750 ms timer, cache a child control handle.
    RVA(0x000bdfe0, 0x3e)
    void winapi_0bdfe0_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_2522(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the in-game network dialog proc
    // (sibling of the lobby proc at 0xbdc00). WM_COMMAND ends the dialog on a set of
    // button IDs; WM_TIMER (0x113) polls the abort deadline and re-posts the cancel.
    RVA(0x000be0a0, 0x1c7)
    i32 CALLBACK winapi_0be0a0_EndDialog_KillTimer_PostMessageA(
        HWND hWnd,
        UINT msg,
        WPARAM wParam,
        LPARAM lParam
    ) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_curState;
                OnLobbyInit_371f(hWnd, g_peerSession);
                return 1;
            case 0x111:
                if (wParam == 0x4ea) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4cd) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4ce) {
                    KillTimer(hWnd, 1);
                    g_peerSession->Notify_2955(0x402, wParam, 1);
                    EndDialog(hWnd, wParam);
                    return 1;
                }
                if (wParam == 0x4c6) {
                    OnLobbyCancel_2ae0(hWnd, g_peerSession);
                    return 1;
                }
                break;
            case 0x113:
                if (g_peerSession->m_564) {
                    KillTimer(hWnd, 1);
                    EndDialog(hWnd, 0x4cd);
                    return 1;
                }
                OnLobbyTimerA_265d(hWnd, g_peerSession);
                OnLobbyTimerC_2185(hWnd, g_peerSession);
                if (g_peerSession->m_520->Poll_148d(0x2710)) {
                    PostMessageA(hWnd, 0x111, 0x4cd, 0);
                }
                return 1;
        }
        return 0;
    }

    // The "client status" CString global (?g_6473d8@@3VCString@@A).
    DATA(0x002473d8)
    extern CString g_clientStatus;
    void InitDropPrompt_be3e0(HWND hWnd, void* ctx); // thunk 0x2185 -> 0xbe3e0 (cdecl)
    // __cdecl(hWnd, ctx): show the "not receiving data" banner, init, arm a timer.
    RVA(0x000be2f0, 0xb9)
    void winapi_0be2f0_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            CString banner;
            if (g_clientStatus.GetLength() != 0) {
                banner.Format("Not Receiving Data From Client: %s", (LPCTSTR)g_clientStatus);
                SetDlgItemTextA(hWnd, 0x44b, (LPCTSTR)banner);
            }
            InitDropPrompt_be3e0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    DATA(0x002496ac)
    extern PeerSession_0be490* g_peerSession; // DAT_006496ac (struct defined above)
    extern "C" char g_emptyString[];          // 0x6293f4
    // __cdecl(hWnd, gate): read the chat-edit text and, if non-empty, submit it.
    RVA(0x000be400, 0x6c)
    void winapi_0be400_GetWindowTextA_SetWindowTextA(HWND hWnd, void* gate) {
        char buf[0x68];
        if (hWnd && gate) {
            HWND edit = GetDlgItem(hWnd, 0x4b7);
            if (edit) {
                if (GetWindowTextA(edit, buf, 0x64) > 0) {
                    g_peerSession->Submit(buf, 1, 1, GetDlgItem(hWnd, 0x4b6));
                    SetWindowTextA(edit, g_emptyString);
                }
            }
        }
    }

    // Session host (arg2). Stop() at RVA 0xb95f0; SetStatus(text, flag) at 0xb7e30
    // (reached via thunk 0x1af0). m_584 marks a normal exit; m_5c4 carries its code.
    struct SessionHost_0be490 {
        char m_pad0[0x584];
        i32 m_584; // +0x584
        char m_pad588[0x5c4 - 0x588];
        i32 m_5c4;                         // +0x5c4
        void Stop();                       // RVA 0xb95f0
        void SetStatus(char* text, i32 f); // RVA 0xb7e30 (thunk 0x1af0)
    };
    DATA(0x002487e0)
    extern char g_sessionFlag; // DAT_006487e0
    // __cdecl(hWnd, session): stop the session and end the dialog appropriately.
    RVA(0x000be490, 0x84)
    void winapi_0be490_EndDialog_KillTimer(HWND hWnd, SessionHost_0be490* session) {
        if (hWnd && session) {
            g_sessionFlag = 0;
            session->Stop();
            if (session->m_584) {
                KillTimer(hWnd, 1);
                EndDialog(hWnd, session->m_5c4);
            } else if (g_peerSession->m_52c) {
                KillTimer(hWnd, 1);
                session->SetStatus("The game session has been terminated.", 0);
                EndDialog(hWnd, 0x4ce);
            } else {
                g_sessionFlag = 0;
            }
        }
    }

    // CString-data ptr (DAT_00649618): the pending drop-in player's name; its
    // CString length lives 8 bytes before the data.
    DATA(0x00249618)
    extern char* g_playerName_649618;
    void Init_2ed7(HWND hWnd, void* ctx); // RVA 0x2ed7 (__cdecl)
    // __cdecl(hWnd, ctx): show a drop-in prompt, init, arm a timer, cache a child.
    RVA(0x000be760, 0x82)
    void winapi_0be760_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];
            if (*(i32*)(g_playerName_649618 - 8)) {
                sprintf(buf, "New Player Drop-In Request: %s", g_playerName_649618);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            Init_2ed7(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // Item resolver at RVA 0x1753 (push slot; stdcall; returns the CWnd-ish item).
    WndItem* __stdcall ResolveItem_1753(i32 slot);
    // __stdcall(id, wParam): if item `id` resolves, set its selection to wParam-1.
    RVA(0x000c2980, 0x28)
    void __stdcall winapi_0c2980_SendMessageA(i32 id, i32 wParam) {
        WndItem* it = ResolveItem_1753(id);
        if (it) {
            SendMessageA(it->m_hwnd, 0x14e, wParam - 1, 0);
        }
    }

    // __thiscall OnInitDialog: chain to CDialog::OnInitDialog (RVA 0x1bac5e),
    // then arm a 50 ms timer on this dialog's HWND (m_1c). Returns TRUE.
    struct TimerDlg_0c2cb0 {
        char m_pad0[0x1c];
        HWND m_1c; // +0x1c
        i32 OnInitDialog();
        i32 BaseOnInitDialog(); // RVA 0x1bac5e (CDialog::OnInitDialog)
    };
    RVA(0x000c2cb0, 0x1f)
    i32 TimerDlg_0c2cb0::OnInitDialog() {
        BaseOnInitDialog();
        SetTimer(m_1c, 1, 0x32, 0);
        return 1;
    }

    // __thiscall(str): resolve dialog item 0x511, then append `str` to it the same
    // way as winapi_0bb3e0 (CRLF prefix when non-empty, scroll the caret).
    struct EditAppendHost_0c2ce0 {
        WndItem* GetItem(i32 id); // thiscall, RVA 0x1be27d
        void Append(char* str);
    };
    RVA(0x000c2ce0, 0xf3)
    void EditAppendHost_0c2ce0::Append(char* str) {
        WndItem* item = GetItem(0x511);
        HWND edit;
        if (!item) {
            edit = 0;
        } else {
            edit = item->m_hwnd;
        }
        if (!edit || !str || !str[0]) {
            return;
        }
        i32 len = GetWindowTextLengthA(edit);
        if (len == 0) {
            SendMessageA(edit, 0xb1, len, -1);
        } else {
            SendMessageA(edit, 0xb1, len, len);
        }
        char buf[0x80];
        buf[0] = 0;
        if (len > 0) {
            strcat(buf, "\r\n");
        }
        strcat(buf, str);
        SendMessageA(edit, 0xc2, 0, (LPARAM)buf);
        SendMessageA(edit, 0xb6, 0, 0x270f);
    }

    // A dialog-item CWnd (HWND at +0x1c) refreshed from the model.
    struct CWndItem_c3e30 {
        char m_pad0[0x1c];
        HWND m_1c;                                 // +0x1c
        void Refresh1ce7db(i32 sel, CString* out); // thiscall RVA 0x1ce7db
    };
    // The replay/recording model singleton at 0x64bd5c IS the CMulti multiplayer
    // game-state (xref-proven). Now that this TU is MFC (see the <Gruntz/Multi.h>
    // include up top -- the old <Win32.h> wall is broken), OnReset drives it through
    // REAL CMulti members/methods: m_isHost (+0x528), m_5b0, the CString members
    // m_5b4/m_5b8, and Commit3ada (ILT 0x3ada -> 0xbccd0). The semantic name is kept.
    // ReconBatch2.cpp owns the canonical DATA; this extern reloc-masks against it.
    DATA(0x0024bd5c)
    extern CMulti* g_replayModel;
    extern "C" char g_emptyStr_6293f4[]; // 0x6293f4
    // The host dialog; m_6c is a dirty flag, GetItem27d resolves a child CWnd.
    struct ReplayDlg_c3e30 {
        char m_pad0[0x6c];
        i32 m_6c;                           // +0x6c
        CWndItem_c3e30* GetItem27d(i32 id); // thiscall RVA 0x1be27d
        void OnReset();
    };
    // __thiscall(): reset the replay name field and refresh the selected item.
    RVA(0x000c3e30, 0xfe)
    void ReplayDlg_c3e30::OnReset() {
        if (g_replayModel->m_isHost != 0) {
            CWndItem_c3e30* item = GetItem27d(0x4ff);
            if (item != 0) {
                i32 r = SendMessageA(item->m_1c, 0x147, 0, 0);
                if (r != -1) {
                    CString name;
                    item->Refresh1ce7db(r, &name);
                    if (name.GetLength() != 0) {
                        m_6c = 0;
                    }
                    g_replayModel->m_5b0 = 0;
                    g_replayModel->m_5b8 = g_emptyStr_6293f4;
                    g_replayModel->m_5b4 = (LPCTSTR)name;
                    g_replayModel->Commit3ada(0);
                }
            }
        }
    }

    // The Battlez multiplayer setup dialog (CBattlezDlg). Refreshes all four player
    // slots' controls (name edit / kind combo / ready checkbox / colour) from the
    // registry roster + the selection owner (the g_64bd5c CMulti singleton), then
    // invalidates the four colour swatches. It reads the network flag
    // (CMulti::m_isHost) and the local player's colour id (CMulti::m_hostIndex).
    DATA(0x0024bd5c)
    extern CMulti* g_64bd5c;
    // Per-slot roster record hung off the registry at +0x150 (stride 0x238).
    struct PlayerSlot_0c50f0 {
        char m_pad0[0x10];
        i32 m_10; // +0x10 combo index base
        i32 m_14; // +0x14 human/computer flag
        i32 m_18; // +0x18 colour id
        i32 m_1c; // +0x1c ready flag
        i32 m_20; // +0x20 in-use flag
        char m_pad24[0x228 - 0x24];
        i32 m_228;                 // +0x228 combo value
        CString FormatName_3e54(); // __thiscall RVA 0x3e54
    };
    // A resolved dialog child (CWnd) whose HWND lives at +0x1c.
    struct DlgWnd_c4230 {
        char m_pad0[0x1c];
        HWND m_1c;                                // +0x1c m_hWnd
        void EnableWindow_1be6a7(i32 bEnable);    // RVA 0x1be6a7 (CWnd::EnableWindow)
        void SetWindowText_1be520(const char* s); // RVA 0x1be520 (CWnd::SetWindowText)
    };
    void __stdcall FocusHelper_1bb23a(HWND); // RVA 0x1bb23a
    struct BattlezDlg_c4230 {
        char m_pad0[0x5c];
        char* m_5c;          // +0x5c base of the per-player colour table (records at +0x16c)
        i32 LocalSlot2d4c(); // RVA 0x2d4c
        DlgWnd_c4230* NameEdit298c(i32 idx);      // RVA 0x298c
        DlgWnd_c4230* KindCombo1929(i32 idx);     // RVA 0x1929
        DlgWnd_c4230* ReadyCheck1159(i32 idx);    // RVA 0x1159
        DlgWnd_c4230* ColourBtn1753(i32 idx);     // RVA 0x1753
        void SyncColour3a5d(i32 idx, i32 val);    // RVA 0x3a5d
        void SyncKind3ffd(i32 idx);               // RVA 0x3ffd
        DlgWnd_c4230* GetDlgItem_1be27d(i32 nID); // RVA 0x1be27d (CWnd::GetDlgItem)
        i32 UpdatePlayers(i32 force);             // RVA 0x0c4230
    };
    // __thiscall(force): refresh every player row from the roster + selection owner.
    // @early-stop
    // EH-representation wall: /GX frame (CString `name` temp) — the per-branch EH
    // state-index stamps ([esp+EHstate] = N) and the aggregate-TU regalloc/spill
    // recolor diverge from retail; code shape + all DIR32 data refs match. 0.68%->62%.
    RVA(0x000c4230, 0x38e)
    i32 BattlezDlg_c4230::UpdatePlayers(i32 force) {
        FocusHelper_1bb23a(g_pGetFocus());
        i32 f1c = 1;
        i32 f18 = 0;
        i32 idx = 0;
        i32 t = this->LocalSlot2d4c();
        i32 localColour = g_64bd5c->m_isHost ? *(i32*)(this->m_5c + t * 0x238 + 0x16c) : 1;
        i32 off = 0;
        do {
            PlayerSlot_0c50f0* slot = (PlayerSlot_0c50f0*)((char*)g_gameReg + off + 0x150);
            if (slot) {
                if (slot->m_18 != g_64bd5c->m_hostIndex && slot->m_14 && slot->m_20) {
                    f18 = 1;
                }
                i32 enName;
                if (g_64bd5c->m_isHost && slot->m_14 == 0) {
                    enName = 1;
                } else {
                    enName = slot->m_18 == g_64bd5c->m_hostIndex ? 1 : 0;
                }
                this->NameEdit298c(idx)->EnableWindow_1be6a7(enName);
                this->KindCombo1929(idx)->EnableWindow_1be6a7(
                    g_64bd5c->m_isHost && localColour == 0 && slot->m_18 != g_64bd5c->m_hostIndex
                        ? 1
                        : 0
                );
                DlgWnd_c4230* ready = this->ReadyCheck1159(idx);
                ready->EnableWindow_1be6a7(slot->m_18 == g_64bd5c->m_hostIndex ? 1 : 0);
                if (slot->m_1c) {
                    if (slot->m_20) {
                        g_pSendMessageA(ready->m_1c, 0xf1, 1, 0);
                    } else {
                        g_pSendMessageA(ready->m_1c, 0xf1, 0, 0);
                    }
                } else if (slot->m_20) {
                    g_pSendMessageA(ready->m_1c, 0xf1, 0, 0);
                    f1c = 0;
                } else {
                    g_pSendMessageA(ready->m_1c, 0xf1, 0, 0);
                }
                this->ColourBtn1753(idx)->EnableWindow_1be6a7(
                    g_64bd5c->m_isHost && slot->m_20 && localColour == 0 ? 1 : 0
                );
                this->SyncColour3a5d(idx, slot->m_20 ? slot->m_228 : 0);
                if (force == 0) {
                    if (this->LocalSlot2d4c() == idx) {
                        goto next;
                    }
                    if (g_64bd5c->m_isHost && slot->m_14 == 0) {
                        goto next;
                    }
                }
                if (slot->m_20) {
                    {
                        CString name = slot->FormatName_3e54();
                        LPCTSTR pch = (LPCTSTR)name;
                        force = 0;
                        this->NameEdit298c(idx)->SetWindowText_1be520(pch);
                    }
                    if (slot->m_14) {
                        g_pSendMessageA(this->KindCombo1929(idx)->m_1c, 0x14e, 4, 0);
                    } else {
                        g_pSendMessageA(this->KindCombo1929(idx)->m_1c, 0x14e, slot->m_10 + 1, 0);
                    }
                } else {
                    this->NameEdit298c(idx)->SetWindowText_1be520(g_emptyString);
                    g_pSendMessageA(this->KindCombo1929(idx)->m_1c, 0x14e, 0, 0);
                }
                this->SyncKind3ffd(idx);
            }
        next:
            off += 0x238;
            idx++;
        } while (off < 0x8e0);
        if (g_64bd5c->m_isHost) {
            DlgWnd_c4230* ok = this->GetDlgItem_1be27d(1);
            if (ok == 0) {
                return 0;
            }
            ok->EnableWindow_1be6a7(f18 & f1c);
        }
        g_pInvalidateRect(this->GetDlgItem_1be27d(0x501)->m_1c, 0, 1);
        g_pInvalidateRect(this->GetDlgItem_1be27d(0x503)->m_1c, 0, 1);
        g_pInvalidateRect(this->GetDlgItem_1be27d(0x505)->m_1c, 0, 1);
        g_pInvalidateRect(this->GetDlgItem_1be27d(0x507)->m_1c, 0, 1);
        return 1;
    }

    // Item resolver at RVA 0x1753 (push slot; stdcall; returns the CWnd-ish item).
    WndItem* __stdcall ResolveItem_1753(i32 slot);
    // __thiscall host for the two near-identical wrappers below: resolve a list
    // item, clear its selection, cache the new count in g_gameReg, then refresh.
    struct SelHost_0c4ee0 {
        void Update0();
        void Update1();
        void Update3();
        void Refresh(); // thiscall, RVA 0x12d5
    };
    RVA(0x000c4ee0, 0x33)
    void SelHost_0c4ee0::Update0() {
        HWND h = ResolveItem_1753(0)->m_hwnd;
        g_gameReg->m_378 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    RVA(0x000c4f30, 0x33)
    void SelHost_0c4ee0::Update1() {
        HWND h = ResolveItem_1753(1)->m_hwnd;
        g_gameReg->m_5b0 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // Item resolver at RVA 0xc27c0 (push id; stdcall; returns the CWnd-ish item).
    WndItem* __stdcall ResolveItem_c27c0(i32 id);
    // __thiscall host for winapi_0c4f80: resolves dialog item 2, clears its
    // selection, caches the new count in g_gameReg->m_7e8, then refreshes.
    struct SelHost_0c4f80 {
        void Update();
        void Refresh(); // RVA 0xc40b0
    };
    RVA(0x000c4f80, 0x33)
    void SelHost_0c4f80::Update() {
        HWND h = ResolveItem_c27c0(2)->m_hwnd;
        g_gameReg->m_7e8 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    RVA(0x000c4fd0, 0x33)
    void SelHost_0c4ee0::Update3() {
        HWND h = ResolveItem_1753(3)->m_hwnd;
        g_gameReg->m_a20 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // PlayerSlot_0c50f0 / the g_64bd5c CMulti singleton are modelled above (shared with
    // BattlezDlg_c4230::UpdatePlayers).
    WndItem* __stdcall ResolveItem_1159(i32 idx); // RVA 0x1159
    void __stdcall Func1d70(i32 flag);            // RVA 0x1d70
    // __thiscall(idx): toggle slot idx's ready flag from its list item, then either
    // re-sync the whole roster or just refresh that one slot.
    void __stdcall Refresh185c(PlayerSlot_0c50f0* slot); // RVA 0x185c
    struct RosterHost_0c50f0 {
        void Toggle(i32 idx);
        void Sync16db(i32 v); // RVA 0x16db
        void Sync227a();      // RVA 0x227a
        void Sync2c0c();      // RVA 0x2c0c
        void Sync38d2();      // RVA 0x38d2
    };
    RVA(0x000c50f0, 0x9b)
    void RosterHost_0c50f0::Toggle(i32 idx) {
        WndItem* it = ResolveItem_1159(idx);
        if (!it) {
            return;
        }
        i32 sel = SendMessageA(it->m_hwnd, 0xf0, 0, 0);
        PlayerSlot_0c50f0* slot = (PlayerSlot_0c50f0*)((char*)g_gameReg + idx * 0x238 + 0x150);
        if (!slot) {
            return;
        }
        if (sel) {
            slot->m_1c = 1;
        } else {
            slot->m_1c = 0;
        }
        if (g_64bd5c->m_isHost) {
            Func1d70(0);
            Sync16db(1);
            Sync227a();
            Sync2c0c();
            Sync38d2();
        } else {
            Refresh185c(slot);
        }
    }

    // One RVA (0x0cfbd0) = a single __thiscall state-pump on an UNIDENTIFIED owner.
    // On state 0x20 it quiesces the level -- stop the streaming voice (SoundStream::
    // Stop @0x137a80, minervainner), flush the sound bank (CGruntzSoundZ::StopAndFlush
    // @0x138530), reset the 0x40b660 / 0x51af90 sub-mgrs -- then posts WM_COMMAND
    // 0x8023 to the top HWND; else advances the counter via CmdHost_090220::Post
    // (m_1c+1).  The WM_COMMAND host is the already-modeled CmdHost_090220 (the old
    // DispOwner_0cfbd0 dedup'd into it above; its m_4 top-window holder is
    // WndOwner_090220 == the old DispWnd_0cfbd0).  All sub-object calls are direct/
    // reloc-masked, so the leaf typing is matching-neutral.
    // unidentified: the owner (`this`) + the +0xc context chain (DispCtx/DispInner)
    // + the 0x40b660/0x51af90 reset targets -- only inbound edge is ILT thunk 0x1c17
    // (no clean ctor/new trace).
    struct SoundStreamRef_0cfbd0 { // = SoundStream::Stop @0x137a80 (minervainner)
        void Stop137a80();
    };
    struct SoundBankRef_0cfbd0 { // = CGruntzSoundZ::StopAndFlush @0x138530
        void StopAndFlush138530();
    };
    struct CmdHostSub54_0cfbd0 { // unidentified sub-mgr, reset @0x40b660
        void Reset40b660();
    };
    struct CmdHostSub60_0cfbd0 { // unidentified sub-mgr, reset @0x51af90
        void Reset51af90();
    };
    struct DispInner_0cfbd0 { // unidentified +0xc->+0x28 context node
        char m_pad0[0x2c];
        SoundStreamRef_0cfbd0* m_2c; // +0x2c
    };
    struct DispCtx_0cfbd0 { // unidentified +0xc context holder
        char m_pad0[0x28];
        DispInner_0cfbd0* m_28; // +0x28
    };
    struct Dispatcher_0cfbd0 { // unidentified: owner of Dispatch @0xcfbd0
        char m_pad0[4];
        CmdHost_090220* m_4; // +0x04  WM_COMMAND host + audio/net/ui subs
        char m_pad8[0xc - 8];
        DispCtx_0cfbd0* m_c; // +0x0c
        char m_pad10[0x1c - 0x10];
        i32 m_1c; // +0x1c  pump state (0x20 => quiesce)
        char m_pad20[0x40 - 0x20];
        i32 m_40; // +0x40
        char m_pad44[0x1bc - 0x44];
        i32 m_1bc; // +0x1bc
        i32 m_1c0; // +0x1c0
        i32 Dispatch();
    };
    RVA(0x000cfbd0, 0x8f)
    i32 Dispatcher_0cfbd0::Dispatch() {
        if (m_1c == 0x20) {
            m_1c0 = 1;
            m_40 = 1;
            DispInner_0cfbd0* inner = m_c->m_28;
            if (inner->m_2c) {
                inner->m_2c->Stop137a80();
            }
            m_4->m_48->StopAndFlush138530();
            m_4->m_54->Reset40b660();
            m_4->m_60->Reset51af90();
            PostMessageA((HWND)m_4->m_4->m_4, 0x111, 0x8023, 0);
            return 1;
        }
        if (m_1bc) {
            PostMessageA((HWND)m_4->m_4->m_4, 0x111, 0x8023, 0);
            return 1;
        }
        m_4->Post(m_1c + 1);
        return 1;
    }

    struct DrawSink_0d00a0 {
        void Blit(i32 a, i32 b, RECT* rc, i32 d); // thiscall, RVA 0x3751
    };
    struct DrawOwner_0d00a0 {
        char m_pad0[0x5c];
        DrawSink_0d00a0* m_5c; // +0x5c
    };
    struct RectSrc_0d00a0 {
        char m_pad0[0x24];
        char* m_24; // +0x24 (its [+0x10] is the source RECT)
    };
    struct BlitHost_0d00a0 {
        char m_pad0[4];
        DrawOwner_0d00a0* m_4; // +0x04
        char m_pad8[0xc - 8];
        RectSrc_0d00a0* m_c; // +0x0c
        void Show(i32 arg);
    };
    RVA(0x000d00a0, 0x5a)
    void BlitHost_0d00a0::Show(i32 arg) {
        RECT src = *(RECT*)(m_c->m_24 + 0x10);
        RECT dst;
        CopyRect(&dst, &src);
        m_4->m_5c->Blit(8, arg, &dst, 0x10);
    }

    // __thiscall(arg): begin an action once (m_500 guards), arm it, notify the host.
    struct ActionSub_0d7220 {
        i32 Accept(i32 arg); // thiscall, RVA 0x1bedde (on this+0x410)
    };
    struct ActionPeer_0d7220 {
        char m_pad0[0x40];
        i32 m_40; // +0x40
    };
    struct ActionHost_0d7220 {
        char m_pad0[0x40c];
        i32 m_40c;              // +0x40c
        ActionSub_0d7220 m_410; // +0x410 (empty view, 1 byte)
        char m_pad411[0x4e4 - 0x411];
        ActionPeer_0d7220* m_4e4; // +0x4e4
        char m_pad4e8[0x500 - 0x4e8];
        i32 m_500; // +0x500
        char m_pad504[0x510 - 0x504];
        i32 m_510; // +0x510
        i32 Begin(i32 arg);
    };
    RVA(0x000d7220, 0x7b)
    i32 ActionHost_0d7220::Begin(i32 arg) {
        if (m_500) {
            return 0;
        }
        if (!m_410.Accept(arg)) {
            return 0;
        }
        m_40c = arg;
        m_510 = 2;
        m_500 = 1;
        PostMessageA(g_gameReg->m_4->m_4, 0x111, 0x816e, 0);
        if (m_4e4) {
            m_4e4->m_40 |= 1;
        }
        return 1;
    }

    // (0xda200 coin-flip re-homed to Rng::CoinFlip::Flip in src/Gruntz/Random.cpp.)

    DATA(0x0024c69c)
    extern i32 g_flag64c69c; // DAT_0064c69c
    struct CmdWnd_0de590 {
        i32 m_0;
        CmdWnd_0de590* m_4; // +0x04
        void Forward();     // thiscall, RVA 0x3f62 (on this->m_4)
    };
    struct CmdHost_0de590 {
        i32 m_0;
        CmdWnd_0de590* m_4; // +0x04
        void Cancel();
    };
    RVA(0x000de590, 0x2e)
    void CmdHost_0de590::Cancel() {
        if (g_flag64c69c) {
            m_4->Forward();
            return;
        }
        PostMessageA((HWND)(m_4->m_4->m_4), 0x111, 0x8027, 0);
    }

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
                i32 v = (i32)g_gameReg->m_58;
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
        void M1834(char* text);         // thiscall, thunk 0x1834
        void M2d97(i32 a, i32 caption); // thiscall, thunk 0x2d97
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
                winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_58, g_dlgInfoText);
                return 1;
            case 0x111:
                if (wParam == 2) {
                    EndDialog(hDlg, 0);
                    return 1;
                }
                if (wParam == 1) {
                    ((DlgSub58_0e3a40*)g_gameReg->m_58)->M1834(g_dlgInfoText);
                    ((DlgSub58_0e3a40*)g_gameReg->m_58)->M2d97(0, 0x81a6);
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
                winapi_0e4850_SetDlgItemTextA(hDlg, g_gameReg->m_58, g_dlgInfoText);
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

    // __thiscall(): begin/end a paint cycle on the top-level window (m_4->m_4->m_4).
    struct PaintWnd_0fac70 {
        char m_pad0[4];
        PaintWnd_0fac70* m_4; // +0x04
    };
    struct PaintHost_0fac70 {
        char m_pad0[4];
        PaintWnd_0fac70* m_4; // +0x04
        i32 Paint();
    };
    RVA(0x000fac70, 0x4c)
    i32 PaintHost_0fac70::Paint() {
        if (!m_4) {
            return 0;
        }
        if (!m_4->m_4) {
            return 0;
        }
        PAINTSTRUCT ps;
        BeginPaint((HWND)m_4->m_4->m_4, &ps);
        EndPaint((HWND)m_4->m_4->m_4, &ps);
        return 1;
    }

    // Free init helper at RVA 0x500930 (__stdcall(int)).
    void __stdcall Prep_500930(i32 flag);
    // A sub-object reached via g_gameReg->m_curState whose Refresh() is at RVA 0x4d8c60.
    struct SubMgr_0fe460 {
        void Refresh(); // RVA 0x4d8c60
    };
    // The screen object this method initialises (RVA 0xfe460).
    struct Screen_0fe460 {
        i32 m_0; // +0x00
        char m_pad4[0x10 - 4];
        RECT m_10; // +0x10
        char m_pad20[0x10c - 0x20];
        i32 m_10c; // +0x10c
        char m_pad110[0x548 - 0x110];
        i32 m_548; // +0x548
        i32 Open();
        void Resize(i32 n);          // RVA 0x4fe3e0
        i32 Validate();              // RVA 0x4ffde0 (thunk via 0x3a08)
        void Activate(i32 a, i32 n); // RVA 0x500d70
    };
    // __thiscall: lay out the 0xa0x0x1e0 screen, validate it, else report error.
    RVA(0x000fe460, 0x83)
    i32 Screen_0fe460::Open() {
        if (m_548 == 0 && m_0 != 1) {
            Prep_500930(1);
            SetRect(&m_10, 0, 0, 0xa0, 0x1e0);
            Resize(1);
            ((SubMgr_0fe460*)g_gameReg->m_curState)->Refresh();
            if (!Validate()) {
                g_gameReg->ReportError(0x80e4, 0x448);
                return 0;
            }
            Activate(m_10c, 3);
        }
        return 1;
    }

    void __stdcall Prep_12fd(i32 mode); // RVA 0x12fd
    struct CGameWnd_fe600 {
        i32 Sub3d55(); // thiscall, RVA 0x3d55 (on g_gameReg->m_curState)
    };
    struct RectWnd_fe600 {
        i32 m_0; // +0x00
        char m_pad4[0x10 - 4];
        RECT m_10; // +0x10
        char m_pad20[0x548 - 0x20];
        i32 m_548; // +0x548
        i32 Reset();
        void Sub194c(i32 v); // thiscall, RVA 0x194c
    };
    RVA(0x000fe600, 0x49)
    i32 RectWnd_fe600::Reset() {
        if (!m_548 && m_0 != 2) {
            Prep_12fd(1);
            SetRect(&m_10, -1, -1, -1, -1);
            Sub194c(2);
            ((CGameWnd_fe600*)g_gameReg->m_curState)->Sub3d55();
        }
        return 1;
    }

    // The blit target reached through the layer node (node->m_2c); Blit13ef90 paints
    // a rect-source into a destination rect with the given mode flags.
    struct BlitTarget_115300 {
        void Blit13ef90(i32 dx, i32 dy, void* src, RECT* rc, i32 flags); // thiscall RVA 0x13ef90
    };
    struct LayerNode_115300 {
        char m_pad0[0x2c];
        BlitTarget_115300* m_2c; // +0x2c
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
        BlitTarget_115300* dst = node->m_2c;
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
        dst->Blit13ef90(dx, dy, srcHandle, &rc2, flags);
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

    i32 __stdcall PlaySound3_136550(i32 a, i32 b, i32 flag); // RVA 0x136550
    // __stdcall(a, b): default the 3rd arg to 0.
    // @early-stop
    // regalloc free-list-pick wall (docs/patterns/select-zero-mask-dest-register.md):
    // body byte-exact except retail loads `a` into edx (`mov edx,[esp+4]`) while our
    // cl picks ecx after eax is taken by `b` - a single free-list register pick, not
    // source-steerable (~98.6%).
    RVA(0x00137720, 0x14)
    i32 __stdcall directx_wrapper_caller_137720_DSOUND_1_DirectSoundCreate(i32 a, i32 b) {
        return PlaySound3_136550(a, b, 0);
    }

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

    // The four CriticalSection thunks (0x16c9c0/d0/e0/f0) are the intended residual:
    // per game-not-CRT matcher policy they wrap the Win32 CRITICAL_SECTION primitives
    // and are not game code to re-home, so they stay here (no owning game class).
    // __cdecl thunk over InitializeCriticalSection.
    RVA(0x0016c9c0, 0xc)
    void winapi_16c9c0_InitializeCriticalSection(CRITICAL_SECTION* cs) {
        InitializeCriticalSection(cs);
    }

    // __cdecl(cs): thin wrapper over DeleteCriticalSection.
    RVA(0x0016c9d0, 0xc)
    void winapi_16c9d0_DeleteCriticalSection(LPCRITICAL_SECTION cs) {
        DeleteCriticalSection(cs);
    }

    // __cdecl thunk over EnterCriticalSection.
    RVA(0x0016c9e0, 0xc)
    void winapi_16c9e0_EnterCriticalSection(CRITICAL_SECTION* cs) {
        EnterCriticalSection(cs);
    }

    // __cdecl thunk over LeaveCriticalSection.
    RVA(0x0016c9f0, 0xc)
    void winapi_16c9f0_LeaveCriticalSection(CRITICAL_SECTION* cs) {
        LeaveCriticalSection(cs);
    }

    // (0x1775f0 PalHost::Apply (with g_palModule_6bf6e0) re-homed to ResLoaders in
    // src/Gruntz/ResourceLoaders.cpp.)

    // @confidence: low
    // @source: winapi:IntersectRect
    // @early-stop
    // Rect-clipping pass over the global viewport rect g_683ea0/ea4/eac/eb0/eb4
    // (GAME code, 1516 B), intersecting via the PTR_IntersectRect function pointer.
    // Deferred to the leaf-first final sweep: a >512B branchy geometry body; a
    // partial under-counts AND diverges its regalloc, so the return-0 normalization
    // artifact is kept per the >512B REVERT rule.
    RVA(0x00179e70, 0x5ec)
    i32 __stdcall winapi_179e70_IntersectRect(i32, i32, i32, i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // CSmackWin::Frame (0x17caa0, the per-frame renderer Pump/Advance call) + its
    // palette-snapshot helper (0x17cd90) - both CSmackWin methods (src/Gruntz/
    // SmackerVideoWindow.cpp), left here for the final sweep: RenderFrame reads m_10 as
    // a Smack* and m_24 as a DDraw surface where the moved open/pump/close use m_10 as
    // an i32 handle / m_24 as a SmkBuf, so folding them into the shared CSmackWin view
    // needs those two fields typed once + casts (verify class identity first).
    // Smacker imports (SmackToBuffer/SmackDoFrame/SmackToBufferRect/SmackNextFrame)
    // + the Smack stream handle come from <smack.h>.
    // The DirectDraw surface the frame is locked/blitted into (manual vtable).
    struct DDSurf_17caa0;
    struct DDSurfVtbl_17caa0 {
        void* s0[25];                                                   // +0x00..+0x60
        i32(__stdcall* Lock)(DDSurf_17caa0*, void*, void*, u32, void*); // +0x64
        void* s26;                                                      // +0x68
        i32(__stdcall* Restore)(DDSurf_17caa0*);                        // +0x6c
        void* s28[4];                                                   // +0x70..+0x7c
        i32(__stdcall* Unlock)(DDSurf_17caa0*, void*);                  // +0x80
    };
    struct DDSurf_17caa0 {
        DDSurfVtbl_17caa0* vptr;
    };
    // The decoded Smacker stream header is the vendored `Smack` (see <smack.h>).
    struct MoviePlayer_17caa0 {
        char m_pad0[0x10];
        Smack* m_10; // +0x10  decoded Smacker stream
        char m_pad14[0x24 - 0x14];
        DDSurf_17caa0* m_24; // +0x24
        char m_pad28[0x9c - 0x28];
        char m_desc[0xac - 0x9c]; // +0x9c DDSURFACEDESC head
        i32 m_ac;                 // +0xac desc.lPitch
        char m_padb0[0xc0 - 0xb0];
        void* m_c0; // +0xc0 desc.lpSurface
        char m_padc4[0x50c - 0xc4];
        i32 m_50c; // +0x50c
        i32 m_510; // +0x510 flags
        i32 m_514; // +0x514 full-frame flag
        char m_pad518[0x520 - 0x518];
        i32 m_520;                          // +0x520
        void Sub17ca10();                   // RVA 0x17ca10
        void Sub17cdf0(i32, i32, i32, i32); // RVA 0x17cdf0 (blit dirty rect)
        i32 RenderFrame();
    };
    // __thiscall(): lock the surface, decode the current frame into it, blit the
    // changed region, then advance to the next frame (0 once the last frame plays).
    RVA(0x0017caa0, 0x13b)
    i32 MoviePlayer_17caa0::RenderFrame() {
        if (m_10->NewPalette && m_520 == 8) {
            Sub17ca10();
        }
        i32 hr = m_24->vptr->Lock(m_24, 0, m_desc, 1, 0);
        while (hr == (i32)0x887601c2) {
            if (m_24->vptr->Restore(m_24) != 0) {
                goto afterLock;
            }
            hr = m_24->vptr->Lock(m_24, 0, m_desc, 1, 0);
        }
        if (hr == 0) {
            SmackToBuffer(m_10, 0, 0, m_ac, m_10->Height, m_c0, m_510);
            SmackDoFrame(m_10);
            m_50c = 1;
            m_24->vptr->Unlock(m_24, m_c0);
        }
    afterLock:
        if (m_514 != 1) {
            while (SmackToBufferRect(m_10, 0) != 0) {
                Sub17cdf0(m_10->LastRectx, m_10->LastRecty, m_10->LastRectw, m_10->LastRecth);
            }
        } else {
            Sub17cdf0(0, 0, m_10->Width, m_10->Height);
        }
        Smack* s = m_10;
        if (s->FrameNum == s->Frames - 1) {
            return 0;
        }
        SmackNextFrame(s);
        return 1;
    }

    // (0x17cd90 PalCache::Snapshot re-homed to ResLoaders in
    // src/Gruntz/ResourceLoaders.cpp.)

    // The source object whose m_4->m_10->m_10 carries the default tile extent.
    struct TileExtent_182ab0 {
        char m_pad0[0x10];
        i32 m_10; // +0x10 width
        i32 m_14; // +0x14 height
    };
    struct TileSrc_182ab0 {
        char m_pad0[0x10];
        TileExtent_182ab0* m_10; // +0x10
    };
    struct TileSrcHost_182ab0 {
        char m_pad0[4];
        TileSrc_182ab0* m_4; // +0x04
    };
    struct Region_182ab0 {
        TileSrcHost_182ab0* m_0; // +0x00
        i32 m_4;                 // +0x04
        RECT m_8;                // +0x08 (left/top/right/bottom = m_8/m_c/m_10/m_14)
        i32 m_18;                // +0x18
        i32 m_1c;                // +0x1c
        i32 m_20;                // +0x20
        char m_pad24[0x40 - 0x24];
        i32 m_40; // +0x40
        i32 Init(TileSrcHost_182ab0* src, i32 a, RECT* rc, i32 d, i32 e, i32 f);
    };
    RVA(0x00182ab0, 0x7b)
    i32 Region_182ab0::Init(TileSrcHost_182ab0* src, i32 a, RECT* rc, i32 d, i32 e, i32 f) {
        if (!src) {
            return 0;
        }
        m_0 = src;
        m_4 = a;
        m_20 = f;
        m_18 = d;
        m_1c = e;
        m_40 = 0;
        if (rc) {
            CopyRect(&m_8, rc);
            return 1;
        }
        m_8.left = 0;
        m_8.top = 0;
        m_8.right = src->m_4->m_10->m_10 - 1;
        m_8.bottom = src->m_4->m_10->m_14 - 1;
        return 1;
    }

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
