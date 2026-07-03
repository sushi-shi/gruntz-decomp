// ProjectileUpdate.cpp - CProjectile::Update @0x61cb0 (graduated from
// src/Stub/Backlog.cpp, was Projectile::vfunc_9). The per-frame projectile tick:
//
//   pings the projectile's sub-logic clock (m_154->m_1a0.Tick(g_6bf3bc)); when it
//   fires (==2) it spawns the type's eye-candy sprite through the HUD sprite
//   factory (g_gameReg->m_30->m_8->CreateSprite) via a 5-slot dense switch on the
//   projectile type m_170:
//     type 2            -> "Projectile"/"Boomerang" impact sprite ("Boomerang")
//     type 9,10,11,21,22 -> "Projectile" impact sprite
//     type 17           -> "TimeBomb" sprite at the resolved detonation cell
//     default           -> re-home the tracked target object (m_260 table) +
//                          re-fire it, or (miss) mark the retry flag.
//   Then the shared "impact" tail reads the "AttackDowntime" tuning from
//   g_buteMgr, stamps the sound/anim scratch block (m_860..m_86c from g_645588),
//   and the always-run finish tail advances the fuse clock (m_10->m_74) and runs
//   the m_220/m_1c4 teardown hooks.
//
// The owning class is a reloc-masked engine vtable (proximity: CGrunt bracket vs
// the "Projectile" string); every callee is an external reloc-masked __thiscall
// thunk and the sprite sub-objects are accessed by raw this+offset. Only offsets /
// code bytes are load-bearing (campaign doctrine).

#include <Mfc.h> // Win32/engine types

#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <rva.h>

DATA(0x002bf3bc)
extern "C" i32 g_6bf3bc; // the sub-logic clock fed to the tick
DATA(0x00245588)
extern "C" i32 g_645588; // sound/anim scratch seed

// The projectile's config setup object reached via spr->m_7c->m_18. Its vtable
// slot +0x44 (a __thiscall Configure, 7 args) arms the sprite against the
// projectile's geometry; on failure the setup's m_154 logic gets the 0x10000 bit.
// The slot is modeled as a pointer-to-member loaded from the vtable so MSVC emits
// `mov edx,[ecx]; call [edx+0x44]` (the class must be COMPLETE before the PMF
// typedef so it stays a 4-byte single-inheritance PMF; docs/patterns/pmf-complete-class-4byte.md).
struct CProjSetupVtbl;
struct CProjSetupLogic {
    char m_pad00[0x8];
    u32 m_8; // +0x08  flags (|= 0x10000 on Configure failure)
};
struct CProjSetup {
    CProjSetupVtbl* m_vptr; // +0x00
    char m_pad04[0x154 - 0x4];
    CProjSetupLogic* m_154; // +0x154
    i32 Configure(i32, i32, i32, i32, i32, i32, i32);
};
typedef i32 (CProjSetup::*CProjConfigureFn)(i32, i32, i32, i32, i32, i32, i32);
struct CProjSetupVtbl {
    char m_pad00[0x44];
    CProjConfigureFn Configure; // +0x44
};
inline i32 CProjSetup::Configure(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g) {
    return (this->*(m_vptr->Configure))(a, b, c, d, e, f, g);
}
// The created eye-candy sprite. m_7c is a per-instance control block: Init at
// +0x10 (__cdecl), the config setup at +0x18.
struct CProjSprite;
struct CProjSpriteCtl {
    void* m_s0[4];                     // slots 0..3
    void(__cdecl* Init)(CProjSprite*); // +0x10
    char m_pad14[0x18 - 0x14];
    CProjSetup* m_18; // +0x18
};
struct CProjSprite {
    char m_pad00[0x7c];
    CProjSpriteCtl* m_7c; // +0x7c
    char m_pad80[0x120 - 0x80];
    i32 m_120; // +0x120
    i32 m_124; // +0x124
};
// The HUD sprite factory (g_gameReg->m_30->m_8), CreateSprite by class NAME.
struct CProjFactory {
    CProjSprite*
    CreateSprite(i32 kind, i32 px, i32 py, i32 hint, const char* name, i32 flags); // 0x1597b0
};
struct CProjFactoryHolder {
    char m_pad00[0x8];
    CProjFactory* m_8; // +0x08
};
// The game-registry singleton (*0x64556c) as this projectile-update TU views it:
// only the +0x30 sprite-factory holder is reached. (Distinct object from the
// projectile-action registry g_projReg in ProjActRegistry.cpp and from the +0x30
// sub-registry in StreamRecordLoaders.cpp - each carried its own placeholder view.)
SIZE_UNKNOWN(CProjGameReg);
struct CProjGameReg {
    char m_pad00[0x30];
    CProjFactoryHolder* m_30; // +0x30
};
DATA(0x0024556c)
extern CProjGameReg* g_gameReg; // *0x64556c

// The projectile's host/world object (this->m_10).
struct CProjHost {
    char m_pad00[0x8];
    u32 m_8; // +0x08  flags (|= 0x20000 on fuse advance)
    char m_pad0c[0x5c - 0xc];
    i32 m_5c; // +0x5c  pixel x
    i32 m_60; // +0x60  pixel y
    char m_pad64[0x74 - 0x64];
    i32 m_74; // +0x74  fuse clock
};
// The projectile's sub-logic (this->m_154). The +0x1a0 sub-object owns the tick
// (0x15c360) and two active-state gates m_20/m_28.
struct CProjTickSub {
    i32 Tick(i32 clock); // 0x15c360 __thiscall
    char m_pad04[0x20 - 0x4];
    i32 m_20; // +0x20
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28
};
struct CProjLogic {
    char m_pad00[0x1a0];
    CProjTickSub m_1a0; // +0x1a0
};
// A tracked-target object held in the m_260 table (default arm). Method 0x1bf9
// re-homes/re-fires it (8 args); m_170 is its type, m_19c a fallback gate.
struct CProjTarget {
    void Refire(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8); // 0x1bf9 thunk
    char m_pad00[0x170];
    i32 m_170; // +0x170
    char m_pad174[0x19c - 0x174];
    i32 m_19c; // +0x19c
};
// The tracked-target table (this->m_260): the target pointers live in a DWORD
// array at +0x1c, and Cleanup (0x2e96) releases a slot (4 args).
struct CProjTargetTable {
    char m_pad00[0x1c];
    CProjTarget* m_1c[1];                         // +0x1c  indexed by (m_204 + m_200*15)
    void Cleanup(i32 a1, i32 a2, i32 a3, i32 a4); // 0x2e96 thunk
};
// The [Grunt]/attribute-tuning registry singleton (canonical CButeMgr):
// GetDword (0x172240) is reloc-masked __thiscall.
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

class CProjectile {
public:
    i32 Update(); // 0x61cb0

    // helpers on `this`
    void GetSpawnPos(i32* out);                   // 0x1a73 thunk __thiscall
    void ArmMode(i32 a1, i32 a2, i32 a3, i32 a4); // 0x3bd9 thunk
    void Teardown3dd7();                          // 0x3dd7 thunk
    void ArmFinish(i32 a1, i32 a2, i32 a3);       // 0x136b thunk
    void Teardown22de();                          // 0x22de thunk

    char m_pad00[0x10];
    CProjHost* m_10; // +0x10
    char m_pad14[0x154 - 0x14];
    CProjLogic* m_154; // +0x154
    char m_pad158[0x170 - 0x158];
    i32 m_170; // +0x170  projectile type
    char m_pad174[0x1c0 - 0x174];
    i32 m_1c0; // +0x1c0  bute section
    i32 m_1c4; // +0x1c4
    char m_pad1c8[0x1e4 - 0x1c8];
    i32 m_1e4; // +0x1e4
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_1ec; // +0x1ec
    i32 m_1f0; // +0x1f0
    char m_pad1f4[0x200 - 0x1f4];
    i32 m_200; // +0x200
    i32 m_204; // +0x204
    i32 m_208; // +0x208
    i32 m_20c; // +0x20c
    char m_pad210[0x214 - 0x210];
    i32 m_214; // +0x214
    i32 m_218; // +0x218
    char m_pad21c[0x220 - 0x21c];
    i32 m_220; // +0x220
    char m_pad224[0x258 - 0x224];
    i32 m_258; // +0x258  sub-type / mode selector
    char m_pad25c[0x260 - 0x25c];
    CProjTargetTable* m_260; // +0x260
    char m_pad264[0x3f0 - 0x264];
    i32 m_3f0; // +0x3f0
    char m_pad3f4[0x460 - 0x3f4];
    i32 m_460; // +0x460
    char m_pad464[0x860 - 0x464];
    i32 m_860; // +0x860
    i32 m_864; // +0x864
    i32 m_868; // +0x868
    i32 m_86c; // +0x86c
};

// @source: string-xref
// @early-stop
// jump-table-data-overlap + flag-spill wall (~16%): logic complete, and the
// prologue + the 5-slot dense switch on m_170 (bias 2, range 0x14) reproduce retail
// byte-for-byte (index table + jump table + the named "Projectile"/"Boomerang"/
// "TimeBomb"/"AttackDowntime" DIR32 strings + the g_gameReg/g_buteMgr/g_645588
// globals). Two compounding walls: (1) cl emits the byte index-table + dword
// jump-table as $L COMDATs while the delinker INLINES the switchdataD_* tables into
// .text, so the tables never pair and objdiff mis-aligns the whole tail (see
// docs/patterns/jumptable-data-overlap.md + switch-jumptable-separate-comdat.md);
// (2) retail keeps the retry flag in callee-saved ebx (`xor ebx,ebx`) but this
// env's cl spills it to a 3rd stack dword (`sub esp,0xc` vs retail `sub esp,8`),
// +4-shifting the TimeBomb arm's [esp+N] out-param slots. Types 9-11 and 21-22
// share one tail-merged "Projectile" arm across two jump-table slots. Neither is
// source-steerable.
RVA(0x00061cb0, 0x34a)
i32 CProjectile::Update() {
    i32 flag = 0;
    if (m_154->m_1a0.Tick(g_6bf3bc) == 2) {
        switch (m_170) {
            case 9:
            case 10:
            case 11: {
                CProjSprite* spr =
                    g_gameReg->m_30->m_8
                        ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0, "Projectile", 0x40003);
                spr->m_7c->Init(spr);
                CProjSetup* s = spr->m_7c->m_18;
                if (s->Configure(m_170, m_1ec, m_1f0, m_208, m_20c, m_10->m_5c, m_10->m_60) == 0) {
                    s->m_154->m_8 |= 0x10000;
                }
                break;
            }
            case 2: {
                CProjSprite* spr =
                    g_gameReg->m_30->m_8
                        ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0, "Boomerang", 0x40003);
                spr->m_7c->Init(spr);
                CProjSetup* s = spr->m_7c->m_18;
                if (s->Configure(m_170, m_1ec, m_1f0, m_208, m_20c, m_10->m_5c, m_10->m_60) == 0) {
                    s->m_154->m_8 |= 0x10000;
                }
                break;
            }
            case 17: {
                i32 pos[2];
                GetSpawnPos(pos);
                CProjSprite* spr =
                    g_gameReg->m_30->m_8->CreateSprite(0, pos[0], pos[1], 0xf, "TimeBomb", 0x40003);
                spr->m_120 = 0;
                spr->m_7c->Init(spr);
                spr->m_124 = m_1ec;
                break;
            }
            case 21:
            case 22: {
                CProjSprite* spr =
                    g_gameReg->m_30->m_8
                        ->CreateSprite(0, m_10->m_5c, m_10->m_60, 0, "Projectile", 0x40003);
                spr->m_7c->Init(spr);
                CProjSetup* s = spr->m_7c->m_18;
                if (s->Configure(m_170, m_1ec, m_1f0, m_208, m_20c, m_10->m_5c, m_10->m_60) == 0) {
                    s->m_154->m_8 |= 0x10000;
                }
                break;
            }
            default: {
                CProjTarget* tgt = m_260->m_1c[m_204 + m_200 * 15];
                if (tgt == 0) {
                    flag = 1;
                    break;
                }
                tgt->Refire(m_170, m_214, m_1ec, m_1f0, m_10->m_5c, m_10->m_60, 0, m_258);
                i32 t = tgt->m_170;
                if (t > 0x16) {
                    t = tgt->m_19c;
                }
                if (t == 1 && m_258 != 0x38) {
                    m_260->Cleanup(m_1ec, m_1f0, 0xb, m_200);
                    return 0;
                }
                break;
            }
        }

        // impact tail (0x61f08)
        m_1e4 = 1;
        i32 dt = g_buteMgr.GetDword((char*)m_1c0, "AttackDowntime");
        if (m_258 == 0x3b) {
            dt = 0;
        }
        m_868 = dt;
        m_86c = 0;
        m_860 = g_645588;
        m_864 = 0;
        m_460 = 0;
        m_3f0 = 0;
        if (m_1c4 != 0) {
            Teardown22de();
        }
        m_218 = 0;
    }

    // finish tail (0x61f74)
    CProjTickSub* sub = &m_154->m_1a0;
    if ((sub->m_28 == 0 || sub->m_20 != 0) && flag == 0) {
        return 0;
    }
    if (m_170 == 2) {
        ArmMode(0, 1, 0, 0);
    }
    CProjHost* h = m_10;
    i32 fuse = h->m_60 + 0x186a0;
    if (h->m_74 != fuse) {
        h->m_74 = fuse;
        h->m_8 |= 0x20000;
    }
    i32 v220 = m_220;
    m_1e4 = 0;
    if (v220 != 0) {
        Teardown3dd7();
        return 0;
    }
    ArmFinish(1, 0, 0);
    return 0;
}
SIZE_UNKNOWN(CProjFactory);
SIZE_UNKNOWN(CProjFactoryHolder);
SIZE_UNKNOWN(CProjHost);
SIZE_UNKNOWN(CProjLogic);
SIZE_UNKNOWN(CProjSetup);
SIZE_UNKNOWN(CProjSetupLogic);
SIZE_UNKNOWN(CProjSetupVtbl);
SIZE_UNKNOWN(CProjSprite);
SIZE_UNKNOWN(CProjSpriteCtl);
SIZE_UNKNOWN(CProjTarget);
SIZE_UNKNOWN(CProjTargetTable);
SIZE_UNKNOWN(CProjTickSub);
