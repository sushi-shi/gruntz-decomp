// GruntResurrectRadius.cpp - the "resurrect gruntz within a tile radius" pass
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp. Sibling of the rock-break
// radius scan (RockBreakParticles.cpp): adjacent RVA (0x7be60 vs 0x7b440), the same
// g_mgrSettings (*0x64556c) singleton, and called by CGrunt::LoadGruntAbilityTuning
// (0x572db). The owning `this` is the grunt-list manager (m_4 = list head);
// CGruntResurrector is the recovered/placeholder identity (no RTTI vtable - a
// non-virtual manager pass). Only offsets / code bytes are load-bearing; helpers are
// reloc-masked externals.
// <Mfc.h> (not <Win32.h>): UserLogic.h pulls afx via ButeMgr.h/String.h, so the
// umbrella must be the MFC superset kept first (PtInRect / RECT / POINT come with
// it; docs/patterns/mfc-wall-is-breakable-switch-to-mfc.md).
#include <Mfc.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/LightFx.h>

#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created sprite) + CGameObjAux
#include <rva.h>

SIZE_UNKNOWN(ResGruntLogic);
struct ResGruntLogic { // grunt->m_38
    char m_pad00[0x8];
    u32 m_8; // +0x08  flags (|= 0x10000 on resurrect)
};
struct ResHost; // grunt->m_6c (opaque; passed through to Resurrect)
SIZE_UNKNOWN(ResGrunt);
struct ResGrunt {
    char m_pad00[0x38];
    ResGruntLogic* m_38; // +0x38
    char m_pad3c[0x54 - 0x3c];
    i32 m_54; // +0x54  x tile
    i32 m_58; // +0x58  y tile
    i32 m_5c; // +0x5c  busy/skip gate
    char m_pad60[0x68 - 0x60];
    i32 m_68;      // +0x68  grunt type index
    ResHost* m_6c; // +0x6c
};
SIZE_UNKNOWN(ResNode);
struct ResNode {
    ResNode* m_next; // +0x00
    char m_pad04[0x4];
    ResGrunt* m_grunt; // +0x08
};
SIZE_UNKNOWN(ResMgrCfgEntry);
struct ResMgrCfgEntry { // g_mgrSettings + 0x150 + type*0x238
    char m_pad00[0x14];
    i32 m_14; // +0x14
    char m_pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    i32 m_24; // +0x24
    char m_pad28[0x2c - 0x28];
    i32 m_2c; // +0x2c
    char m_pad30[0x38 - 0x30];
    CBattlezMapConfig m_38; // +0x38
    char m_pad3c[0x238 - 0x3c];
};
// The factory (m_world->m_8) is the canonical CSpriteFactory (<Gruntz/SpriteFactory.h>);
// the created "LightFx" eye-candy sprite is the shared CGameObject whose +0x7c
// CGameObjAux carries the Init driver (+0x10) and the per-class setup slot m_18
// (+0x18) - here the LightFx flash config below, downcast at the site.
SIZE_UNKNOWN(ResFactoryHost);
struct ResFactoryHost {
    char m_pad00[0x8];
    CSpriteFactory* m_8; // +0x08
};
SIZE_UNKNOWN(ResSettings);
struct ResSettings {
    char m_pad00[0x30];
    ResFactoryHost* m_world; // +0x30
    char m_pad34[0x134 - 0x34];
    i32 m_134; // +0x134  resurrect mode
    char m_pad138[0x150 - 0x138];
    ResMgrCfgEntry m_150[1]; // +0x150  per-type config (stride 0x238)
};
SIZE_UNKNOWN(ResButeMgr);
struct ResButeMgr {
    i32 GetInt(char* sec, char* key); // CButeMgr::GetInt FUN_00171af0
};
DATA(0x002453d8)
extern ResButeMgr g_resButeMgr;
DATA(0x0024556c)
extern ResSettings* g_resSettings;
SIZE_UNKNOWN(CGruntResurrector);
struct CGruntResurrector {
    char m_pad00[0x4];
    ResNode* m_4; // +0x04  grunt list head
    // FUN_000040bb __thiscall: spawn/resurrect one grunt (ret != -1 on success).
    i32 Resurrect(
        i32 type,
        i32 px,
        i32 py,
        i32 a3,
        i32 a4,
        ResHost* host,
        i32 a6,
        i32 a7,
        i32 aiType,
        i32 radius,
        i32 a10,
        i32 a11,
        i32 a12
    );
    void Notify(ResNode* node); // FUN_001b4ac7 __thiscall
    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);
};

// @early-stop
// regalloc/frame-layout wall (~65%): instruction selection, calls, constants,
// strings + the rect/loop/spawn structure are byte-faithful, but retail
// frame-allocates the `node` loop variable (a dedicated 4-byte slot at [esp+0x14]
// inside a 0x18 frame) while this /O2 recompile reuses an incoming-arg slot, yielding
// a 0x14 frame and a +4 cascade across every [esp+N] operand. Not source-steerable
// (the slot-vs-frame choice is the allocator's). Logic complete. See
// docs/patterns/zero-register-pinning.md + const-materialize-into-reg-vs-immediate.md.
RVA(0x0007be60, 0x21e)
i32 CGruntResurrector::LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r) {
    RECT rect;
    i32 hx = cx >> 5;
    i32 hy = cy >> 5;
    rect.left = hx - r;
    rect.right = hx + r;
    rect.top = hy - r;
    rect.bottom = hy + r;

    for (ResNode* node = m_4; node != 0; node = node->m_next) {
        ResGrunt* g = node->m_grunt;
        if (g->m_5c != 0) {
            continue;
        }
        POINT pt;
        pt.x = g->m_54;
        pt.y = g->m_58;
        if (!PtInRect(&rect, pt)) {
            continue;
        }

        i32 type = g->m_68;
        ResSettings* s = g_resSettings;
        ResMgrCfgEntry* cfg = &s->m_150[type];
        i32 aiType = 0;
        i32 ok = 0;
        i32 px = (g->m_54 << 5) + 0x10;
        i32 py = (g->m_58 << 5) + 0x10;

        if (s->m_134 == 1) {
            i32 radius = 0;
            if (cfg->m_14 == 0) {
                aiType = g_resButeMgr.GetInt("Grunt", "RessurectAIType");
                radius = g_resButeMgr.GetInt("Grunt", "RessurectAIRadius");
            }
            if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, aiType, radius, 0, 0, 0) != -1) {
                ok = 1;
            }
        } else if (cfg->m_20 != 0 && cfg->m_2c == 0 && cfg->m_24 == 0) {
            if (cfg->m_14 != 0) {
                if (Resurrect(type, px, py, 0x186a0, 3, g->m_6c, 0, 0, 0, 0, 0, 0, 0) != -1) {
                    ok = 1;
                }
            } else if (cfg->m_38.Method_030990(g->m_54, g->m_58) != 0) {
                ok = 1;
            }
        }

        if (ok) {
            g->m_38->m_8 |= 0x10000;
            Notify(node);
            CGameObject* spr =
                g_resSettings->m_world->m_8->CreateSprite(0, px, py, 0xf4240, "LightFx", 0x40003);
            spr->m_7c->Init(spr);
            ((CLightFx*)spr->m_7c->m_logic)
                ->Activate((i32) "GAME_LIGHTING_FLASH", (i32) "GAME_FLASH", 8, 1);
        }
    }
    return 1;
}
