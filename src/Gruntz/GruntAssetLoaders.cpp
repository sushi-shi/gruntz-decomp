// GruntAssetLoaders.cpp - the mechanical CGrunt asset / sprite / tuning loaders
// (re-homed from src/Stub/ApiCallers.cpp). Each (re)loads a slab of the grunt's
// per-direction sprite-name cells, pose-index lookups, or spell-effect tuning from
// the literal .rodata key strings + the global config (g_buteMgr / g_mgrSettings).
// Class-split from Grunt.cpp purely to keep the TU small; matching-neutral. Only
// the OFFSETS + code bytes are load-bearing (names are placeholders).
//
// The per-direction entrance-cell records live at CGrunt+0x470, stride 0x68, two
// CString fields each (+0 and +4). They predate the named m_474 member, so they are
// reached by raw offset (the documented naming-independent-codegen exception).
#include <Gruntz/BattlezData.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/AniElement.h>
#include <rva.h>
#include <string.h>
#include <Bute/ButeMgr.h> // CButeMgr g_buteMgr (GetIntDef / GetDwordDef)
#include <Globals.h>
// The former per-TU CDDrawBlitParam / CAniAdvanceCursor / CDDrawSubMgrLeaf / CGruntSprite
// / CGruntAnimPlayer facet views are folded onto the real classes: the animation player's
// name/sprite/geometry setters are now methods of CEntranceAnimPlayer / CHudSprite
// (<Gruntz/Grunt.h>), the geometry sub-player setter is CEntranceAnimSub::SetGeometry, and
// the death-pose lookup is CEntranceSpriteMgr::LookupValue_06b2a0 - reached directly.

extern CButeMgr g_buteMgr; // ?g_buteMgr@@3VCButeMgr@@A

// The wingz-duration FP constants (reloc-masked .rodata doubles).

// The global running game clock (reloc-masked).
extern "C" u32 g_645588;
// The global manager pointer (the object at *0x64556c; reloc-masked).
extern CGameRegistry* g_pGameRegistry;
// The scratch CString[] the ScratchResolve reject paths tear down (reloc-masked).
extern CAnimScratchString* g_animScratch;
extern i32 g_animScratchCount;

static void GruntScratchTeardown() {
    CAnimScratchString* slot = g_animScratch;
    i32 cnt = g_animScratchCount;
    while (cnt != 0) {
        if (slot != 0) {
            ((CString*)slot)->~CString();
        }
        slot++;
        cnt--;
    }
}

// LoadWingzGruntSprites @0x68880 was re-homed to GruntEntranceMove.cpp and
// LoadGruntAbilityTuning @0x57100 to GruntCombat.cpp (wave3-I): their retail
// bodies + private .data cells sit inside the 0x67850 / 0x56f80 grunt objs.

// ---------------------------------------------------------------------------
// CGrunt::LoadGruntDeathAnimations(int deathType, int a2)   @0x60150   (ret 8)
// The grunt death dispatch: tear down the running anim state + retire the 7 HUD
// stat sprites, latch a fresh "C" death anim-set node, then switch on the death
// type (0..0xf, table @0x460ee0) to resolve + apply the matching GRUNTZ_DEATHZ_*
// sprite set, fire the on-screen death cue, and run the shared finalize/tail.
//
// The per-direction tile-attribute (n/t = 0x6e/0x74) splits FALL vs QUICKFALL.
// The g->m_7c sub-object the early arrival-notify drives.

// The death anim-set key (DAT_0060cc90) + the per-type GRUNTZ_DEATHZ_* keys.
static const char s_dAnimKeyC[] = "C";
static const char s_DEATHZ_SQUASH[] = "GRUNTZ_DEATHZ_SQUASH";
static const char s_DEATHZ_SINK[] = "GRUNTZ_DEATHZ_SINK";
static const char s_DEATHZ_HOLE[] = "GRUNTZ_DEATHZ_HOLE";
static const char s_DEATHZ_SHATTER[] = "GRUNTZ_DEATHZ_SHATTER";
static const char s_DEATHZ_FREEZE[] = "GRUNTZ_DEATHZ_FREEZE";
static const char s_DEATHZ_BURN[] = "GRUNTZ_DEATHZ_BURN";
static const char s_DEATHZ_QUICKFALL[] = "GRUNTZ_DEATHZ_QUICKFALL";
static const char s_DEATHZ_FALL[] = "GRUNTZ_DEATHZ_FALL";
static const char s_DEATHZ_FALL2[] = "GRUNTZ_DEATHZ_FALL2";
static const char s_DEATHZ_QUICKFALL2[] = "GRUNTZ_DEATHZ_QUICKFALL2";
static const char s_DEATHZ_ELECTROCUTE[] = "GRUNTZ_DEATHZ_ELECTROCUTE";
static const char s_DEATHZ_MELT[] = "GRUNTZ_DEATHZ_MELT";
static const char s_DEATHZ_KAROKE[] = "GRUNTZ_DEATHZ_KAROKE";
static const char s_DEATHZ_EXPLODE[] = "GRUNTZ_DEATHZ_EXPLODE";
static const char s_EXITZ_DRAIN[] = "GRUNTZ_EXITZ_DRAIN";
static const char s_dEXITZ[] = "GRUNTZ_EXITZ";
static const char s_dExitKeyB[] = "B";
static const char s_NORMALGRUNT_DEATH[] = "GRUNTZ_NORMALGRUNT_DEATH";

// The grunt death type LoadGruntDeathAnimations dispatches on (deathType, 0..0xf);
// each name is confirmed by its case's GRUNTZ_DEATHZ_* sprite key above. deathType 1
// (normal) + 13 fall through to the default NORMALGRUNT_DEATH path. Same immediates as
// the bare labels -> naming is matching-neutral.
enum GruntDeathType {
    DEATH_DROP = 0,        // entrance-drop; no death sprite (NotifyEntranceDrop)
    DEATH_NORMAL = 1,      // default NORMALGRUNT_DEATH (also the pathA re-fire value)
    DEATH_SQUASH = 2,      // GRUNTZ_DEATHZ_SQUASH
    DEATH_HOLE = 3,        // GRUNTZ_DEATHZ_HOLE
    DEATH_SINK = 4,        // GRUNTZ_DEATHZ_SINK
    DEATH_MELT = 5,        // GRUNTZ_DEATHZ_MELT
    DEATH_SHATTER = 6,     // GRUNTZ_DEATHZ_SHATTER (FREEZE anim)
    DEATH_BURN = 7,        // GRUNTZ_DEATHZ_BURN
    DEATH_FALL = 8,        // GRUNTZ_DEATHZ_FALL / QUICKFALL by tile attr
    DEATH_ELECTROCUTE = 9, // GRUNTZ_DEATHZ_ELECTROCUTE
    DEATH_KAROKE = 10,     // GRUNTZ_DEATHZ_KAROKE
    DEATH_EXPLODE = 11,    // GRUNTZ_DEATHZ_EXPLODE
    DEATH_DRAIN = 12,      // GRUNTZ_EXITZ_DRAIN
    DEATH_FALL2 = 14,      // GRUNTZ_DEATHZ_FALL2 / QUICKFALL2 by tile attr
    DEATH_QUICKFALL = 15,  // GRUNTZ_DEATHZ_QUICKFALL (FALL anim)
};

// Resolve the active-anim descriptor's first-element frame number.
#define DEATH_FRAME()                                                                              \
    (m_154->m_1b4->m_records.m_nSize > 0 ? ((i32*)*m_154->m_1b4->m_records.m_pData)[0x14 / 4]      \
                                         : ((i32*)0)[0x14 / 4])

// Fire the on-screen death cue (CueA) when the grunt point is visible.
#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGameRegistry* _g = g_pGameRegistry;                                                       \
        if (GruntPointVisible(_g->m_world->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {           \
            _g->m_cueSink->CueA(this, (tag), -1, 0, -1, -1);                                       \
        }                                                                                          \
    } while (0)

// @early-stop
// switch tail cross-jump wall (~big body): the prologue (anim-state teardown, the 7
// HUD-sprite retire chain, the powered-up reset gate, the CommitStruckTile, the "C"
// anim-set re-latch, the m_154/m_10 dirty stamps, the arrival-notify gate) and every
// one of the 16 death-type cases (LookupValue / m_10map.Lookup resolve, SetGeometryEx
// / m_1a0.SetGeometry, the GameApplyLookupSprite first-frame stamp, the FALL/QUICKFALL
// tile-attribute split, the on-screen CueA/CueSpawn cues, the NORMALGRUNT_DEATH + m_38c
// finalize tail) are byte-faithful in shape/offsets/symbols/constants. Residue: retail
// cross-jumps the shared cue-emit (0x60d0c), entrance-drop finalize (0x60d15) and
// m_170==1 geometry path (0x60c0a) into the middle of the case body (between case 11 and
// case 12) - structured C++ places them after the switch, permuting the block layout +
// the ebx/edi/ebp zero/0x10000/0x20000 register pins across the 16 near-identical cases.
// Source-invariant (the documented switch-tail-merge + register-pin wall); deferred to
// the final sweep.
RVA(0x00060150, 0xd90)
i32 CGrunt::LoadGruntDeathAnimations(i32 deathType, i32 a2) {
    if (m_deathAnimStarted != 0) {
        return 0;
    }

    StepAnimDispatchB(); // 0x6a6d0
    ClearSubA();         // 0x57c10
    ClearSubB();         // 0x57ce0

    m_10->m_40 &= ~8;
    m_deathAnimStarted = 1;
    m_health = 0;
    m_entranceCommitted = 0;

    if (m_healthSprite) {
        m_healthSprite->m_8 |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite) {
        m_staminaSprite->m_8 |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_8 |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_8 |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_8 |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite) {
        m_powerupSprite->m_8 |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite) {
        m_selectedSprite->m_8 |= 0x10000;
        m_selectedSprite = 0;
    }

    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0); // 0x62e10
    }
    m_tileMgr->CommitStruckTile(m_tileOwnerHi, m_tileOwnerLo, 1); // 0x78260

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_dAnimKeyC);

    m_154->m_8 |= 1;
    if (m_10->m_74 != 0x15f90) {
        m_10->m_74 = 0x15f90;
        m_10->m_8 |= 0x20000;
    }

    if (a2 != -1) {
        m_370 = a2;
        (*(CBattlezData**)((char*)g_pGameRegistry + 0x7c))->BumpWin(a2, m_tileOwnerHi); // 0xfcc50
    }

    switch (deathType) {
        case DEATH_SQUASH: // GRUNTZ_DEATHZ_SQUASH
            if (m_entranceReason == 1) {
                m_prevEntranceDesc = m_154->m_1b4;
                m_154->ApplyGeometryDirect(m_poseDeath, 0);
                goto pathA;
            }
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_SQUASH);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_SQUASH, DEATH_FRAME());
            DEATH_CUE(0x35b);
            goto finalize;

        case DEATH_DROP:
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            m_154->m_8 |= 0x10000;
            goto tail;

        case DEATH_SINK: // GRUNTZ_DEATHZ_SINK
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_SINK);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_SINK, DEATH_FRAME());
            DEATH_CUE(0x35a);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;

        case DEATH_HOLE: // GRUNTZ_DEATHZ_HOLE
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_HOLE);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_HOLE, DEATH_FRAME());
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_SHATTER: // GRUNTZ_DEATHZ_SHATTER (apply FREEZE)
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_SHATTER);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_FREEZE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;

        case DEATH_BURN: // GRUNTZ_DEATHZ_BURN
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_BURN);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_BURN, DEATH_FRAME());
            DEATH_CUE(0x352);
            goto finalize;

        case DEATH_QUICKFALL: // GRUNTZ_DEATHZ_QUICKFALL (apply FALL), snap to tile center
            m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
            m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_QUICKFALL);
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_FALL, DEATH_FRAME());
            if (m_10->m_74 != -1) {
                m_10->m_74 = -1;
                m_10->m_8 |= 0x20000;
            }
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_FALL: { // FALL / QUICKFALL by tile attribute
            CTileGrid* grid = g_pGameRegistry->m_tileGrid;
            i32 attr = ((i32*)grid->m_8[m_10->m_60 >> 5])[(m_10->m_5c >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_QUICKFALL);
                tag = 0x357;
                if (m_10->m_74 != -1) {
                    m_10->m_74 = -1;
                    m_10->m_8 |= 0x20000;
                }
                m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
                m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            } else {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_FALL);
            }
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_FALL2: { // FALL2 / QUICKFALL2 by tile attribute
            CTileGrid* grid = g_pGameRegistry->m_tileGrid;
            i32 attr = ((i32*)grid->m_8[m_10->m_60 >> 5])[(m_10->m_5c >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue_06b2a0(s_DEATHZ_QUICKFALL2);
                tag = 0x357;
                if (m_10->m_74 != -1) {
                    m_10->m_74 = -1;
                    m_10->m_8 |= 0x20000;
                }
                m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
                m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            } else {
                CSprite* out = 0;
                ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)
                    ->Lookup(s_DEATHZ_FALL2, (CObject*&)out);
                m_poseDeath = (i32)out;
            }
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_ELECTROCUTE: { // GRUNTZ_DEATHZ_ELECTROCUTE
            CSprite* out = 0;
            ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)
                ->Lookup(s_DEATHZ_ELECTROCUTE, (CObject*&)out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_ELECTROCUTE, DEATH_FRAME());
            DEATH_CUE(0x353);
            goto finalize;
        }

        case DEATH_MELT: {     // GRUNTZ_DEATHZ_MELT
            ApplySetState1(1); // 0x4322
            CSprite* out = 0;
            ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)->Lookup(s_DEATHZ_MELT, (CObject*&)out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_MELT, DEATH_FRAME());
            DEATH_CUE(0x359);
            goto finalize;
        }

        case DEATH_KAROKE: { // GRUNTZ_DEATHZ_KAROKE
            CSprite* out = 0;
            ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)->Lookup(s_DEATHZ_KAROKE, (CObject*&)out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->ApplyGeometryDirect(m_poseDeath, 0);
            m_154->CacheFrame(s_DEATHZ_KAROKE, DEATH_FRAME());
            DEATH_CUE(0x358);
            goto tail;
        }

        case DEATH_EXPLODE: { // GRUNTZ_DEATHZ_EXPLODE
            if (m_entranceReason == 1) {
                m_prevEntranceDesc = m_154->m_1b4;
                m_154->m_1a0.SetGeometry(m_poseDeath);
                goto pathA;
            }
            CSprite* out = 0;
            ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)->Lookup(s_DEATHZ_EXPLODE, (CObject*&)out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->CacheFrame(s_DEATHZ_EXPLODE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;
        }

        case DEATH_DRAIN: { // GRUNTZ_EXITZ_DRAIN (apply EXITZ), re-latch "B"
            CSprite* out = 0;
            ((CMapStringToOb*)&m_154->m_c->m_2c->m_10map)->Lookup(s_EXITZ_DRAIN, (CObject*&)out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->CacheFrame(s_dEXITZ, DEATH_FRAME());
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(s_dExitKeyB);
            goto tail;
        }

        default:
            m_prevEntranceDesc = m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->CacheFirstFrame(*(char**)&m_44c);
            {
                CGameRegistry* g = g_pGameRegistry;
                CCueRect* r = (CCueRect*)((char*)g->m_world->m_24->m_5c + 0x40);
                i32 x = m_10->m_5c;
                i32 y = m_10->m_60;
                if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
                    g->m_cueSink->CueSpawn(this, 3, -1, -1, -1);
                }
            }
            // block A: NORMALGRUNT_DEATH override
            if (m_entranceReason == 0x14 && g_pGameRegistry->m_134 != 1) {
                m_154->ApplyLookupGeometry(s_NORMALGRUNT_DEATH, 0);
                m_154->CacheFirstFrame(s_NORMALGRUNT_DEATH);
            }
            goto tail;
    }

pathA:
    m_154->CacheFirstFrame(*(char**)&m_44c);
    {
        CGameRegistry* g = g_pGameRegistry;
        if (GruntPointVisible(g->m_world->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
            g->m_cueSink->CueSpawn(this, 3, -1, -1, -1);
        }
    }
    deathType = DEATH_NORMAL;
    goto tail;

finalize:
    m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);

tail:
    // block B: m_38c finalize cue
    if (m_entranceReason == 0x14 && g_pGameRegistry->m_134 != 1) {
        m_tileMgr->NotifyDeathTile(m_10->m_5c, m_10->m_60, m_38c);
    }
    if (m_arrivalState == 0xd) {
        TryPowerupAtTile();
    }
    m_gruntKind = 0;
    m_deathType = deathType;
    return 0;
}

#undef DEATH_FRAME
#undef DEATH_CUE

SIZE_UNKNOWN(CDeathReg7c);
SIZE_UNKNOWN(CGruntSndRes);
SIZE_UNKNOWN(CGruntSndResMgr);
SIZE_UNKNOWN(CGruntSndSlot);
