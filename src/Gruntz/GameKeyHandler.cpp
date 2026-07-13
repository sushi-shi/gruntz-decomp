// GameKeyHandler.cpp - the in-game keyboard/cheat input dispatcher
// (C:\Proj\Gruntz). The biggest backlog megafunction (5850 B): the PLAY-state
// key handler, CGamePlayInput::DispatchKey(vk, lparam), which routes a virtual-
// key code to its game/cheat action.
//
// `this` (esi) is a >0x500-byte PLAY-state object; it reads the game-mgr
// singleton g_gameReg (*0x64556c), the dev/render-state g_spawnConfig
// (*0x645578, its +0x18 flags byte gated by 0x20 = "cheats enabled"), the area
// index g_curPlayer (0x644c54), the recycled-node free list g_coordPool.m_freeHead /
// g_coordPool.m_linkOffset, and a set of cheat-enable globals (g_gruntDestruction/B/C/D); it
// posts WM_COMMAND (0x111) to the host window via PostMessageA. Every callee is
// an engine method reached through a reloc-masked __thiscall ILT thunk; the only
// string it clears before most actions is "GAME_TABHIGHLIGHT1" (the hint sprite:
// Lookup in the image registry, then free via g_sndCueTag).
//
// Top: a 5-flag transition guard, then a 3-way modal split on the level state
// (m_2dc's m_550/m_554): dialog (Y/Enter accept, N/Esc reject), paused (Q quit,
// plus S/R/N/O cheats), else the normal gameplay map. The normal map is one long
// cmp/je ladder over the vk codes (HUD toggles, letter/digit cheats, numpad
// recorder/teleport keys, alt-modified arrows), ending in a two-level jump-table
// switch over the F-keys / numpad debug keys (key-0xC in 0..0x84).
//
// CARCASS doctrine: `this` and its engine sub-objects are unmatched engine
// classes, accessed by raw this+offset (a deliberate, naming-independent choice
// for this externally-coupled handler). Every callee body is external (no-body,
// reloc-masked rel32); the data globals are named so their DIR32 operands
// reloc-mask too. Only the offsets / code bytes are load-bearing.

#include <Wap32/Object.h> // CObject (MFC) + windows.h/PostMessageA via <Mfc.h> (afx first)
#include <Gruntz/BoundaryTailViews.h> // CObj23d90 (fuzzy-identity 0x23d90 grid-snap blit)
#include <rva.h>

// ---------------------------------------------------------------------------
// Named globals (so their DIR32 operands reloc-mask in objdiff).
// ---------------------------------------------------------------------------
extern "C" void* g_gameReg; // 0x64556c  _g_mgrSettings (game-mgr singleton)
extern i32 g_sndCueTag;     // 0x61ab24  ?g_sndCueTag@@3HA (hint-sprite free tag)
// g_devState was a SECOND NAME for g_spawnConfig (0x245578) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" void* g_spawnConfig;
// g_areaIdx was a SECOND NAME for g_curPlayer (0x244c54 current player index) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" i32 g_curPlayer;
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
extern FreeNodePool g_coordPool;
// g_cheatA was a SECOND NAME for g_gruntDestruction (0x2455a4 cheat toggle) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" i32 g_gruntDestruction;
// g_cheatB was a SECOND NAME for g_gruntCreation (0x2455a8 cheat toggle) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" i32 g_gruntCreation;
// g_cheatC was a SECOND NAME for g_gooPuddlez (0x2455ac cheat toggle) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" i32 g_gooPuddlez;
// g_cheatD was a SECOND NAME for g_explosionz (0x2455f8 cheat toggle) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" i32 g_explosionz;

// External engine receiver handle: every method is declared (no body) so its
// `call rel32` reloc-masks. Receivers are passed via the (EO*) cast so each
// `mov ecx,recv; call` falls out.
// Minimal typed views of the real receiver classes (folded from the old generic
// `EO` megaview): declared-only + reloc-masked, so each `call rel32` still masks,
// but the SYMBOL is now the real class method (defined in its own unit) - the call
// links and EO is gone. Heavy real headers are avoided (CARCASS doctrine); the
// mangled names match by class+method+signature.
struct CStatusBarMgr {
    i32 RefreshState();
    i32 Deactivate();
    void CommitSlot(i32 a);
    i32 SetTabState(i32 a, i32 b);
    i32 ActivateSlot(i32 a);
    i32 HlClickGroup2(i32 a);
    void AdvanceTab(i32 a);
    i32 HlClickGroup1(i32 a);
    i32 SetTab(i32 a, i32 b);
    i32 HlClickGroup0(i32 a);
};
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (group/cell/puddle dispatch)
struct CGroupSel {
    i32 CenterOnGroup(i32 a);
};
#include <Gruntz/FontConfig.h> // canonical CFontConfig (EndInput; non-virtual, cast-neutral)
#include <Gruntz/SoundCue.h>   // CSndHost (its +0x10 IS the real MFC CMapStringToOb)
#include <Gruntz/GruntzMgr.h>  // canonical CGruntzMgr (score/run/finish helpers)
// CObj23d90 (the 0x23d90 grid-snap blit) is the canonical view in
// <Gruntz/BoundaryTailViews.h>, included below; its Blit body is re-homed here.
struct EngineLabelBacklog {
    i32 LoadExplosionSprites(i32 a, i32 b, i32 c, i32 d);
};
struct CChatBoxOwner {
    void ProcessCheatInput(i32 a, i32 b);
};
// The +0x488 ring buffer is a real MFC CDWordArray (SetAtGrow @0x1b5144, InsertAt
// @0x1b516b, RemoveAt @0x1b5200) and the +0x10 name map a real CMapStringToOb
// (Lookup @0x1b8008 -> CObject*&); both from <Mfc.h>, no local views. (??_7CMapStringToOb
// @0x1eafd4 is in the library vtable catalog.)

// Free-standing engine helpers (no `this`, reloc-masked, callee-cleanup).
void __stdcall FreeHintSprite(i32 tag, i32 a, i32 b, i32 c); // 0x25fe
void __stdcall Fn213f(i32 a, i32 b);                         // 0x213f
void __stdcall Fn2135(i32 a);                                // 0x2135

// External engine array element in the game-mgr's area table at reg+0x150
// (0x238-byte stride: drives the `idx*71` strength-reduced index addressing).
struct RegArea {
    char _pad[0x238];
};

#define M(o, off) (*(i32*)((char*)(o) + (off)))
#define W(o, off) (*(i16*)((char*)(o) + (off)))
#define P(o, off) (*(void**)((char*)(o) + (off)))

// The recurring "clear GAME_TABHIGHLIGHT1 hint" idiom: through base->m_30->m_28,
// if its +0x30 sub-flag is clear, Lookup the hint sprite in the table at +0x10
// and free it via g_sndCueTag when present. Inlined at ~15 sites; a fresh `found`
// local per site so MSVC colors its stack slot from local liveness (as retail).
#define CLEAR_TAB_HINT(base)                                                                       \
    do {                                                                                           \
        CSndHost* _s = (CSndHost*)P(P(base, 0x30), 0x28);                                          \
        if (_s->m_emitGate == 0) {                                                                 \
            void* found = 0;                                                                       \
            _s->m_10.Lookup("GAME_TABHIGHLIGHT1", found);                                          \
            if (found != 0)                                                                        \
                FreeHintSprite(g_sndCueTag, 0, 0, 0);                                              \
        }                                                                                          \
    } while (0)

class CGamePlayInput {
public:
    i32 DispatchKey(i32 vk, i32 lparam);
    // self-receiver (esi) engine callees.
    i32 Fn2c7f();              // 0x2c7f
    void Fn385a(i32 a);        // 0x385a
    void Fn2e28(i32 a, i32 b); // 0x2e28
    void Fn3c15(i32 a);        // 0x3c15
    void Fn17a8(i32 a);        // 0x17a8
    void Fn35da(i32 a, i32 b); // 0x35da
    char m_pad[4];
};

// 0x23d90 (CObj23d90::Blit, the grid-snap blit DispatchKey drives via a
// (CObj23d90*)(P(h,0x6c)) receiver) lives in its home TU per the interval
// dossier: src/Gruntz/GruntzCmdMgr.cpp (the command-TU sandwich).

// ===========================================================================
// @early-stop
// ~78.5% (from a 5.4% "return 1" stub) - the full 5850-byte dispatcher is
// reconstructed run-by-run: 5-flag guard, 3-way modal split, ~40 key handlers
// (letter/digit/numpad cheats), the recorder-node free-list churn, and the
// two-level F-key jump table all match retail's LOGIC. Residual = a holistic
// megafunction regalloc cascade: retail pins the four callee-saved regs to
// {esi=this, edi=key, ebx=0, ebp=1} and RELOADS this->m_4/this->m_2dc per block;
// caching them as `host`/`level` locals (what scores highest) instead pins
// ebp=host and spills level, flipping ebp's role in every handler's register
// operands and holding the frame at `sub esp,0x8` vs retail's `sub esp,0x10`
// (so every `add esp,0x8`/`ret` epilogue + all [esp+N] slot offsets shift).
// Spelling this->m_4/m_2dc inline (macro, reloaded) DOES recover retail's exact
// ebx=0/ebp=1 allocation but scores LOWER (74%), because the per-use reload
// `mov reg,[esi+N]` instructions insert/delete against the objdiff sequence more
// than the register-operand diffs cost - i.e. the byte-correct spelling loses to
// the cached-locals spelling on the fuzzy metric. Three spellings measured
// (locals-both 78.5 / host-inline+level-local 75.9 / both-inline 74.1); locals
// banked. Deferred to the final sweep. See docs/patterns/
// megafunction-cached-locals-vs-reload-regalloc.md.
RVA(0x000cbcc0, 0x16da)
i32 CGamePlayInput::DispatchKey(i32 vk, i32 lparam) {
    void* self = this;

    // Top guard (cbcc9): any of five transition flags set -> swallow (ret 1).
    if (M(self, 0x484) != 0) {
        return 1;
    }
    if (M(self, 0x4ec) != 0) {
        return 1;
    }
    if (M(self, 0x4f8) != 0) {
        return 1;
    }
    if (M(self, 0x500) != 0) {
        return 1;
    }
    if (M(P(self, 0x4), 0xc) != 0) {
        return 1;
    }

    void* host = P(self, 0x4);
    void* level = P(self, 0x2dc);
    i32 key = vk;

    // ---- 3-way modal split (cbd1b) ------------------------------------------
    if (M(level, 0x550) != 0 || M(level, 0x554) != 0) {
        if (M(level, 0x554) != 0) {
            // dialog mode (cbd3b)
            if (key == 0x59 || key == 0xd) {
                if (M(g_gameReg, 0x134) == 1) {
                    CLEAR_TAB_HINT(host);
                    if (M(P(g_gameReg, 0x68), 0x288) == 1) {
                        ((CGruntzMgr*)(g_gameReg))->UpdateScoreHud();
                    }
                    PostMessageA((HWND)P(P(P(host, 0x4), 0x4), 0x4), 0x111, 0x8023, 0);
                    return 1;
                }
                CLEAR_TAB_HINT(host);
                ((CGruntzMgr*)(host))->AccrueScoreTime();
                return 1;
            }
            if (key == 0x4e || key == 0x1b) {
                CLEAR_TAB_HINT(host);
                this->Fn385a(0);
                return 1;
            }
            // else fall through to the normal map
        } else {
            // paused mode (cbe51)
            if (key == 0x51) {
                if (M(g_gameReg, 0x134) == 1) {
                    CLEAR_TAB_HINT(host);
                    if (M(P(g_gameReg, 0x68), 0x288) == 1) {
                        ((CGruntzMgr*)(g_gameReg))->UpdateScoreHud();
                    }
                    PostMessageA((HWND)P(P(P(host, 0x4), 0x4), 0x4), 0x111, 0x8023, 0);
                }
                return 1;
            }
            // paused-only cheats S/R/N/O (cbee0)
            if (key == 0x53 && M(g_gameReg, 0x134) == 1) {
                CLEAR_TAB_HINT(host);
                ((CGruntzMgr*)(host))->AccrueScoreTime();
            }
            if (key == 0x52) {
                if (M(host, 0x134) == 1 && M(P(g_gameReg, 0x68), 0x288) != 1) {
                    CLEAR_TAB_HINT(host);
                    void* r = P(g_gameReg, 0x4);
                    PostMessageA((HWND)P(r, 0x4), 0x111, 0x806b, 0);
                }
                return 1;
            }
            if (key == 0x4e) {
                if (M(host, 0x134) == 1 && M(P(g_gameReg, 0x68), 0x288) == 1) {
                    CLEAR_TAB_HINT(host);
                    ((CGruntzMgr*)(host))->AccrueScoreTime();
                }
                return 1;
            }
            if (key == 0x4f) {
                if (M(host, 0x134) != 1 && M(P(self, 0x2dc), 0x578) != 0) {
                    CLEAR_TAB_HINT(host);
                    this->Fn385a(0);
                }
                return 1;
            }
        }
    }

    // ===== normal gameplay key map (cc0ab) ==================================
    // Enter (cc0b6)
    if (key == 0xd) {
        void* rec = P(self, 0x2e0);
        if (M(rec, 0x10) != 0) {
            ((CChatBoxOwner*)(rec))->ProcessCheatInput(0xd, lparam);
        } else {
            ((CFontConfig*)(P(rec, 0x14)))->EndInput();
            M(rec, 0x10) = 1;
            ((CChatBoxOwner*)(P(self, 0x2e0)))->ProcessCheatInput(0xd, lparam);
        }
        return 1;
    }
    // Esc (cc109)
    if (key == 0x1b) {
        void* h68 = P(host, 0x68);
        void* n = P(h68, 0x23c);
        if (n != 0) {
            M(n, 0x8) |= 0x10000;
            M(h68, 0x23c) = 0;
        }
        M(h68, 0x230) = 0;
        void* rec = P(self, 0x2e0);
        if (M(rec, 0x10) != 0) {
            this->Fn2c7f();
            ((CFontConfig*)(P(P(self, 0x2e0), 0x14)))->EndInput();
            M(P(self, 0x2e0), 0x10) = 0;
            return 1;
        }
        if (this->Fn2c7f() != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(g_gameReg);
        if (M(g_gameReg, 0xc) != 0) {
            M(g_gameReg, 0xc) ^= 1;
            ((CGruntzMgr*)(g_gameReg))->FinishLevel(M(g_gameReg, 0xc), 1);
        }
        this->Fn3c15(1);
        return 1;
    }
    // gate (cc1ed): recorder busy / mode gate closed -> swallow
    if (M(P(self, 0x2e0), 0x10) != 0) {
        return 1;
    }
    if (M(P(g_gameReg, 0x68), 0x400) == 0) {
        return 1;
    }

    // ---- letter cheats (cc21b) ---------------------------------------------
    void* dev = g_spawnConfig;
    // Tab (cc221): cycle the active area to the next non-empty
    if (key == 0x9) {
        i32 idx = M(self, 0x514);
        i32 pick;
        RegArea* area;
        if (M(dev, 0x18) & 1) {
            pick = idx - 1;
            if (pick < 0) {
                pick = 3;
            }
            area = &((RegArea*)((char*)g_gameReg + 0x150))[pick];
            while (pick != idx) {
                if (M(area, 0x28) == 0 || (M(area, 0x2c) == 0 && M(area, 0x24) == 0)) {
                    break;
                }
                pick--;
                if (pick < 0) {
                    pick = 3;
                }
                area = &((RegArea*)((char*)g_gameReg + 0x150))[pick];
            }
        } else {
            pick = idx + 1;
            if (pick >= 4) {
                pick = 0;
            }
            area = &((RegArea*)((char*)g_gameReg + 0x150))[pick];
            while (pick != idx) {
                if (M(area, 0x28) == 0 || (M(area, 0x2c) == 0 && M(area, 0x24) == 0)) {
                    break;
                }
                pick++;
                if (pick >= 4) {
                    pick = 0;
                }
                area = &((RegArea*)((char*)g_gameReg + 0x150))[pick];
            }
        }
        if (M(area, 0x28) != 0 && M(area, 0x2c) == 0 && M(area, 0x24) == 0) {
            M(self, 0x514) = pick;
            this->Fn2e28(M(area, 0x220), M(area, 0x224));
        }
    }
    // H (cc30b): jump to the current area's default cue
    if (key == 0x48) {
        RegArea* a = &((RegArea*)((char*)g_gameReg + 0x150))[g_curPlayer];
        if (a == 0) {
            return 1;
        }
        this->Fn2e28(M(a, 0x220), M(a, 0x224));
        return 1;
    }
    // Q (cc350): toggle the pause flag
    if (key == 0x51) {
        if ((M(dev, 0x18) & 0x20) == 0) {
            return 1;
        }
        void* h = P(self, 0x4);
        if (M(h, 0xc) != 0) {
            M(h, 0xc) ^= 1;
            ((CGruntzMgr*)(P(self, 0x4)))->FinishLevel(M(h, 0xc), 1);
        }
        void* s = P(P(P(self, 0x4), 0x30), 0x28);
        if (M(s, 0x30) == 0) {
            void* found = 0;
            ((CSndHost*)s)->m_10.Lookup("GAME_TABHIGHLIGHT1", found);
            if (found != 0) {
                FreeHintSprite(g_sndCueTag, 0, 0, 0);
            }
        }
        return 1;
    }
    // Z (cc3dd)
    if (key == 0x5a) {
        ((CTriggerMgr*)(P(g_gameReg, 0x68)))->EnqueueGroupCells();
        return 1;
    }
    // C (cc3f9)
    if (key == 0x43) {
        ((CGroupSel*)(P(g_gameReg, 0x68)))->CenterOnGroup(M(dev, 0x18) & 0x20);
        return 1;
    }
    // T (cc41c)
    if (key == 0x54) {
        this->Fn2c7f();
        ((CTriggerMgr*)(P(g_gameReg, 0x68)))->ToggleRegionA();
        return 1;
    }
    // Y (cc444)
    if (key == 0x59) {
        this->Fn2c7f();
        ((CTriggerMgr*)(P(g_gameReg, 0x68)))->ToggleRegionB();
        return 1;
    }
    // Space (cc46d): recorder step / recycle node churn
    if (key == 0x20) {
        if (M(dev, 0x18) & 0x20) {
            void* obj = P(P(P(self, 0xc), 0x24), 0x5c);
            i32 v0 = M(obj, 0x84);
            i32 v1 = M(obj, 0x88);
            i32* slot;
            if (M(self, 0x490) < 4) {
                void* head = g_coordPool.m_freeHead;
                void* nx = P(head, 0);
                if (nx != 0) {
                    slot = (i32*)((char*)head + 4);
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = 0;
                }
            } else {
                slot = (i32*)P(P(self, 0x48c), 0);
                ((CDWordArray*)((char*)self + 0x488))->RemoveAt(0, 1);
                i32 c = M(self, 0x49c) - 1;
                M(self, 0x49c) = c;
                if (c < 0) {
                    M(self, 0x49c) = M(self, 0x490) - 1;
                }
            }
            slot[0] = v0;
            slot[1] = v1;
            if (M(self, 0x49c) != M(self, 0x490) - 1) {
                ((CDWordArray*)((char*)self + 0x488))->InsertAt(M(self, 0x49c) + 1, (DWORD)slot, 1);
                M(self, 0x49c) = M(self, 0x49c) + 1;
                return 1;
            }
            ((CDWordArray*)((char*)self + 0x488))->SetAtGrow(M(self, 0x490), (DWORD)slot);
            M(self, 0x49c) = M(self, 0x49c) + 1;
            return 1;
        }
        if (M(self, 0x490) == 0) {
            return 1;
        }
        if (M(dev, 0x18) & 1) {
            i32 c = M(self, 0x49c) - 1;
            M(self, 0x49c) = c;
            if (c < 0) {
                M(self, 0x49c) = M(self, 0x490) - 1;
            }
        } else {
            i32 c = M(self, 0x49c) + 1;
            M(self, 0x49c) = c;
            if (c >= M(self, 0x490)) {
                M(self, 0x49c) = 0;
            }
        }
        void* e = P(P(self, 0x48c), M(self, 0x49c) * 4);
        this->Fn2e28(M(e, 0), M(e, 0x4));
        return 1;
    }
    // Backspace (cc5da): delete the current recorder node
    if (key == 0x8) {
        if (M(self, 0x490) <= 0) {
            return 1;
        }
        i32 cur = M(self, 0x49c);
        if (cur < 0) {
            return 1;
        }
        void* node = P(P(self, 0x48c), cur * 4);
        ((CDWordArray*)((char*)self + 0x488))->RemoveAt(cur, 1);
        node = (char*)node - g_coordPool.m_linkOffset;
        P(node, 0) = g_coordPool.m_freeHead;
        g_coordPool.m_freeHead = node;
        i32 c = M(self, 0x49c) - 1;
        M(self, 0x49c) = c;
        if (c != -1) {
            return 1;
        }
        if (M(self, 0x490) == 0) {
            return 1;
        }
        M(self, 0x49c) = M(self, 0x490) - 1;
        return 1;
    }
    // M (cc668)
    if (key == 0x4d && (M(dev, 0x18) & 0x20)) {
        ((CGruntzMgr*)(g_gameReg))->SetSoundLevelState(M(g_gameReg, 0x14) == 0);
        return 1;
    }
    // V (cc692)
    if (key == 0x56 && (M(dev, 0x18) & 0x20)) {
        M(g_gameReg, 0x100) = (M(g_gameReg, 0x100) == 0);
        return 1;
    }
    // A (cc6bf)
    if (key == 0x41) {
        if (M(level, 0x354) != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host);
        void* lv = P(self, 0x2dc);
        if (M(lv, 0x548) != 0) {
            return 1;
        }
        if (M(lv, 0) == 2) {
            ((CStatusBarMgr*)(lv))->RefreshState();
        }
        if (M(lv, 0x10c) != 2) {
            ((CStatusBarMgr*)(lv))->SetTabState(2, 3);
            ((CStatusBarMgr*)(lv))->Deactivate();
        } else {
            ((CStatusBarMgr*)(lv))->Deactivate();
        }
        return 1;
    }
    // S (cc76e)
    if (key == 0x53) {
        if (M(dev, 0x18) & 0x20) {
            void* h68 = P(g_gameReg, 0x10);
            ((CGruntzMgr*)(h68))->SetRunState(h68 == 0);
            return 1;
        }
        if (M(level, 0x354) != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host);
        void* lv = P(self, 0x2dc);
        if (M(lv, 0x548) != 0) {
            return 1;
        }
        if (M(lv, 0) == 2) {
            ((CStatusBarMgr*)(lv))->RefreshState();
        }
        if (M(lv, 0x10c) != 3) {
            ((CStatusBarMgr*)(lv))->SetTabState(3, 3);
            ((CStatusBarMgr*)(lv))->Deactivate();
        } else {
            ((CStatusBarMgr*)(lv))->Deactivate();
        }
        return 1;
    }
    // D (cc842)
    if (key == 0x44) {
        if (M(level, 0x354) != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host);
        void* lv = P(self, 0x2dc);
        if (M(lv, 0x548) != 0) {
            return 1;
        }
        if (M(lv, 0) == 2) {
            ((CStatusBarMgr*)(lv))->RefreshState();
        }
        if (M(lv, 0x10c) != 1) {
            ((CStatusBarMgr*)(lv))->SetTabState(1, 3);
            ((CStatusBarMgr*)(lv))->Deactivate();
        } else {
            ((CStatusBarMgr*)(lv))->Deactivate();
        }
        return 1;
    }
    // F (cc8f1)
    if (key == 0x46) {
        if (M(level, 0x354) != 0) {
            return 1;
        }
        if (M(g_gameReg, 0x134) == 1) {
            return 1;
        }
        CLEAR_TAB_HINT(host);
        ((CStatusBarMgr*)(P(self, 0x2dc)))->AdvanceTab(M(g_spawnConfig, 0x18) & 1);
        return 1;
    }
    // G (cc986)
    if (key == 0x47) {
        if (M(level, 0x354) != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host);
        void* lv = P(self, 0x2dc);
        if (M(lv, 0x548) != 0) {
            return 1;
        }
        if (M(lv, 0) == 2) {
            ((CStatusBarMgr*)(lv))->RefreshState();
        }
        if (M(lv, 0x10c) != 5) {
            ((CStatusBarMgr*)(lv))->SetTabState(5, 3);
        }
        ((CStatusBarMgr*)(lv))->SetTab(5, 1);
        ((CStatusBarMgr*)(lv))->Deactivate();
        return 1;
    }

    // ---- numpad arrows: extended vs numpad via lparam bit 0x1000000 (cca3c) -
    if (lparam & 0x1000000) {
        if (key == 0x25) {
            M(self, 0x4b8) |= 1;
            return 1;
        }
        if (key == 0x27) {
            M(self, 0x4b8) |= 4;
            return 1;
        }
        if (key == 0x26) {
            M(self, 0x4b8) |= 2;
            return 1;
        }
        if (key == 0x28) {
            M(self, 0x4b8) |= 8;
            return 1;
        }
        if (key == 0x2d || key == 0x2e || key == 0x24 || key == 0x23 || key == 0x21
            || key == 0x22) {
            return 1;
        }
    }
    // recorder-place / teleport key group -> ccfd3
    if (key == 0x61 || key == 0x62 || key == 0x63 || key == 0x64 || key == 0x65 || key == 0x66
        || key == 0x67 || key == 0x68 || key == 0x69 || key == 0x90 || key == 0x6f || key == 0x6a
        || key == 0x24 || key == 0x23 || key == 0x21 || key == 0x22 || key == 0xc || key == 0x26
        || key == 0x28 || key == 0x25 || key == 0x27 || key == 0x2d || key == 0x2e || key == 0x6e) {
        goto recorder_place;
    }
    // I (ccbe3): teleport marker place
    if (key == 0x49) {
        if (g_gruntCreation == 0) {
            return 1;
        }
        RegArea* a = &((RegArea*)((char*)g_gameReg + 0x150))[g_curPlayer];
        if (a == 0) {
            return 1;
        }
        if (M(P(g_gameReg, 0x68), g_curPlayer * 4 + 0x10c) >= M(a, 0x228)) {
            return 1;
        }
        void* h = P(self, 0x4);
        i32 my = M(self, 0x154);
        i32* r = (i32*)((char*)P(P(h, 0x30), 0x24) + 0x10);
        i32 x0 = r[0];
        i32 y0 = r[1];
        i32 x1 = r[2];
        i32 y1 = r[3];
        i32 mx = M(self, 0x150);
        if (mx >= x1 || mx < x0 || my >= y1 || my < y0) {
            return 1;
        }
        ((CObj23d90*)(P(h, 0x6c)))->Blit(1, g_curPlayer, W(self, 0x150), W(self, 0x154), 0);
        return 1;
    }
    // P (ccca9): on bounds-fail or after the action, fall through to the x
    // handler (retail's `jge ccd39` / no return after Fn3003), not return.
    if (key == 0x50) {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (M(g_gameReg, 0x134) == 2) {
            return 1;
        }
        void* h = P(self, 0x4);
        i32 mx = M(self, 0x150);
        void* q = P(P(h, 0x30), 0x24);
        i32* r = (i32*)((char*)q + 0x10);
        i32 x0 = r[0];
        i32 y0 = r[1];
        i32 x1 = r[2];
        i32 y1 = r[3];
        i32 my = M(self, 0x154);
        if (!(mx >= x1 || mx < x0 || my >= y1 || my < y0)) {
            void* g = (char*)P(q, 0x5c) + 0x40;
            i32 by = M(g, 0x4) - M(q, 0x14) + my;
            i32 bx = M(g, 0) - M(q, 0x10) + mx;
            ((CTriggerMgr*)(P(P(self, 0x4), 0x68)))->SpawnPuddle(bx, by, 0, 0, 1, 0x19);
        }
    }
    // x (ccd39): teleport
    if (key == 0x78) {
        if (g_explosionz == 0) {
            return 1;
        }
        void* h = P(self, 0x4);
        i32 my = M(self, 0x154);
        void* q = P(P(h, 0x30), 0x24);
        void* g = (char*)P(q, 0x5c) + 0x40;
        i32 by = ((M(g, 0x4) - M(q, 0x14) + my) & ~0x1f) + 0x10;
        i32 bx = ((M(self, 0x150) - M(q, 0x10) + M(g, 0)) & ~0x1f) + 0x10;
        ((EngineLabelBacklog*)(P(g_gameReg, 0x68)))->LoadExplosionSprites(bx, by, -1, 1);
        return 1;
    }
    // K (ccda8)
    if (key == 0x4b) {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 outA = 0;
        i32 outB = 0;
        i32 r = (i32)((CTriggerMgr*)(P(P(self, 0x4), 0x68)))
                    ->ScreenToCell(M(self, 0x150), M(self, 0x154), &outB, &outA, 5);
        if (r == 0) {
            return 1;
        }
        ((CTriggerMgr*)(P(P(self, 0x4), 0x68)))->CellDispatch(outB, outA, 0, -1);
        return 1;
    }
    // digit cheats 1-9 (cce0f)
    if (key == 0x31) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(1);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (key == 0x32) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(2);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (key == 0x33) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(3);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (key == 0x34) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(4);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (key == 0x35) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(5);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (key == 0x36) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(6);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (key == 0x37) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(7);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (key == 0x38) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(8);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (key == 0x39) {
        if (M(g_spawnConfig, 0x18) & 0x20) {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->RebuildSelectionList(9);
        } else {
            ((CTriggerMgr*)(P(g_gameReg, 0x68)))->CenterSelectionGroup(9);
        }
        return 1;
    }
    return 1; // non-recorder key that matched nothing -> cd38b

recorder_place:
    // ccfd3: place a recorder node / cancel a pending place (reached only via the
    // numpad-key goto above). The (recorder-level, phase, key) gating decides
    // whether the numpad key is a valid place (return 1) or falls to the level-
    // skip switch (goto tail_default).
    {
        if (M(self, 0x4f0) != 0) {
            return 1;
        }
        if (M(self, 0x368) != 0) {
            M(self, 0x368) = 0;
            ((CStatusBarMgr*)(P(self, 0x2dc)))->CommitSlot(0);
            this->Fn17a8(0);
            if (key != 0x2d) {
                goto tail_default;
            }
            return 1;
        }
        if (M(self, 0x36c) == 0) {
            goto tail_default2;
        }
        i32 st = M(self, 0x2f4);
        i32 ph = M(P(self, 0x2dc), 0x360);
        i32 lvl;
        if (st >= 0x22) {
            lvl = 2;
        } else {
            lvl = (st >= 0x17);
        }
        M(self, 0x36c) = 0;
        if (key == 0x2e || key == 0x6e) {
            Fn2135(st);
            this->Fn17a8(0);
            return 1;
        }
        Fn213f(0, st);
        this->Fn17a8(0);
        if (lvl == 0) {
            if (ph == 0) {
                if (key != 0x90) {
                    goto tail_default;
                }
                return 1;
            }
            if (ph == 1) {
                if (key == 0x67) {
                    return 1;
                }
                if (key != 0x24) {
                    goto tail_default;
                }
                return 1;
            }
            if (ph == 2) {
                if (key == 0x64) {
                    return 1;
                }
                if (key != 0x25) {
                    goto tail_default;
                }
                return 1;
            }
            if (key == 0x61) {
                return 1;
            }
            if (key != 0x23) {
                goto tail_default;
            }
            return 1;
        }
        if (lvl == 1) {
            if (ph == 0) {
                if (key != 0x6f) {
                    goto tail_default;
                }
                return 1;
            }
            if (ph == 1) {
                if (key == 0x68) {
                    return 1;
                }
                if (key != 0x26) {
                    goto tail_default;
                }
                return 1;
            }
            if (ph == 2) {
                if (key != 0xc) {
                    goto tail_default;
                }
                return 1;
            }
            if (key == 0x62) {
                return 1;
            }
            if (key != 0x28) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 0) {
            if (key != 0x6a) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 1) {
            if (key == 0x69) {
                return 1;
            }
            if (key != 0x21) {
                goto tail_default;
            }
            return 1;
        }
        if (ph == 2) {
            if (key == 0x66) {
                return 1;
            }
            if (key != 0x27) {
                goto tail_default;
            }
            return 1;
        }
        if (key == 0x63) {
            return 1;
        }
        if (key != 0x22) {
            goto tail_default;
        }
        return 1;
    }

tail_default:
    // cd22c
    {
        void* h68 = P(g_gameReg, 0x68);
        M(h68, 0x2a8) = 0;
        this->Fn35da(0, 0);
    }
tail_default2:
    // cd24a
    if (M(P(self, 0x2dc), 0x354) != 0) {
        return 1;
    }
    {
        // F-key / numpad debug two-level switch (cd25e): key-0xC in 0..0x84
        void* lv = P(self, 0x2dc);
        switch (key) {
            case 0x0c:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(2);
                return 1;
            case 0x21:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(1);
                return 1;
            case 0x22:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(3);
                return 1;
            case 0x23:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(3);
                return 1;
            case 0x24:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(1);
                return 1;
            case 0x25:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(2);
                return 1;
            case 0x26:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(1);
                return 1;
            case 0x27:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(2);
                return 1;
            case 0x28:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(3);
                return 1;
            case 0x2d:
                ((CStatusBarMgr*)(lv))->ActivateSlot(-1);
                return 1;
            case 0x61:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(3);
                return 1;
            case 0x62:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(3);
                return 1;
            case 0x63:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(3);
                return 1;
            case 0x64:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(2);
                return 1;
            case 0x65:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(2);
                return 1;
            case 0x66:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(2);
                return 1;
            case 0x67:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(1);
                return 1;
            case 0x68:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(1);
                return 1;
            case 0x69:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(1);
                return 1;
            case 0x6a:
                ((CStatusBarMgr*)(lv))->HlClickGroup2(0);
                return 1;
            case 0x6f:
                ((CStatusBarMgr*)(lv))->HlClickGroup1(0);
                return 1;
            case 0x90:
                ((CStatusBarMgr*)(lv))->HlClickGroup0(0);
                return 1;
        }
    }
    return 1;
}

#undef M
#undef W
#undef P
#undef E
#undef CLEAR_TAB_HINT

SIZE_UNKNOWN(CGamePlayInput);
SIZE(RegArea, 0x238);
SIZE_UNKNOWN(EO);
