// ProjectileUpdate.cpp - the grunt's per-frame ATTACK-FIRE step @0x61cb0
// (graduated from src/Stub/Backlog.cpp as "Projectile::vfunc_9", then briefly
// misclaimed as CProjectile::Update - both disproven, see IDENTITY below).
//
//   Ticks the grunt's attack animation (((CAniAdvanceCursor*)&m_154->m_1a0)->Advance_15c360((i32)g_6bf3bc)); when it
//   reaches the fire cue (==2) it dispatches on the grunt's tool kind m_170:
//     ranged (2 Boomerang / 9,10,11,21,22 -> "Projectile" / 17 TimeBomb):
//       spawn the projectile eye-candy sprite through the HUD sprite factory
//       (g_gameReg->m_world->m_8->CreateSprite) and arm the created sprite's
//       CProjectile setup object via its slot-17 virtual LoadProjectileSprites;
//     melee (default): deliver the hit to the grunt occupying the neighbor grid
//       cell (m_tileMgr cell [m_neighborRow + m_neighborCol*15]), or mark the
//       retry flag on a miss.
//   The shared "impact" tail reads the "AttackDowntime" tuning for the grunt's
//   anim-set bute section, stamps the attack-downtime timer record
//   (m_860..m_86c from g_645588), and the always-run finish tail advances the
//   bound object's z-order key (m_object->m_latchedAnimId = screenY + 100000)
//   and runs the m_poweredUp/m_1c4 teardown hooks.
//
// IDENTITY (proven, fold DEFERRED on the CGrunt ODR merge): the owner is CGrunt and
// 0x61cb0 is CGrunt's VTABLE SLOT 9 (== CGrunt::UserLogicVfunc7, now declared OVERRIDE in
// Grunt.h; thunk 0x119f, origin CUserLogic slot 9 - anchored by slots 14/15 ==
// 0x1730/0x3607).
//
// WHY NOT FOLDED (real blocker = ODR, not ownership): this method spawns CProjectiles
// and dispatches their slot-17 (LoadProjectileSprites), so it MUST include
// <Gruntz/Projectile.h> (-> the canonical <Gruntz/MovingLogic.h> CMovingLogic). But
// <Gruntz/Grunt.h> REDEFINES the whole CUserBase/CUserLogic/CMovingLogic/CProjectile
// family with a different (CGruntHud*-m_10, inline-ctor, i32-vtable) layout, so the two
// headers ODR-CONFLICT in one TU (empirically verified: `redefinition of CProjectile/
// CMovingLogic/CUserLogic/CUserBase`). Defining this as CGrunt::UserLogicVfunc7 therefore needs
// the documented CGrunt/CMovingLogic ODR merge (MovingLogic.h NOTE) to land first -
// then this view dissolves mechanically (its member names already mirror Grunt.h). Until
// then the grunt `this` is a CGrunt facet; the sub-objects it reaches ARE folded onto
// the shared canonicals (m_object CGameObject*, m_154 CProjRenderObj*, the spawned setup
// object CProjectile*, g_gameReg CGameRegistry*, g_buteMgr CButeMgr).
// Field agreement with <Gruntz/Grunt.h> (read-proven):
// +0x1c0 m_animSetName (CString - the bute section), +0x1ec/+0x1f0 m_tileOwnerHi/
// Lo (== LoadProjectileSprites' launcher-cell args), +0x200/+0x204 m_neighborCol/
// m_neighborRow (the melee cell index), +0x218 m_combatActive (cleared after
// impact), +0x220 m_poweredUp, +0x258 m_gruntKind (0x38/0x3b checks - the same
// family as Grunt.h's 0x37 TimePerTile check), +0x260 m_tileMgr (CGruntTileMgr*),
// +0x860 timer record base. DISPROOF of the old CProjectile claim: the canonical
// CProjectile ctor (0x126e0, 99% byte-proven) constructs a CObList at +0x204
// where this fn reads m_neighborRow as an int. NOT foldable this cycle:
// include/Gruntz/Grunt.h + the grunt TUs are owned by a parallel worker
// (matcher-2), and CGrunt lacks the slot-9 OVERRIDE decl + a few of these
// members; the view below carries Grunt.h's canonical member names so the fold
// is mechanical. CONFLICT to reconcile on fold: Grunt.h names +0x170
// "m_entranceReason (entrance-reason / movement state)" but this fn switches it
// over the GRUNTZ tool kinds (2/9/10/11/17/21/22) and passes it as the
// projectile kind -> the slot is the grunt's current tool/attack kind here.

#include <Mfc.h> // Win32/engine types
#include <Gruntz/Grunt.h>
#include <Gruntz/TriggerMgr.h>

#include <Bute/ButeMgr.h>         // canonical CButeMgr (one shape)
#include <Gruntz/GameRegistry.h>  // canonical game-registry singleton (*0x64556c)
#include <Gruntz/Projectile.h>    // canonical CProjectile (slot-17 LoadProjectileSprites)
                                  // + CProjRenderObj/CProjAnim (the +0x154 anim player)
#include <Gruntz/SpriteFactory.h> // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created sprite + the bound object)
#include <rva.h>

// The +0x1a0 anim sub-object's Tick @0x15c360 is CAniAdvanceCursor::Advance_15c360; TU-local decl.
class CAniAdvanceCursor {
public:
    i32 Advance_15c360(i32 clock);
};

DATA(0x002bf3bc)
extern "C" i32 g_6bf3bc; // the sub-logic clock fed to the tick
// g_645588 (the running game clock stamped into the downtime timer record) comes
// from <Gruntz/MovingLogic.h> (extern "C" u32; Projectile.cpp owns the DATA pin).

// The created eye-candy sprite is the shared CGameObject (<Gruntz/UserLogic.h>);
// its +0x7c CGameObjAux control block carries the Init driver (+0x10) and the
// per-class setup slot m_18 (+0x18) - for "Projectile"/"Boomerang"/"TimeBomb"
// the bound setup/logic object IS a CProjectile (downcast per site; the slot is
// generically typed on the canonical aux; proven-heterogeneous across classes).
// The HUD sprite factory reached as g_gameReg->m_world->m_8 is the canonical
// CSpriteFactory (shared <Gruntz/SpriteFactory.h>).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // the one game-registry singleton (*0x64556c)

// A target grunt occupying a tile cell (== CGrunt; fold deferred with the owner
// view below). The 0x1bf9 thunk delivers this attack to it (8 args); m_toolKind
// (+0x170, the same slot as the attacker's) is its kind, m_19c a fallback gate
// (both slots exist on Grunt.h's CGrunt).
// The grunt's path/occupancy tile manager (== CGruntTileMgr, <Gruntz/Grunt.h>
// CGrunt::m_tileMgr +0x260; fold deferred - the canonical models methods only,
// this +0x1c cell array is an additive member for the fold). The per-cell grunt
// pointers live in a pointer array at +0x1c, indexed [row + col*15] (15-row
// column-major grid); Cleanup (0x2e96 thunk) releases a slot (4 args).
// The [Grunt]/attribute-tuning registry singleton (canonical CButeMgr):
// GetDword (0x172240) is reloc-masked __thiscall.
DATA(0x002453d8)
extern CButeMgr g_buteMgr;

// The attacking grunt (== CGrunt; the DEFERRED-fold view - member names mirror
// <Gruntz/Grunt.h> so dissolving this view is mechanical once Grunt.h unlocks).
class CGruntFireView {
public:
    i32 Update(); // 0x61cb0 == CGrunt vtable slot 9 (UserLogicVfunc7 override)

    // helpers on `this` (CGrunt engine methods, reloc-masked thunks)
    void GetSpawnPos(i32* out);                   // 0x1a73 thunk __thiscall
    void ArmMode(i32 a1, i32 a2, i32 a3, i32 a4); // 0x3bd9 thunk
    void Teardown3dd7();                          // 0x3dd7 thunk
    void ArmFinish(i32 a1, i32 a2, i32 a3);       // 0x136b thunk
    void Teardown22de();                          // 0x22de thunk

    char m_pad00[0x10];
    CGameObject* m_object; // +0x10  == CUserLogic::m_object (bound engine object)
    char m_pad14[0x154 - 0x14];
    // +0x154  the anim player (Grunt.h: CEntranceAnimPlayer* m_154 - the same
    // engine class Projectile.h views as CProjRenderObj: anim sub @+0x1a0,
    // state gates @+0x1c0/+0x1c8).
    CProjRenderObj* m_154;
    char m_pad158[0x170 - 0x158];
    i32 m_toolKind; // +0x170  tool/attack kind (Grunt.h names this m_entranceReason - see hdr)
    char m_pad174[0x1c0 - 0x174];
    char* m_animSetName; // +0x1c0  == CGrunt::m_animSetName (CString payload; bute section)
    i32 m_1c4;           // +0x1c4
    char m_pad1c8[0x1e4 - 0x1c8];
    i32 m_entranceActive; // +0x1e4  == CGrunt::m_entranceActive (here: impact latch 1 -> 0)
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_tileOwnerHi; // +0x1ec  == CGrunt::m_tileOwnerHi (launcher tile row)
    i32 m_tileOwnerLo; // +0x1f0  == CGrunt::m_tileOwnerLo (launcher tile col)
    char m_pad1f4[0x200 - 0x1f4];
    i32 m_neighborCol; // +0x200  == CGrunt::m_neighborCol (melee target cell)
    i32 m_neighborRow; // +0x204  == CGrunt::m_neighborRow
    i32 m_208;         // +0x208  target pixel X (fed to LoadProjectileSprites' sx)
    i32 m_20c;         // +0x20c  target pixel Y (sy)
    char m_pad210[0x214 - 0x210];
    i32 m_214;          // +0x214
    i32 m_combatActive; // +0x218  == CGrunt::m_combatActive (cleared after impact)
    char m_pad21c[0x220 - 0x21c];
    i32 m_poweredUp; // +0x220  == CGrunt::m_poweredUp (teardown gate)
    char m_pad224[0x258 - 0x224];
    i32 m_gruntKind; // +0x258  == CGrunt::m_gruntKind (0x38/0x3b special-cases)
    char m_pad25c[0x260 - 0x25c];
    CTriggerMgr* m_tileMgr; // +0x260  == CGrunt::m_tileMgr (CGruntTileMgr*)
    char m_pad264[0x3f0 - 0x264];
    i32 m_3f0; // +0x3f0
    char m_pad3f4[0x460 - 0x3f4];
    i32 m_460; // +0x460
    char m_pad464[0x860 - 0x464];
    i32 m_860; // +0x860  == CGrunt::m_860 (attack-downtime timer record base)
    i32 m_864; // +0x864
    i32 m_868; // +0x868  downtime duration
    i32 m_86c; // +0x86c
};
SIZE_UNKNOWN(CGruntFireView);

// @source: string-xref
// @early-stop
// jump-table-data-overlap wall (fuzzy % is an alignment artifact): logic complete
// and byte-verified vs retail (`sema disasm 0x61cb0 --diff`): the prologue is
// exact (`sub esp,8`; the retry flag in callee-saved ebx - the old view's
// flag-spill wall DISSOLVED with this model), the slot-17 virtual dispatch is the
// exact retail shape (`mov edx,[edi]; mov ecx,edi; call [edx+0x44]`, then
// `mov edi,[edi+0x154]; or [edi+0x8],0x10000`), and the 5-slot dense switch on
// m_toolKind (bias 2, range 0x14) + the named "Projectile"/"Boomerang"/"TimeBomb"
// /"AttackDowntime" DIR32 strings + g_gameReg/g_buteMgr/g_645588 reproduce
// retail. Residues: (1) DOMINANT - cl emits the byte index-table + dword
// jump-table as $L COMDATs while the delinker INLINES the switchdataD_* tables
// into .text, so the tables never pair and objdiff mis-aligns the ENTIRE switch
// tail as inserted lines (docs/patterns/jumptable-data-overlap.md +
// switch-jumptable-separate-comdat.md - the % floor, not a logic diff);
// (2) minor - retail schedules the two zeroing xors (ebp/ebx) AFTER the Tick
// call, cl hoists them into the call-setup latency slots (2-line reorder, not
// source-steerable). Types 9-11 and 21-22 share one tail-merged "Projectile"
// arm across two jump-table slots.
RVA(0x00061cb0, 0x34a)
i32 CGruntFireView::Update() {
    i32 flag = 0;
    if (((CAniAdvanceCursor*)&m_154->m_1a0)->Advance_15c360((i32)g_6bf3bc) == 2) {
        switch (m_toolKind) {
            case 9:
            case 10:
            case 11: {
                CGameObject* spr = g_gameReg->m_world->m_8->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_7c->Init(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_toolKind,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_08 |= 0x10000;
                }
                break;
            }
            case 2: {
                CGameObject* spr = g_gameReg->m_world->m_8->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Boomerang",
                    0x40003
                );
                spr->m_7c->Init(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_toolKind,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_08 |= 0x10000;
                }
                break;
            }
            case 17: {
                i32 pos[2];
                GetSpawnPos(pos);
                CGameObject* spr = g_gameReg->m_world->m_8
                                       ->CreateSprite(0, pos[0], pos[1], 0xf, "TimeBomb", 0x40003);
                spr->m_120 = 0;
                spr->m_7c->Init(spr);
                spr->m_124 = m_tileOwnerHi;
                break;
            }
            case 21:
            case 22: {
                CGameObject* spr = g_gameReg->m_world->m_8->CreateSprite(
                    0,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    "Projectile",
                    0x40003
                );
                spr->m_7c->Init(spr);
                CProjectile* s = (CProjectile*)spr->m_7c->m_logic;
                if (s->LoadProjectileSprites(
                        m_toolKind,
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        m_208,
                        m_20c,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    == 0) {
                    s->m_sprite->m_08 |= 0x10000;
                }
                break;
            }
            default: {
                // melee: hit the grunt at the neighbor grid cell (15-row grid).
                CGrunt* tgt = (CGrunt*)m_tileMgr->m_grid[m_neighborRow + m_neighborCol * 15];
                if (tgt == 0) {
                    flag = 1;
                    break;
                }
                tgt->TakeHit(
                    m_toolKind,
                    m_214,
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_object->m_screenX,
                    m_object->m_screenY,
                    0,
                    m_gruntKind
                );
                i32 t = tgt->m_entranceReason;
                if (t > 0x16) {
                    t = tgt->m_19c;
                }
                if (t == 1 && m_gruntKind != 0x38) {
                    m_tileMgr->CellDispatch(m_tileOwnerHi, m_tileOwnerLo, 0xb, m_neighborCol);
                    return 0;
                }
                break;
            }
        }

        // impact tail (0x61f08)
        m_entranceActive = 1;
        i32 dt = g_buteMgr.GetDword(m_animSetName, "AttackDowntime");
        if (m_gruntKind == 0x3b) {
            dt = 0;
        }
        m_868 = dt;
        m_86c = 0;
        m_860 = (i32)g_645588;
        m_864 = 0;
        m_460 = 0;
        m_3f0 = 0;
        if (m_1c4 != 0) {
            Teardown22de();
        }
        m_combatActive = 0;
    }

    // finish tail (0x61f74)
    CProjRenderObj* r = m_154;
    if ((r->m_1c8 == 0 || r->m_1c0 != 0) && flag == 0) {
        return 0;
    }
    if (m_toolKind == 2) {
        ArmMode(0, 1, 0, 0);
    }
    CGameObject* h = m_object;
    i32 zkey = h->m_screenY + 0x186a0;
    if (h->m_latchedAnimId != zkey) {
        h->m_latchedAnimId = zkey;
        h->m_flags |= 0x20000;
    }
    i32 v220 = m_poweredUp;
    m_entranceActive = 0;
    if (v220 != 0) {
        Teardown3dd7();
        return 0;
    }
    ArmFinish(1, 0, 0);
    return 0;
}
