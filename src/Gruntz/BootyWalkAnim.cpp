// BootyWalkAnim.cpp - the per-frame update of the booty ("WARP" spell) walking-
// grunt animation state machine. A sibling
// of BzState::BuildBootyGruntIdleAnimation (src/Gruntz/BootyMessages.cpp) on the
// same booty game-state object: `this` carries the per-player grunt arrays at
// +0x2c8 / +0x2d8, the active-player step index m_stepIndex, and the sub-state flags
// m_walkStarted / m_soundStarted; m_initGate gates the init vs step path.
//
// IDENTITY recovered by string-xref (the "GRUNTZ_PICKUPS_<W/A/R/P>" WARP cycle,
// "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D" / "GAME_FLAGRISE" cues). The Bz* object graph is
// the shared <Gruntz/BzState.h>; the booty-state class, g_mgrSettings sub-objects and
// every callee are unmatched engine code reached by raw this+offset / reloc-masked
// external thunks; only the offsets / code bytes are load-bearing (campaign
// doctrine). Modeled with real <Mfc.h> CString so cl emits the same /GX EH frame +
// the same WARP-letter jump-table CString build idiom.

#include <Ints.h>
#include <Mfc.h> // CString letter temp (/GX) + operator+

#include <Gruntz/BzState.h>

#include <rva.h>
#include <Globals.h>

// Per-player idle-sprite id table (0x5e9068), the wand/flag cue tag + enable gate,
// the wrap-safe draw clock, the "A" action-key string, and the inline-RNG state.
DATA(0x0021ab24)
extern i32 g_sndCueTag; // 0x61ab24
DATA(0x0021ab20)
extern i32 g_sndEnabled; // 0x61ab20
DATA(0x002bf3c0)
extern "C" u32 g_killCueClock; // 0x6bf3c0
DATA(0x0020a454)
extern char s_actKeyA[]; // "A" (0x60a454)
extern "C" {
    DATA(0x002c4650)
    extern u32(__stdcall* g_pTimeGetTime)(); // PTR_timeGetTime_006c4650
}

// ===========================================================================
// UpdateBootyWalkingGruntz @0x1b690 - the per-frame booty walking-grunt state
// machine. Init path (m_initGate != 0): seeds each player's idle/WARP-pickup grunt
// (a 4-way "W/A/R/P" letter jump-table CString build). Step path (m_initGate == 0):
// scrolls the reveal, plays the wand/flag cues on a rate-limited clock, and
// advances m_stepIndex across the players, seeding an inline-RNG cue on completion.
// ===========================================================================
// @early-stop
// ~90.5% - complete + correct reconstruction (all logic, externs, globals, and the
// s_actKeyA "A" reloc named). Residual is layered documented walls, none steerable
// from source:
//  (1) global regalloc register-SELECTION: retail pins the persistent 0 constant in
//      ebx (`xor ebx,ebx` at entry) whereas cl selects ebp for it, cascading to
//      flipped `cmp` operand order (`cmp ecx,ebx` vs `cmp ebp,ecx`) and to ebp=1 vs
//      immediate-1 materializations across the paths.
//  (2) /GX EH-frame: the delinked `$L..`/`$T..` EH-state table + no-`__except_list`
//      frame vs cl's representation (docs/seh-eh.md); plus two return-0 exits that
//      run the CString `letter` destructor emit an INLINE EH-teardown epilogue
//      (`xor eax,eax` first) instead of retail's `jmp` to the single shared return-0
//      block (a tail-merge cl did not perform).
//  (3) peephole: two `m_visFlags &= ~1` sites where cl's loaded value lands in a
//      byte-addressable reg -> `andb al,0xFE` vs retail's dword `andl ecx,-2`.
//  (4) scheduling: g_sndCueTag is load-hoisted early in retail's GAME_FLAGRISE arm,
//      late in cl's.
//  (5) jumptable-data-overlap: the two 4-entry WARP `$L` jump tables + pooled W/R/P
//      `??_C@` string-constant labels (docs/patterns/jumptable-data-overlap.md).
RVA(0x0001b690, 0x7bf)
i32 BzState::UpdateBootyWalkingGruntz() {
    BzLevelRecord* rec = g_mgrSettings->m_levelRecord;
    if (rec->m_suppressGate != 0) {
        return 1;
    }
    i32 n = rec->m_levelIndex;
    if (n > 0x24) {
        return 1;
    }
    if (m_stepIndex >= 4) {
        return 1;
    }

    if (m_initGate != 0) {
        // ---- init path ----
        if (n < 0x24) {
            for (i32 i = 0; i < 4; i++) {
                if (i <= (g_mgrSettings->m_levelRecord->m_levelIndex - 1) % 4) {
                    m_visSprites[i]->m_visFlags |= 1;
                    m_animSprites[i]->m_spriteId = g_idleSpriteIds[i];
                    m_animSprites[i]->m_timer = 0xdc;
                    m_animSprites[i]->m_visFlags &= ~1;
                    if (g_mgrSettings->m_levelRecord->GetRecordValue(i) == 0) {
                        m_animSprites[i]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    } else {
                        CString letter;
                        switch (i) {
                            case 0:
                                letter = "W";
                                break;
                            case 1:
                                letter = s_actKeyA;
                                break;
                            case 2:
                                letter = "R";
                                break;
                            case 3:
                                letter = "P";
                                break;
                        }
                        m_animSprites[i]->CacheFirstFrame("GRUNTZ_PICKUPS");
                        m_animSprites[i]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    m_visSprites[i]->m_spriteId = g_idleSpriteIds[i];
                    m_visSprites[i]->m_timer = 0xdc;
                    m_visSprites[i]->m_visFlags &= ~1;
                    m_animSprites[i]->m_visFlags |= 1;
                }
            }
        }
        m_stepIndex = 4;
        return 1;
    }

    // ---- step path (m_initGate == 0) ----
    if (m_visSprites[0]->m_spriteId != g_idleSpriteIds[0]) {
        for (i32 k = 0; k < 4; k++) {
            m_visSprites[k]->m_spriteId -= 10;
        }
    }
    if (m_stepIndex == 0 && (m_animSprites[0]->m_visFlags & 1)) {
        m_animSprites[0]->m_visFlags &= ~1;
        m_animSprites[0]->m_spriteId = g_idleSpriteIds[0];
        m_animSprites[0]->m_timer = 0x1f4;
    }

    if (m_soundStarted == 0 && m_animSprites[m_stepIndex]->m_timer <= 0x195) {
        if (g_mgrSettings->m_levelRecord->GetRecordValue(m_stepIndex) == 0) {
            m_soundStarted = 1;
            BzSoundSet* ss = g_mgrSettings->m_soundHolder->m_soundSet;
            if (ss->m_playing == 0) {
                BzSoundEntry* res = 0;
                ss->m_findTable.Find("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", &res);
                if (res != 0) {
                    res->Play(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }

    if (m_soundStarted != 0) {
        BzSoundSet* ss = g_mgrSettings->m_soundHolder->m_soundSet;
        BzSoundEntry* res = 0;
        ss->m_findTable.Find("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", &res);
        if (res == 0) {
            return 1;
        }
        if (res->m_player->IsPlaying() != 0) {
            m_visSprites[m_stepIndex]->m_visFlags ^= 1;
        } else {
            m_visSprites[m_stepIndex]->m_visFlags |= 1;
        }
    }

    if (m_walkStarted == 0 && m_animSprites[m_stepIndex]->m_timer <= 0xdc) {
        {
            CString letter;
            switch (m_stepIndex) {
                case 0:
                    letter = "W";
                    break;
                case 1:
                    letter = s_actKeyA;
                    break;
                case 2:
                    letter = "R";
                    break;
                case 3:
                    letter = "P";
                    break;
            }
            i32 sel = g_mgrSettings->m_selSource->GetSel(0, 0);
            if (sel != 0) {
                if (g_mgrSettings->m_levelRecord->GetRecordValue(m_stepIndex) != 0) {
                    BzSoundSet* ss = g_mgrSettings->m_soundHolder->m_soundSet;
                    if (ss->m_playing == 0) {
                        BzSoundEntry* res = 0;
                        ss->m_findTable.Find("GAME_FLAGRISE", &res);
                        if (res != 0 && g_sndEnabled != 0) {
                            u32 clock = g_killCueClock;
                            if (clock - res->m_lastPlayed >= res->m_interval) {
                                res->m_lastPlayed = clock;
                                res->m_player->ConfigurePlay(g_sndCueTag, 0, 0, 0);
                            }
                        }
                    }
                    m_animSprites[m_stepIndex]->CacheFirstFrame("GRUNTZ_PICKUPS");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    BzSprite* g = m_animSprites[m_stepIndex];
                    g->m_58 = 1;
                    g->m_50 = 0xa;
                    g->m_selHandle = sel;
                    m_visSprites[m_stepIndex]->m_visFlags |= 1;
                    u32 x;
                    if (!(g_randSeeded & 1)) {
                        g_randSeeded |= 1;
                        x = g_pTimeGetTime();
                    } else {
                        x = g_randSeed;
                    }
                    g_randSeed = x * 214013 + 2531011;
                    g_mgrSettings->m_cuePlayer
                        ->Play(0, 0x3bf, (((i32)g_randSeed >> 16) & 0x7fff) % 0x11, 1, -1, -1);
                    m_walkStarted = 1;
                } else {
                    m_animSprites[m_stepIndex]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_animSprites[m_stepIndex]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    BzSprite* g = m_animSprites[m_stepIndex];
                    g->m_58 = 1;
                    g->m_50 = 0xa;
                    g->m_selHandle = sel;
                    m_visSprites[m_stepIndex]->m_visFlags |= 1;
                    m_stepIndex++;
                    g_mgrSettings->m_cuePlayer->Play(0, 0x441, 0, 1, -1, -1);
                    if (m_stepIndex == g_mgrSettings->m_levelRecord->m_levelIndex % 4) {
                        m_stepIndex = 4;
                        return 1;
                    }
                    if (m_stepIndex < 4) {
                        m_animSprites[m_stepIndex]->m_visFlags &= ~1;
                        m_animSprites[m_stepIndex]->m_spriteId = g_idleSpriteIds[m_stepIndex];
                        m_animSprites[m_stepIndex]->m_timer = 0x1f4;
                        m_soundStarted = 0;
                        m_walkStarted = 0;
                    }
                }
            }
        }
    } else if (m_walkStarted != 0) {
        BzSpriteSub* sub = &m_animSprites[m_stepIndex]->m_completion;
        if (sub->m_armed != 0 && sub->m_done == 0) {
            m_stepIndex++;
            if (m_stepIndex == g_mgrSettings->m_levelRecord->m_levelIndex % 4) {
                m_stepIndex = 4;
                return 1;
            }
            if (m_stepIndex < 4) {
                m_animSprites[m_stepIndex]->m_visFlags &= ~1;
                m_animSprites[m_stepIndex]->m_spriteId = g_idleSpriteIds[m_stepIndex];
                m_animSprites[m_stepIndex]->m_timer = 0x1f4;
                m_walkStarted = 0;
                m_soundStarted = 0;
            }
        }
    } else {
        m_animSprites[m_stepIndex]->m_timer -= 3;
    }
    return 0;
}
