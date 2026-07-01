// BootyWalkAnim.cpp - the per-frame update of the booty ("WARP" spell) walking-
// grunt animation state machine. A sibling
// of BzState::BuildBootyGruntIdleAnimation (src/Gruntz/BootyMessages.cpp) on the
// same booty game-state object: `this` carries the per-player grunt arrays at
// +0x2c8 / +0x2d8, the active-player step index m_2e8, and the sub-state flags
// m_2ec / m_2f0; m_1b4 gates the init vs step path.
//
// IDENTITY recovered by string-xref (the "GRUNTZ_PICKUPS_<W/A/R/P>" WARP cycle,
// "GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D" / "GAME_FLAGRISE" cues). The booty-state class,
// g_mgrSettings sub-objects and every callee are unmatched engine code reached by
// raw this+offset / reloc-masked external thunks; only the offsets / code bytes are
// load-bearing (campaign doctrine). Modeled with real <Mfc.h> CString so cl emits
// the same /GX EH frame + the same WARP-letter jump-table CString build idiom.

#include <Ints.h>
#include <Mfc.h> // CString letter temp (/GX) + operator+

#include <rva.h>

// ---------------------------------------------------------------------------
// g_mgrSettings (0x64556c) sub-objects. m_7c is the per-level record; m_30 the
// ambient sound holder; m_60 the sound-cue player; m_74 the selection source.
struct BzLevelRecord {
    char m_pad00[0x4];
    i32 m_4; // +0x04  area/level index
    i32 m_8; // +0x08  suppress gate

    i32 GetRecordValue(i32 idx); // 0xfced0 (thunk 0x2bf3), __thiscall(int)
};
struct BzSoundPlayer {                          // BzSoundEntry::m_10
    i32 IsPlaying();                            // 0x1353f0
    void ConfigurePlay(i32 tag, i32, i32, i32); // 0x1360d0
};
struct BzSoundEntry {
    i32 Play(i32 tag, i32, i32, i32); // 0x1f940 (thunk 0x25fe)
    char m_pad00[0x10];
    BzSoundPlayer* m_10; // +0x10  player sub-object
    u32 m_14;            // +0x14  last-played stamp
    u32 m_18;            // +0x18  interval
};
struct BzFindTable {
    void Find(const char* name, BzSoundEntry** out); // 0x1b8438 (Lookup)
};
struct BzSoundSet {
    char m_pad00[0x10];
    BzFindTable m_10; // +0x10
    char m_pad14[0x30 - 0x14];
    i32 m_30; // +0x30  is-playing gate
};
struct BzSoundHolder {
    char m_pad00[0x28];
    BzSoundSet* m_28; // +0x28
};
struct BzCuePlayer {                         // g_mgrSettings->m_60
    void Play(i32, i32, i32, i32, i32, i32); // thunk 0x39f4, __thiscall 6-arg
};
struct BzSelSource {      // g_mgrSettings->m_74
    i32 GetSel(i32, i32); // thunk 0x4165
};
struct BzGameReg {
    char m_pad00[0x30];
    BzSoundHolder* m_30; // +0x30
    char m_pad34[0x60 - 0x34];
    BzCuePlayer* m_60; // +0x60
    char m_pad64[0x74 - 0x64];
    BzSelSource* m_74; // +0x74
    char m_pad78[0x7c - 0x78];
    BzLevelRecord* m_7c; // +0x7c
};
extern "C" BzGameReg* g_mgrSettings; // *0x64556c

// The per-player idle/walking grunt sprite. CacheFirstFrame caches the named first
// frame; ApplyLookupGeometry resolves its cycle geometry. Both reloc-masked
// __thiscall. m_1a0 is a completion sub-object (m_28 armed / m_20 done).
struct BzSpriteSub { // sprite + 0x1a0
    char m_pad00[0x20];
    i32 m_20; // +0x20  done flag  (sprite+0x1c0)
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28  armed flag (sprite+0x1c8)
};
struct BzSprite {
    void CacheFirstFrame(const char* name);             // 0x150540
    i32 ApplyLookupGeometry(const char* name, i32 def); // 0x1505b0
    char m_pad00[0x40];
    i32 m_40; // +0x40  visibility flags
    char m_pad44[0x4c - 0x44];
    i32 m_4c; // +0x4c  selection handle
    i32 m_50; // +0x50
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x58
    i32 m_5c; // +0x5c  sprite id
    i32 m_60; // +0x60  timer
    char m_pad64[0x1a0 - 0x64];
    BzSpriteSub m_1a0; // +0x1a0
};

// Per-player idle-sprite id table (0x5e9068), the wand/flag cue tag + enable gate,
// the wrap-safe draw clock, the "A" action-key string, and the inline-RNG state.
DATA(0x001e9068)
extern i32 g_idleSpriteIds[4]; // 0x5e9068
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
DATA(0x002c127d)
extern u8 g_randSeeded; // 0x6c127d  seed-init flag (bit 0)
DATA(0x002c1288)
extern i32 g_randSeed; // 0x6c1288  LCG seed

// The booty/secret game-state object hosting the walking-grunt tick.
class BzState {
public:
    i32 UpdateBootyWalkingGruntz(); // 0x1b690

    char m_pad00[0x1b4];
    i32 m_1b4; // +0x1b4  init/step gate
    char m_pad1b8[0x2c8 - 0x1b8];
    BzSprite* m_2c8[4]; // +0x2c8  per-player idle sprites (visibility)
    BzSprite* m_2d8[4]; // +0x2d8  per-player idle sprites (animation)
    i32 m_2e8;          // +0x2e8  active-player step index
    i32 m_2ec;          // +0x2ec  sub-state flag
    i32 m_2f0;          // +0x2f0  sub-state flag
};

// ===========================================================================
// UpdateBootyWalkingGruntz @0x1b690 - the per-frame booty walking-grunt state
// machine. Init path (m_1b4 != 0): seeds each player's idle/WARP-pickup grunt
// (a 4-way "W/A/R/P" letter jump-table CString build). Step path (m_1b4 == 0):
// scrolls the reveal, plays the wand/flag cues on a rate-limited clock, and
// advances m_2e8 across the players, seeding an inline-RNG cue on completion.
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
//  (3) peephole: two `m_40 &= ~1` sites where cl's loaded value lands in a
//      byte-addressable reg -> `andb al,0xFE` vs retail's dword `andl ecx,-2`.
//  (4) scheduling: g_sndCueTag is load-hoisted early in retail's GAME_FLAGRISE arm,
//      late in cl's.
//  (5) jumptable-data-overlap: the two 4-entry WARP `$L` jump tables + pooled W/R/P
//      `??_C@` string-constant labels (docs/patterns/jumptable-data-overlap.md).
RVA(0x0001b690, 0x7bf)
i32 BzState::UpdateBootyWalkingGruntz() {
    BzLevelRecord* rec = g_mgrSettings->m_7c;
    if (rec->m_8 != 0) {
        return 1;
    }
    i32 n = rec->m_4;
    if (n > 0x24) {
        return 1;
    }
    if (m_2e8 >= 4) {
        return 1;
    }

    if (m_1b4 != 0) {
        // ---- init path ----
        if (n < 0x24) {
            for (i32 i = 0; i < 4; i++) {
                if (i <= (g_mgrSettings->m_7c->m_4 - 1) % 4) {
                    m_2c8[i]->m_40 |= 1;
                    m_2d8[i]->m_5c = g_idleSpriteIds[i];
                    m_2d8[i]->m_60 = 0xdc;
                    m_2d8[i]->m_40 &= ~1;
                    if (g_mgrSettings->m_7c->GetRecordValue(i) == 0) {
                        m_2d8[i]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                        m_2d8[i]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
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
                        m_2d8[i]->CacheFirstFrame("GRUNTZ_PICKUPS");
                        m_2d8[i]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    }
                } else {
                    m_2c8[i]->m_5c = g_idleSpriteIds[i];
                    m_2c8[i]->m_60 = 0xdc;
                    m_2c8[i]->m_40 &= ~1;
                    m_2d8[i]->m_40 |= 1;
                }
            }
        }
        m_2e8 = 4;
        return 1;
    }

    // ---- step path (m_1b4 == 0) ----
    if (m_2c8[0]->m_5c != g_idleSpriteIds[0]) {
        for (i32 k = 0; k < 4; k++) {
            m_2c8[k]->m_5c -= 10;
        }
    }
    if (m_2e8 == 0 && (m_2d8[0]->m_40 & 1)) {
        m_2d8[0]->m_40 &= ~1;
        m_2d8[0]->m_5c = g_idleSpriteIds[0];
        m_2d8[0]->m_60 = 0x1f4;
    }

    if (m_2f0 == 0 && m_2d8[m_2e8]->m_60 <= 0x195) {
        if (g_mgrSettings->m_7c->GetRecordValue(m_2e8) == 0) {
            m_2f0 = 1;
            BzSoundSet* ss = g_mgrSettings->m_30->m_28;
            if (ss->m_30 == 0) {
                BzSoundEntry* res = 0;
                ss->m_10.Find("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", &res);
                if (res != 0) {
                    res->Play(g_sndCueTag, 0, 0, 0);
                }
            }
        }
    }

    if (m_2f0 != 0) {
        BzSoundSet* ss = g_mgrSettings->m_30->m_28;
        BzSoundEntry* res = 0;
        ss->m_10.Find("GRUNTZ_WANDGRUNT_WANDZGRUNTUI1D", &res);
        if (res == 0) {
            return 1;
        }
        if (res->m_10->IsPlaying() != 0) {
            m_2c8[m_2e8]->m_40 ^= 1;
        } else {
            m_2c8[m_2e8]->m_40 |= 1;
        }
    }

    if (m_2ec == 0 && m_2d8[m_2e8]->m_60 <= 0xdc) {
        {
            CString letter;
            switch (m_2e8) {
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
            i32 sel = g_mgrSettings->m_74->GetSel(0, 0);
            if (sel != 0) {
                if (g_mgrSettings->m_7c->GetRecordValue(m_2e8) != 0) {
                    BzSoundSet* ss = g_mgrSettings->m_30->m_28;
                    if (ss->m_30 == 0) {
                        BzSoundEntry* res = 0;
                        ss->m_10.Find("GAME_FLAGRISE", &res);
                        if (res != 0 && g_sndEnabled != 0) {
                            u32 clock = g_killCueClock;
                            if (clock - res->m_14 >= res->m_18) {
                                res->m_14 = clock;
                                res->m_10->ConfigurePlay(g_sndCueTag, 0, 0, 0);
                            }
                        }
                    }
                    m_2d8[m_2e8]->CacheFirstFrame("GRUNTZ_PICKUPS");
                    m_2d8[m_2e8]->ApplyLookupGeometry("GRUNTZ_PICKUPS_" + letter, 0);
                    BzSprite* g = m_2d8[m_2e8];
                    g->m_58 = 1;
                    g->m_50 = 0xa;
                    g->m_4c = sel;
                    m_2c8[m_2e8]->m_40 |= 1;
                    u32 x;
                    if (!(g_randSeeded & 1)) {
                        g_randSeeded |= 1;
                        x = g_pTimeGetTime();
                    } else {
                        x = g_randSeed;
                    }
                    g_randSeed = x * 214013 + 2531011;
                    g_mgrSettings->m_60
                        ->Play(0, 0x3bf, (((i32)g_randSeed >> 16) & 0x7fff) % 0x11, 1, -1, -1);
                    m_2ec = 1;
                } else {
                    m_2d8[m_2e8]->CacheFirstFrame("GRUNTZ_NORMALGRUNT_SOUTH_IDLE");
                    m_2d8[m_2e8]->ApplyLookupGeometry("GRUNTZ_NORMALGRUNT_IDLE4", 0);
                    BzSprite* g = m_2d8[m_2e8];
                    g->m_58 = 1;
                    g->m_50 = 0xa;
                    g->m_4c = sel;
                    m_2c8[m_2e8]->m_40 |= 1;
                    m_2e8++;
                    g_mgrSettings->m_60->Play(0, 0x441, 0, 1, -1, -1);
                    if (m_2e8 == g_mgrSettings->m_7c->m_4 % 4) {
                        m_2e8 = 4;
                        return 1;
                    }
                    if (m_2e8 < 4) {
                        m_2d8[m_2e8]->m_40 &= ~1;
                        m_2d8[m_2e8]->m_5c = g_idleSpriteIds[m_2e8];
                        m_2d8[m_2e8]->m_60 = 0x1f4;
                        m_2f0 = 0;
                        m_2ec = 0;
                    }
                }
            }
        }
    } else if (m_2ec != 0) {
        BzSpriteSub* sub = &m_2d8[m_2e8]->m_1a0;
        if (sub->m_28 != 0 && sub->m_20 == 0) {
            m_2e8++;
            if (m_2e8 == g_mgrSettings->m_7c->m_4 % 4) {
                m_2e8 = 4;
                return 1;
            }
            if (m_2e8 < 4) {
                m_2d8[m_2e8]->m_40 &= ~1;
                m_2d8[m_2e8]->m_5c = g_idleSpriteIds[m_2e8];
                m_2d8[m_2e8]->m_60 = 0x1f4;
                m_2ec = 0;
                m_2f0 = 0;
            }
        }
    } else {
        m_2d8[m_2e8]->m_60 -= 3;
    }
    return 0;
}

SIZE_UNKNOWN(BzCuePlayer);
SIZE_UNKNOWN(BzSelSource);
SIZE_UNKNOWN(BzSoundPlayer);
SIZE_UNKNOWN(BzSpriteSub);
