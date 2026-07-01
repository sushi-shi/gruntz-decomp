#include <Win32.h>

#include <rva.h>
#include <string.h>

// Auto-generated API-caller stubs from docs/api-caller-name-plan.tsv.
// Greenfield only: tracked/already-tried and named-untracked library rows are intentionally excluded.
// One stub is emitted per RVA; rows with multiple API categories are merged.

// A window-host chain hung off the game registry: its m_4 is the top-level HWND.
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
    void* m_2c; // +0x2c
    char m_pad30[0x54 - 0x30];
    void* m_54; // +0x54
    void* m_58; // +0x58
    char m_pad5c[0x100 - 0x5c];
    i32 m_100; // +0x100
    char m_pad104[0x118 - 0x104];
    i32 m_118; // +0x118
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
struct GameObj510 {
    char m_pad0[0x510];
    i32 m_510; // +0x510
};
DATA(0x0064556c)
extern CGameReg* g_gameReg;

// Miles Sound System (AIL) imports - reached through the IAT (ff 15 [__imp]).
extern "C" {
    __declspec(dllimport) i32 __stdcall AIL_set_XMIDI_master_volume(i32 driver, i32 volume);
    __declspec(dllimport) i32 __stdcall AIL_start_sequence(i32 seq);
    __declspec(dllimport) i32 __stdcall AIL_set_sequence_loop_count(i32 seq, i32 count);
    __declspec(dllimport) i32 __stdcall AIL_resume_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_startup();
    __declspec(dllimport) i32 __stdcall AIL_midiOutOpen(i32* driver, i32 dunno, i32 devid);
    __declspec(dllimport) i32 __stdcall AIL_XMIDI_master_volume(i32 driver);
    __declspec(dllimport) void __stdcall AIL_end_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_set_sequence_tempo(i32 seq, i32 tempo, i32 ms);
    __declspec(dllimport) void __stdcall AIL_shutdown();
    __declspec(dllimport) void __stdcall AIL_stop_sequence(i32 seq);
    __declspec(dllimport) void __stdcall AIL_set_sequence_volume(i32 seq, i32 volume, i32 ms);
    __declspec(dllimport) void __stdcall AIL_release_sequence_handle(i32 seq);
    __declspec(dllimport) i32 __stdcall AIL_allocate_sequence_handle(i32 driver);
    __declspec(dllimport) i32 __stdcall AIL_init_sequence(i32 seq, void* xmidi, i32 seqNum);
}

// The AIL MIDI driver handle (DAT_00653c5c), 0 when no driver is open.
DATA(0x00653c5c)
extern i32 g_ailMidiDriver;

// Monotonic counter naming auto-generated MIDI sequences ("MIDI%i", DAT_00653c60).
DATA(0x00653c60)
extern i32 g_midiSeqCounter;

// Cached AIL driver handle passed to AIL_set_* (DAT_00653c64).
DATA(0x00653c64)
extern i32 g_ailDriver64;

// MS-CRT-style LCG RNG state shared by the timeGetTime random helpers.
DATA(0x006c127d)
extern char g_rngSeeded; // bit0 set once the generator has been seeded
DATA(0x006c1288)
extern i32 g_rngState; // the current 32-bit LCG state

// Per-frame cached random bit used by the deterministic coin-flip helper.
DATA(0x0064c22c)
extern char g_coinRolled; // bit0 set once this frame's coin was rolled
DATA(0x0064c26c)
extern i32 g_coinValue; // the cached 0/1 result

// GetDlgItem(hWnd,0x4b6) cache (DAT_00648ce0), shared by several timer wrappers.
DATA(0x00648ce0)
extern HWND g_dlgItem_648ce0;

// The Rez heap allocator (_RezAlloc, defined in EngineExternFns.cpp).
extern "C" void* RezAlloc(u32 size);

namespace ApiCallerStubs {
    // Fake placeholder host: these ApiCaller stubs are __thiscall (disasm shows
    // they take `this` in ecx) but their real owning class isn't recovered yet.
    // Membership surfaces the implicit `this` + __thiscall ABI; explicit args are
    // the N/4 from the callee's `ret N`.
    struct ThisStubOwnerUnknown {
        i32 winapi_015fe0_SendMessageA(i32);
        i32 winapi_032ce0_IntersectRect(i32);
        i32 winapi_075c60_CopyRect_SetRect(i32, i32, i32, i32, i32, i32);
        i32 winapi_0c46b0_KillTimer_timeGetTime_wsprintfA();
        i32 winapi_0c7ec0_timeGetTime(i32, i32, i32);
        i32 winapi_0d7520_wsprintfA(i32, i32, i32, i32);
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
        void LoadPickupSprites(i32, i32, i32, i32, i32);
        void LoadBombGruntRunConfig2();
        void LoadFreezeSpellAssets();
        void LoadFinishLevelSprite(i32);
        void LoadMonologoSprite();
        void LoadStateImages_vfunc8();
        // LoadImageBanks (0x0cffe0), LoadActionTileSprites (0x0db600),
        // LoadLevelSounds (0x0db6c0), LoadLevelImages (0x0db7e0) re-homed as CPlay
        // methods in src/Gruntz/CPlay.cpp.
        void LoadWarlordSprites(i32, i32);
        void LoadLevelPreviewScreen();
        // LoadGruntzPalette (0x0e2d10) re-homed to src/Gruntz/SpriteRefTable.cpp.
        // BuildResourceTabStatusBar (0xe8a70), BuildStatzTabStatusBar (0xe9600) and
        // BuildMultiplayerTabStatusBar (0xea1f0) re-homed as CSbTab methods in
        // src/Gruntz/StatusBarTabBuilders.cpp.
        void LoadGameAssetNamespaces(i32, i32, i32);
        void UpdateDestructButtonStatusBar2(i32);
        void DebugPrintf();
        void Stub_1c152f(i32);
        void Stub_1ccae7(i32, i32, i32);
        void Stub_1ccbfc(i32, i32, i32, i32);
    };

    // ---- Proximity-attributed owners (HIGH, both-sides RVA bracket;
    // docs/tu-spatial-structure.md). These stubs were ThisStubOwnerUnknown;
    // their real classes live in their own TUs - these are minimal placeholder
    // hosts so each stub files under its attributed class (matching-neutral). ----
    struct CFader {
        i32 winapi_17e620_GetTickCount(i32, i32, i32);
    };
    struct CGrunt {
        i32 winapi_057db0_IntersectRect();
        void LoadGruntCombatAnimations(i32, i32, i32, i32, i32, i32, i32, i32);
    };
    struct CGruntSpawnConfig {
        // LoadGruntSpawnConfig (0x11afb0) re-homed as a real CGruntSpawnConfig method
        // in src/Gruntz/CGruntSpawnConfig.cpp.
        i32 winapi_11b3b0_timeGetTime(i32, i32, i32, i32, i32, i32);
    };
    struct CMulti {
        i32 winapi_0b6b40_timeGetTime_wsprintfA();
        i32 winapi_0b6e90_SetRect();
    };

    // @confidence: low
    // @source: winapi:CopyRect;SetRect
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

    // @confidence: low
    // @source: winapi:timeGetTime
    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    RVA(0x0000cd00, 0x46)
    i32 winapi_00cd00_timeGetTime() {
        i32 seed;
        if (!(g_rngSeeded & 1)) {
            g_rngSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rngState;
        }
        g_rngState = seed * 214013 + 2531011;
        return (g_rngState >> 0x10) & 0x7fff;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // __thiscall(lo, hi, a3, a4): roll a random value in [lo,hi] (lazily-seeded
    // LCG; coin-flip endpoints when the span is empty) and cache it + the params.
    struct RngBox_00cd70 {
        char m_pad0[0x40];
        i32 m_40; // +0x40 lo
        i32 m_44; // +0x44 hi
        i32 m_48; // +0x48
        i32 m_4c; // +0x4c
        i32 m_50; // +0x50 rolled value
        i32 m_54; // +0x54 (1 once rolled)
        void Roll(i32 lo, i32 hi, i32 a3, i32 a4);
    };
    RVA(0x0000cd70, 0xe5)
    void RngBox_00cd70::Roll(i32 lo, i32 hi, i32 a3, i32 a4) {
        i32 span = hi - lo + 1;
        m_40 = lo;
        m_44 = hi;
        m_48 = a3;
        m_4c = a4;
        i32 seed;
        if (span == 0) {
            if (!(g_rngSeeded & 1)) {
                g_rngSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_rngState;
            }
            g_rngState = seed * 214013 + 2531011;
            if (g_rngState & 0x10000) {
                m_54 = 1;
                m_50 = lo;
            } else {
                m_54 = 1;
                m_50 = hi;
            }
            return;
        }
        if (!(g_rngSeeded & 1)) {
            g_rngSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rngState;
        }
        g_rngState = seed * 214013 + 2531011;
        m_54 = 1;
        m_50 = lo + ((g_rngState >> 0x10) & 0x7fff) % span;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
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
            PostMessageA(m_4->m_4->m_4, 0x111, 0x8023, 0);
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:GetWindow;GetWindowLongA;SetWindowLongA
    // @stub
    RVA(0x00014d00, 0xa68)
    i32 __stdcall winapi_014d00_GetWindow_GetWindowLongA_SetWindowLongA(i32) {
        return 0;
    }

    // @confidence: low
    // A CWnd-ish object whose HWND lives at +0x1c (returned by the dialog-item
    // resolver thunks that several wrappers below call).
    struct WndItem {
        char m_pad0[0x1c];
        HWND m_hwnd; // +0x1c
    };
    // Dialog-item resolver at RVA 0x15ac0 (reached through the 0x1e7e jmp-thunk).
    WndItem* __stdcall ResolveItem_15ac0(i32 id);

    // @source: winapi:SendMessageA
    // __stdcall(id, wParam): set the resolved item's listbox selection (0x14e).
    RVA(0x00015cc0, 0x23)
    i32 __stdcall winapi_015cc0_SendMessageA(i32 id, i32 wParam) {
        WndItem* it = ResolveItem_15ac0(id);
        return SendMessageA(it->m_hwnd, 0x14e, wParam, 0);
    }

    // @source: winapi:SendMessageA
    // __stdcall(id): send 0x147 (clear listbox selection) to the resolved item.
    RVA(0x00015d00, 0x20)
    void __stdcall winapi_015d00_SendMessageA(i32 id) {
        WndItem* it = ResolveItem_15ac0(id);
        SendMessageA(it->m_hwnd, 0x147, 0, 0);
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // Dialog-item resolver at RVA 0x33a0 (stdcall, returns a CWnd-ish whose HWND is +0x1c).
    WndItem* __stdcall ResolveItem_33a0(i32 id);
    // __stdcall(id): clear listbox selection (0x147) of the resolved item; return +1.
    RVA(0x00015d30, 0x21)
    i32 __stdcall winapi_015d30_SendMessageA(i32 id) {
        WndItem* it = ResolveItem_33a0(id);
        return SendMessageA(it->m_hwnd, 0x147, 0, 0) + 1;
    }

    // @source: winapi:SendMessageA
    // __stdcall(id, wParam): set the resolved item's listbox selection to wParam-1.
    RVA(0x00015d70, 0x24)
    i32 __stdcall winapi_015d70_SendMessageA(i32 id, i32 wParam) {
        WndItem* it = ResolveItem_33a0(id);
        return SendMessageA(it->m_hwnd, 0x14e, wParam - 1, 0);
    }

    // @confidence: low
    // @source: winapi:CreateSolidBrush;FillRect;GetClientRect
    // @stub
    RVA(0x000160f0, 0x245)
    i32 winapi_0160f0_CreateSolidBrush_FillRect_GetClientRect() {
        return 0;
    }

    // @confidence: low
    // @confidence: low
    // @source: winapi:SendMessageA
    // @stub
    RVA(0x000180e0, 0x23f)
    i32 __stdcall winapi_0180e0_SendMessageA(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
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

    // @confidence: low
    // @source: winapi:timeGetTime
    // __stdcall(hi, lo): lazily-seeded LCG random in [lo,hi]. When the span is
    // empty (hi==lo-1) it coin-flips between the endpoints on bit 0x10000.
    RVA(0x00019f50, 0xb2)
    i32 __stdcall winapi_019f50_timeGetTime(i32 lo, i32 hi) {
        i32 span = hi - lo + 1;
        i32 seed;
        if (span != 0) {
            if (!(g_rngSeeded & 1)) {
                g_rngSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_rngState;
            }
            g_rngState = seed * 214013 + 2531011;
            return lo + ((g_rngState >> 0x10) & 0x7fff) % span;
        }
        if (!(g_rngSeeded & 1)) {
            g_rngSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rngState;
        }
        g_rngState = seed * 214013 + 2531011;
        if (g_rngState & 0x10000) {
            return lo;
        }
        return hi;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    RVA(0x0001a700, 0x6b6)
    i32 winapi_01a700_CopyRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
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

    // @confidence: low
    // @source: winapi:DrawTextA
    // @stub
    RVA(0x00021f20, 0x162)
    i32 __stdcall winapi_021f20_DrawTextA(i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetAsyncKeyState;SelectObject
    // @stub
    RVA(0x00022160, 0x18e)
    i32 __stdcall winapi_022160_GetAsyncKeyState_SelectObject(i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SelectObject;SetBkColor;SetBkMode;SetTextColor
    // @stub
    RVA(0x00022810, 0x22a)
    i32 __stdcall winapi_022810_DrawTextA_SelectObject_SetBkColor_SetBkMode_SetTextColor(
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
    ) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // __thiscall(src): clip the (0,0,m_c,m_10) box against the optional src rect
    // (right/bottom inclusive→+1), store the clipped rect at +0x60 and its w/h.
    struct ClipHost_02b340 {
        char m_pad0[0xc];
        i32 m_c;  // +0x0c
        i32 m_10; // +0x10
        char m_pad14[0x60 - 0x14];
        RECT m_rc60; // +0x60
        i32 m_w70;   // +0x70
        i32 m_h74;   // +0x74
        void Clip(const RECT* src);
    };
    RVA(0x0002b340, 0xaa)
    void ClipHost_02b340::Clip(const RECT* src) {
        RECT a, b;
        b.left = 0;
        b.top = 0;
        b.right = m_c;
        b.bottom = m_10;
        if (src) {
            a.left = src->left;
            a.top = src->top;
            a.right = src->right + 1;
            a.bottom = src->bottom + 1;
        } else {
            a.left = 0;
            a.top = 0;
            a.right = m_c;
            a.bottom = m_10;
        }
        if (!IntersectRect(&m_rc60, &a, &b)) {
            m_rc60 = a;
        }
        m_w70 = m_rc60.right - m_rc60.left;
        m_h74 = m_rc60.bottom - m_rc60.top;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x0002c690, 0xdb4)
    i32 __stdcall winapi_02c690_IntersectRect(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    // proximity: CBattlezSpawnMgr_or_CGruntSpawnMgr@-0xc80 | GridUnit@+0x1710
    RVA(0x00032ce0, 0x448)
    i32 ThisStubOwnerUnknown::winapi_032ce0_IntersectRect(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x00033520, 0xbc3)
    i32 __stdcall winapi_033520_IntersectRect(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // __cdecl(hWnd): mirror checkbox 0x46d into the game state + enable ctrl 0x470.
    RVA(0x00036d00, 0x40)
    void winapi_036d00_EnableWindow_GetDlgItem_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            i32 state = IsDlgButtonChecked(hWnd, 0x46d);
            g_gameReg->Method92340(state);
            EnableWindow(GetDlgItem(hWnd, 0x470), state);
        }
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // __cdecl(hWnd): mirror checkbox 0x475 into m_100 + enable ctrl 0x476 by it.
    RVA(0x00036d50, 0x3c)
    void winapi_036d50_EnableWindow_GetDlgItem_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            i32 checked = IsDlgButtonChecked(hWnd, 0x475);
            g_gameReg->m_100 = checked;
            EnableWindow(GetDlgItem(hWnd, 0x476), checked);
        }
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetDlgItem;IsDlgButtonChecked
    // __cdecl(hWnd): mirror checkbox 0x471 into the game state + enable ctrl 0x472.
    RVA(0x00036da0, 0x40)
    void winapi_036da0_EnableWindow_GetDlgItem_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            i32 state = IsDlgButtonChecked(hWnd, 0x471);
            g_gameReg->Method3df5(state);
            EnableWindow(GetDlgItem(hWnd, 0x472), state);
        }
    }

    // @confidence: low
    // @source: winapi:IsDlgButtonChecked
    // __cdecl: cache the checkbox state into g_gameReg->m_118.
    RVA(0x00036e10, 0x26)
    void winapi_036e10_IsDlgButtonChecked(HWND hWnd) {
        if (g_gameReg) {
            g_gameReg->m_118 = IsDlgButtonChecked(hWnd, 0x455);
        }
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;GetScrollInfo
    // __cdecl(hDlg, id): read the scroll position of dialog item `id`.
    RVA(0x00036ec0, 0x41)
    i32 winapi_036ec0_GetDlgItem_GetScrollInfo(HWND hDlg, i32 id) {
        HWND h = GetDlgItem(hDlg, id);
        if (!h) {
            return 0;
        }
        SCROLLINFO si;
        si.cbSize = 0x1c;
        si.fMask = SIF_POS;
        GetScrollInfo(h, SB_CTL, &si);
        return si.nPos;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetScrollInfo
    // __cdecl(hDlg, id, pos, max): set dialog item `id`'s scroll range/page/pos.
    RVA(0x000371e0, 0x5b)
    void winapi_0371e0_GetDlgItem_SetScrollInfo(HWND hDlg, i32 id, i32 pos, i32 max) {
        HWND h = GetDlgItem(hDlg, id);
        if (h) {
            SCROLLINFO si;
            si.nMax = max;
            si.cbSize = 0x1c;
            si.fMask = 0x17;
            si.nMin = 1;
            si.nPage = 0xa;
            si.nPos = pos;
            SetScrollInfo(h, SB_CTL, &si, FALSE);
        }
    }

    // @confidence: low
    // @source: winapi:GetScrollInfo;SetScrollInfo
    // @stub
    RVA(0x00037260, 0x1fd)
    i32 winapi_037260_GetScrollInfo_SetScrollInfo() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // __stdcall(hDlg, id, lo, hi): find the list item whose data == MAKELONG(lo,hi)
    // and select it. Returns 1 if found.
    RVA(0x00038150, 0x91)
    i32 __stdcall winapi_038150_GetDlgItem(HWND hDlg, i32 id, i32 lo, i32 hi) {
        HWND list = GetDlgItem(hDlg, id);
        if (!list) {
            return 0;
        }
        i32 searching = 1;
        i32 i = 0;
        while (searching) {
            i32 data = SendMessageA(list, 0x150, i, 0);
            if (data != -1) {
                i32 itemLo = data & 0xffff;
                i32 itemHi = (u32)data >> 0x10;
                if (itemLo == lo && itemHi == hi) {
                    if (SendMessageA(list, 0x147, 0, 0) != i) {
                        SendMessageA(list, 0x14e, i, 0);
                    }
                    return 1;
                }
            } else {
                searching = 0;
            }
            i++;
        }
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
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

    // @confidence: low
    // @source: winapi:PostMessageA
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
                PostMessageA(m_4->m_4->m_4, 0x111, 0x8023, 0);
            } else {
                PostMessageA(m_4->m_4->m_4, 0x111, 0x8027, 0);
            }
        }
        return 1;
    }

    // @confidence: low
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
    // @source: winapi:PostMessageA;PtInRect
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

    // @confidence: low
    // @source: winapi:CreateRectRgn;DrawTextA;SetRect
    // @stub
    RVA(0x00039a60, 0x179)
    i32 winapi_039a60_CreateRectRgn_DrawTextA_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SendMessageA
    // @stub
    RVA(0x0003af90, 0x194)
    i32 winapi_03af90_GetDlgItem_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // @stub
    RVA(0x0003b1a0, 0x118)
    i32 winapi_03b1a0_GetDlgItem() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetDlgItem
    // The shared MFC CString scratch globals used to assemble custom-level paths;
    // m_data (the CString data ptr) is at offset 0.
    struct GameCStr_62c25c {
        char* m_data;               // +0x00
        void Assign(const char* s); // operator=  RVA 0x1b9e74 (thiscall)
        void Append(const char* s); // operator+= RVA 0x1ba0c8 (thiscall)
        void Reset();               // RVA 0x1b9c69 (thiscall)
    };
    DATA(0x0062c25c)
    extern GameCStr_62c25c g_customPath;
    DATA(0x0062c264)
    extern GameCStr_62c25c g_customName;
    i32 GetGameRootDir_11fc10(char* buf, i32 size); // RVA 0x11fc10 (cdecl)
    i32 FileExists_4282(const char* path);          // RVA 0x4282 (cdecl)
    // __cdecl(hWnd): build "<root>\Custom\<sel>.WWD" for the selected custom level.
    RVA(0x0003b310, 0x10d)
    i32 winapi_03b310_GetDlgItem(HWND hWnd) {
        char itemText[256];
        char dirBuf[256];
        HWND lb = GetDlgItem(hWnd, 0x3fc);
        if (!lb) {
            return 0;
        }
        i32 sel = SendMessageA(lb, 0x188, 0, 0);
        if (sel == -1) {
            return 0;
        }
        if (SendMessageA(lb, 0x189, sel, (LPARAM)itemText) == -1) {
            return 0;
        }
        if (!GetGameRootDir_11fc10(dirBuf, 0xfe)) {
            return 0;
        }
        g_customPath.Assign(dirBuf);
        g_customPath.Append("\\Custom\\");
        g_customPath.Append(itemText);
        g_customPath.Append(".WWD");
        if (!FileExists_4282(g_customPath.m_data)) {
            g_customPath.Reset();
            return 0;
        }
        g_customName.Assign(itemText);
        return 1;
    }

    // 0x4a9f0 (CGrunt::winapi_04a9f0_CopyRect_OffsetRect) reconstructed in
    // src/Gruntz/Grunt.cpp.

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x00057db0, 0x8f8)
    i32 CGrunt::winapi_057db0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect;SetRect
    // @stub
    // proximity: CTriggerMgr@-0x170 | CTerrainTileLoader@+0x230
    RVA(0x00075c60, 0x1ba)
    i32 ThisStubOwnerUnknown::winapi_075c60_CopyRect_SetRect(i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // @source: winapi:PtInRect
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

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x00078060, 0x18d)
    i32 __stdcall winapi_078060_SetRect(i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA;wsprintfA
    // @stub
    RVA(0x000862f0, 0x3d5a)
    i32 __stdcall winapi_0862f0_PostMessageA_wsprintfA(i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // __thiscall(l, t, r, b): the object IS the RECT being initialised.
    struct RectHost_08c380 {
        RECT m_rc;
        void Set(i32 l, i32 t, i32 r, i32 b);
    };
    RVA(0x0008c380, 0x1e)
    void RectHost_08c380::Set(i32 l, i32 t, i32 r, i32 b) {
        SetRect(&m_rc, l, t, r, b);
    }

    // @confidence: low
    // @source: winapi:SetRect
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

    // @confidence: low
    // @source: winapi:MessageBoxA
    // The shared caption buffer (DAT_0060aac8) passed as the MessageBoxA title.
    DATA(0x0060aac8)
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
        i32 Show(i32 type, i32 text);
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
    i32 MsgHost_08ee70::Show(i32 type, i32 text) {
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

    // @confidence: low
    // @source: winapi:CreateProcessA;RegQueryValueA
    // @stub
    RVA(0x0008f120, 0x168)
    i32 __stdcall winapi_08f120_CreateProcessA_RegQueryValueA(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
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

    // @confidence: low
    // @source: winapi:PostMessageA
    // __thiscall(int code): clamp code into (0,0x29] and post a 0x111 command.
    struct WndOwner_090220 {
        i32 m_0;
        HWND m_4; // +0x04 = HWND
    };
    struct CmdHost_090220 {
        i32 m_0;
        WndOwner_090220* m_4; // +0x04
        void Post(i32 code);
    };
    RVA(0x00090220, 0x2f)
    void CmdHost_090220::Post(i32 code) {
        if (code > 0 && code <= 0x29) {
            i32 v = (code == 0x29) ? 1 : code;
            PostMessageA(m_4->m_4, 0x111, 0x807f, v);
        }
    }

    // @confidence: low
    // @source: winapi:CreateProcessA;wsprintfA
    // __stdcall(exe, dir): build "<dir>\<exe>" and launch it with dir as the cwd.
    RVA(0x00090860, 0xd3)
    i32 __stdcall winapi_090860_CreateProcessA_wsprintfA(char* exe, char* dir) {
        char cmdline[256];
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (dir && *dir) {
            i32 len = strlen(dir);
            if (len > 0 && dir[len - 1] == '\\') {
                wsprintfA(cmdline, "%s%s", dir, exe);
            } else {
                wsprintfA(cmdline, "%s\\%s", dir, exe);
            }
        } else {
            wsprintfA(cmdline, "%s", exe);
        }
        if (dir && *dir == 0) {
            dir = 0;
        }
        return CreateProcessA(0, cmdline, 0, 0, 0, 0, 0, dir, &si, &pi);
    }

    // @confidence: low
    // @source: winapi:PostMessageA
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

    // @source: winapi:EndDialog
    // The settings dialog's 12 numeric edit fields, cached as globals between the
    // WM_INITDIALOG load and the IDOK store.
    DATA(0x0064526c)
    extern i32 g_dlgVal_64526c;
    DATA(0x006452d0)
    extern i32 g_dlgVal_6452d0;
    DATA(0x00645268)
    extern i32 g_dlgVal_645268;
    DATA(0x00645568)
    extern i32 g_dlgVal_645568;
    DATA(0x00645538)
    extern i32 g_dlgVal_645538;
    DATA(0x006451a4)
    extern i32 g_dlgVal_6451a4;
    DATA(0x006452d4)
    extern i32 g_dlgVal_6452d4;
    DATA(0x006452a8)
    extern i32 g_dlgVal_6452a8;
    DATA(0x00645558)
    extern i32 g_dlgVal_645558;
    DATA(0x00645560)
    extern i32 g_dlgVal_645560;
    DATA(0x0064555c)
    extern i32 g_dlgVal_64555c;
    DATA(0x00645564)
    extern i32 g_dlgVal_645564;
    // __stdcall DlgProc(hDlg, msg, wParam, lParam): a numeric settings dialog.
    RVA(0x00092ab0, 0x20d)
    i32 __stdcall winapi_092ab0_EndDialog(HWND hDlg, u32 msg, i32 wParam, i32 lParam) {
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

    // @confidence: low
    // @source: winapi:ValidateRect
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

    // @confidence: low
    // @source: winapi:PostMessageA
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
            PostMessageA(m_4->m_4->m_4, 0x111, 0x8036, 0);
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:EndDialog
    DATA(0x00645ca4)
    extern i32 g_dlg645ca4; // DAT_00645ca4 (the active dialog HWND)
    i32 __cdecl DlgFallback_215d(HWND hDlg, i32 wParam, i32 cur); // RVA 0x215d
    void __cdecl DlgInit_2ee6(HWND hDlg, i32 v);                  // RVA 0x2ee6
    // __stdcall DlgProc(hDlg, msg, wParam, lParam).
    RVA(0x0009dff0, 0x8c)
    i32 __stdcall winapi_09dff0_EndDialog(HWND hDlg, i32 msg, i32 wParam, i32 lParam) {
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

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // An item whose validity is probed by Check2694; its display name is at +0x14.
    struct NameItem_09e2d0 {
        char m_pad0[0x14];
        char m_14[1]; // +0x14 (name string)
    };
    i32 __cdecl Check2694(NameItem_09e2d0* item); // RVA 0x2694
    // "(Empty)" fallback display string (s_(Empty)_006113e0).
    // __cdecl(hWnd, item, id3, id4, id5, id6): label item into id3, enable 4 ctrls.
    RVA(0x0009e2d0, 0x84)
    void winapi_09e2d0_SetDlgItemTextA(
        HWND hWnd,
        NameItem_09e2d0* item,
        i32 id3,
        i32 id4,
        i32 id5,
        i32 id6
    ) {
        i32 flag;
        if (Check2694(item)) {
            SetDlgItemTextA(hWnd, id3, item->m_14);
            flag = 1;
        } else {
            SetDlgItemTextA(hWnd, id3, "(Empty)");
            flag = 0;
        }
        EnableWindow(GetDlgItem(hWnd, id3), flag);
        EnableWindow(GetDlgItem(hWnd, id4), flag);
        EnableWindow(GetDlgItem(hWnd, id5), flag);
        EnableWindow(GetDlgItem(hWnd, id6), flag);
    }

    // @confidence: low
    // @source: winapi:EndDialog;PostMessageA
    // @stub
    RVA(0x0009e390, 0x243)
    i32 winapi_09e390_EndDialog_PostMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime;wsprintfA
    // @stub
    RVA(0x000b6b40, 0x29e)
    i32 CMulti::winapi_0b6b40_timeGetTime_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    RVA(0x000b6e90, 0x34d)
    i32 CMulti::winapi_0b6e90_SetRect() {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x1780b0 (DPLAYX.#1)
    // The DirectPlay application GUID (DAT_0060fab8), passed by value to Connect.
    DATA(0x0060fab8)
    extern GUID g_dplayAppGuid;
    DATA(0x00648cf0)
    extern i32 g_isHost_648cf0; // DAT_00648cf0: nonzero when hosting
    struct DPlayConn_0b77a0 {
        i32 Connect(i32 sessionFlags, GUID guid); // thiscall, RVA 0x1780b0
    };
    struct DPlaySub_0b77a0 {
        void Init(i32 handle); // thiscall, RVA 0x158dc0
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
        m_c->m_4->Init(0);
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

    // @confidence: low
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
    // @source: winapi:LoadStringA
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

    // @confidence: low
    // @source: winapi:GetWindowTextLengthA
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

    // winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA (0x0bb700) re-homed as CNetMgr::winapi_0bb700_GetAsyncKeyState_Sleep_timeGetTime_wsprintfA in src/Stub/CNetMgr.cpp (proximity HIGH).

    // winapi_0bba10_Sleep (0x0bba10) re-homed as CNetMgr::winapi_0bba10_Sleep in src/Stub/CNetMgr.cpp (proximity HIGH).

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
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

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
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

    // @confidence: low
    // @source: winapi:EndDialog;KillTimer
    // DAT_0064557c: the active modeless dialog HWND, cached on entry/init.
    DATA(0x0064557c)
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
    i32 __stdcall winapi_0bdc00_EndDialog_KillTimer(HWND hWnd, u32 msg, u32 wParam, i32 lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_2c;
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

    // @confidence: low
    // Init helper at RVA 0xbddb0 (__cdecl(hWnd, ctx)).
    void Init_bddb0(HWND hWnd, void* ctx);
    // @source: winapi:GetDlgItem;SetTimer
    // __cdecl(hWnd, ctx): init, arm a 500 ms timer, cache a child control handle.
    RVA(0x000bdd60, 0x3e)
    void winapi_0bdd60_GetDlgItem_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            Init_bddb0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x1f4, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // @confidence: low
    // @source: winapi:GetDlgItem;SetTimer
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

    // @source: winapi:EndDialog;KillTimer;PostMessageA
    // __stdcall DlgProc(hWnd, msg, wParam, lParam): the in-game network dialog proc
    // (sibling of the lobby proc at 0xbdc00). WM_COMMAND ends the dialog on a set of
    // button IDs; WM_TIMER (0x113) polls the abort deadline and re-posts the cancel.
    RVA(0x000be0a0, 0x1c7)
    i32 __stdcall
    winapi_0be0a0_EndDialog_KillTimer_PostMessageA(HWND hWnd, u32 msg, u32 wParam, i32 lParam) {
        g_curDlg_64557c = hWnd;
        if (PreHandleLobbyMsg_38c3(hWnd, msg, wParam, lParam)) {
            return 1;
        }
        switch (msg) {
            case 0x110:
                g_curDlg_64557c = hWnd;
                g_peerSession = (PeerSession_0be490*)g_gameReg->m_2c;
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

    // @confidence: low
    // @source: winapi:GetDlgItem;SetDlgItemTextA;SetTimer
    // A scratch CString (m_data at +0; CStringData length is 8 bytes before m_data).
    struct GameCStr_be2f0 {
        char* m_data;      // +0x00
        GameCStr_be2f0();  // ctor RVA 0x1b9b93 (thiscall)
        ~GameCStr_be2f0(); // dtor RVA 0x1b9cde (thiscall)
    };
    // The "client status" CString global (?g_6473d8@@3VCString@@A).
    DATA(0x006473d8)
    extern GameCStr_be2f0 g_clientStatus;
    void FormatCStr_1b2cf5(GameCStr_be2f0* dst, const char* fmt, const char* a); // RVA 0x1b2cf5
    void InitDropPrompt_be3e0(HWND hWnd, void* ctx); // thunk 0x2185 -> 0xbe3e0 (cdecl)
    // __cdecl(hWnd, ctx): show the "not receiving data" banner, init, arm a timer.
    RVA(0x000be2f0, 0xb9)
    void winapi_0be2f0_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            GameCStr_be2f0 banner;
            if (*(i32*)(g_clientStatus.m_data - 8) != 0) {
                FormatCStr_1b2cf5(
                    &banner,
                    "Not Receiving Data From Client: %s",
                    g_clientStatus.m_data
                );
                SetDlgItemTextA(hWnd, 0x44b, banner.m_data);
            }
            InitDropPrompt_be3e0(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // @confidence: low
    // @source: winapi:GetWindowTextA;SetWindowTextA
    DATA(0x006496ac)
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

    // @confidence: low
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
    DATA(0x006487e0)
    extern char g_sessionFlag; // DAT_006487e0
    // @source: winapi:EndDialog;KillTimer
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

    // @confidence: low
    // @source: winapi:GetDlgItem;SetDlgItemTextA;SetTimer
    // CString-data ptr (DAT_00649618): the pending drop-in player's name; its
    // CString length lives 8 bytes before the data.
    DATA(0x00649618)
    extern char* g_playerName_649618;
    i32 __cdecl Format_11f890(char* buf, const char* fmt, ...); // RVA 0x11f890
    void Init_2ed7(HWND hWnd, void* ctx);                       // RVA 0x2ed7 (__cdecl)
    // __cdecl(hWnd, ctx): show a drop-in prompt, init, arm a timer, cache a child.
    RVA(0x000be760, 0x82)
    void winapi_0be760_GetDlgItem_SetDlgItemTextA_SetTimer(HWND hWnd, void* ctx) {
        if (hWnd && ctx) {
            char buf[0x80];
            if (*(i32*)(g_playerName_649618 - 8)) {
                Format_11f890(buf, "New Player Drop-In Request: %s", g_playerName_649618);
                SetDlgItemTextA(hWnd, 0x44b, buf);
            }
            Init_2ed7(hWnd, ctx);
            SetTimer(hWnd, 1, 0x2ee, 0);
            g_dlgItem_648ce0 = GetDlgItem(hWnd, 0x4b6);
        }
    }

    // @confidence: low
    // @source: winapi:GetWindow;SendMessageA
    // @stub
    RVA(0x000c1aa0, 0x2f8)
    i32 winapi_0c1aa0_GetWindow_SendMessageA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
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

    // @confidence: low
    // __thiscall OnInitDialog: chain to CDialog::OnInitDialog (RVA 0x1bac5e),
    // then arm a 50 ms timer on this dialog's HWND (m_1c). Returns TRUE.
    struct TimerDlg_0c2cb0 {
        char m_pad0[0x1c];
        HWND m_1c; // +0x1c
        i32 OnInitDialog();
        i32 BaseOnInitDialog(); // RVA 0x1bac5e (CDialog::OnInitDialog)
    };
    // @source: winapi:SetTimer
    RVA(0x000c2cb0, 0x1f)
    i32 TimerDlg_0c2cb0::OnInitDialog() {
        BaseOnInitDialog();
        SetTimer(m_1c, 1, 0x32, 0);
        return 1;
    }

    // @confidence: low
    // @source: winapi:GetWindowTextLengthA
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

    // @confidence: low
    // @source: winapi:CreateSolidBrush;FillRect;GetClientRect
    // @stub
    RVA(0x000c2e20, 0x21d)
    i32 winapi_0c2e20_CreateSolidBrush_FillRect_GetClientRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // A scratch CString with the MFC ctor/dtor/operator= (m_data at +0).
    struct CStr_c3e30 {
        char* m_data;               // +0x00
        CStr_c3e30();               // ctor RVA 0x1b9b93 (thiscall)
        ~CStr_c3e30();              // dtor RVA 0x1b9cde (thiscall)
        void Assign(const char* s); // operator= RVA 0x1b9e74 (thiscall)
    };
    // A dialog-item CWnd (HWND at +0x1c) refreshed from the model.
    struct CWndItem_c3e30 {
        char m_pad0[0x1c];
        HWND m_1c;                                    // +0x1c
        void Refresh1ce7db(i32 sel, CStr_c3e30* out); // thiscall RVA 0x1ce7db
    };
    // The replay/recording model singleton at DAT_0064bd5c.
    struct ReplayModel_64bd5c {
        char m_pad0[0x528];
        i32 m_528; // +0x528 enabled gate
        char m_pad52c[0x5b0 - 0x52c];
        i32 m_5b0;            // +0x5b0
        CStr_c3e30 m_5b4;     // +0x5b4
        CStr_c3e30 m_5b8;     // +0x5b8
        void Commit3ada(i32); // thiscall thunk RVA 0x3ada
    };
    DATA(0x0064bd5c)
    extern ReplayModel_64bd5c* g_replayModel;
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
        if (g_replayModel->m_528 != 0) {
            CWndItem_c3e30* item = GetItem27d(0x4ff);
            if (item != 0) {
                i32 r = SendMessageA(item->m_1c, 0x147, 0, 0);
                if (r != -1) {
                    CStr_c3e30 name;
                    item->Refresh1ce7db(r, &name);
                    if (*(i32*)(name.m_data - 8) != 0) {
                        m_6c = 0;
                    }
                    g_replayModel->m_5b0 = 0;
                    g_replayModel->m_5b8.Assign(g_emptyStr_6293f4);
                    g_replayModel->m_5b4.Assign(name.m_data);
                    g_replayModel->Commit3ada(0);
                }
            }
        }
    }

    // @confidence: low
    // @source: winapi:GetFocus;SendMessageA
    // @stub
    RVA(0x000c4230, 0x38e)
    i32 __stdcall winapi_0c4230_GetFocus_SendMessageA(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:KillTimer;timeGetTime;wsprintfA
    // @stub
    // proximity: CMultiStartDlg@-0x1e70 | CDroppedObject@+0x24a0
    RVA(0x000c46b0, 0x371)
    i32 ThisStubOwnerUnknown::winapi_0c46b0_KillTimer_timeGetTime_wsprintfA() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:SendMessageA
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

    // @confidence: low
    // @source: winapi:SendMessageA
    RVA(0x000c4f30, 0x33)
    void SelHost_0c4ee0::Update1() {
        HWND h = ResolveItem_1753(1)->m_hwnd;
        g_gameReg->m_5b0 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // @confidence: low
    // Item resolver at RVA 0xc27c0 (push id; stdcall; returns the CWnd-ish item).
    WndItem* __stdcall ResolveItem_c27c0(i32 id);
    // __thiscall host for winapi_0c4f80: resolves dialog item 2, clears its
    // selection, caches the new count in g_gameReg->m_7e8, then refreshes.
    struct SelHost_0c4f80 {
        void Update();
        void Refresh(); // RVA 0xc40b0
    };
    // @source: winapi:SendMessageA
    RVA(0x000c4f80, 0x33)
    void SelHost_0c4f80::Update() {
        HWND h = ResolveItem_c27c0(2)->m_hwnd;
        g_gameReg->m_7e8 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    RVA(0x000c4fd0, 0x33)
    void SelHost_0c4ee0::Update3() {
        HWND h = ResolveItem_1753(3)->m_hwnd;
        g_gameReg->m_a20 = SendMessageA(h, 0x147, 0, 0) + 1;
        Refresh();
    }

    // @confidence: low
    // @source: winapi:SendMessageA
    // Per-slot state array hung off the registry at +0x150 (stride 0x238).
    struct PlayerSlot_0c50f0 {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c
    };
    // The selection owner at DAT_0064bd5c; its m_528 gates the broadcast refresh.
    struct SelOwner_0c50f0 {
        char m_pad0[0x528];
        i32 m_528; // +0x528
    };
    DATA(0x0064bd5c)
    extern SelOwner_0c50f0* g_64bd5c;
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
        if (g_64bd5c->m_528) {
            Func1d70(0);
            Sync16db(1);
            Sync227a();
            Sync2c0c();
            Sync38d2();
        } else {
            Refresh185c(slot);
        }
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    // proximity: CDroppedObject@-0x1190 | CTileTriggerContainer@+0x780
    RVA(0x000c7ec0, 0x5f5)
    i32 ThisStubOwnerUnknown::winapi_0c7ec0_timeGetTime(i32, i32, i32) {
        return 0;
    }

    // ResetForMode (0x0c8a10) re-homed (analyzed) as CPlay::ResetForMode in
    // src/Gruntz/CPlay.cpp.

    // OnKeyCommand (0x0cbaf0) re-homed (byte-exact) as CPlay::OnKeyCommand in
    // src/Gruntz/CPlay.cpp.

    // @confidence: low
    // Sub-objects reached through the dispatcher's members; each has a __thiscall
    // reset/notify with no args (RVAs 0x137a80/0x138530/0x40b660/0x51af90).
    struct DispWnd_0cfbd0 {
        char m_pad0[4];
        DispWnd_0cfbd0* m_4; // +0x04
    };
    struct DispBoard_0cfbd0 {
        void Reset137a80(); // RVA 0x137a80
    };
    struct DispAudio_0cfbd0 {
        void Reset138530(); // RVA 0x138530
    };
    struct DispNet_0cfbd0 {
        void Reset40b660(); // RVA 0x40b660 (thunk 0x28ab)
    };
    struct DispUi_0cfbd0 {
        void Reset51af90(); // RVA 0x51af90 (thunk 0x244b)
    };
    struct DispOwner_0cfbd0 {
        char m_pad0[4];
        DispWnd_0cfbd0* m_4; // +0x04 -> m_4 -> m_4 = HWND
        char m_pad8[0x48 - 8];
        DispAudio_0cfbd0* m_48; // +0x48
        char m_pad4c[0x54 - 0x4c];
        DispNet_0cfbd0* m_54; // +0x54
        char m_pad58[0x60 - 0x58];
        DispUi_0cfbd0* m_60; // +0x60
        void Post(i32 code); // RVA 0x90220
    };
    struct DispInner_0cfbd0 {
        char m_pad0[0x2c];
        DispBoard_0cfbd0* m_2c; // +0x2c
    };
    struct DispCtx_0cfbd0 {
        char m_pad0[0x28];
        DispInner_0cfbd0* m_28; // +0x28
    };
    struct Dispatcher_0cfbd0 {
        char m_pad0[4];
        DispOwner_0cfbd0* m_4; // +0x04
        char m_pad8[0xc - 8];
        DispCtx_0cfbd0* m_c; // +0x0c
        char m_pad10[0x1c - 0x10];
        i32 m_1c; // +0x1c
        char m_pad20[0x40 - 0x20];
        i32 m_40; // +0x40
        char m_pad44[0x1bc - 0x44];
        i32 m_1bc; // +0x1bc
        i32 m_1c0; // +0x1c0
        i32 Dispatch();
    };
    // @source: winapi:PostMessageA
    RVA(0x000cfbd0, 0x8f)
    i32 Dispatcher_0cfbd0::Dispatch() {
        if (m_1c == 0x20) {
            m_1c0 = 1;
            m_40 = 1;
            DispInner_0cfbd0* inner = m_c->m_28;
            if (inner->m_2c) {
                inner->m_2c->Reset137a80();
            }
            m_4->m_48->Reset138530();
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

    // @confidence: low
    // @source: winapi:CopyRect
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

    // FindStartPointAt (0x0d5f90) re-homed (analyzed) as CPlay::FindStartPointAt and
    // ResetPlayState (0x0d60b0) as CPlay::ResetPlayState in src/Gruntz/CPlay.cpp.

    // @confidence: low
    // @source: winapi:PostMessageA
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

    // @confidence: low
    // @source: winapi:wsprintfA
    // @stub
    // proximity: CLoadingBar@-0xe0 | CArchiveLoadRec@+0x4b0
    RVA(0x000d7520, 0x3b9)
    i32 ThisStubOwnerUnknown::winapi_0d7520_wsprintfA(i32, i32, i32, i32) {
        return 0;
    }

    // ResetViewport (0x0d8c60) re-homed (byte-exact) as CPlay::ResetViewport in
    // src/Gruntz/CPlay.cpp.

    // 0x0d95f0 (winapi_0d95f0_wsprintfA - the level/grunt info-text panel painter)
    // graduated to src/Gruntz/GruntInfoText.cpp (needs the /GX EH frame).

    // @confidence: low
    // @source: winapi:timeGetTime
    // __thiscall coin-flip: deterministic ((m_1c+1)%2) in replay mode, otherwise a
    // once-per-frame random bit lazily seeded from timeGetTime.
    struct CoinHost_0da200 {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c
        i32 Flip();
    };
    RVA(0x000da200, 0x9b)
    i32 CoinHost_0da200::Flip() {
        CGameReg* gr = g_gameReg;
        if (gr->m_134 == 1 && gr->m_130 == 0) {
            return (m_1c + 1) % 2;
        }
        if (!(g_coinRolled & 1)) {
            i32 seed;
            g_coinRolled |= 1;
            if (!(g_rngSeeded & 1)) {
                g_rngSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_rngState;
            }
            g_rngState = seed * 214013 + 2531011;
            g_coinValue = ((g_rngState >> 0x10) & 0x7fff) % 2;
        }
        return g_coinValue;
    }

    // @confidence: low
    // @source: winapi:PostMessageA
    DATA(0x0064c69c)
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
        PostMessageA(m_4->m_4->m_4, 0x111, 0x8027, 0);
    }

    // @confidence: low
    // @source: winapi:EndDialog
    DATA(0x0064c86c)
    extern i32 g_dlg64c86c; // DAT_0064c86c (the active dialog HWND)
    DATA(0x00613a9c)
    extern i32 g_dlgSel613a9c;                                    // DAT_00613a9c
    i32 __cdecl DlgFallback_1302(HWND hDlg, i32 wParam, i32 cur); // RVA 0x1302
    void __cdecl DlgInit_2e05(HWND hDlg, i32 v);                  // RVA 0x2e05
    // __stdcall DlgProc(hDlg, msg, wParam, lParam).
    RVA(0x000e35f0, 0x77)
    i32 __stdcall winapi_0e35f0_EndDialog(HWND hDlg, i32 msg, i32 wParam, i32 lParam) {
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
    DATA(0x0064c864)
    extern char* g_dlgInfoText;

    // @confidence: low
    // @source: winapi:EndDialog
    // The gameReg->m_58 dialog helper sub-object; its M1834/M2d97 thunks live in
    // this TU. (m_58 is reused elsewhere as an int/void* gate, so cast locally.)
    struct DlgSub58_0e3a40 {
        void M1834(char* text);         // thiscall, thunk 0x1834
        void M2d97(i32 a, i32 caption); // thiscall, thunk 0x2d97
    };
    // The SetDlgItemTextA helper (RVA 0xe4850) is reached here via thunk 0x103c.
    // __stdcall DialogProc: OK closes; Cancel runs the helper sub-object; init fills.
    RVA(0x000e3a40, 0xb0)
    i32 __stdcall winapi_0e3a40_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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

    // @confidence: low
    // @source: winapi:EndDialog
    // __stdcall DialogProc: OK/Cancel close the dialog; WM_INITDIALOG fills a line.
    RVA(0x000e3b20, 0x86)
    i32 __stdcall winapi_0e3b20_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
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

    // @confidence: low
    // @source: winapi:EndDialog
    // __stdcall DlgProc(hDlg, msg, wParam, lParam): OK/Cancel end the dialog.
    RVA(0x000e3be0, 0x52)
    i32 __stdcall winapi_0e3be0_EndDialog(HWND hDlg, i32 msg, i32 wParam, i32 lParam) {
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

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // __cdecl(hWnd, item, id3, id4, id5, id6): label item into id3, then enable the
    // first two controls unconditionally and the last two only if the item is valid.
    RVA(0x000e3e80, 0x86)
    void winapi_0e3e80_SetDlgItemTextA(
        HWND hWnd,
        NameItem_09e2d0* item,
        i32 id3,
        i32 id4,
        i32 id5,
        i32 id6
    ) {
        i32 flag;
        if (Check2694(item)) {
            SetDlgItemTextA(hWnd, id3, item->m_14);
            flag = 1;
        } else {
            SetDlgItemTextA(hWnd, id3, "(Empty)");
            flag = 0;
        }
        EnableWindow(GetDlgItem(hWnd, id3), 1);
        EnableWindow(GetDlgItem(hWnd, id4), 1);
        EnableWindow(GetDlgItem(hWnd, id5), flag);
        EnableWindow(GetDlgItem(hWnd, id6), flag);
    }

    // @confidence: low
    // @source: winapi:SetDlgItemTextA
    // __cdecl: SetDlgItemTextA(hWnd, 0x40d, &item->text) when all ptrs non-null.
    RVA(0x000e4850, 0x29)
    void winapi_0e4850_SetDlgItemTextA(HWND hWnd, void* gate, char* item) {
        if (hWnd && gate && item) {
            SetDlgItemTextA(hWnd, 0x40d, item + 0x14);
        }
    }

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    // proximity: CFileIO@-0x920 | CSBI_WellGoo@+0x360
    RVA(0x000e6020, 0x288)
    i32
    ThisStubOwnerUnknown::winapi_0e6020_SetRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    // proximity: CGrunt@-0x620 | CUserLogic@+0x1b70
    RVA(0x000ecc90, 0x86a)
    i32 ThisStubOwnerUnknown::winapi_0ecc90_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    // proximity: CGrunt@-0x1380 | CUserLogic@+0xe10
    RVA(0x000ed9f0, 0x8dd)
    i32 ThisStubOwnerUnknown::winapi_0ed9f0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect;PtInRect
    // @stub
    // proximity: CUserLogic@-0x2620 | CGrunt@+0x18d0
    RVA(0x000f0e20, 0x928)
    i32 ThisStubOwnerUnknown::winapi_0f0e20_IntersectRect_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x000f36a0, 0x78e)
    i32 winapi_0f36a0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:PtInRect
    // @stub
    // proximity: CGrunt@-0x17d0 | CAttract@+0x5f00
    RVA(0x000f42f0, 0x1193)
    i32 ThisStubOwnerUnknown::winapi_0f42f0_PtInRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    // proximity: isolated (no near matched neighbour)
    RVA(0x000f60f0, 0xb30)
    i32 ThisStubOwnerUnknown::winapi_0f60f0_IntersectRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FreeLibrary
    struct MidiDrv_0f8e20 {
        char m_pad0[0x14];
        void(__cdecl* m_14)(i16); // +0x14 function pointer member
    };
    DATA(0x0064e0b8)
    extern i32 g_midiOpen_64e0b8;
    DATA(0x0064e0b0)
    extern MidiDrv_0f8e20* g_midiDrv_64e0b0;
    DATA(0x0064e0a4)
    extern i16 g_midiPort_64e0a4;
    DATA(0x0064dd28)
    extern i16 g_midiDev_64dd28;
    DATA(0x0064e0a8)
    extern HMODULE g_midiLib_64e0a8;
    void __cdecl MidiShutdown_3382(); // RVA 0x3382
    // __cdecl(): tear the MIDI driver down and free its DLL.
    RVA(0x000f8e20, 0x56)
    void winapi_0f8e20_FreeLibrary() {
        if (g_midiOpen_64e0b8 && g_midiDrv_64e0b0 && g_midiPort_64e0a4) {
            MidiShutdown_3382();
            g_midiDrv_64e0b0->m_14(g_midiDev_64dd28);
            FreeLibrary(g_midiLib_64e0a8);
            g_midiLib_64e0a8 = 0;
            g_midiOpen_64e0b8 = 0;
        }
    }

    // @confidence: low
    // @source: winapi:BeginPaint;EndPaint
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

    // @confidence: low
    // Free init helper at RVA 0x500930 (__stdcall(int)).
    void __stdcall Prep_500930(i32 flag);
    // A sub-object reached via g_gameReg->m_2c whose Refresh() is at RVA 0x4d8c60.
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
    // @source: winapi:SetRect
    // __thiscall: lay out the 0xa0x0x1e0 screen, validate it, else report error.
    RVA(0x000fe460, 0x83)
    i32 Screen_0fe460::Open() {
        if (m_548 == 0 && m_0 != 1) {
            Prep_500930(1);
            SetRect(&m_10, 0, 0, 0xa0, 0x1e0);
            Resize(1);
            ((SubMgr_0fe460*)g_gameReg->m_2c)->Refresh();
            if (!Validate()) {
                g_gameReg->ReportError(0x80e4, 0x448);
                return 0;
            }
            Activate(m_10c, 3);
        }
        return 1;
    }

    // 0xfe520 (CSBI_RectOnly::winapi_0fe520_SetRect) reconstructed in
    // src/Gruntz/SBI_RectOnly.cpp.

    // @confidence: low
    // @source: winapi:SetRect
    void __stdcall Prep_12fd(i32 mode); // RVA 0x12fd
    struct CGameWnd_fe600 {
        i32 Sub3d55(); // thiscall, RVA 0x3d55 (on g_gameReg->m_2c)
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
            ((CGameWnd_fe600*)g_gameReg->m_2c)->Sub3d55();
        }
        return 1;
    }

    // @confidence: low
    // @source: winapi:SetRect
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

    // @confidence: low
    // @source: winapi:CopyRect;OffsetRect
    // @stub
    RVA(0x00115930, 0x15b)
    i32 winapi_115930_CopyRect_OffsetRect() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // __thiscall(src): copy src into this RECT; return this.
    struct RectHost_115b30 {
        RECT m_rc;
        RectHost_115b30* Copy(const RECT* src);
    };
    RVA(0x00115b30, 0x15)
    RectHost_115b30* RectHost_115b30::Copy(const RECT* src) {
        CopyRect(&m_rc, src);
        return this;
    }

    // @confidence: low
    // @source: winapi:SetActiveWindow;SetFocus
    // __cdecl: activate + focus the same window.
    RVA(0x00118930, 0x15)
    void winapi_118930_SetActiveWindow_SetFocus(HWND hWnd) {
        SetActiveWindow(hWnd);
        SetFocus(hWnd);
    }

    // @confidence: low
    // @source: winapi:OutputDebugStringA
    // __cdecl(status): trace a _heapchk() status code.
    RVA(0x00118b50, 0x5b)
    void winapi_118b50_OutputDebugStringA(i32 status) {
        switch (status) {
            case -3:
                OutputDebugStringA("Heap return value: _HEAPBADBEGIN\n");
                return;
            case -4:
                OutputDebugStringA("Heap return value: _HEAPBADNODE\n");
                return;
            case -6:
                OutputDebugStringA("Heap return value: _HEAPBADPTR\n");
                return;
            case -1:
                OutputDebugStringA("Heap return value: _HEAPEMPTY\n");
                return;
            case -2:
                OutputDebugStringA("Heap return value: _HEAPOK\n");
                return;
            default:
                OutputDebugStringA("Heap return value: Unknown return value!\n");
                return;
        }
    }

    // @confidence: low
    // @source: winapi:IsIconic
    // __cdecl(hWnd, msg, wParam): block screen-saver / monitor-power while not iconic.
    RVA(0x001192d0, 0x39)
    i32 winapi_1192d0_IsIconic(HWND hWnd, i32 msg, i32 wParam) {
        if (msg == 0x112) {
            i32 sc = wParam & 0xfff0;
            if (sc == 0xf140 || sc == 0xf170) {
                if (!IsIconic(hWnd)) {
                    return 1;
                }
            }
        }
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x0011b3b0, 0x338)
    i32 CGruntSpawnConfig::winapi_11b3b0_timeGetTime(i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    RVA(0x0011b7c0, 0x304)
    i32 __stdcall winapi_11b7c0_timeGetTime(i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetLastError;HeapValidate;HeapWalk
    // @stub
    RVA(0x001206b0, 0x1ad)
    i32 winapi_1206b0_GetLastError_HeapValidate_HeapWalk() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetCurrentThreadId;TlsSetValue
    // @stub
    RVA(0x00123d10, 0x8c)
    i32 winapi_123d10_GetCurrentThreadId_TlsSetValue() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    struct AppModule_136a30 {
        char m_pad0[8];
        HINSTANCE m_8; // +0x08 = the resource module handle
    };
    AppModule_136a30* AppModule_1d3631(); // RVA 0x1d3631 (global accessor)
    struct WaveHost_136a30 {
        char m_pad0[0x78];
        i32 m_78; // +0x78
        i32 LoadWave(const char* name, i32 a, i32 b);
        i32 Use136910(void* data, i32 a, i32 b); // thiscall, RVA 0x136910
    };
    // __thiscall(name, a, b): find/load/lock a WAVE resource, hand it to Use136910.
    RVA(0x00136a30, 0x76)
    i32 WaveHost_136a30::LoadWave(const char* name, i32 a, i32 b) {
        if (!m_78) {
            return 0;
        }
        HINSTANCE mod1 = AppModule_1d3631()->m_8;
        HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
        if (!hRsrc) {
            return 0;
        }
        HINSTANCE mod2 = AppModule_1d3631()->m_8;
        HGLOBAL hRes = LoadResource(mod2, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use136910(data, a, b);
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // The probe object (arg1) whose Ready (RVA 0x135440) gates loading.
    struct WaveProbe_136ce0 {
        i32 Ready(); // thiscall, RVA 0x135440
    };
    struct WaveHost2_136ce0 {
        char m_pad0[0x78];
        i32 m_78; // +0x78
        i32 LoadWave(WaveProbe_136ce0* probe, const char* name, i32 arg);
        i32 Use136bd0(WaveProbe_136ce0* probe, void* data, i32 arg); // thiscall, RVA 0x136bd0
    };
    // __thiscall(probe, name, arg): if the probe is ready, find/load/lock the WAVE.
    RVA(0x00136ce0, 0x92)
    i32 WaveHost2_136ce0::LoadWave(WaveProbe_136ce0* probe, const char* name, i32 arg) {
        if (!m_78) {
            return 0;
        }
        if (probe->Ready() == 0) {
            return 1;
        }
        HINSTANCE mod1 = AppModule_1d3631()->m_8;
        HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
        if (!hRsrc) {
            return 0;
        }
        HINSTANCE mod2 = AppModule_1d3631()->m_8;
        HGLOBAL hRes = LoadResource(mod2, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use136bd0(probe, data, arg);
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x136550 (DSOUND.#1_DirectSoundCreate)
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

    // @confidence: low
    // @source: winapi:timeGetTime
    // __thiscall(timestamp): throttle to >0x64 ms since the last tick, query the
    // source (m_8), wrap localC against m_c into the window, then run the work pass.
    struct TickSource_137e30 {
        i32 Read(i32* outHi, i32* outLo); // thiscall, RVA 0x135a20
    };
    struct Throttle_137e30 {
        char m_pad0[8];
        TickSource_137e30* m_8; // +0x08
        i32 m_c;                // +0x0c
        i32 m_10;               // +0x10
        i32 m_14;               // +0x14
        char m_pad18[0x1c - 0x18];
        i32 m_1c; // +0x1c
        char m_pad20[0x28 - 0x20];
        i32 m_28; // +0x28
        i32 Tick(i32 timestamp);
        i32 Work(i32 a, i32 b); // RVA 0x137f30
    };
    RVA(0x00137e30, 0x98)
    i32 Throttle_137e30::Tick(i32 timestamp) {
        if (!m_1c) {
            return 1;
        }
        i32 t = (timestamp == -1) ? (i32)timeGetTime() : timestamp;
        if ((u32)t <= (u32)(m_28 + 0x64)) {
            return 1;
        }
        m_28 = t;
        i32 hi, lo;
        if (!m_8->Read(&hi, &lo)) {
            return 0;
        }
        i32 v;
        if ((u32)hi >= (u32)m_c) {
            if (hi == m_c) {
                v = m_10;
            } else {
                v = hi - m_c;
            }
        } else {
            v = m_10 + hi - m_c;
        }
        if ((u32)v < (u32)m_14) {
            return 1;
        }
        return Work(m_c, v) != 0;
    }

    // @confidence: low
    // A device object whose Prepare(flag) lives at RVA 0x135a70.
    struct Device_1380d0 {
        i32 Prepare(i32 flag); // RVA 0x135a70
    };
    // __thiscall(timestamp) host: timestamp -1 means "now"; prep the device, then
    // run the work pass (RVA 0x137f30) over m_c/m_10. Returns whether it ran.
    struct Timer_1380d0 {
        char m_pad0[0x8];
        Device_1380d0* m_8; // +0x08
        i32 m_c;            // +0x0c
        i32 m_10;           // +0x10
        char m_pad14[0x20 - 0x14];
        i32 m_20; // +0x20
        char m_pad24[0x28 - 0x24];
        i32 m_28; // +0x28
        i32 Tick(i32 timestamp);
        i32 Work(i32 a, i32 b); // RVA 0x137f30
    };
    // @source: winapi:timeGetTime
    RVA(0x001380d0, 0x4e)
    i32 Timer_1380d0::Tick(i32 timestamp) {
        i32 t = (timestamp == -1) ? (i32)timeGetTime() : timestamp;
        m_28 = t;
        m_c = 0;
        if (!m_8->Prepare(0)) {
            return 0;
        }
        m_20 = 0;
        return Work(m_c, m_10) != 0;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_midiOutOpen@12;_AIL_startup@0
    // __thiscall(driver, seq, skipInit): record the AIL handles, optionally start
    // the AIL MIDI driver. m_28 = whether the driver is usable.
    struct AilHost_138490 {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c
        i32 m_20; // +0x20
        i32 m_24; // +0x24
        i32 m_28; // +0x28
        i32 Init(i32 driver, i32 seq, i32 skipInit);
    };
    RVA(0x00138490, 0x5e)
    i32 AilHost_138490::Init(i32 driver, i32 seq, i32 skipInit) {
        m_24 = driver;
        m_20 = seq;
        m_1c = 0;
        m_28 = 1;
        g_ailDriver64 = driver;
        if (!skipInit) {
            AIL_startup();
            if (AIL_midiOutOpen(&g_ailMidiDriver, 0, -1) != 0 || g_ailMidiDriver == 0) {
                m_28 = 0;
            }
        }
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_shutdown@0
    // A polymorphic sub whose slot 12 (vtable +0x30) tears down its sequence.
    struct AilSlot_1384f0 {
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual void v8();
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void Release(); // slot 12 == vtable +0x30
    };
    struct AilMgr_1384f0 {
        char m_pad0[0x1c];
        AilSlot_1384f0* m_1c; // +0x1c
        i32 m_20;             // +0x20
        void Shutdown();
        void Reset138530(); // thiscall, RVA 0x138530
    };
    // __thiscall(): reset, release the active sequence, reset again, shut AIL down.
    RVA(0x001384f0, 0x3b)
    void AilMgr_1384f0::Shutdown() {
        Reset138530();
        if (m_1c) {
            m_1c->Release();
        }
        Reset138530();
        m_20 = 0;
        m_1c = 0;
        g_ailMidiDriver = 0;
        AIL_shutdown();
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_XMIDI_master_volume@8
    // __stdcall(volume 0..100): scale to 0..127 and push to the XMIDI driver.
    RVA(0x00138950, 0x70)
    i32 __stdcall thirdparty_138950_AIL_set_XMIDI_master_volume_8(i32 volume) {
        i32 scaled;
        if (!g_ailMidiDriver) {
            return 0;
        }
        if (volume <= 0) {
            scaled = 0;
        } else if (volume >= 100) {
            scaled = 0x7f;
        } else {
            scaled = volume * 127 / 100;
        }
        AIL_set_XMIDI_master_volume(g_ailMidiDriver, scaled);
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_XMIDI_master_volume@4
    // __cdecl(): read the XMIDI master volume and rescale 0..127 -> 0..100.
    RVA(0x001389c0, 0x47)
    i32 thirdparty_1389c0_AIL_XMIDI_master_volume_4() {
        if (!g_ailMidiDriver) {
            return 0x64;
        }
        i32 v = AIL_XMIDI_master_volume(g_ailMidiDriver);
        if (v <= 0) {
            return 0;
        }
        if (v >= 0x7f) {
            return 0x64;
        }
        return v * 100 / 127;
    }

    // @source: thirdparty:_AIL_allocate_sequence_handle@4;_AIL_init_sequence@12;_AIL_release_sequence_handle@4
    // An owned XMIDI sequence: copies the song bytes into a Rez buffer, names it,
    // and allocates + initialises an AIL sequence handle against the MIDI driver.
    struct AilSeq_138c20 {
        char m_pad0[4];
        char m_name[0x48 - 4]; // +0x04 sequence name buffer
        i32 m_48;              // +0x48
        char m_pad4c[0x50 - 0x4c];
        i32 m_50;   // +0x50
        i32 m_54;   // +0x54
        i32 m_58;   // +0x58 AIL sequence handle
        void* m_5c; // +0x5c owned song bytes
        i32 Init(void* data, u32 size, char* name);
    };
    // __thiscall(data, size, name): seed a sequence record from `size` bytes of
    // XMIDI at `data`; auto-name "MIDI%i" when `name` is null.
    RVA(0x00138c20, 0x122)
    i32 AilSeq_138c20::Init(void* data, u32 size, char* name) {
        if (!data) {
            return 0;
        }
        if (size < 4) {
            return 0;
        }
        if (!g_ailMidiDriver) {
            return 0;
        }
        ++g_midiSeqCounter;
        m_48 = 0;
        m_54 = 100;
        m_50 = 100;
        if (name) {
            strcpy(m_name, name);
        } else {
            Format_11f890(m_name, "MIDI%i", g_midiSeqCounter);
        }
        if (!m_5c) {
            m_5c = RezAlloc(size);
            if (!m_5c) {
                return 0;
            }
            memcpy(m_5c, data, size);
        }
        m_58 = AIL_allocate_sequence_handle(g_ailMidiDriver);
        if (!m_58) {
            return 0;
        }
        if (AIL_init_sequence(m_58, m_5c, 0) == 0) {
            AIL_release_sequence_handle(m_58);
            m_58 = 0;
            return 0;
        }
        return 1;
    }

    // AIL sequence player. The virtual at slot 8 (vtable +0x20) gates playback;
    // the sequence handle lives at m_58, loop/cursor state at m_44/m_48/m_4c.
    struct AilSeq {
        // vptr at +0x00 (compiler-managed). Eight leading virtuals put CanPlay at
        // vtable offset 0x20; slot 12 (+0x30) is the teardown hook.
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual i32 CanPlay(); // slot 8 == vtable +0x20
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void Teardown(); // slot 12 == vtable +0x30
        char m_pad4[0x44 - 4];
        i32 m_44; // +0x44
        i32 m_48; // +0x48
        i32 m_4c; // +0x4c
        i32 m_50; // +0x50
        i32 m_54; // +0x54
        i32 m_58; // +0x58
        i32 m_5c; // +0x5c
        i32 Play(i32 cursor, i32 loop);
        i32 Resume(i32 restart);
        i32 SetLoop(i32 loop);
        i32 ResumeGate(); // the m_138f60 helper
        i32 Stop();
        i32 SetTempo(i32 tempo, i32 unused);
        void ReleaseHandle();
        i32 Pause();
        i32 SetVolume(i32 volume, i32 ms);
    };
    extern "C" void RezFree_call(void* p); // RVA 0x1b9b82 (cdecl)

    // @confidence: low
    // @source: thirdparty:_AIL_release_sequence_handle@4
    // __thiscall(): tear down, release the AIL sequence handle + Rez buffer.
    RVA(0x00138dd0, 0x36)
    void AilSeq::ReleaseHandle() {
        Teardown();
        if (m_58) {
            AIL_release_sequence_handle(m_58);
            m_58 = 0;
        }
        if (m_5c) {
            RezFree_call((void*)m_5c);
            m_5c = 0;
        }
    }

    // @source: thirdparty:_AIL_set_sequence_loop_count@8;_AIL_start_sequence@4
    // __thiscall(cursor, loop): start the sequence; if looping, clear loop count.
    RVA(0x00138e10, 0x4a)
    i32 AilSeq::Play(i32 cursor, i32 loop) {
        if (!CanPlay()) {
            return 0;
        }
        m_4c = cursor;
        m_48 = loop;
        AIL_start_sequence(m_58);
        if (loop) {
            AIL_set_sequence_loop_count(m_58, 0);
        }
        m_44 = 0;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_end_sequence@4
    // __thiscall(): if playable, end the sequence and clear the paused flag.
    RVA(0x00138e60, 0x26)
    i32 AilSeq::Stop() {
        if (!CanPlay()) {
            return 0;
        }
        AIL_end_sequence(m_58);
        m_44 = 0;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_stop_sequence@4
    // __thiscall(): pause the sequence the first time, counting nested pauses.
    RVA(0x00138e90, 0x3a)
    i32 AilSeq::Pause() {
        if (!CanPlay()) {
            return 0;
        }
        if (!ResumeGate()) {
            return 0;
        }
        if (m_44 == 0) {
            AIL_stop_sequence(m_58);
        }
        m_44++;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_resume_sequence@4
    // __thiscall(restart): count down the resume delay and re-issue the resume.
    RVA(0x00138ed0, 0x4f)
    i32 AilSeq::Resume(i32 restart) {
        if (!CanPlay()) {
            return 0;
        }
        if (ResumeGate()) {
            return 1;
        }
        if (m_44 > 0) {
            m_44--;
            if (restart) {
                m_44 = 0;
            }
            if (m_44 <= 0) {
                AIL_resume_sequence(m_58);
            }
        }
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_tempo@12
    // __thiscall(tempo, ms): if playable, set the sequence tempo and cache it.
    RVA(0x00138f90, 0x32)
    i32 AilSeq::SetTempo(i32 tempo, i32 ms) {
        if (!CanPlay()) {
            return 0;
        }
        AIL_set_sequence_tempo(m_58, tempo, ms);
        m_54 = tempo;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_volume@12
    // __thiscall(volume 0..100, ms): scale to 0..127 and set the sequence volume.
    RVA(0x00138fd0, 0x5e)
    i32 AilSeq::SetVolume(i32 volume, i32 ms) {
        if (!CanPlay()) {
            return 0;
        }
        i32 scaled;
        if (volume <= 0) {
            scaled = 0;
        } else if (volume >= 100) {
            scaled = 0x7f;
        } else {
            scaled = volume * 127 / 100;
        }
        AIL_set_sequence_volume(m_58, scaled, ms);
        m_50 = volume;
        return 1;
    }

    // @confidence: low
    // @source: thirdparty:_AIL_set_sequence_loop_count@8
    // __thiscall(loop): update the cached loop flag, re-arming the driver count.
    RVA(0x00139030, 0x4c)
    i32 AilSeq::SetLoop(i32 loop) {
        if (!CanPlay()) {
            return 0;
        }
        if (m_48 != loop) {
            m_48 = loop;
            if (loop) {
                AIL_set_sequence_loop_count(m_58, 0);
            } else {
                AIL_set_sequence_loop_count(m_58, 1);
            }
        }
        return 1;
    }

    // (0x13d4c0 was WndHolder_13d4c0::Destroy; recovered as CGameWnd::OnClose -
    // the WM_CLOSE handler, vtable slot 4 - and migrated to src/Wap32/GameWnd.cpp.
    // The "WndHolder_13d4c0" placeholder class is CGameWnd: m_4 (HWND) / m_c
    // (destroyed flag) are the CGameWnd ctor-zeroed fields.)

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    // proximity: CDDSurface@-0x4d0 | CFileImage@+0x4b0
    RVA(0x0013f460, 0x2da)
    i32 ThisStubOwnerUnknown::winapi_13f460_CopyRect(i32, i32) {
        return 0;
    }

    // The app HINSTANCE used as the resource module (DAT_00683ee0).
    DATA(0x00683ee0)
    extern HINSTANCE g_resModule;

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // The header of the locked RT_BITMAP resource (its +0xe must be 8).
    struct ResHdr_144270 {
        i32 m_0; // +0x00 (payload size; data follows at +m_0+0x400)
        i32 m_4; // +0x04
        i32 m_8; // +0x08
        char m_padc[0xe - 0xc];
        i16 m_e; // +0x0e (must be 8)
    };
    struct ResLoad_144270 {
        char m_pad0[0x10];
        i32 m_10; // +0x10 (set to 0x6c after a 0x6c-byte zero-fill)
        i32 m_14; // +0x14
        i32 m_18; // +0x18
        i32 m_1c; // +0x1c
        char m_pad20[0x78 - 0x20];
        i32 m_78;                        // +0x78
        i32 Init(i32 saved);             // thiscall, RVA 0x13e0a0
        void Parse(char* data, i32 two); // thiscall, RVA 0x13ece0
        i32 Load(i32 a, char* name, i32 c);
    };
    // __thiscall(a, name, c): find/load/lock the named RT_BITMAP, validate its
    // header (+0xe==8), zero a 0x6c-byte block, seed the loader fields, init it,
    // then parse the payload that follows the 0x400-byte header.
    RVA(0x00144270, 0xd2)
    i32 ResLoad_144270::Load(i32 a, char* name, i32 c) {
        HRSRC hr = FindResourceA(g_resModule, name, (LPCSTR)2);
        if (!hr) {
            return 0;
        }
        HGLOBAL hg = LoadResource(g_resModule, hr);
        if (!hg) {
            return 0;
        }
        ResHdr_144270* p = (ResHdr_144270*)LockResource(hg);
        if (!p) {
            return 0;
        }
        i32 saved = p->m_8;
        if (p->m_e != 8) {
            return 0;
        }
        memset(&m_10, 0, 0x6c);
        m_10 = 0x6c;
        m_78 = c | 0x40;
        m_14 = 7;
        m_1c = p->m_4;
        m_18 = c;
        if (!Init(saved)) {
            return 0;
        }
        Parse((char*)p + p->m_0 + 0x400, 2);
        return 1;
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    struct PalLoad_1479e0 {
        i32 Apply(i32 a, PALETTEENTRY* pal, i32 c); // thiscall, RVA 0x147390
        i32 Load(i32 a, char* name, i32 c);
    };
    // __thiscall(a, name, c): load the named PALETTE resource as 256 RGB triples,
    // expand to PALETTEENTRY[256] (flags 0), and apply it.
    RVA(0x001479e0, 0xbb)
    i32 PalLoad_1479e0::Load(i32 a, char* name, i32 c) {
        PALETTEENTRY pal[256];
        HRSRC hr = FindResourceA(g_resModule, name, "PALETTE");
        if (!hr) {
            return 0;
        }
        HGLOBAL hg = LoadResource(g_resModule, hr);
        if (!hg) {
            return 0;
        }
        char* src = (char*)LockResource(hg);
        if (!src) {
            return 0;
        }
        for (i32 i = 0; i < 256; i++) {
            pal[i].peRed = src[0];
            pal[i].peGreen = src[1];
            pal[i].peBlue = src[2];
            pal[i].peFlags = 0;
            src += 3;
        }
        return Apply(a, pal, c);
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // The surface this palette describes: vtable slot 4 (+0x10) fills a 256-entry
    // system-palette snapshot. The DIRPAL.CPP source-path string for the logger.
    struct PalSurface_147f30;
    struct PalSurfVtbl_147f30 {
        void* s0[4];
        i32(__stdcall* Snapshot)(PalSurface_147f30*, i32, i32, i32 count, void* dst); // +0x10
    };
    struct PalSurface_147f30 {
        PalSurfVtbl_147f30* m_vptr;
    };
    // The DDraw error logger (RVA 0x141400 = CDirectDrawMgr::GetErrorString) and the
    // Rez allocator backing the working-copy buffer.
    struct CDirectDrawMgr {
        static void GetErrorString(char* file, i32 line, i32 hr);
    };
    struct PalCtx_147f30;
    struct PalCtx_147f30 {
        char m_pad0[4];
        PalSurface_147f30* m_4; // +0x04
        char m_pad8[0xc - 8];
        char* m_c; // +0x0c (the 0x400 source palette buffer)
        char m_pad10[0x14 - 0x10];
        i32 m_14;   // +0x14
        char* m_18; // +0x18 (lazily-allocated 0x400 working copy)
        char m_1c;  // +0x1c
        char m_1d;  // +0x1d
        char m_1e;  // +0x1e
        char m_pad1f[0x20 - 0x1f];
        i32 m_20;        // +0x20
        i32 m_24;        // +0x24 (timeGetTime stamp)
        i32 m_28;        // +0x28
        i32 m_2c;        // +0x2c
        i32 m_30;        // +0x30
        i32 m_34;        // +0x34 (1 once set up)
        void Teardown(); // thiscall, RVA 0x148250
        void Finalize(); // thiscall, RVA 0x1480a0
        void Setup6(i32 a, i32 b, char c3, char c4, char c5, i32 a6);
        void Setup4(i32 a, i32 b, i32 a3, i32 a4);
    };
    // __thiscall(a,b,c3,c4,c5,a6): rebuild the palette snapshot, cache the params
    // (3 bytes at +0x1c), timestamp, lazily clone the source palette, then finalize.
    RVA(0x00147f30, 0xbe)
    void PalCtx_147f30::Setup6(i32 a, i32 b, char c3, char c4, char c5, i32 a6) {
        if (m_34) {
            Teardown();
        }
        i32 err = m_4->m_vptr->Snapshot(m_4, 0, 0, 0x100, m_c);
        if (err) {
            CDirectDrawMgr::GetErrorString("C:\\Proj\\DDrawMgr\\DIRPAL.CPP", 0x311, err);
        }
        m_2c = a;
        m_30 = b;
        m_20 = a6;
        m_24 = timeGetTime();
        m_28 = -1;
        m_14 = 0;
        m_1c = c3;
        m_1d = c4;
        m_1e = c5;
        if (!m_18) {
            m_18 = (char*)RezAlloc(0x400);
        }
        for (i32 i = 0; i < 0x400; i += 4) {
            *(i32*)(m_18 + i) = *(i32*)(m_c + i);
        }
        m_34 = 1;
        Finalize();
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // __thiscall(a,b,a3,a4): same as Setup6 but stores a3 at +0x14 and uses the
    // 0x34b log line; returns void.
    RVA(0x00147ff0, 0xa9)
    void PalCtx_147f30::Setup4(i32 a, i32 b, i32 a3, i32 a4) {
        if (m_34) {
            Teardown();
        }
        i32 err = m_4->m_vptr->Snapshot(m_4, 0, 0, 0x100, m_c);
        if (err) {
            CDirectDrawMgr::GetErrorString("C:\\Proj\\DDrawMgr\\DIRPAL.CPP", 0x34b, err);
        }
        m_2c = a;
        m_30 = b;
        m_20 = a4;
        m_24 = timeGetTime();
        m_14 = a3;
        m_28 = -1;
        if (!m_18) {
            m_18 = (char*)RezAlloc(0x400);
        }
        for (i32 i = 0; i < 0x400; i += 4) {
            *(i32*)(m_18 + i) = *(i32*)(m_c + i);
        }
        m_34 = 1;
        Finalize();
    }

    // @confidence: low
    // @source: winapi:CreateDCA;DeleteDC;GetSystemPaletteEntries
    // @stub
    // proximity: CDDPalette@-0x8e0 | CFileImage@+0x290
    RVA(0x001485b0, 0x162)
    i32 ThisStubOwnerUnknown::winapi_1485b0_CreateDCA_DeleteDC_GetSystemPaletteEntries() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    // proximity: CImage@-0x7e0 | CRemusNode@+0x9e0
    RVA(0x00153ff0, 0x280)
    i32 ThisStubOwnerUnknown::winapi_153ff0_CopyRect(i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
    // @stub
    // proximity: CImage@-0xf40 | CRemusNode@+0x280
    RVA(0x00154750, 0x275)
    i32 ThisStubOwnerUnknown::winapi_154750_CopyRect(i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // A second MS-CRT-style LCG, seeded lazily from timeGetTime.
    DATA(0x006c278c)
    extern char g_rng2Seeded; // bit0 set once seeded
    DATA(0x006c2798)
    extern i32 g_rng2State; // 32-bit LCG state
    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    RVA(0x0015cbe0, 0x46)
    i32 winapi_15cbe0_timeGetTime() {
        i32 seed;
        if (!(g_rng2Seeded & 1)) {
            g_rng2Seeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_rng2State;
        }
        g_rng2State = seed * 214013 + 2531011;
        return (g_rng2State >> 0x10) & 0x7fff;
    }

    // @confidence: low
    // __thiscall(RECT*): cache the bounds rect + derived size/center, then recompute.
    struct GeoHost_161e80 {
        char m_pad0[0x50];
        RECT m_50; // +0x50 bounds
        char m_pad60[0x70 - 0x60];
        i32 m_70; // +0x70 width
        i32 m_74; // +0x74 height
        i32 m_78; // +0x78 half-width
        i32 m_7c; // +0x7c half-height
        void Build(RECT* pRect);
        void Recompute(); // RVA 0x161c90
    };
    // @source: winapi:CopyRect
    RVA(0x00161e80, 0x79)
    void GeoHost_161e80::Build(RECT* pRect) {
        if (pRect->left != (LONG)0x80000000) {
            RECT local;
            CopyRect(&local, pRect);
            m_50 = local;
            i32 width = m_50.right - m_50.left + 1;
            i32 height = m_50.bottom - m_50.top + 1;
            m_70 = width;
            m_74 = height;
            m_78 = width / 2;
            m_7c = height / 2;
            Recompute();
        }
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SetBkMode;SetTextColor
    // A polymorphic DC source: GetDC is vtable slot 0x44 (#17), Done is slot 0x68 (#26).
    struct DcSink_164380 {
        virtual void v0();
        virtual void v1();
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual void v8();
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void v12();
        virtual void v13();
        virtual void v14();
        virtual void v15();
        virtual void v16();
        virtual i32 GetDC(HDC* out); // slot 17 == vtable +0x44
        virtual void v18();
        virtual void v19();
        virtual void v20();
        virtual void v21();
        virtual void v22();
        virtual void v23();
        virtual void v24();
        virtual void v25();
        virtual void Done(HDC dc); // slot 26 == vtable +0x68
    };
    struct CounterWnd_164380 {
        char m_pad0[8];
        DcSink_164380* m_8; // +0x08
    };
    struct DrawHost_164380 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawCount(RECT* rc, i32 n);
    };
    // __thiscall(rc, n): print n centred into rc using the counter window's DC.
    RVA(0x00164380, 0x98)
    void DrawHost_164380::DrawCount(RECT* rc, i32 n) {
        char buf[0x20];
        Format_11f890(buf, "%i", n);
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
        w->m_8->Done(hdc);
    }

    // @confidence: low
    // @source: winapi:DrawTextA;SetBkMode;SetTextColor
    struct DrawHost2_164420 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawLabel(RECT* rc, char* text);
    };
    // __thiscall(rc, text): print text centred into rc using the counter window's DC.
    RVA(0x00164420, 0x79)
    void DrawHost2_164420::DrawLabel(RECT* rc, char* text) {
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, text, strlen(text), rc, 0x25);
        w->m_8->Done(hdc);
    }

    // 0x1644a0 (CDDrawSurfacePair::directx_wrapper_caller_1644a0_...) reconstructed
    // in src/Gruntz/CDDrawSurfacePair.cpp.

    // @confidence: low
    // @source: winapi:SetRect
    // @stub
    // proximity: CGameLevel@-0x1e0 | CWwdGrid@+0x220
    RVA(0x00168080, 0x1f6)
    i32 ThisStubOwnerUnknown::winapi_168080_SetRect(i32, i32, i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:InitializeCriticalSection
    // __cdecl thunk over InitializeCriticalSection.
    RVA(0x0016c9c0, 0xc)
    void winapi_16c9c0_InitializeCriticalSection(CRITICAL_SECTION* cs) {
        InitializeCriticalSection(cs);
    }

    // @confidence: low
    // @source: winapi:DeleteCriticalSection
    // __cdecl(cs): thin wrapper over DeleteCriticalSection.
    RVA(0x0016c9d0, 0xc)
    void winapi_16c9d0_DeleteCriticalSection(LPCRITICAL_SECTION cs) {
        DeleteCriticalSection(cs);
    }

    // @confidence: low
    // @source: winapi:EnterCriticalSection
    // __cdecl thunk over EnterCriticalSection.
    RVA(0x0016c9e0, 0xc)
    void winapi_16c9e0_EnterCriticalSection(CRITICAL_SECTION* cs) {
        EnterCriticalSection(cs);
    }

    // @confidence: low
    // @source: winapi:LeaveCriticalSection
    // __cdecl thunk over LeaveCriticalSection.
    RVA(0x0016c9f0, 0xc)
    void winapi_16c9f0_LeaveCriticalSection(CRITICAL_SECTION* cs) {
        LeaveCriticalSection(cs);
    }

    // The five CImagePool surface-node factories (0x174fe0/0x1750e0/0x1751f0/
    // 0x1752f0/0x1753f0) are reconstructed in src/Image/ImagePool.cpp.

    // 0x1757c0 (CreateDIBSection) removed: already matched as CImage::DecodeBmpHeader in src/Image/Image.cpp.

    // @confidence: low
    // @source: winapi:DeleteObject
    extern "C" void RezFree_call(void* p); // RVA 0x1b9b82 (cdecl)
    struct GdiOwner_175c90 {
        char m_pad0[0x428];
        HGDIOBJ m_428; // +0x428 (a GDI object)
        void* m_42c;   // +0x42c
        void* m_430;   // +0x430 (a Rez-allocated buffer)
        char m_pad434[0x458 - 0x434];
        i32 m_458; // +0x458
        void Cleanup();
    };
    // __thiscall(): release the cached GDI object + buffer and clear the slots.
    RVA(0x00175c90, 0x45)
    void GdiOwner_175c90::Cleanup() {
        if (m_428) {
            DeleteObject(m_428);
            m_428 = 0;
        }
        if (m_430) {
            RezFree_call(m_430);
            m_430 = 0;
        }
        m_42c = 0;
        m_458 = 0;
    }

    // @confidence: low
    // @source: winapi:CreatePalette
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps(); // RVA 0x1770a0
    // __thiscall(flags, src): build a 256-entry LOGPALETTE from src and realize it.
    struct PalBuilder_176df0 {
        HPALETTE m_0;     // +0x00
        LOGPALETTE m_pal; // +0x04 (palVersion/palNumEntries/palPalEntry[1])
        char m_pad_entries[0x408 - (4 + 4 + 4)];
        i32 m_408; // +0x408
        i32 m_40c; // +0x40c
        i32 Build(PALETTEENTRY* src, i32 flags);
        void Tune1770e0(); // RVA 0x1770e0
    };
    RVA(0x00176df0, 0x71)
    i32 PalBuilder_176df0::Build(PALETTEENTRY* src, i32 flags) {
        m_408 = flags;
        m_pal.palNumEntries = 0x100;
        m_pal.palVersion = 0x300;
        DWORD* s = (DWORD*)src;
        PALETTEENTRY* d = m_pal.palPalEntry;
        i32 i = 0x100;
        do {
            *(DWORD*)d = *s++;
            d->peFlags = 0;
            d++;
        } while (--i);
        if (winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() && !(flags & 1)) {
            Tune1770e0();
            m_40c = 1;
        }
        m_0 = CreatePalette(&m_pal);
        return m_0 != 0;
    }

    // @confidence: low
    // @source: winapi:DeleteObject
    // __thiscall: delete the owned GDI object, then clear a far flag.
    struct DeleteObjHost_177070 {
        HGDIOBJ m_obj; // +0x00
        char m_pad[0x408 - 4];
        i32 m_408; // +0x408
        void Run();
    };
    RVA(0x00177070, 0x22)
    void DeleteObjHost_177070::Run() {
        if (m_obj) {
            DeleteObject(m_obj);
            m_obj = 0;
        }
        m_408 = 0;
    }

    // @confidence: low
    // @source: winapi:CreateICA;DeleteDC;GetDeviceCaps
    // __cdecl(): does the display device support a palette? (RC_PALETTE bit)
    RVA(0x001770a0, 0x3a)
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() {
        HDC ic = CreateICA("DISPLAY", 0, 0, 0);
        if (ic) {
            i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
            DeleteDC(ic);
            return caps;
        }
        return 0;
    }

    // @confidence: low
    // @source: winapi:CreateDCA;DeleteDC;GetSystemPaletteEntries
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD(); // RVA 0x177160
    // __thiscall(): snapshot the reserved system-palette entries, marking the
    // interior animatable range PC_RESERVED (peFlags=1).
    RVA(0x001770e0, 0x7c)
    void PalBuilder_176df0::Tune1770e0() {
        winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD();
        HDC dc = CreateDCA("DISPLAY", 0, 0, 0);
        i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
        i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
        i32 half = numReserved / 2;
        GetSystemPaletteEntries(dc, 0, half, m_pal.palPalEntry);
        GetSystemPaletteEntries(
            dc,
            sizePal - half,
            half,
            &m_pal.palPalEntry[m_pal.palNumEntries - half]
        );
        for (i32 i = half; i < sizePal - half; i++) {
            m_pal.palPalEntry[i].peFlags = 1;
        }
        DeleteDC(dc);
    }

    // @confidence: low
    // @source: winapi:CreatePalette;DeleteObject;GetDC;RealizePalette;ReleaseDC
    // __cdecl: realize an all-black 256-entry palette on the screen DC to reset it.
    RVA(0x00177160, 0x81)
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
        char buf[4 + 256 * sizeof(PALETTEENTRY)];
        LOGPALETTE* lp = (LOGPALETTE*)buf;
        HDC hdc = GetDC(0);
        lp->palVersion = 0x300;
        lp->palNumEntries = 256;
        for (i32 i = 0; i < 256; i++) {
            lp->palPalEntry[i].peRed = 0;
            lp->palPalEntry[i].peGreen = 0;
            lp->palPalEntry[i].peBlue = 0;
            lp->palPalEntry[i].peFlags = 4;
        }
        HPALETTE hpal = CreatePalette(lp);
        if (hpal) {
            HPALETTE old = SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);
            DeleteObject(SelectPalette(hdc, old, FALSE));
        }
        ReleaseDC(0, hdc);
    }

    // @confidence: low
    // @source: winapi:FindResourceA;LoadResource;LockResource
    // The resource-module handle for palette lookups (DAT_006bf6e0).
    DATA(0x006bf6e0)
    extern HINSTANCE g_palModule_6bf6e0;
    struct PalHost_1775f0 {
        i32 Apply(const char* name, i32 arg);
        i32 Use176e70(void* data, i32 arg); // thiscall, RVA 0x176e70
    };
    // __thiscall(name, arg): find/load/lock a PALETTE resource, hand it on.
    RVA(0x001775f0, 0x62)
    i32 PalHost_1775f0::Apply(const char* name, i32 arg) {
        HINSTANCE mod = g_palModule_6bf6e0;
        if (!mod) {
            return 0;
        }
        HRSRC hRsrc = FindResourceA(mod, name, "PALETTE");
        if (!hRsrc) {
            return 0;
        }
        HGLOBAL hRes = LoadResource(mod, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use176e70(data, arg);
    }

    // @confidence: low
    // @source: winapi:IntersectRect
    // @stub
    RVA(0x00179e70, 0x5ec)
    i32 __stdcall winapi_179e70_IntersectRect(i32, i32, i32, i32, i32, i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: directx-wrapper-caller:calls 0x17c040 (DirectDrawCreate)
    // @stub
    RVA(0x0017c2a0, 0x14e)
    i32 __stdcall directx_wrapper_caller_17c2a0_DirectDrawCreate(i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:ShowCursor
    // @stub
    // proximity: CDDPageMgr@-0x3b0 | CSeverusWorker@+0x300
    RVA(0x0017c3f0, 0x11f)
    i32 ThisStubOwnerUnknown::winapi_17c3f0_ShowCursor(
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
    ) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:ShowCursor
    // A polymorphic sub: slot 24 (+0x60) finalizes, slot 1 (+0x04) deletes it.
    struct CursSink_17c510 {
        virtual void v0();
        virtual void Destroy(i32 del); // slot 1 == vtable +0x04
        // slots 2..23 elided as a gap so Finish lands at +0x60
        virtual void v2();
        virtual void v3();
        virtual void v4();
        virtual void v5();
        virtual void v6();
        virtual void v7();
        virtual void v8();
        virtual void v9();
        virtual void v10();
        virtual void v11();
        virtual void v12();
        virtual void v13();
        virtual void v14();
        virtual void v15();
        virtual void v16();
        virtual void v17();
        virtual void v18();
        virtual void v19();
        virtual void v20();
        virtual void v21();
        virtual void v22();
        virtual void v23();
        virtual void Finish(); // slot 24 == vtable +0x60
    };
    struct CursHost_17c510 {
        i32 m_0; // +0x00
        i32 m_4; // +0x04 active flag
        char m_pad8[0x53c - 8];
        CursSink_17c510* m_53c; // +0x53c
        void Teardown();
        void CloseSmacker(); // RVA 0x17c9b0
        void Free17d6b0();   // RVA 0x17d6b0
        void Free17cc80();   // RVA 0x17cc80
    };
    // __thiscall(): tear the playback object down and restore the cursor.
    RVA(0x0017c510, 0x5e)
    void CursHost_17c510::Teardown() {
        if (!m_4) {
            return;
        }
        CloseSmacker();
        Free17d6b0();
        m_0 = 0;
        m_4 = 0;
        Free17cc80();
        if (m_53c) {
            m_53c->Finish();
            if (m_53c) {
                m_53c->Destroy(1);
            }
            m_53c = 0;
        }
        ShowCursor(1);
    }

    // @confidence: low
    // @source: thirdparty:_SmackOpen@12;_SmackSoundUseDirectSound@4
    // Smacker imports (IAT): route audio through DirectSound, then open the stream.
    extern "C" __declspec(dllimport) void __stdcall SmackSoundUseDirectSound(void* ds);
    extern "C" __declspec(dllimport) i32 __stdcall SmackOpen(i32 src, u32 flags, i32 buf);
    // A releasable sub-buffer reached via manual vtable dispatch (slot +0x08).
    struct SmkBufVtbl_17c570;
    struct SmkBuf_17c570 {
        SmkBufVtbl_17c570* m_vptr;
    };
    struct SmkBufVtbl_17c570 {
        void* s0[2];
        void(__stdcall* Release)(SmkBuf_17c570*); // +0x08
    };
    struct SmkPlayer_17c570 {
        char m_pad0[4];
        i32 m_4; // +0x04 active flag
        i32 m_8; // +0x08
        char m_pad0c[0x10 - 0xc];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x24 - 0x14];
        SmkBuf_17c570* m_24; // +0x24
        SmkBuf_17c570* m_28; // +0x28
        char m_pad2c[0x508 - 0x2c];
        void* m_508; // +0x508 DirectSound
        char m_pad50c[0x514 - 0x50c];
        i32 m_514; // +0x514
        char m_pad518[0x538 - 0x518];
        i32 m_538;                                    // +0x538
        i32 Begin(i32 a2, i32 useDS, i32 a4, i32 a5); // RVA 0x17cfc0
        void CloseSmacker();                          // RVA 0x17c9b0
        i32 OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);
        i32 OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5);
    };
    // __thiscall(src,a2,useDS,a4,a5): open a Smacker stream (0xfe000 flags, plus
    // 0x100000 when DirectSound is requested), begin playback, roll back on failure.
    RVA(0x0017c570, 0xc0)
    i32 SmkPlayer_17c570::OpenLo(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
        if (!m_4) {
            return 0;
        }
        SmackSoundUseDirectSound(m_508);
        m_514 = a2;
        u32 flags;
        if (useDS == 1) {
            m_538 = useDS;
            flags = 0x100000;
        } else {
            m_538 = 0;
            flags = 0;
        }
        flags |= 0xfe000;
        m_10 = SmackOpen(src, flags, -1);
        if (!m_10) {
            return 0;
        }
        m_8 = 1;
        i32 r = Begin(a2, useDS, a4, a5);
        if (r) {
            return r;
        }
        if (m_24) {
            m_24->m_vptr->Release(m_24);
            m_24 = 0;
        }
        if (m_28) {
            m_28->m_vptr->Release(m_28);
            m_28 = 0;
        }
        CloseSmacker();
        return r;
    }

    // @confidence: low
    // @source: thirdparty:_SmackOpen@12;_SmackSoundUseDirectSound@4
    // __thiscall(src,a2,useDS,a4,a5): same as OpenLo but with the 0xff000 flag set.
    RVA(0x0017c630, 0xc0)
    i32 SmkPlayer_17c570::OpenHi(i32 src, i32 a2, i32 useDS, i32 a4, i32 a5) {
        if (!m_4) {
            return 0;
        }
        SmackSoundUseDirectSound(m_508);
        m_514 = a2;
        u32 flags;
        if (useDS == 1) {
            flags = 0x100000;
            m_538 = useDS;
        } else {
            m_538 = 0;
            flags = 0;
        }
        flags |= 0xff000;
        m_10 = SmackOpen(src, flags, -1);
        if (!m_10) {
            return 0;
        }
        m_8 = 1;
        i32 r = Begin(a2, useDS, a4, a5);
        if (r) {
            return r;
        }
        if (m_24) {
            m_24->m_vptr->Release(m_24);
            m_24 = 0;
        }
        if (m_28) {
            m_28->m_vptr->Release(m_28);
            m_28 = 0;
        }
        CloseSmacker();
        return r;
    }

    // @source: thirdparty:_SmackGoto@8;_SmackWait@4
    // Smacker playback advance: the per-frame "step / loop" driver.
    extern "C" __declspec(dllimport) i32 __stdcall SmackWait(i32 smk);
    extern "C" __declspec(dllimport) void __stdcall SmackSoundOnOff(i32 smk, i32 on);
    extern "C" __declspec(dllimport) void __stdcall SmackGoto(i32 smk, u32 frame);
    struct Movie_17c8e0 {
        char m_pad0[4];
        i32 m_4; // +0x04 active flag
        char m_pad8[0x10 - 8];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x1c - 0x14];
        i32 m_1c; // +0x1c pending command
        char m_pad20[0x86a0 - 0x20];
        i32 m_86a0;                     // +0x86a0 loop counter
        i32 Frame();                    // RVA 0x17caa0 (renders the next frame)
        i32 Pump(i32 flags, i32 count); // RVA 0x17c790
        i32 Advance(i32 cmd, i32 loops);
    };

    // @source: thirdparty,winapi:_SmackWait@4 | DispatchMessageA;PeekMessageA;TranslateMessage
    // __thiscall(flags, count): pump the Win32 queue while a Smacker movie plays; abort
    // with 1/0x100 on a key/mouse event the abort flags select, else render the next
    // frame and re-loop until `count` plays elapse (count==-1 loops forever).
    RVA(0x0017c790, 0x14a)
    i32 Movie_17c8e0::Pump(i32 flags, i32 count) {
        if (!m_4 || count < -1 || count == 0) {
            return 0;
        }
        m_86a0 = 1;
        MSG msg;
        for (;;) {
            if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
                if (msg.message == 0x104) {
                    continue;
                }
                if (msg.message == 0x105) {
                    continue;
                }
                if (msg.message == 0x100) {
                    if (flags & 1) {
                        return 1;
                    }
                    continue;
                }
                if (msg.message == 0x201 || msg.message == 0x204 || msg.message == 0x203
                    || msg.message == 0x206) {
                    if (flags & 0x100) {
                        return 0x100;
                    }
                    continue;
                }
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            } else {
                if (SmackWait(m_10)) {
                    continue;
                }
                if (Frame()) {
                    continue;
                }
                if (count != -1 && ++m_86a0 > count) {
                    return 0x11111111;
                }
                SmackSoundOnOff(m_10, 0);
                SmackGoto(m_10, 1);
                SmackSoundOnOff(m_10, 1);
            }
        }
    }
    // __thiscall(cmd, loops): wait for the stream, render a frame, and on EOF loop
    // back to the start until `loops` is exhausted (loops==-1 loops forever).
    RVA(0x0017c8e0, 0xca)
    i32 Movie_17c8e0::Advance(i32 cmd, i32 loops) {
        if (!cmd || !m_4 || loops < -1 || loops == 0) {
            return 0;
        }
        i32 result = 1;
        if (m_86a0 == 0) {
            m_86a0 = result;
        }
        if (SmackWait(m_10) == 0) {
            i32 saved = m_1c;
            m_1c = cmd;
            result = Frame();
            if (result == 0) {
                if (loops == -1 || ++m_86a0 <= loops) {
                    SmackSoundOnOff(m_10, 0);
                    SmackGoto(m_10, 1);
                    SmackSoundOnOff(m_10, 1);
                    result = 1;
                }
            }
            m_1c = saved;
        }
        if (result == 0) {
            m_86a0 = 0;
        }
        return result;
    }

    // @confidence: low
    // Smacker import (IAT) + the Rez allocator's free (RVA 0x1b9b82).
    extern "C" __declspec(dllimport) u32 __stdcall SmackClose(i32 smk);
    extern "C" void RezFree_call(void* p); // RVA 0x1b9b82 (cdecl)
    // The embedded sub-player whose Shutdown() lives at RVA 0x17b570.
    struct SmackSub_17c9b0 {
        void Shutdown(); // RVA 0x17b570
    };
    struct SmackHost_17c9b0 {
        char m_pad0[8];
        i32 m_8; // +0x08 active flag
        char m_pad0c[0x10 - 0xc];
        i32 m_10; // +0x10 Smacker handle
        char m_pad14[0x534 - 0x14];
        void* m_534; // +0x534 Rez buffer
        char m_pad538[0x540 - 0x538];
        SmackSub_17c9b0 m_540; // +0x540 sub-player
        i32 Close();
    };
    // @source: thirdparty:_SmackClose@4
    // __thiscall: shut the sub-player, close the Smacker stream, free buffers.
    RVA(0x0017c9b0, 0x5b)
    i32 SmackHost_17c9b0::Close() {
        if (!m_8) {
            return 0;
        }
        m_540.Shutdown();
        if (!m_10) {
            return 0;
        }
        SmackClose(m_10);
        m_10 = 0;
        if (m_534) {
            RezFree_call(m_534);
            m_534 = 0;
        }
        m_8 = 0;
        return 1;
    }

    // @source: thirdparty:_SmackDoFrame@4;_SmackNextFrame@4;_SmackToBuffer@28
    extern "C" __declspec(dllimport) void __stdcall
    SmackToBuffer(void* smk, i32 left, i32 top, i32 pitch, i32 height, void* buf, i32 flags);
    extern "C" __declspec(dllimport) void __stdcall SmackDoFrame(void* smk);
    extern "C" __declspec(dllimport) i32 __stdcall SmackToBufferRect(void* smk, i32 flags);
    extern "C" __declspec(dllimport) void __stdcall SmackNextFrame(void* smk);
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
    // The decoded Smacker stream header.
    struct Smack_17caa0 {
        char m_pad0[4];
        i32 m_4; // +0x04 width
        i32 m_8; // +0x08 height
        i32 m_c; // +0x0c frame count
        char m_pad10[0x68 - 0x10];
        i32 m_68; // +0x68
        char m_pad6c[0x374 - 0x6c];
        i32 m_374; // +0x374 current frame
        char m_pad378[0x380 - 0x378];
        i32 m_380; // +0x380 dirty-rect left
        i32 m_384; // +0x384 dirty-rect top
        i32 m_388; // +0x388 dirty-rect right
        i32 m_38c; // +0x38c dirty-rect bottom
    };
    struct MoviePlayer_17caa0 {
        char m_pad0[0x10];
        Smack_17caa0* m_10; // +0x10
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
        if (m_10->m_68 && m_520 == 8) {
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
            SmackToBuffer(m_10, 0, 0, m_ac, m_10->m_8, m_c0, m_510);
            SmackDoFrame(m_10);
            m_50c = 1;
            m_24->vptr->Unlock(m_24, m_c0);
        }
    afterLock:
        if (m_514 != 1) {
            while (SmackToBufferRect(m_10, 0) != 0) {
                Sub17cdf0(m_10->m_380, m_10->m_384, m_10->m_388, m_10->m_38c);
            }
        } else {
            Sub17cdf0(0, 0, m_10->m_4, m_10->m_8);
        }
        Smack_17caa0* s = m_10;
        if (s->m_374 == s->m_c - 1) {
            return 0;
        }
        SmackNextFrame(s);
        return 1;
    }

    // @confidence: low
    // @source: winapi:GetDC;GetSystemPaletteEntries;ReleaseDC
    struct PalCache_17cd90 {
        char m_pad0[0x108];
        PALETTEENTRY m_108[0x100]; // +0x108
        void Snapshot(HWND hWnd);
    };
    // __thiscall(hWnd): read the system palette, then blank every entry to a
    // reserved black so a remap can be rebuilt against it.
    RVA(0x0017cd90, 0x58)
    void PalCache_17cd90::Snapshot(HWND hWnd) {
        HDC hdc = GetDC(hWnd);
        GetSystemPaletteEntries(hdc, 0, 0x100, m_108);
        for (i32 i = 0; i < 0x100; i++) {
            m_108[i].peRed = 0;
            m_108[i].peBlue = 0;
            m_108[i].peGreen = 0;
            m_108[i].peFlags = 4;
        }
        ReleaseDC(hWnd, hdc);
    }

    // @confidence: low
    // @source: winapi:GetTickCount
    // @stub
    RVA(0x0017e620, 0x13b)
    i32 CFader::winapi_17e620_GetTickCount(i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:timeGetTime
    // @stub
    // proximity: CFaderSine@-0x10 | CFaderTileRender@+0x2810
    RVA(0x0017fe00, 0x12d)
    i32 ThisStubOwnerUnknown::winapi_17fe00_timeGetTime(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CopyRect
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

    // @confidence: low
    // @source: winapi:CreateDialogIndirectParamA;GetSystemMetrics;GlobalLock
    // @stub
    RVA(0x001ba677, 0x188)
    i32 winapi_1ba677_CreateDialogIndirectParamA_GetSystemMetrics_GlobalLock() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:DestroyWindow;GlobalFree;GlobalUnlock
    // @stub
    RVA(0x001ba819, 0x7c)
    i32 __stdcall winapi_1ba819_DestroyWindow_GlobalFree_GlobalUnlock(i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;FindResourceA;IsWindowEnabled;LoadResource;LockResource
    // @stub
    RVA(0x001ba9d2, 0x100)
    i32 winapi_1ba9d2_EnableWindow_FindResourceA_IsWindowEnabled_LoadResource_() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:EnableWindow;GetActiveWindow;SetActiveWindow
    // @stub
    RVA(0x001baaef, 0x48)
    i32 winapi_1baaef_EnableWindow_GetActiveWindow_SetActiveWindow() {
        return 0;
    }

    // @confidence: low
    // @source: winapi:CallWindowProcA;GetPropA;RemovePropA;SetWindowLongA
    // @stub
    RVA(0x001bb31b, 0x111)
    i32 __stdcall
    winapi_1bb31b_CallWindowProcA_GetPropA_RemovePropA_SetWindowLongA(i32, i32, i32, i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:GetClassInfoA;RegisterClassA
    // @stub
    RVA(0x001bbff4, 0x93)
    i32 __stdcall winapi_1bbff4_GetClassInfoA_RegisterClassA(i32) {
        return 0;
    }

    // @confidence: low
    // @source: winapi:FreeLibrary
    // __thiscall: free the owned module handle if present.
    struct LibHost_1bf577 {
        HMODULE m_0; // +0x00
        void Run();
    };
    RVA(0x001bf577, 0xe)
    void LibHost_1bf577::Run() {
        if (m_0) {
            FreeLibrary(m_0);
        }
    }

    // @confidence: low
    // @source: winapi:GlobalFree
    // __thiscall(): free the owned global handle at +0x00 if present.
    struct GlobalOwner_1c09de {
        HGLOBAL m_0; // +0x00
        void Free();
    };
    RVA(0x001c09de, 0xe)
    void GlobalOwner_1c09de::Free() {
        if (m_0) {
            GlobalFree(m_0);
        }
    }

    // LoadGruntAbilityTuning (0x57100) re-homed as CGrunt::LoadGruntAbilityTuning
    // in src/Gruntz/GruntAssetLoaders.cpp.

    // @confidence: med
    // @source: string-xref
    // @stub
    RVA(0x000597a0, 0x1345)
    void CGrunt::LoadGruntCombatAnimations(i32, i32, i32, i32, i32, i32, i32, i32) {}

    // LoadGruntDeathAnimations (0x60150) re-homed as CGrunt::LoadGruntDeathAnimations
    // in src/Gruntz/GruntAssetLoaders.cpp.

    // LoadVehicleGruntAnimations (0x63db0) re-homed as
    // CGrunt::LoadVehicleGruntAnimations in src/Gruntz/Grunt.cpp (trace owner CGrunt).

    // BuildGruntExitAnimation (0x641b0) re-homed as CGrunt::BuildGruntExitAnimation
    // in src/Gruntz/Grunt.cpp (the trace owner; sibling of BuildEntranceAnimation).

    // LoadBombGruntRunConfig (0x65630) re-homed as CGrunt::RunMoveConfig in
    // src/Gruntz/Grunt.cpp (trace owner CGrunt; the move-config / entrance dispatch).

    // @confidence: med
    // @source: string-xref
    // @stub
    // proximity: CUserLogic@-0x420 | CGrunt@+0x19d0
    RVA(0x00065e80, 0x12b8)
    void ThisStubOwnerUnknown::LoadPickupSprites(i32, i32, i32, i32, i32) {}

    // LoadWingzGruntSprites (0x68880) re-homed as CGrunt::LoadWingzGruntSprites
    // in src/Gruntz/GruntAssetLoaders.cpp.

    // LoadTeleporterGooConfig (0x6eb80) re-homed as CGooWellMgr::LoadTeleporterGooConfig
    // in src/Gruntz/GooWellMgr.cpp (owner = g_gameReg->m_68's class, the goo-well /
    // respawn / win-condition manager).

    // LoadGruntCombatTuning (0x7b930) re-homed as CGruntTileMgr::CombatCue in
    // src/Gruntz/CGruntTileMgr.cpp (owner = CGrunt::m_260's class).

    // LoadMonologoSprite (0x090d10) re-homed as CGruntzMgr::LoadMonologoSprite in
    // src/Gruntz/GruntzMgr.cpp (the trace owner; this in ecx == CGruntzMgr).

    // LoadMenuSelectSprite (0x0ba620) re-homed as CNetMgr::LoadMenuSelectSprite in src/Stub/CNetMgr.cpp (proximity HIGH).

    // LoadImageBanks (0x0cffe0) re-homed as CPlay::LoadImageBanks in
    // src/Gruntz/CPlay.cpp.

    // LoadSBITextEdges (0x0d1710) re-homed as CPlay::LoadSBITextEdges in src/Stub/CPlay.cpp (proximity HIGH).

    // @confidence: med
    // @source: string-xref
    // @stub
    // proximity: CGameModeObj@-0x70 | CPlay@+0x9d0
    RVA(0x000d65d0, 0x7a4)
    void ThisStubOwnerUnknown::LoadWarlordSprites(i32, i32) {}

    // LoadActionTileSprites (0x0db600), LoadLevelSounds (0x0db6c0) and
    // LoadLevelImages (0x0db7e0) re-homed (byte-exact) as CPlay methods in
    // src/Gruntz/CPlay.cpp, alongside their GAME-namespace siblings 0x0db8a0/
    // 0x0db930/0x0db9b0. Their loader-view structs (CActionTileOwner / CSoundOwner
    // / CActionResRegistry / CSoundResRegistry) remain defined in Backlog.cpp.

    // BuildMusicCategoryTable (0x0dba30) re-homed (byte-exact) as
    // CPlay::BuildMusicCategoryTable in src/Gruntz/CPlay.cpp.

    // BuildWorldLevelPath (0x0dbc80) re-homed as CPlay::BuildWorldLevelPath in src/Stub/CPlay.cpp (proximity HIGH).

    // SetEffectSpriteDurations (0x0dc060) re-homed (analyzed) as
    // CPlay::SetEffectSpriteDurations in src/Gruntz/CPlay.cpp.

    // BuildGruntNamespaceList (0x0dd050) re-homed as CPlay::BuildGruntNamespaceList in src/Stub/CPlay.cpp (proximity HIGH).

    // BuildWarlordNameTable (0x0dd340) re-homed (analyzed) as
    // CPlay::BuildWarlordNameTable in src/Gruntz/CPlay.cpp.

    // BuildSpriteImageKeyTable (0x0dd540) and BuildAnizKeyTable (0x0ddaa0) re-homed
    // (byte-exact) as CPlay methods in src/Gruntz/CPlay.cpp.

    // BuildToolToyColorTable (0x0e2400) re-homed (byte-exact) as
    // CSpriteRefTable::BuildToolToyColorTable in src/Gruntz/SpriteRefTable.cpp.

    // LoadToolToyPalettes (0x0e2980) re-homed (byte-exact) as
    // CSpriteRefTable::LoadToolToyPalettes in src/Gruntz/SpriteRefTable.cpp.

    // LoadGruntzPalette (0x0e2d10) re-homed as CSpriteRefTable::LoadGruntzPalette in
    // src/Gruntz/SpriteRefTable.cpp (the trace owner; m_4 == CSpriteRefTable::m_04).
    // Its CPalette* loader-view structs remain defined in Backlog.cpp.

    // BuildResourceTabStatusBar (0xe8a70), BuildStatzTabStatusBar (0xe9600) and
    // BuildMultiplayerTabStatusBar (0xea1f0) re-homed as CSbTab methods in
    // src/Gruntz/StatusBarTabBuilders.cpp.

    // LoadGruntSpawnConfig (0x11afb0) re-homed as a real CGruntSpawnConfig method
    // in src/Gruntz/CGruntSpawnConfig.cpp.

} // namespace ApiCallerStubs
