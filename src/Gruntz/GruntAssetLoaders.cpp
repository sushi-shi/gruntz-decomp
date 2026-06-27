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
#include <Gruntz/Grunt.h>
#include <rva.h>
#include <string.h>
#include <Bute/ButeMgr.h> // CButeMgr g_buteMgr (GetIntDef / GetDwordDef)

extern CButeMgr g_buteMgr; // ?g_buteMgr@@3VCButeMgr@@A

// The wingz-duration FP constants (reloc-masked .rodata doubles).
DATA(0x001e9a48)
extern double g_wingzScale; // 0x5e9a48
DATA(0x001e9a50)
extern double g_wingzBias; // 0x5e9a50

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
            slot->Release();
        }
        slot++;
        cnt--;
    }
}

// The per-direction sprite-name key strings (literal .rodata; reloc-masked).
static const char s_NW_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_ITEM";
static const char s_N_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTH_ITEM";
static const char s_NE_ITEM[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_ITEM";
static const char s_W_ITEM[] = "GRUNTZ_WINGZGRUNT_WEST_ITEM";
static const char s_E_ITEM[] = "GRUNTZ_WINGZGRUNT_EAST_ITEM";
static const char s_SW_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_ITEM";
static const char s_S_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTH_ITEM";
static const char s_SE_ITEM[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_ITEM";
static const char s_NW_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_WALK";
static const char s_N_WALK[] = "GRUNTZ_WINGZGRUNT_NORTH_WALK";
static const char s_NE_WALK[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_WALK";
static const char s_W_WALK[] = "GRUNTZ_WINGZGRUNT_WEST_WALK";
static const char s_E_WALK[] = "GRUNTZ_WINGZGRUNT_EAST_WALK";
static const char s_SW_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_WALK";
static const char s_S_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTH_WALK";
static const char s_SE_WALK[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_WALK";
static const char s_NW_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHWEST_IDLE";
static const char s_N_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTH_IDLE";
static const char s_NE_IDLE[] = "GRUNTZ_WINGZGRUNT_NORTHEAST_IDLE";
static const char s_W_IDLE[] = "GRUNTZ_WINGZGRUNT_WEST_IDLE";
static const char s_E_IDLE[] = "GRUNTZ_WINGZGRUNT_EAST_IDLE";
static const char s_SW_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHWEST_IDLE";
static const char s_S_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTH_IDLE";
static const char s_SE_IDLE[] = "GRUNTZ_WINGZGRUNT_SOUTHEAST_IDLE";
static const char s_WG_ITEM[] = "GRUNTZ_WINGZGRUNT_ITEM";
static const char s_WG_WALK[] = "GRUNTZ_WINGZGRUNT_WALK";
static const char s_WG_IDLE1[] = "GRUNTZ_WINGZGRUNT_IDLE1";
static const char s_WG_IDLE2[] = "GRUNTZ_WINGZGRUNT_IDLE2";
static const char s_WG_IDLE3[] = "GRUNTZ_WINGZGRUNT_IDLE3";
static const char s_WG_IDLE4[] = "GRUNTZ_WINGZGRUNT_IDLE4";
static const char s_WG_IDLE5[] = "GRUNTZ_WINGZGRUNT_IDLE5";

// The entrance-cell record fields (raw offset; +0x470 = field0, +0x474 = field1).
#define CELL_F0(k) (*(CString*)((char*)this + 0x470 + (k)*0x68))
#define CELL_F1(k) (*(CString*)((char*)this + 0x474 + (k)*0x68))
#define WLOOKUP(key) (m_154->m_c->m_2c->m_10map.Lookup((key), &_out))

// ---------------------------------------------------------------------------
// CGrunt::LoadWingzGruntSprites(int enable)   @0x68880   (ret 4)
// When the grunt's wingz timer is active (enable != 0) it stamps the flying ITEM
// sprite name into every direction cell, sets up the wingz-duration timer and the
// pose-index lookups against the flying set, then fires the on-screen spawn cue.
// When disabled it retires the wingz-time HUD sprite and re-stamps the normal
// WALK/IDLE walking set. Both paths finish by re-stamping the current entrance-cell
// frame keyed by the active anim type code ("D" = walk pose, "A" = idle pose).
// @early-stop
// out-param zero-init scheduling wall (docs/patterns/outparam-zeroinit-scheduling.md):
// every cell operator=, the 8 Lookup blocks, the cue, the ScratchResolve/strcmp tail
// and the entrance-cell frame re-stamp are byte-correct in shape/offsets/symbols/CFG.
// Residue (compounded over 8 lookups): retail reuses the consumed `enable` arg slot as
// the single lookup `out` local (esp+0x20, 21 refs) under a `sub esp,0xc` frame that
// also spills a dead `reason=m_entranceCell[2]`, and SINKS each lookup's `out=0` store
// past the &out/key pushes; cl allocates a fresh out slot (esp+0x14, no frame) and
// hoists the zero-init. Source-invariant (the documented Lookup-family scheduling
// coin-flip); deferred to the final sweep. ~75%.
RVA(0x00068880, 0x67c)
i32 CGrunt::LoadWingzGruntSprites(i32 enable) {
    CSprite* _out;
    if (enable != 0) {
        m_wingzEnabled = 1;
        m_898 = (i32)((double)m_wingzTime * g_wingzScale - g_wingzBias);
        m_89c = 0;
        m_890 = (i32)g_645588;
        m_894 = 0;
        CreateWingzTimeSprite();

        CELL_F1(0) = s_NW_ITEM;
        CELL_F1(1) = s_N_ITEM;
        CELL_F1(2) = s_NE_ITEM;
        CELL_F1(3) = s_W_ITEM;
        CELL_F1(4) = s_N_ITEM;
        CELL_F1(5) = s_E_ITEM;
        CELL_F1(6) = s_SW_ITEM;
        CELL_F1(7) = s_S_ITEM;
        CELL_F1(8) = s_SE_ITEM;
        CELL_F0(0) = s_NW_ITEM;
        CELL_F0(1) = s_N_ITEM;
        CELL_F0(2) = s_NE_ITEM;
        CELL_F0(3) = s_W_ITEM;
        CELL_F0(4) = s_N_ITEM;
        CELL_F0(5) = s_E_ITEM;
        CELL_F0(6) = s_SW_ITEM;
        CELL_F0(7) = s_S_ITEM;
        CELL_F0(8) = s_SE_ITEM;

        _out = 0;
        WLOOKUP(s_WG_ITEM);
        m_poseWalk = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_ITEM);
        m_poseIdle[2] = 0;
        m_poseIdle[0] = (i32)_out;
        m_poseIdle[1] = (i32)_out;
        m_poseIdle4 = 0;
        m_poseIdle5 = 0;

        CGameRegistry* g = g_pGameRegistry;
        i32 y = m_10->m_60;
        i32 x = m_10->m_5c;
        CCueRect* r = (CCueRect*)((char*)g->m_30->m_24->m_5c + 0x40);
        if (x < r->right && x >= r->left && y < r->bottom && y >= r->top) {
            g->m_60->CueSpawn(this, 8, -1, -1, -1);
        }
    } else {
        m_wingzEnabled = 0;
        m_898 = 0;
        m_89c = 0;
        if (m_wingzTimeSprite != 0) {
            m_wingzTimeSprite->m_8 |= 0x10000;
            m_wingzTimeSprite = 0;
        }

        CELL_F0(0) = s_NW_WALK;
        CELL_F0(1) = s_N_WALK;
        CELL_F0(2) = s_NE_WALK;
        CELL_F0(3) = s_W_WALK;
        CELL_F0(4) = s_N_WALK;
        CELL_F0(5) = s_E_WALK;
        CELL_F0(6) = s_SW_WALK;
        CELL_F0(7) = s_S_WALK;
        CELL_F0(8) = s_SE_WALK;
        CELL_F1(0) = s_NW_IDLE;
        CELL_F1(1) = s_N_IDLE;
        CELL_F1(2) = s_NE_IDLE;
        CELL_F1(3) = s_W_IDLE;
        CELL_F1(4) = s_N_IDLE;
        CELL_F1(5) = s_E_IDLE;
        CELL_F1(6) = s_SW_IDLE;
        CELL_F1(7) = s_S_IDLE;
        CELL_F1(8) = s_SE_IDLE;

        _out = 0;
        WLOOKUP(s_WG_WALK);
        m_poseWalk = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_IDLE1);
        m_poseIdle[0] = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_IDLE2);
        m_poseIdle[1] = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_IDLE3);
        m_poseIdle[2] = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_IDLE4);
        m_poseIdle4 = (i32)_out;
        _out = 0;
        WLOOKUP(s_WG_IDLE5);
        m_poseIdle5 = (i32)_out;
    }

    // Re-stamp the current entrance-cell frame keyed by the active anim type.
    CAnimNameRecord* rec = g_animNameResolver.ScratchResolve(m_14->m_1c);
    GruntScratchTeardown();
    if (strcmp(rec->m_name, g_codeD) == 0) {
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseWalk);
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];
        i32 idx = 3 * m_entranceCell[0] + m_entranceCell[1];
        char* buf = GruntStrGetBuffer((char*)this + 0x470 + idx * 0x68, 0);
        m_154->GameApplyLookupSprite(buf, frame);
        return 1;
    }

    CAnimNameRecord* rec2 = g_animNameResolver.ScratchResolve(m_14->m_1c);
    GruntScratchTeardown();
    if (strcmp(rec2->m_name, g_codeA) == 0) {
        m_prevEntranceDesc = (i32)m_154->m_1b4;
        m_154->m_1a0.SetGeometry(m_poseIdle[0]);
        CEntranceAnimDescColl* desc = m_154->m_1b4;
        i32* elem = desc->m_10 > 0 ? *desc->m_c : 0;
        i32 frame = elem[0x14 / 4];
        i32 idx = 3 * m_entranceCell[0] + m_entranceCell[1];
        char* buf = GruntStrGetBuffer((char*)this + 0x474 + idx * 0x68, 0);
        m_154->GameApplyLookupSprite(buf, frame);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGrunt::LoadGruntAbilityTuning(int forced)   @0x57100   (ret 4)
// Fire the grunt's spell-ability effect for the given (or randomly-rolled)
// ability index: play the GAME_ATTACK launch sound (when the slot is armed),
// then dispatch on the index to spawn the matching LightFx flash + area tuning
// cue (FreezeRadius/HealthRadius/RessurectionRadius/ToyzRadius/TeleportRadius) or
// the 4-direction rolling-ball sprites.
//
// The ability-sound resource chain (m_158 -> m_c -> m_28): the GAME_ATTACK map is
// at +0x10, the armed-flag at +0x30.
struct CGruntSndSlot {
    char m_pad0[0x30];
    i32 m_30; // +0x30  armed flag (0 = fire the sound)
};
struct CGruntSndRes {
    char m_pad0[0x28];
    CGruntSndSlot* m_28; // +0x28
};
struct CGruntSndResMgr {
    char m_pad0[0xc];
    CGruntSndRes* m_c; // +0x0c
};

// The launch-sound cue tag (reloc-masked global) + the __stdcall sound player.
DATA(0x0021ab24)
extern i32 g_sndCueTag; // ?g_sndCueTag@@3HA
void __stdcall PlayAttackSound(i32 tag, i32 a, i32 b, i32 c); // thunk 0x25fe -> 0x1f940

// The spell-effect key strings (CreateSprite/Activate/ApplyName take const char*).
static const char s_GAME_ATTACK[] = "GAME_ATTACK";
static const char s_GAME_FLASH[] = "GAME_FLASH";
static const char s_GAME_LIGHTING_FLASH[] = "GAME_LIGHTING_FLASH";
static const char s_LightFx[] = "LightFx";
static const char s_RollingBall[] = "RollingBall";
static const char s_LEVEL_ROLLINGBALL_NORTH[] = "LEVEL_ROLLINGBALL_NORTH";
static const char s_LEVEL_ROLLINGBALL_EAST[] = "LEVEL_ROLLINGBALL_EAST";
static const char s_LEVEL_ROLLINGBALL_SOUTH[] = "LEVEL_ROLLINGBALL_SOUTH";
static const char s_LEVEL_ROLLINGBALL_WEST[] = "LEVEL_ROLLINGBALL_WEST";
// The bute config tag/keys (GetIntDef/GetDwordDef take char*).
static char s_Spellz[] = "Spellz";
static char s_FreezeRadius[] = "FreezeRadius";
static char s_HealthRadius[] = "HealthRadius";
static char s_RessurectionRadius[] = "RessurectionRadius";
static char s_ToyzRadius[] = "ToyzRadius";
static char s_TeleportRadius[] = "TeleportRadius";
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
static char s_RollingBallzTime[] = "RollingBallzTime";

// @early-stop
// Create*-sprite register coin-flip wall (~90.5%): the prologue (incl. the ebp=0 +
// 4th-saved-reg pin recovered via the rolling-ball m_7c temps), the random-index pick,
// the GAME_ATTACK sound gate, every switch case (CreateSprite, the init vtable call,
// Activate, the GetIntDef-into-CombatCue/ResurrectCue area cue) and the cross-case
// tail-merge are byte-correct in shape/offsets/symbols/CFG. Residue is the documented
// Create*-sprite scheduling coin-flip (which callee-saved reg holds m_180/m_17c/g per
// case; the same wall the 7 CGrunt Create* carry at 99% each, compounded over the 10
// CreateSprite calls) + a 1-instr `cmp edi,ebp` operand-order flip. Deferred to the
// final sweep.
RVA(0x00057100, 0x577)
i32 CGrunt::LoadGruntAbilityTuning(i32 forced) {
    i32 idx = forced;
    if (forced == 0) {
        i32 m = 3;
        if (g_pGameRegistry->m_134 != 1) {
            m = 6;
        }
        if (m == 0) {
            idx = GruntRand() & 1;
        } else {
            idx = GruntRand() % m + 1;
        }
    }

    CGruntSndSlot* slot = m_158->m_c->m_28;
    if (slot->m_30 == 0) {
        GruntSoundEntry* sout = 0;
        ((GruntSoundMap*)((char*)slot + 0x10))->Lookup(s_GAME_ATTACK, &sout);
        if (sout != 0) {
            PlayAttackSound(g_sndCueTag, 0, 0, 0);
        }
    }

    switch (idx) {
        case 1: { // freeze
            CHudSprite* spr = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY, 0xf4240, s_LightFx, 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 9, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX, m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_FreezeRadius, 8), 4, -1);
        }
        case 2: { // health
            CHudSprite* spr = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY, 0xf4240, s_LightFx, 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 2, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX, m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_HealthRadius, 8), 3, -1);
        }
        case 3: { // resurrection
            CHudSprite* spr = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY, 0xf4240, s_LightFx, 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 8, 1);
            return m_tileMgr->ResurrectCue(
                m_lastTilePxX, m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_RessurectionRadius, 8));
        }
        case 4: { // toyz
            CHudSprite* spr = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY, 0xf4240, s_LightFx, 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 7, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX, m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_ToyzRadius, 8), 5, -1);
        }
        case 5: { // teleport
            CHudSprite* spr = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY, 0xf4240, s_LightFx, 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 3, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX, m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_TeleportRadius, 8), 2, -1);
        }
        case 6: { // rolling ball (4 directions)
            CHudSprite* n = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY - 0x20, 0, s_RollingBall, 0x40003);
            n->ApplyName(s_LEVEL_ROLLINGBALL_NORTH);
            CSpriteInner* ni = n->m_7c;
            ni->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            n->m_124 = 0;
            n->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* e = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX + 0x20, m_lastTilePxY, 0, s_RollingBall, 0x40003);
            e->ApplyName(s_LEVEL_ROLLINGBALL_EAST);
            CSpriteInner* ei = e->m_7c;
            ei->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            e->m_124 = 0;
            e->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* s = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX, m_lastTilePxY + 0x20, 0, s_RollingBall, 0x40003);
            s->ApplyName(s_LEVEL_ROLLINGBALL_SOUTH);
            CSpriteInner* si = s->m_7c;
            si->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            s->m_124 = 0;
            s->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* w = g_pGameRegistry->m_30->m_8->CreateSprite(
                0, m_lastTilePxX - 0x20, m_lastTilePxY, 0, s_RollingBall, 0x40003);
            w->ApplyName(s_LEVEL_ROLLINGBALL_WEST);
            CSpriteInner* wi = w->m_7c;
            wi->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            w->m_124 = 0;
            w->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);
            return 1;
        }
        default:
            return 0;
    }
}
