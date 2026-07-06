// GruntTileMgr.cpp - the grunt path/occupancy sub-manager's spell-area cue
// (re-homed from src/Stub/ApiCallers.cpp). CombatCue scans the tile-mgr's 4x15
// grunt grid (this+0x1c) for grunts whose 14x14-px HUD box overlaps the cue's
// pixel AABB (centred on the caster, +-((radius<<5)+7)), then applies the
// per-tier spell effect: a per-cell tile effect (tiers 1/6/7), a random-tile
// teleport-scatter + LightFx flash (tier 2), the health/freeze/toyz LightFx
// flashes (tiers 3/4/5). The board dims (this+0x22c -> m_24 -> m_5c -> {w,h})
// bound the teleport re-roll. Class-split from Grunt.cpp; matching-neutral.
#include <Gruntz/Grunt.h>
#include <rva.h>
#include <Bute/ButeMgr.h> // CButeMgr g_buteMgr (GetIntDef)

extern CButeMgr g_buteMgr; // ?g_buteMgr@@3VCButeMgr@@A

// The global running game clock (reloc-masked).
extern "C" u32 g_645588;

// The LightFx / flash key strings (reloc-masked .rodata literals).
static const char s_LightFx[] = "LightFx";
static const char s_GAME_FLASH[] = "GAME_FLASH";
static const char s_GAME_LIGHTING_FLASH[] = "GAME_LIGHTING_FLASH";
// The combat-timeout bute keys (GetIntDef takes char*).
static char s_Grunt[] = "Grunt";
static char s_CombatTimeout[] = "CombatTimeout";

// @early-stop
// prologue-scheduling + grid-pointer-register regalloc wall (~82%): the body is
// byte-correct in shape/offsets/symbols/CFG (the 4x15 grid scan, the +-((r<<5)+7)
// AABB, the tail-merged tier-1/6/7 ApplyCellEffect cases, the tier-2 teleport
// re-roll loop, the tier-3/5/4 health/toyz/freeze LightFx flashes + shared Activate
// tail all match instruction-for-instruction). Residual is the documented prologue
// coin-flip: retail spills `this` to esp+0x14 and materialises the running grid
// pointer with `lea 0x1c(ecx),eax` (this preserved, pointer in eax/memory), while cl
// spills `this` to esp+0x10 and reuses ecx via `add 0x1c,ecx` (pointer in ecx) - a
// one-slot shift that cascades displacement bytes through the whole prologue + loop
// control. Source-invariant; deferred to the final sweep.
RVA(0x0007b930, 0x3b7)
i32 CGruntTileMgr::CombatCue(i32 x, i32 y, i32 radius, i32 tier, i32 flag) {
    i32 r = radius << 5;
    i32 xLo = x - r - 7;
    i32 yLo = y - r - 7;
    i32 xHi = x + r + 7;
    i32 yHi = y + r + 7;
    i32 rangeA = m_22c->m_24->m_5c->m_28 - 2;
    i32 rangeB = m_22c->m_24->m_5c->m_2c - 2;

    CGrunt** p = &m_grid[0][0];
    for (i32 i = 0; i < 4; i++) {
        for (i32 j = 0; j < 15; j++, p++) {
            CGrunt* g = *p;
            if (g == 0) {
                continue;
            }
            if (g->m_entranceCommitted == 0) {
                continue;
            }
            if (g->m_entranceDropActive != 0) {
                continue;
            }
            i32 gx = g->m_10->m_5c;
            i32 gy = g->m_10->m_60;
            i32 lx = gx - 7;
            i32 ly = gy - 7;
            i32 hx = lx + 14;
            i32 hy = ly + 14;
            if (xLo <= hx && xHi >= lx && yLo <= hy && yHi >= ly) {
                switch (tier) {
                    case 1:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 0, flag);
                        }
                        break;
                    case 6:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 0xb, flag);
                        }
                        break;
                    case 7:
                        if (g->m_gruntKind != 0x38) {
                            ApplyCellEffect(i, j, 2, flag);
                        }
                        break;
                    case 2: { // teleport-scatter
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 done = 0;
                        do {
                            i32 dx = rangeA ? GruntRand() % rangeA + 1 : GruntRand() & 1;
                            i32 dy = rangeB ? GruntRand() % rangeB + 1 : GruntRand() & 1;
                            if (g->TeleportMove(dx, dy, 0, 1)) {
                                CHudSprite* spr =
                                    (CHudSprite*)g_pGameRegistry->m_world->m_8
                                        ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                                done = 1;
                                spr->m_7c->m_init(spr);
                                spr->m_7c->m_18
                                    ->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 3, 1);
                            }
                        } while (done == 0);
                        break;
                    }
                    case 3: { // health
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->m_health = 0x64;
                        g->UpdateCombatTimer();
                        g->m_combatTimeoutLo =
                            g_buteMgr.GetIntDef(s_Grunt, s_CombatTimeout, 0x1388);
                        g->m_combatTimeoutHi = 0;
                        g->m_combatClockLo = g_645588;
                        g->m_combatClockHi = 0;
                        CHudSprite* spr =
                            (CHudSprite*)g_pGameRegistry->m_world->m_8
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->m_init(spr);
                        spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 2, 1);
                        break;
                    }
                    case 5: { // toyz
                        if (gx == x && gy == y) {
                            break;
                        }
                        i32 toy = GruntRand() % 9 + 0x17;
                        if (toy == 0x1e) {
                            toy = 0x20;
                        }
                        g->SetMoveStateA(toy, 1, 0, 0);
                        CHudSprite* spr =
                            (CHudSprite*)g_pGameRegistry->m_world->m_8
                                ->CreateSprite(0, gx, gy, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->m_init(spr);
                        spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 7, 1);
                        break;
                    }
                    case 4: { // freeze
                        if (gx == x && gy == y) {
                            break;
                        }
                        g->FreezeApply();
                        CGruntHud* h = g->m_10;
                        CHudSprite* spr =
                            (CHudSprite*)g_pGameRegistry->m_world->m_8
                                ->CreateSprite(0, h->m_5c, h->m_60, 0xf4240, s_LightFx, 0x40003);
                        spr->m_7c->m_init(spr);
                        spr->m_7c->m_18->Activate(s_GAME_LIGHTING_FLASH, s_GAME_FLASH, 9, 1);
                        break;
                    }
                }
            }
        }
    }
    return 1;
}
