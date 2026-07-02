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
#include <Globals.h>

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
#define CELL_F0(k) (*(CString*)((char*)this + 0x470 + (k) * 0x68))
#define CELL_F1(k) (*(CString*)((char*)this + 0x474 + (k) * 0x68))
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
extern i32 g_sndCueTag;                                       // ?g_sndCueTag@@3HA
void __stdcall PlayAttackSound(i32 tag, i32 a, i32 b, i32 c); // thunk 0x25fe -> 0x1f940

// The GAME_ATTACK sound cue tag (const char*). The other spell-effect key
// strings (CreateSprite/Activate/ApplyName args) are spelled as inline literals
// below so cl pools them into the shared read-only ??_C@ constants retail uses.
static const char s_GAME_ATTACK[] = "GAME_ATTACK";
// The bute config tag/keys (GetIntDef/GetDwordDef take char*).
static char s_Spellz[] = "Spellz";
static char s_FreezeRadius[] = "FreezeRadius";
static char s_HealthRadius[] = "HealthRadius";
static char s_RessurectionRadius[] = "RessurectionRadius";
static char s_ToyzRadius[] = "ToyzRadius";
static char s_TeleportRadius[] = "TeleportRadius";
static char s_RollingBallzSpeed[] = "RollingBallzSpeed";
static char s_RollingBallzTime[] = "RollingBallzTime";

// The random grunt spell/ability effect LoadGruntAbilityTuning dispatches on (idx);
// each name is confirmed by its case comment + its "Spellz" bute radius key. Same
// immediates as the bare labels -> naming is matching-neutral.
enum SpellzEffect {
    SPELLZ_FREEZE = 1,       // FreezeRadius
    SPELLZ_HEALTH = 2,       // HealthRadius
    SPELLZ_RESURRECTION = 3, // RessurectionRadius
    SPELLZ_TOYZ = 4,         // ToyzRadius
    SPELLZ_TELEPORT = 5,     // TeleportRadius
    SPELLZ_ROLLINGBALL = 6,  // RollingBallzSpeed/Time (spawns 4 directional ballz)
};

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
        case SPELLZ_FREEZE: { // freeze
            CHudSprite* spr =
                g_pGameRegistry->m_30->m_8
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 9, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_FreezeRadius, 8),
                4,
                -1
            );
        }
        case SPELLZ_HEALTH: { // health
            CHudSprite* spr =
                g_pGameRegistry->m_30->m_8
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 2, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_HealthRadius, 8),
                3,
                -1
            );
        }
        case SPELLZ_RESURRECTION: { // resurrection
            CHudSprite* spr =
                g_pGameRegistry->m_30->m_8
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 8, 1);
            return m_tileMgr->ResurrectCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_RessurectionRadius, 8)
            );
        }
        case SPELLZ_TOYZ: { // toyz
            CHudSprite* spr =
                g_pGameRegistry->m_30->m_8
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 7, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_ToyzRadius, 8),
                5,
                -1
            );
        }
        case SPELLZ_TELEPORT: { // teleport
            CHudSprite* spr =
                g_pGameRegistry->m_30->m_8
                    ->CreateSprite(0, m_lastTilePxX, m_lastTilePxY, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->m_init(spr);
            spr->m_7c->m_18->Activate("GAME_LIGHTING_FLASH", "GAME_FLASH", 3, 1);
            return m_tileMgr->CombatCue(
                m_lastTilePxX,
                m_lastTilePxY,
                g_buteMgr.GetIntDef(s_Spellz, s_TeleportRadius, 8),
                2,
                -1
            );
        }
        case SPELLZ_ROLLINGBALL: { // rolling ball (4 directions)
            CHudSprite* n = g_pGameRegistry->m_30->m_8->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY - 0x20,
                0,
                "RollingBall",
                0x40003
            );
            n->ApplyName("LEVEL_ROLLINGBALL_NORTH");
            CSpriteInner* ni = n->m_7c;
            ni->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            n->m_124 = 0;
            n->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* e = g_pGameRegistry->m_30->m_8->CreateSprite(
                0,
                m_lastTilePxX + 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            e->ApplyName("LEVEL_ROLLINGBALL_EAST");
            CSpriteInner* ei = e->m_7c;
            ei->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            e->m_124 = 0;
            e->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* s = g_pGameRegistry->m_30->m_8->CreateSprite(
                0,
                m_lastTilePxX,
                m_lastTilePxY + 0x20,
                0,
                "RollingBall",
                0x40003
            );
            s->ApplyName("LEVEL_ROLLINGBALL_SOUTH");
            CSpriteInner* si = s->m_7c;
            si->m_bc = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzSpeed, 0x3e8);
            s->m_124 = 0;
            s->m_118 = (i32)g_buteMgr.GetDwordDef(s_Spellz, s_RollingBallzTime, 0x3e8);

            CHudSprite* w = g_pGameRegistry->m_30->m_8->CreateSprite(
                0,
                m_lastTilePxX - 0x20,
                m_lastTilePxY,
                0,
                "RollingBall",
                0x40003
            );
            w->ApplyName("LEVEL_ROLLINGBALL_WEST");
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

// ---------------------------------------------------------------------------
// CGrunt::LoadGruntDeathAnimations(int deathType, int a2)   @0x60150   (ret 8)
// The grunt death dispatch: tear down the running anim state + retire the 7 HUD
// stat sprites, latch a fresh "C" death anim-set node, then switch on the death
// type (0..0xf, table @0x460ee0) to resolve + apply the matching GRUNTZ_DEATHZ_*
// sprite set, fire the on-screen death cue, and run the shared finalize/tail.
//
// The per-direction tile-attribute (n/t = 0x6e/0x74) splits FALL vs QUICKFALL.
// The g->m_7c sub-object the early arrival-notify drives.
struct CDeathReg7c {
    void Notify(i32 a, i32 b); // 0xfcc50 (__thiscall, 2 args)
};

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
    (m_154->m_1b4->m_10 > 0 ? (*m_154->m_1b4->m_c)[0x14 / 4] : ((i32*)0)[0x14 / 4])

// Fire the on-screen death cue (CueA) when the grunt point is visible.
#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGameRegistry* _g = g_pGameRegistry;                                                       \
        if (GruntPointVisible(_g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {              \
            _g->m_60->CueA(this, (tag), -1, 0, -1, -1);                                            \
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
    if (*(i32*)((char*)this + 0x368) != 0) {
        return 0;
    }

    StepAnimDispatchB(); // 0x6a6d0
    ClearSubA();         // 0x57c10
    ClearSubB();         // 0x57ce0

    m_10->m_40 &= ~8;
    *(i32*)((char*)this + 0x368) = 1;
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
        *(i32*)((char*)this + 0x218) = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        Stub_062e10(1, 0, 0); // 0x62e10
    }
    m_tileMgr->CommitStruckTile(m_tileOwnerHi, m_tileOwnerLo, 1); // 0x78260

    m_prevAnimSetNode = (i32)m_14->m_1c;
    m_14->m_1c = (void*)EntranceLookupAnimSet(s_dAnimKeyC);

    m_154->m_8 |= 1;
    if (m_10->m_74 != 0x15f90) {
        m_10->m_74 = 0x15f90;
        m_10->m_8 |= 0x20000;
    }

    if (a2 != -1) {
        *(i32*)((char*)this + 0x370) = a2;
        (*(CDeathReg7c**)((char*)g_pGameRegistry + 0x7c))->Notify(a2, m_tileOwnerHi); // 0xfcc50
    }

    switch (deathType) {
        case DEATH_SQUASH: // GRUNTZ_DEATHZ_SQUASH
            if (m_entranceReason == 1) {
                m_prevEntranceDesc = (i32)m_154->m_1b4;
                m_154->SetGeometryEx(m_poseDeath, 0);
                goto pathA;
            }
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_SQUASH);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_SQUASH, DEATH_FRAME());
            DEATH_CUE(0x35b);
            goto finalize;

        case DEATH_DROP:
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            m_154->m_8 |= 0x10000;
            goto tail;

        case DEATH_SINK: // GRUNTZ_DEATHZ_SINK
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_SINK);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_SINK, DEATH_FRAME());
            DEATH_CUE(0x35a);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;

        case DEATH_HOLE: // GRUNTZ_DEATHZ_HOLE
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_HOLE);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_HOLE, DEATH_FRAME());
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_SHATTER: // GRUNTZ_DEATHZ_SHATTER (apply FREEZE)
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_SHATTER);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_FREEZE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;

        case DEATH_BURN: // GRUNTZ_DEATHZ_BURN
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_BURN);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_BURN, DEATH_FRAME());
            DEATH_CUE(0x352);
            goto finalize;

        case DEATH_QUICKFALL: // GRUNTZ_DEATHZ_QUICKFALL (apply FALL), snap to tile center
            m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
            m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_QUICKFALL);
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            if (m_10->m_74 != -1) {
                m_10->m_74 = -1;
                m_10->m_8 |= 0x20000;
            }
            DEATH_CUE(0x357);
            goto finalize;

        case DEATH_FALL: { // FALL / QUICKFALL by tile attribute
            CTileGrid* grid = g_pGameRegistry->m_70;
            i32 attr = ((i32*)grid->m_8[m_10->m_60 >> 5])[(m_10->m_5c >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_QUICKFALL);
                tag = 0x357;
                if (m_10->m_74 != -1) {
                    m_10->m_74 = -1;
                    m_10->m_8 |= 0x20000;
                }
                m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
                m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            } else {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_FALL);
            }
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_FALL2: { // FALL2 / QUICKFALL2 by tile attribute
            CTileGrid* grid = g_pGameRegistry->m_70;
            i32 attr = ((i32*)grid->m_8[m_10->m_60 >> 5])[(m_10->m_5c >> 5) * 7 + 4];
            i32 tag = 0x355;
            if (attr == 0x6e || attr == 0x74) {
                m_poseDeath = (i32)m_154->m_c->m_2c->LookupValue(s_DEATHZ_QUICKFALL2);
                tag = 0x357;
                if (m_10->m_74 != -1) {
                    m_10->m_74 = -1;
                    m_10->m_8 |= 0x20000;
                }
                m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
                m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
            } else {
                CSprite* out = 0;
                m_154->m_c->m_2c->m_10map.Lookup(s_DEATHZ_FALL2, &out);
                m_poseDeath = (i32)out;
            }
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_FALL, DEATH_FRAME());
            DEATH_CUE(tag);
            m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);
            Step6a060();
            goto tail;
        }

        case DEATH_ELECTROCUTE: { // GRUNTZ_DEATHZ_ELECTROCUTE
            CSprite* out = 0;
            m_154->m_c->m_2c->m_10map.Lookup(s_DEATHZ_ELECTROCUTE, &out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_ELECTROCUTE, DEATH_FRAME());
            DEATH_CUE(0x353);
            goto finalize;
        }

        case DEATH_MELT: {     // GRUNTZ_DEATHZ_MELT
            ApplySetState1(1); // 0x4322
            CSprite* out = 0;
            m_154->m_c->m_2c->m_10map.Lookup(s_DEATHZ_MELT, &out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_MELT, DEATH_FRAME());
            DEATH_CUE(0x359);
            goto finalize;
        }

        case DEATH_KAROKE: { // GRUNTZ_DEATHZ_KAROKE
            CSprite* out = 0;
            m_154->m_c->m_2c->m_10map.Lookup(s_DEATHZ_KAROKE, &out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->SetGeometryEx(m_poseDeath, 0);
            m_154->GameApplyLookupSprite(s_DEATHZ_KAROKE, DEATH_FRAME());
            DEATH_CUE(0x358);
            goto tail;
        }

        case DEATH_EXPLODE: { // GRUNTZ_DEATHZ_EXPLODE
            if (m_entranceReason == 1) {
                m_prevEntranceDesc = (i32)m_154->m_1b4;
                m_154->m_1a0.SetGeometry(m_poseDeath);
                goto pathA;
            }
            CSprite* out = 0;
            m_154->m_c->m_2c->m_10map.Lookup(s_DEATHZ_EXPLODE, &out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->GameApplyLookupSprite(s_DEATHZ_EXPLODE, DEATH_FRAME());
            DEATH_CUE(0x354);
            goto finalize;
        }

        case DEATH_DRAIN: { // GRUNTZ_EXITZ_DRAIN (apply EXITZ), re-latch "B"
            CSprite* out = 0;
            m_154->m_c->m_2c->m_10map.Lookup(s_EXITZ_DRAIN, &out);
            m_poseDeath = (i32)out;
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->GameApplyLookupSprite(s_dEXITZ, DEATH_FRAME());
            m_prevAnimSetNode = (i32)m_14->m_1c;
            m_14->m_1c = (void*)EntranceLookupAnimSet(s_dExitKeyB);
            goto tail;
        }

        default:
            m_prevEntranceDesc = (i32)m_154->m_1b4;
            m_154->m_1a0.SetGeometry(m_poseDeath);
            m_154->GameApplyName(*(char**)((char*)this + 0x44c));
            {
                CGameRegistry* g = g_pGameRegistry;
                i32* rect = (i32*)(g->m_30->m_24->m_5c + 0x40);
                i32 x = m_10->m_5c;
                i32 y = m_10->m_60;
                if (x < rect[2] && x >= rect[0] && y < rect[3] && y >= rect[1]) {
                    g->m_60->CueSpawn(this, 3, -1, -1, -1);
                }
            }
            // block A: NORMALGRUNT_DEATH override
            if (m_entranceReason == 0x14 && g_pGameRegistry->m_134 != 1) {
                m_154->GameApplyLookupGeometry(s_NORMALGRUNT_DEATH, 0);
                m_154->GameApplyName(s_NORMALGRUNT_DEATH);
            }
            goto tail;
    }

pathA:
    m_154->GameApplyName(*(char**)((char*)this + 0x44c));
    {
        CGameRegistry* g = g_pGameRegistry;
        if (GruntPointVisible(g->m_30->m_24->m_5c + 0x40, m_10->m_5c, m_10->m_60)) {
            g->m_60->CueSpawn(this, 3, -1, -1, -1);
        }
    }
    deathType = DEATH_NORMAL;
    goto tail;

finalize:
    m_tileMgr->NotifyEntranceDrop(m_tileOwnerHi, m_tileOwnerLo, 0);

tail:
    // block B: m_38c finalize cue
    if (m_entranceReason == 0x14 && g_pGameRegistry->m_134 != 1) {
        m_tileMgr->NotifyDeathTile(m_10->m_5c, m_10->m_60, *(i32*)((char*)this + 0x38c));
    }
    if (*(i32*)((char*)this + 0x2d0) == 0xd) {
        TryPowerupAtTile();
    }
    m_gruntKind = 0;
    *(i32*)((char*)this + 0x360) = deathType;
    return 0;
}

#undef DEATH_FRAME
#undef DEATH_CUE

SIZE_UNKNOWN(CDeathReg7c);
SIZE_UNKNOWN(CGruntSndRes);
SIZE_UNKNOWN(CGruntSndResMgr);
SIZE_UNKNOWN(CGruntSndSlot);
