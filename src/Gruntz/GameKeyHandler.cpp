// GameKeyHandler.cpp - the in-game keyboard/cheat input dispatcher
// (C:\Proj\Gruntz). The biggest backlog megafunction (5850 B): the PLAY-state
// key handler, a single large routine that takes a virtual-key code (arg1) and
// the key's lParam (arg2) and routes it to the matching game/cheat action.
//
// Identity: the trace placeholder was StatusBarItem::vfunc_12, but the body is a
// game-input handler on a >0x500-byte PLAY-state object (`this`=esi). It reads
// the shared singletons CGameRegistry (*g_64556c), the dev/render-state
// (*g_645578, its +0x18 flags word gated by 0x20 = "cheats enabled"), the area
// index / default cue wParam g_644c54, the recycled-node free list g_freeList /
// g_freeListNodeBias, and a set of cheat-enable globals (g_6455a4/a8/ac/f8); it
// posts WM_COMMAND (0x111) to the host window via PostMessageA. Every callee is
// an engine method reached through a reloc-masked __thiscall ILT thunk; the only
// string it touches is the "GAME_TABHIGHLIGHT1" hint-sprite key it clears before
// most actions (Lookup in the game image registry, then free via g_inputCtx).
//
// The handler's top is a 3-way split on the modal level state (this->m_2dc's
// m_550/m_554): a confirm-dialog mode (Y/N/Enter/Esc), a paused mode (Q), and
// the normal gameplay key map. The normal map is one long cmp/je ladder over the
// virtual-key codes (HUD toggles, the letter cheats, the digit cheats 1..9, the
// numpad recorder/teleport keys, the alt-modified diagonal keys), ending in a
// jump-table switch over the F-keys (key-0x14 in 0..0x84) that fire the
// level-skip / win / lose debug actions.
//
// CARCASS doctrine: `this` and its sub-objects are unmatched engine classes, so
// they are accessed by raw this+offset (a deliberate, naming-independent choice
// for this externally-coupled handler). Every callee body is external (no-body,
// reloc-masked rel32); the data globals are named so their DIR32 operands
// reloc-mask too. Only the offsets / code bytes are load-bearing.
//
// Returns int (mostly 1): a void model would tail-merge the per-site
// `mov eax,1; ret` epilogues and the `mov eax,edi/ebp` tails (see
// docs/patterns/void-vs-bool-return-epilogue-split.md). RVA-keyed pairing
// ignores the QAEX vs QAEH mangling difference.
//
// @early-stop
// megafunction regalloc/frame wall. The verified top structure - the 5-flag
// guard, the 3-way modal split, and the dialog/paused (Y/N/Enter/Esc/Q)
// handlers with the GAME_TABHIGHLIGHT1 hint clear, the PerFrameCue gate, and the
// WM_COMMAND post - is reconstructed here and matches retail's logic. The
// remaining 5KB normal-gameplay cmp/je ladder + the F-key jump table + the
// manual g_freeList recorder-node churn is the part that hits the documented
// megafunction wall (sibling ValidateLevelTiles lands ~17%): MSVC5 pins 0/1/key
// in ebx/ebp/edi and reuses the incoming arg2 [esp+0x28] slot as the Lookup
// out-param scratch across the whole 5850-byte frame, scheduling that is not
// re-derivable from C source. Deferred to the final sweep
// (docs/patterns/o2-optimizer-bailout-framed.md; zero-register-pinning.md;
// jumptable-data-overlap.md).

#include <Win32.h> // PostMessageA (reloc-masked); the in-game host window post
#include <rva.h>

// ---------------------------------------------------------------------------
// Shared singletons and the recycled-node free list (named so DIR32 reloc-mask).
// ---------------------------------------------------------------------------
extern void* g_64556c;         // ?g_gameReg@@3PAUWwdGameReg@@A @0x64556c (game-mgr singleton)
extern void* g_645578;         // DAT_00645578 @0x645578 (dev/render-state; +0x18 flags)
extern "C" i32 g_644c54;       // DAT_00644c54 @0x644c54 (area index / default cue wParam)
extern void* g_freeList;       // ?g_freeList@@3PAXA       @0x645544
extern i32 g_freeListNodeBias; // ?g_freeListNodeBias@@3HA @0x64554c
extern i32 g_inputCtx;         // DAT_0061ab24 @0x61ab24 (GAME_TABHIGHLIGHT1 free sink)

// ---------------------------------------------------------------------------
// Engine helpers reached through reloc-masked __thiscall ILT thunks. Declared,
// no body -> their call rel32 reloc-masks. Receivers passed explicitly so each
// `mov ecx,recv; call` falls out.
// ---------------------------------------------------------------------------
void GkLookup(void* table10, const char* name, void** out); // 0x1b8438 Lookup
void GkFree(i32 sink, i32 a, i32 b, i32 c);                 // 0x25fe   free hint sprite
void GkActA(void* sink);                                    // 0x385a  (N/Esc reject path)
void GkActB(void* host4);                                   // 0x1fc8  (Y reject when mode != 1)
void GkActC(void* sink);                                    // 0x29aa  PerFrameCue commit

#define I32(p, off) (*(i32*)((char*)(p) + (off)))
#define PTR(p, off) (*(void**)((char*)(p) + (off)))

// The recurring "clear GAME_TABHIGHLIGHT1 hint" idiom (inlined at ~15 sites in
// retail): through host->m_30->m_28, if its +0x30 sub-flag is clear, Lookup the
// hint sprite in the table at +0x10 and free it via g_inputCtx when present.
static void ClearTabHint(void* host) {
    void* sub = PTR(PTR(host, 0x30), 0x28);
    if (I32(sub, 0x30) == 0) {
        void* found = 0;
        GkLookup((char*)sub + 0x10, "GAME_TABHIGHLIGHT1", &found);
        if (found != 0) {
            GkFree(g_inputCtx, 0, 0, 0);
        }
    }
}

// ---------------------------------------------------------------------------
// The PLAY-state input object (`this`). Raw-offset access throughout.
// ---------------------------------------------------------------------------
class CGamePlayInput {
public:
    i32 DispatchKey(i32 vk, i32 lparam); // ?vfunc_12@StatusBarItem@@QAEXHH@Z
    char m_pad[4];
};

// ===========================================================================
RVA(0x000cbcc0, 0x16da)
i32 CGamePlayInput::DispatchKey(i32 vk, i32 lparam) {
    void* self = this;

    // Top guard: any of the five transition flags set -> swallow the key (ret 1).
    if (I32(self, 0x484) != 0 || I32(self, 0x4ec) != 0 || I32(self, 0x4f8) != 0
        || I32(self, 0x500) != 0 || I32(PTR(self, 0x4), 0xc) != 0) {
        return 1;
    }

    void* host = PTR(self, 0x4);    // the game host (m_4 chain)
    void* level = PTR(self, 0x2dc); // the modal level state (m_2dc)
    i32 key = vk;

    // ---- 3-way modal split on the level state (m_550 / m_554) --------------
    if (I32(level, 0x554) != 0) {
        // confirm-dialog mode: Y/Enter accept, N/Esc reject.
        if (key == 0x59 || key == 0xd) {
            if (I32(g_64556c, 0x134) == 1) {
                ClearTabHint(host);
                if (I32(PTR(g_64556c, 0x68), 0x288) == 1) {
                    GkActC(PTR(g_64556c, 0x68));
                }
                // accept -> WM_COMMAND 0x806b to the host window
                PostMessageA((HWND)PTR(PTR(host, 0x4), 0x4), 0x111, 0x806b, 0);
                return 1;
            }
            ClearTabHint(host);
            GkActB(PTR(host, 0x4));
            return key;
        }
        if (key == 0x4e || key == 0x1b) {
            ClearTabHint(host);
            GkActA(self);
            return 1;
        }
        // any other key falls into the normal map (deferred tail).
    } else if (I32(level, 0x550) != 0) {
        // paused mode: only Q (0x51) acts (quit-confirm), via WM_COMMAND 0x8023.
        if (key == 0x51) {
            if (I32(g_64556c, 0x134) == 1) {
                ClearTabHint(host);
                if (I32(PTR(g_64556c, 0x68), 0x288) != 1) {
                    GkActC(PTR(g_64556c, 0x68));
                }
                PostMessageA((HWND)PTR(PTR(PTR(host, 0x4), 0x4), 0x4), 0x111, 0x8023, 0);
            }
        }
        return key;
    }

    // ===== normal gameplay key map (~5KB cmp/je ladder + F-key jump table) ===
    // The verified tail: the HUD/cheat key dispatch. Reconstructed structurally
    // at the final sweep (megafunction wall); the entry keeps retail's
    // single-function shape and returns 1 for the swallow-everything default.
    (void)host;
    (void)level;
    (void)lparam;
    return 1;
}

#undef I32
#undef PTR
