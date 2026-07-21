#include <Gruntz/GruntSpawnConfig.h> // the +0x60 cue-sink/spawn-config object (complete type for the cue calls)
#include <Bute/ButeTree.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TypeKeyColl.h> // CButeTree::Find - g_buteTree @0x6bf620
#include <Gruntz/BattlezData.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_0c world root (m_animRegistry hop)
#include <DDrawMgr/DDrawSubMgrLeaf.h> // m_0c->m_animRegistry (the anim-key catalog)
#include <Gruntz/TriggerMgr.h>        // the ONE CTriggerMgr
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)
#include <Gruntz/AniElement.h>
#include <rva.h>
#include <string.h>
#include <Bute/ButeMgr.h> // CButeMgr g_buteMgr (GetIntDef / GetDwordDef)
#include <Globals.h>

static void GruntScratchTeardown() {
    CAnimScratchString* slot = (reinterpret_cast<CAnimScratchString*>(g_typeColl.m_alloc));
    i32 cnt = g_typeColl.m_grown;
    while (cnt != 0) {
        if (slot != 0) {
            (reinterpret_cast<CString*>(slot))->~CString();
        }
        slot++;
        cnt--;
    }
}

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

#define DEATH_FRAME()                                                                              \
    (m_38->m_1a0.m_14->m_records.GetSize() > 0                                                     \
         ? (reinterpret_cast<i32*>(m_38->m_1a0.m_14->m_records.GetAt(0)))[0x14 / 4]                \
         : ((i32*)0)[0x14 / 4])

#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                             \
        if (GruntPointVisible(                                                                     \
                reinterpret_cast<i32>(&_g->m_world->m_level->m_mainPlane->m_originX),                              \
                m_object->m_screenX,                                                                   \
                m_object->m_screenY                                                                    \
            )) {                                                                                   \
            _g->m_cueSink->CueA(this, (tag), -1, 0, -1, -1);                                       \
        }                                                                                          \
    } while (0)

// @early-stop
// switch tail cross-jump wall (~big body): the prologue (anim-state teardown, the 7
// HUD-sprite retire chain, the powered-up reset gate, the CommitStruckTile, the "C"
// anim-set re-latch, the m_154/m_object dirty stamps, the arrival-notify gate) and every
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

    m_object->m_stateFlags &= ~8;
    m_deathAnimStarted = 1;
    m_health = 0;
    m_entranceCommitted = 0;

    if (m_healthSprite) {
        m_healthSprite->m_flags |= 0x10000;
        m_healthSprite = 0;
    }
    if (m_staminaSprite) {
        m_staminaSprite->m_flags |= 0x10000;
        m_staminaSprite = 0;
    }
    if (m_toySprite) {
        m_toySprite->m_flags |= 0x10000;
        m_toySprite = 0;
    }
    if (m_toyTimeSprite) {
        m_toyTimeSprite->m_flags |= 0x10000;
        m_toyTimeSprite = 0;
    }
    if (m_wingzTimeSprite) {
        m_wingzTimeSprite->m_flags |= 0x10000;
        m_wingzTimeSprite = 0;
    }
    if (m_powerupSprite) {
        m_powerupSprite->m_flags |= 0x10000;
        m_powerupSprite = 0;
    }
    if (m_selectedSprite) {
        m_selectedSprite->m_flags |= 0x10000;
        m_selectedSprite = 0;
    }

    if (m_poweredUp != 0 && m_neighborValid == 0) {
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0); // 0x62e10
    }
    m_tileMgr->RemoveCellRecord(m_tileOwnerHi, m_tileOwnerLo, 1); // 0x78260

    m_prevAnimSetNode = m_14->m_1c;
    m_14->m_1c = static_cast<void*>(g_buteTree.Find(s_dAnimKeyC));

    m_38->m_flags |= 1;
    if (m_object->m_sortKey != 0x15f90) {
        m_object->m_sortKey = 0x15f90;
        m_object->m_flags |= 0x20000;
    }

    if (a2 != -1) {
        m_370 = a2;
        g_gameReg->m_scoreHud->BumpWin(a2, m_tileOwnerHi); // 0xfcc50 (+0x7c m_scoreHud)
    }

    switch (deathType) {
        case DEATH_SQUASH: // GRUNTZ_DEATHZ_SQUASH
            if (m_entranceReason == 1) {
                m_value = m_38->m_1a0.m_14;
                m_38->ApplyGeometryDirect(m_poseDeath, 0);
                goto pathA;
            }
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_SQUASH));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_SQUASH, DEATH_FRAME());
            DEATH_CUE(0x35b);
            goto finalize;

        case DEATH_DROP:
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            m_38->m_flags |= 0x10000;
            goto tail;

        case DEATH_SINK: // GRUNTZ_DEATHZ_SINK
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_SINK));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_SINK, DEATH_FRAME());
            DEATH_CUE(0x35a);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;

        case DEATH_HOLE: // GRUNTZ_DEATHZ_HOLE
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_HOLE));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_HOLE, DEATH_FRAME());
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_SHATTER: // GRUNTZ_DEATHZ_SHATTER (apply FREEZE)
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_SHATTER));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_FREEZE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;

        case DEATH_BURN: // GRUNTZ_DEATHZ_BURN
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_BURN));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_BURN, DEATH_FRAME());
            DEATH_CUE(0x352);
            goto finalize;

        case DEATH_QUICKFALL: // GRUNTZ_DEATHZ_QUICKFALL (apply FALL), snap to tile center
            m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
            m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            m_poseDeath =
                static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_QUICKFALL));
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            if (m_object->m_sortKey != -1) {
                m_object->m_sortKey = -1;
                m_object->m_flags |= 0x20000;
            }
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_FALL: { // FALL / QUICKFALL by tile attribute
            CTileGrid* grid = g_gameReg->m_tileGrid;
            i32 attr = ((grid->m_rowInts[m_object->m_screenY >> 5]))[(m_object->m_screenX >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(
                    s_DEATHZ_QUICKFALL
                ));
                tag = 0x357;
                if (m_object->m_sortKey != -1) {
                    m_object->m_sortKey = -1;
                    m_object->m_flags |= 0x20000;
                }
                m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
                m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            } else {
                m_poseDeath =
                    static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(s_DEATHZ_FALL));
            }
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_FALL2: { // FALL2 / QUICKFALL2 by tile attribute
            CTileGrid* grid = g_gameReg->m_tileGrid;
            i32 attr = ((grid->m_rowInts[m_object->m_screenY >> 5]))[(m_object->m_screenX >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = static_cast<CAniElement*>(m_38->OwnerMgr()->m_animRegistry->LookupValue_06b2a0(
                    s_DEATHZ_QUICKFALL2
                ));
                tag = 0x357;
                if (m_object->m_sortKey != -1) {
                    m_object->m_sortKey = -1;
                    m_object->m_flags |= 0x20000;
                }
                m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
                m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
            } else {
                void* out_ob = 0;
                m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_DEATHZ_FALL2, out_ob);
                m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement (the ANI-factory class)
            }
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_ELECTROCUTE: { // GRUNTZ_DEATHZ_ELECTROCUTE
            void* out_ob = 0;
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_DEATHZ_ELECTROCUTE, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_ELECTROCUTE, DEATH_FRAME());
            DEATH_CUE(0x353);
            goto finalize;
        }

        case DEATH_MELT: {     // GRUNTZ_DEATHZ_MELT
            ApplySetState1(1); // 0x4322
            void* out_ob = 0;
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_DEATHZ_MELT, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_MELT, DEATH_FRAME());
            DEATH_CUE(0x359);
            goto finalize;
        }

        case DEATH_KAROKE: { // GRUNTZ_DEATHZ_KAROKE
            void* out_ob = 0;
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_DEATHZ_KAROKE, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement
            m_value = m_38->m_1a0.m_14;
            m_38->ApplyGeometryDirect(m_poseDeath, 0);
            m_38->ApplyLookupSprite(s_DEATHZ_KAROKE, DEATH_FRAME());
            DEATH_CUE(0x358);
            goto tail;
        }

        case DEATH_EXPLODE: { // GRUNTZ_DEATHZ_EXPLODE
            if (m_entranceReason == 1) {
                m_value = m_38->m_1a0.m_14;
                m_38->m_1a0.Setup_15c2d0(m_poseDeath);
                goto pathA;
            }
            void* out_ob = 0;
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_DEATHZ_EXPLODE, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup_15c2d0(m_poseDeath);
            m_38->ApplyLookupSprite(s_DEATHZ_EXPLODE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;
        }

        case DEATH_DRAIN: { // GRUNTZ_EXITZ_DRAIN (apply EXITZ), re-latch "B"
            void* out_ob = 0;
            m_38->OwnerMgr()->m_animRegistry->m_10.Lookup(s_EXITZ_DRAIN, out_ob);
            m_poseDeath = static_cast<CAniElement*>(out_ob); // the ANIM registry's values ARE CAniElement
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup_15c2d0(m_poseDeath);
            m_38->ApplyLookupSprite(s_dEXITZ, DEATH_FRAME());
            m_prevAnimSetNode = m_14->m_1c;
            m_14->m_1c = static_cast<void*>(g_buteTree.Find(s_dExitKeyB));
            goto tail;
        }

        default:
            m_value = m_38->m_1a0.m_14;
            m_38->m_1a0.Setup_15c2d0(m_poseDeath);
            m_38->ApplyName(*reinterpret_cast<char**>(&m_44c));
            {
                CGruntzMgr* g = g_gameReg;
                CCueRect* r = reinterpret_cast<CCueRect*>(&g->m_world->m_level->m_mainPlane->m_originX);
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
                    g->m_cueSink->CueSpawn(this, 3, -1, -1, -1);
                }
            }
            // block A: NORMALGRUNT_DEATH override
            if (m_entranceReason == 0x14 && g_gameReg->m_134 != 1) {
                m_38->ApplyLookupGeometry(s_NORMALGRUNT_DEATH, 0);
                m_38->ApplyName(s_NORMALGRUNT_DEATH);
            }
            goto tail;
    }

pathA:
    m_38->ApplyName(*reinterpret_cast<char**>(&m_44c));
    {
        CGruntzMgr* g = g_gameReg;
        if (GruntPointVisible(
                reinterpret_cast<i32>(&g->m_world->m_level->m_mainPlane->m_originX),
                m_object->m_screenX,
                m_object->m_screenY
            )) {
            g->m_cueSink->CueSpawn(this, 3, -1, -1, -1);
        }
    }
    deathType = DEATH_NORMAL;
    goto tail;

finalize:
    m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);

tail:
    // block B: m_38c finalize cue
    if (m_entranceReason == 0x14 && g_gameReg->m_134 != 1) {
        SpawnTileFx(m_object->m_screenX, m_object->m_screenY, m_38c);
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
