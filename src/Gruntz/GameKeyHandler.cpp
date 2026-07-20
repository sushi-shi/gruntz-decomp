// GameKeyHandler.cpp - the in-game keyboard/cheat input dispatcher
// (C:\Proj\Gruntz). The biggest backlog megafunction (5850 B): the PLAY-state
// key handler, CPlay::DispatchKey(vk, lparam), which routes a virtual-
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
// `this` and its sub-objects are TYPED against the real engine classes - self=CPlay,
// host=CGruntzMgr (the m_4 owner), level/lv=CStatusBarMgr (m_guts),
// dev=StateMgrBZ (g_spawnConfig), rec=CChatBoxOwner (m_hitTest), g_gameReg=CGruntzMgr,
// area=GruntzPlayer (g_gameReg->m_options[]). Every offset-cast site is typed - the
// render context is
// m_c->m_level(CGameLevel)->m_mainPlane(CPlaneRender) with m_planeCtx/m_originX/m_snappedX,
// the frame gate is CGameMgr::m_frameGate (+0xc), the goal is CTmGoal, the recorder ring
// nodes are CoordPoolNode, and the 0x23d90 blit is CGruntzCmdMgr::BlitTileMarker on
// m_cmdSubMgr (the ex-CObj23d90 view).
// Every callee body is external (reloc-masked rel32). Only offsets / code bytes are load-bearing.

#include <Wap32/Object.h>      // CObject (MFC) + windows.h/PostMessageA via <Mfc.h> (afx first)
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <Gruntz/CurPlayer.h>  // g_curPlayer
#include <rva.h>

// ---------------------------------------------------------------------------
// Named globals (so their DIR32 operands reloc-mask in objdiff).
// ---------------------------------------------------------------------------
// g_devState was a SECOND NAME for g_spawnConfig (0x245578) - same address,
// so nothing ever defined it. The canonical `StateMgrBZ* g_spawnConfig` comes from
// <Gruntz/Play.h> (included below); the dev->m_18 flag reads still mask.
// g_areaIdx was a SECOND NAME for g_curPlayer (0x244c54 current player index) - same address,
// so nothing ever defined it. Unified onto the canonical.
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) are
// fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
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

// External engine receivers - all now the REAL canonical classes (declared-only
// methods stay reloc-masked, so each `call rel32` masks; the SYMBOL is the real
// class method defined in its own unit). No .cpp-local receiver views remain.
#include <Gruntz/StatusBarMgr.h> // canonical CStatusBarMgr (the +0x2dc guts: tab/slot dispatch)
#include <Gruntz/ChatBoxOwner.h> // canonical CChatBoxOwner (+0x2e0 chat/cheat text sink)
#include <Gruntz/TriggerMgr.h> // canonical CTriggerMgr (group/cell/puddle dispatch + CenterOnGroup)
#include <Gruntz/GruntzCmdMgr.h> // canonical CGruntzCmdMgr (m_cmdSubMgr: BlitTileMarker @0x23d90)
#include <Gruntz/FontConfig.h>   // canonical CFontConfig (EndInput; non-virtual, cast-neutral)
#include <Gruntz/SoundCue.h>     // CSndHost (its +0x10 IS the real MFC CMapStringToOb)
#include <Gruntz/GruntzMgr.h>    // canonical CGruntzMgr (score/run/finish helpers) + GruntzPlayer
#include <Gruntz/Play.h>         // canonical CPlay - the PLAY-state object DispatchKey runs on
// CObj23d90 (the 0x23d90 grid-snap blit) is the canonical view in
// <Gruntz/BoundaryTailViews.h>, included below; its Blit body is re-homed here.
// The +0x488 ring buffer is a real MFC CDWordArray (SetAtGrow @0x1b5144, InsertAt
// @0x1b516b, RemoveAt @0x1b5200) and the +0x10 name map a real CMapStringToOb
// (Lookup @0x1b8008 -> CObject*&); both from <Mfc.h>, no local views. (??_7CMapStringToOb
// @0x1eafd4 is in the library vtable catalog.)

// Free-standing engine helpers (no `this`, reloc-masked, callee-cleanup).
void __stdcall FreeHintSprite(i32 tag, i32 a, i32 b, i32 c); // 0x25fe
void __stdcall Fn213f(i32 a, i32 b);                         // 0x213f
void __stdcall Fn2135(i32 a);                                // 0x2135

// The game-mgr's area table at reg+0x150 IS the canonical GruntzPlayer m_options[4]
// (each 0x238 bytes: drives the `idx*71` strength-reduced index addressing) -
// <Gruntz/GruntzPlayer.h> via GruntzMgr.h above. No local area-slot view.

// The game-mgr singleton (*0x64556c). Declared here - AFTER GruntzMgr.h - so the type
// is the complete CGruntzMgr (per Play.h's note, each TU declares the singleton with the
// view type it needs). extern "C" -> the symbol is `g_gameReg`, so retyping is byte-neutral.

// The recurring "clear GAME_TABHIGHLIGHT1 hint" idiom: through base->m_30->m_28,
// if its +0x30 sub-flag is clear, Lookup the hint sprite in the table at +0x10
// and free it via g_sndCueTag when present. Inlined at ~15 sites; a fresh `found`
// local per site so MSVC colors its stack slot from local liveness (as retail).
#define CLEAR_TAB_HINT(sndHost)                                                                    \
    do {                                                                                           \
        CSndHost* _s = (sndHost);                                                                  \
        if (_s->m_emitGate == 0) {                                                                 \
            void* found = 0;                                                                       \
            _s->m_10.Lookup("GAME_TABHIGHLIGHT1", found);                                          \
            if (found != 0)                                                                        \
                FreeHintSprite(g_sndCueTag, 0, 0, 0);                                              \
        }                                                                                          \
    } while (0)

// DispatchKey's `this` (esi) IS the canonical CPlay (Play.h). Its self-receiver (esi)
// engine callees are all real CPlay methods (thunk RVAs resolved to their bodies):
//   Fn2c7f 0x2c7f->0xda2d0 CPlay::FlushPendingOps    Fn385a 0x385a->0xd6560 ReleaseLevelOverlay
//   Fn2e28 0x2e28->0xd5f00 CPlay::ResetGoals         Fn3c15 0x3c15->0xd6440 EnterOverlayDrag
//   Fn17a8 0x17a8->0xd1b30 CPlay::SetCursorFrame     Fn35da 0x35da->0xd0120 LoadCursorSprites
// (all declared on CPlay in Play.h; reloc-masked non-virtual __thiscall calls).

// 0x23d90 (CGruntzCmdMgr::BlitTileMarker, the grid-snap blit DispatchKey drives
// on m_cmdSubMgr) lives in its home TU per the interval dossier:
// src/Gruntz/GruntzCmdMgr.cpp (the command-TU sandwich).

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
i32 CPlay::Vslot0c(i32 vk, i32 lparam) {
    CPlay* self = this;

    // Top guard (cbcc9): any of five transition flags set -> swallow (ret 1).
    if (self->m_hudSuppressed != 0) {
        return 1;
    }
    if (self->m_renderDisabled != 0) {
        return 1;
    }
    if (self->m_inGame != 0) {
        return 1;
    }
    if (self->m_paused != 0) {
        return 1;
    }
    if (self->m_4->m_frameGate != 0) {
        return 1;
    }

    CGruntzMgr* host = self->m_4;
    CStatusBarMgr* level = self->m_guts;
    i32 key = vk;

    // ---- 3-way modal split (cbd1b) ------------------------------------------
    if (level->m_toggleActive != 0 || level->m_toggleHandle != 0) {
        if (level->m_toggleHandle != 0) {
            // dialog mode (cbd3b)
            if (key == 0x59 || key == 0xd) {
                if (g_gameReg->m_134 == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
                    return 1;
                }
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                return 1;
            }
            if (key == 0x4e || key == 0x1b) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                this->ReleaseLevelOverlay(0);
                return 1;
            }
            // else fall through to the normal map
        } else {
            // paused mode (cbe51)
            if (key == 0x51) {
                if (g_gameReg->m_134 == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    if (g_gameReg->m_cmdGrid->m_phase == 1) {
                        g_gameReg->UpdateScoreHud();
                    }
                    PostMessageA(host->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
                }
                return 1;
            }
            // paused-only cheats S/R/N/O (cbee0)
            if (key == 0x53 && g_gameReg->m_134 == 1) {
                CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
            }
            if (key == 0x52) {
                if (host->m_134 == 1 && g_gameReg->m_cmdGrid->m_phase != 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    CGameWnd* r = g_gameReg->m_gameWnd;
                    PostMessageA(r->m_hwnd, 0x111, 0x806b, 0);
                }
                return 1;
            }
            if (key == 0x4e) {
                if (host->m_134 == 1 && g_gameReg->m_cmdGrid->m_phase == 1) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    (static_cast<CGruntzMgr*>((host)))->AccrueScoreTime();
                }
                return 1;
            }
            if (key == 0x4f) {
                if (host->m_134 != 1 && self->m_guts->m_578 != 0) {
                    CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
                    this->ReleaseLevelOverlay(0);
                }
                return 1;
            }
        }
    }

    // ===== normal gameplay key map (cc0ab) ==================================
    // Enter (cc0b6)
    if (key == 0xd) {
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_10 != 0) {
            rec->ProcessCheatInput(0xd, lparam);
        } else {
            rec->m_14->EndInput();
            rec->m_10 = 1;
            self->m_hitTest->ProcessCheatInput(0xd, lparam);
        }
        return 1;
    }
    // Esc (cc109)
    if (key == 0x1b) {
        CTriggerMgr* h68 = host->m_cmdGrid;
        CTmGoal* n = h68->m_goal;
        if (n != 0) {
            n->m_8 |= 0x10000; // the "released" bit
            h68->m_goal = 0;
        }
        h68->m_armed = 0;
        CChatBoxOwner* rec = self->m_hitTest;
        if (rec->m_10 != 0) {
            this->FlushPendingOps();
            self->m_hitTest->m_14->EndInput();
            self->m_hitTest->m_10 = 0;
            return 1;
        }
        if (this->FlushPendingOps() != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(g_gameReg->m_world->m_soundRegistry);
        if (g_gameReg->m_frameGate != 0) {
            g_gameReg->m_frameGate ^= 1;
            g_gameReg->FinishLevel(g_gameReg->m_frameGate, 1);
        }
        this->EnterOverlayDrag(1);
        return 1;
    }
    // gate (cc1ed): recorder busy / mode gate closed -> swallow
    if (self->m_hitTest->m_10 != 0) {
        return 1;
    }
    if (g_gameReg->m_cmdGrid->m_groupFlag == 0) {
        return 1;
    }

    // ---- letter cheats (cc21b) ---------------------------------------------
    StateMgrBZ* dev = g_spawnConfig;
    // Tab (cc221): cycle the active area to the next non-empty
    if (key == 0x9) {
        i32 idx = self->m_514;
        i32 pick;
        GruntzPlayer* area;
        if (dev->m_18 & 1) {
            pick = idx - 1;
            if (pick < 0) {
                pick = 3;
            }
            area = &g_gameReg->m_options[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick--;
                if (pick < 0) {
                    pick = 3;
                }
                area = &g_gameReg->m_options[pick];
            }
        } else {
            pick = idx + 1;
            if (pick >= 4) {
                pick = 0;
            }
            area = &g_gameReg->m_options[pick];
            while (pick != idx) {
                if (area->m_joined == 0 || (area->m_doneFlag == 0 && area->m_clearedRound == 0)) {
                    break;
                }
                pick++;
                if (pick >= 4) {
                    pick = 0;
                }
                area = &g_gameReg->m_options[pick];
            }
        }
        if (area->m_joined != 0 && area->m_doneFlag == 0 && area->m_clearedRound == 0) {
            self->m_514 = pick;
            this->ResetGoals(area->m_focusX, area->m_focusY);
        }
    }
    // H (cc30b): jump to the current area's default cue
    if (key == 0x48) {
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == 0) {
            return 1;
        }
        this->ResetGoals(a->m_focusX, a->m_focusY);
        return 1;
    }
    // Q (cc350): toggle the pause flag
    if (key == 0x51) {
        if ((dev->m_18 & 0x20) == 0) {
            return 1;
        }
        CGruntzMgr* h = self->m_4;
        if (h->m_frameGate != 0) {
            h->m_frameGate ^= 1;
            self->m_4->FinishLevel(h->m_frameGate, 1);
        }
        CSndHost* s = self->m_4->m_world->m_soundRegistry;
        if (s->m_emitGate == 0) {
            void* found = 0;
            s->m_10.Lookup("GAME_TABHIGHLIGHT1", found);
            if (found != 0) {
                FreeHintSprite(g_sndCueTag, 0, 0, 0);
            }
        }
        return 1;
    }
    // Z (cc3dd)
    if (key == 0x5a) {
        g_gameReg->m_cmdGrid->EnqueueGroupCells();
        return 1;
    }
    // C (cc3f9)
    if (key == 0x43) {
        g_gameReg->m_cmdGrid->CenterOnGroup(dev->m_18 & 0x20);
        return 1;
    }
    // T (cc41c)
    if (key == 0x54) {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionA();
        return 1;
    }
    // Y (cc444)
    if (key == 0x59) {
        this->FlushPendingOps();
        g_gameReg->m_cmdGrid->ToggleRegionB();
        return 1;
    }
    // Space (cc46d): recorder step / recycle node churn
    if (key == 0x20) {
        if (dev->m_18 & 0x20) {
            CPlaneRender* obj = self->m_c->m_level->m_mainPlane;
            i32 v0 = obj->m_snappedX;
            i32 v1 = obj->m_snappedY;
            i32* slot;
            if (self->arr488Count() < 4) {
                CoordPoolNode* head = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
                CoordPoolNode* nx = head->m_next;
                if (nx != 0) {
                    slot = reinterpret_cast<i32*>(&head->m_coord);
                    g_coordPool.m_freeHead = nx;
                } else {
                    slot = 0;
                }
            } else {
                // m_488 is the documented MFC DUAL-BAND object: retail calls BOTH the
                // CPtrArray band (GetAt/SetSize, typed below/in Play.cpp) and the
                // CDWordArray band (RemoveAt/InsertAt/SetAtGrow, the casts here) on this
                // ONE array - byte-identical classes, both COMDAT bands linked. The casts
                // are band selectors, retail-faithful (same verdict as the m_10map pair).
                slot = static_cast<i32*>(self->m_488.GetAt(0));
                (reinterpret_cast<CDWordArray*>(&self->m_488))->RemoveAt(0, 1);
                i32 c = self->m_49c - 1;
                self->m_49c = c;
                if (c < 0) {
                    self->m_49c = self->arr488Count() - 1;
                }
            }
            slot[0] = v0;
            slot[1] = v1;
            if (self->m_49c != self->arr488Count() - 1) {
                (reinterpret_cast<CDWordArray*>(&self->m_488))->InsertAt(self->m_49c + 1, reinterpret_cast<DWORD>(slot), 1);
                self->m_49c = self->m_49c + 1;
                return 1;
            }
            (reinterpret_cast<CDWordArray*>(&self->m_488))->SetAtGrow(self->arr488Count(), reinterpret_cast<DWORD>(slot));
            self->m_49c = self->m_49c + 1;
            return 1;
        }
        if (self->arr488Count() == 0) {
            return 1;
        }
        if (dev->m_18 & 1) {
            i32 c = self->m_49c - 1;
            self->m_49c = c;
            if (c < 0) {
                self->m_49c = self->arr488Count() - 1;
            }
        } else {
            i32 c = self->m_49c + 1;
            self->m_49c = c;
            if (c >= self->arr488Count()) {
                self->m_49c = 0;
            }
        }
        i32* e = static_cast<i32*>(self->m_488.GetAt(self->m_49c));
        this->ResetGoals(e[0], e[1]);
        return 1;
    }
    // Backspace (cc5da): delete the current recorder node
    if (key == 0x8) {
        if (self->arr488Count() <= 0) {
            return 1;
        }
        i32 cur = self->m_49c;
        if (cur < 0) {
            return 1;
        }
        CoordPoolNode* node =
            reinterpret_cast<CoordPoolNode*>((reinterpret_cast<char*>(self->m_488.GetAt(cur)) - g_coordPool.m_linkOffset));
        (reinterpret_cast<CDWordArray*>(&self->m_488))->RemoveAt(cur, 1);
        node->m_next = static_cast<CoordPoolNode*>(g_coordPool.m_freeHead);
        g_coordPool.m_freeHead = node;
        i32 c = self->m_49c - 1;
        self->m_49c = c;
        if (c != -1) {
            return 1;
        }
        if (self->arr488Count() == 0) {
            return 1;
        }
        self->m_49c = self->arr488Count() - 1;
        return 1;
    }
    // M (cc668)
    if (key == 0x4d && (dev->m_18 & 0x20)) {
        g_gameReg->SetSoundLevelState(g_gameReg->m_musicEnabled == 0);
        return 1;
    }
    // V (cc692)
    if (key == 0x56 && (dev->m_18 & 0x20)) {
        g_gameReg->m_isVoiceEnabled = (g_gameReg->m_isVoiceEnabled == 0);
        return 1;
    }
    // A (cc6bf)
    if (key == 0x41) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 2) {
            lv->SetTabState(2, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }
    // S (cc76e)
    if (key == 0x53) {
        if (dev->m_18 & 0x20) {
            g_gameReg->SetRunState(g_gameReg->m_soundEnabled == 0);
            return 1;
        }
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 3) {
            lv->SetTabState(3, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }
    // D (cc842)
    if (key == 0x44) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 1) {
            lv->SetTabState(1, 3);
            lv->Deactivate();
        } else {
            lv->Deactivate();
        }
        return 1;
    }
    // F (cc8f1)
    if (key == 0x46) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        if (g_gameReg->m_134 == 1) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        self->m_guts->AdvanceTab(g_spawnConfig->m_18 & 1);
        return 1;
    }
    // G (cc986)
    if (key == 0x47) {
        if (level->m_hitTestDisabled != 0) {
            return 1;
        }
        CLEAR_TAB_HINT(host->m_world->m_soundRegistry);
        CStatusBarMgr* lv = self->m_guts;
        if (lv->m_hlBusy != 0) {
            return 1;
        }
        if (lv->m_position == 2) {
            lv->RefreshState();
        }
        if (lv->m_activeTab != 5) {
            lv->SetTabState(5, 3);
        }
        lv->SetTab(5, 1);
        lv->Deactivate();
        return 1;
    }

    // ---- numpad arrows: extended vs numpad via lparam bit 0x1000000 (cca3c) -
    if (lparam & 0x1000000) {
        if (key == 0x25) {
            self->m_scrollEdgeLock |= 1;
            return 1;
        }
        if (key == 0x27) {
            self->m_scrollEdgeLock |= 4;
            return 1;
        }
        if (key == 0x26) {
            self->m_scrollEdgeLock |= 2;
            return 1;
        }
        if (key == 0x28) {
            self->m_scrollEdgeLock |= 8;
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
        GruntzPlayer* a = &g_gameReg->m_options[g_curPlayer];
        if (a == 0) {
            return 1;
        }
        if (g_gameReg->m_cmdGrid->m_rowCount[g_curPlayer] >= a->m_comboSel) {
            return 1;
        }
        CGruntzMgr* h = self->m_4;
        i32 my = self->m_cursorY;
        LevelCoordRect* r = &h->m_world->m_level->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 mx = self->m_cursorX;
        if (mx >= x1 || mx < x0 || my >= y1 || my < y0) {
            return 1;
        }
        h->m_cmdSubMgr
            ->BlitTileMarker(1, g_curPlayer, *reinterpret_cast<i16*>(&self->m_cursorX), *reinterpret_cast<i16*>(&self->m_cursorY), 0);
        return 1;
    }
    // P (ccca9): on bounds-fail or after the action, fall through to the x
    // handler (retail's `jge ccd39` / no return after Fn3003), not return.
    if (key == 0x50) {
        if (g_gooPuddlez == 0) {
            return 1;
        }
        if (g_gameReg->m_134 == 2) {
            return 1;
        }
        CGruntzMgr* h = self->m_4;
        i32 mx = self->m_cursorX;
        CGameLevel* q = h->m_world->m_level;
        LevelCoordRect* r = &q->m_planeCtx;
        i32 x0 = r->left;
        i32 y0 = r->top;
        i32 x1 = r->right;
        i32 y1 = r->bottom;
        i32 my = self->m_cursorY;
        if (!(mx >= x1 || mx < x0 || my >= y1 || my < y0)) {
            CPlaneRender* g = q->m_mainPlane;
            i32 by = g->m_originY - q->m_planeCtx.top + my;
            i32 bx = g->m_originX - q->m_planeCtx.left + mx;
            host->m_cmdGrid->SpawnPuddle(bx, by, 0, 0, 1, 0x19);
        }
    }
    // x (ccd39): teleport
    if (key == 0x78) {
        if (g_explosionz == 0) {
            return 1;
        }
        CGruntzMgr* h = self->m_4;
        i32 my = self->m_cursorY;
        CGameLevel* q = h->m_world->m_level;
        CPlaneRender* g = q->m_mainPlane;
        i32 by = ((g->m_originY - q->m_planeCtx.top + my) & ~0x1f) + 0x10;
        i32 bx = ((self->m_cursorX - q->m_planeCtx.left + g->m_originX) & ~0x1f) + 0x10;
        g_gameReg->m_cmdGrid->LoadExplosionSprites(bx, by, -1, 1);
        return 1;
    }
    // K (ccda8)
    if (key == 0x4b) {
        if (g_gruntDestruction == 0) {
            return 1;
        }
        i32 outA = 0;
        i32 outB = 0;
        i32 r =
            reinterpret_cast<i32>(host->m_cmdGrid->ScreenToCell(self->m_cursorX, self->m_cursorY, &outB, &outA, 5));
        if (r == 0) {
            return 1;
        }
        host->m_cmdGrid->CellDispatch(outB, outA, 0, -1);
        return 1;
    }
    // digit cheats 1-9 (cce0f)
    if (key == 0x31) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(1);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(1);
        }
        return 1;
    }
    if (key == 0x32) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(2);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(2);
        }
        return 1;
    }
    if (key == 0x33) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(3);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(3);
        }
        return 1;
    }
    if (key == 0x34) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(4);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(4);
        }
        return 1;
    }
    if (key == 0x35) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(5);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(5);
        }
        return 1;
    }
    if (key == 0x36) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(6);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(6);
        }
        return 1;
    }
    if (key == 0x37) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(7);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(7);
        }
        return 1;
    }
    if (key == 0x38) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(8);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(8);
        }
        return 1;
    }
    if (key == 0x39) {
        if (g_spawnConfig->m_18 & 0x20) {
            g_gameReg->m_cmdGrid->RebuildSelectionList(9);
        } else {
            g_gameReg->m_cmdGrid->CenterSelectionGroup(9);
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
        if (self->m_4f0 != 0) {
            return 1;
        }
        if (self->m_dragInhibit1 != 0) {
            self->m_dragInhibit1 = 0;
            self->m_guts->CommitSlot(0);
            this->SetCursorFrame(0);
            if (key != 0x2d) {
                goto tail_default;
            }
            return 1;
        }
        if (self->m_dragInhibit2 == 0) {
            goto tail_default2;
        }
        i32 st = self->m_cursorFrame;
        i32 ph = self->m_guts->m_pendingHlRow;
        i32 lvl;
        if (st >= 0x22) {
            lvl = 2;
        } else {
            lvl = (st >= 0x17);
        }
        self->m_dragInhibit2 = 0;
        if (key == 0x2e || key == 0x6e) {
            Fn2135(st);
            this->SetCursorFrame(0);
            return 1;
        }
        Fn213f(0, st);
        this->SetCursorFrame(0);
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
        g_gameReg->m_cmdGrid->m_pendingFxKind = 0;
        this->LoadCursorSprites(0, 0);
    }
tail_default2:
    // cd24a
    if (self->m_guts->m_hitTestDisabled != 0) {
        return 1;
    }
    {
        // F-key / numpad debug two-level switch (cd25e): key-0xC in 0..0x84
        CStatusBarMgr* lv = self->m_guts;
        switch (key) {
            case 0x0c:
                lv->HlClickGroup1(2);
                return 1;
            case 0x21:
                lv->HlClickGroup2(1);
                return 1;
            case 0x22:
                lv->HlClickGroup2(3);
                return 1;
            case 0x23:
                lv->HlClickGroup0(3);
                return 1;
            case 0x24:
                lv->HlClickGroup0(1);
                return 1;
            case 0x25:
                lv->HlClickGroup0(2);
                return 1;
            case 0x26:
                lv->HlClickGroup1(1);
                return 1;
            case 0x27:
                lv->HlClickGroup2(2);
                return 1;
            case 0x28:
                lv->HlClickGroup1(3);
                return 1;
            case 0x2d:
                lv->ActivateSlot(-1);
                return 1;
            case 0x61:
                lv->HlClickGroup0(3);
                return 1;
            case 0x62:
                lv->HlClickGroup1(3);
                return 1;
            case 0x63:
                lv->HlClickGroup2(3);
                return 1;
            case 0x64:
                lv->HlClickGroup0(2);
                return 1;
            case 0x65:
                lv->HlClickGroup1(2);
                return 1;
            case 0x66:
                lv->HlClickGroup2(2);
                return 1;
            case 0x67:
                lv->HlClickGroup0(1);
                return 1;
            case 0x68:
                lv->HlClickGroup1(1);
                return 1;
            case 0x69:
                lv->HlClickGroup2(1);
                return 1;
            case 0x6a:
                lv->HlClickGroup2(0);
                return 1;
            case 0x6f:
                lv->HlClickGroup1(0);
                return 1;
            case 0x90:
                lv->HlClickGroup0(0);
                return 1;
        }
    }
    return 1;
}

#undef CLEAR_TAB_HINT
