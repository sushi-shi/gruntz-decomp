// Boomerang.cpp - the CBoomerang projectile ctor (its own TU; no other CBoomerang
// method is reconstructed yet, so this is the class's first/real TU). Re-homed from
// src/Stub/Boomerang.cpp.
//
// Object-ctor archetype (no EH): forward the single by-value ctor arg to the real
// CProjectile base ctor (??0CProjectile@@QAE@PAUCGameObject@@@Z @0xdec60, engine fn,
// not matched -> declared-only, reloc-masked rel32 call via thunk 0x37d8), the
// compiler stamps the derived vftable into [this] (implicit, real polymorphic
// class), then OR a flag bit into the inherited +0x154 render object's [+8] dword.
//
// CBoomerang : CProjectile (RTTI-proven, vftable 0x5e792c; vtable_hierarchy --tree).
// The real base is the fully-modeled CProjectile (<Gruntz/Projectile.h>); cl emits
// ??_7CBoomerang@@6B@ from CProjectile's 18-slot vtable with CBoomerang's five
// overrides applied (slots 0/1/2/16/17 - all declared-only, reloc-masked), plus the
// implicit post-base-ctor vptr stamp. The base ctor stays DECLARED only (out-of-line;
// its `call` reloc-masks to 0xdec60 via thunk 0x37d8). Replaces the old fabricated
// `CProjectileBase` stand-in.
#include <Gruntz/Boomerang.h> // CBoomerang : CProjectile (+return-trajectory fields, sizeof 0x260)
#include <Gruntz/Grunt.h>     // CGrunt (launcher grunt return-record) + CGruntArchive
#include <Gruntz/GameRegistry.h> // g_gameReg (m_world gate, m_cmdGrid launcher-cell grid)
#include <Globals.h>             // g_coordPool.m_freeHead, g_coordPool.m_linkOffset, g_frameTime
#include <rva.h>
#include <Io/FileMem.h> // CFileMemBase - the CGruntArchive stream (Read/Write dispatch)

// pi - the boomerang's circular-flight phase constant. Owned here (Boomerang is its
// sole referencer and 0x1eaae8 falls in this TU's data band); <Globals.h> keeps the
// reference extern. Re-homed out of the generated src/Globals.cpp (2026-07-14).
DATA(0x001eaae8)
const double g_projPhase0 = 3.1415927;

// @confidence: high
// @source: rtti-vptr
// @early-stop
// vptr-store-schedule wall (~97.3%): retail emits the implicit vptr store BEFORE
// the m_sprite (+0x154) load; MSVC5 /O2 fills the post-base-call latency slot with
// the independent m_sprite load and sinks the store after it. A real polymorphic
// class (implicit vptr init), so the store is no longer source-orderable - the
// schedule is the optimizer's, not steerable. Byte-exact otherwise.
RVA(0x000e0650, 0x2b)
CBoomerang::CBoomerang(CGameObject* owner) : CProjectile(owner) {
    // vptr stamp is IMPLICIT (real polymorphic class).
    m_sprite->m_flags |= 0x2000002;
}

// The game registry singleton (the m_world resource gate + m_cmdGrid launcher grid).
extern "C" CGameRegistry* g_gameReg;

// Shared free-list globals (recycle the launcher grunt's occupied-coord payloads);
// DATA-pinned in Projectile.cpp, so extern-only here (no duplicate DATA binding).
#include <Gruntz/FreeNodePool.h> // the coord-node pool object @0x645540
// The pool's INTERIOR FIELDS - m_freeHead (+0x04) and m_linkOffset (+0x0c) - used to be
// declared here as the standalone globals g_coordPool.m_freeHead / g_coordPool.m_linkOffset. They are not
// globals: they are fields of g_coordPool (DEFINED in src/Gruntz/GameText.cpp), which is
// why the free-list push/pop code reads exactly [pool+4] and [pool+0xc].
extern "C" u32 g_frameTime; // 0x245588  running game clock (return-record base)

// The boomerang return-trajectory constants (.rdata doubles). DATA-pinned here (the
// only referencing TU) so the fmul/fdivr loads reloc-mask against the named symbols.
DATA(0x001eaad8)
extern const double g_boomHalf = 0.5; // midpoint scale (0.5)
DATA(0x001eaae0)
extern const double g_boomTimeScale = 0.03125;
DATA(0x001eaaf0)
extern const double g_boomRetC3 = 0.0625;
DATA(0x001eaaf8)
extern const double g_boomRetC4 = -500.0;

// CBoomerang::LoadProjectileSprites @0xe0690 - vtable slot 17. Forward to the base
// CProjectile loader (bail on failure); then compute the boomerang RETURN trajectory:
// the arc origin is the midpoint of the launch (owner) and target positions, the
// direction is the half-vector to it, the velocity scale derives from the phase-0
// constant over (timePerTile * flightDist), and the phase is reset to 0. Finally, the
// launcher grunt (grid cell (a,b) of the command grid) is stamped with a return-timer
// record (m_278 = clock, m_280 = return time) and its occupied-coord list is recycled
// onto the global free-list + cleared. m_launched is reset to 0.
//
// @early-stop
// x87-scheduling wall (same family as CProjectile::LoadProjectileSprites ~58% /
// StepMotion / MovingSlot16): the base forward, the integer grunt-cell lookup + the
// return-record stamp + the free-list recycle/RemoveAll are byte-shaped; the residue is
// the dense fxch-laden x87 stack schedule of the origin/dir/velScale trajectory block
// (0xe06e3..0xe0793) which MSVC5 emits from the trajectory expression and is not
// steerable from C source, plus the unnamed engine-call relocs. Logic complete; parked.
RVA(0x000e0690, 0x1a9)
i32 CBoomerang::LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1) {
    if (CProjectile::LoadProjectileSprites(kind, a, b, sx, sy, t0, t1) == 0) {
        return 0;
    }
    CGameObject* owner = m_object;
    m_launchX = owner->m_screenX;
    m_launchY = owner->m_screenY;
    double d = g_projPhase0 / ((double)(u32)m_timePerTile * g_boomTimeScale * m_flightDist);
    double originX = ((double)m_targetX + (double)owner->m_screenX) * g_boomHalf;
    double originY = ((double)m_targetY + (double)owner->m_screenY) * g_boomHalf;
    m_originX = originX;
    m_originY = originY;
    m_dirX = originX - (double)owner->m_screenX;
    m_dirY = originY - (double)owner->m_screenY;
    m_phase = 0.0;
    m_velScale = d;
    CGrunt* g = *(CGrunt**)((char*)g_gameReg->m_cmdGrid + (15 * a + b) * 4 + 0x1c);
    if (g != 0) {
        g->m_280 = (i32)(d * m_flightDist * g_boomRetC3 - g_boomRetC4);
        g->m_284 = 0;
        g->m_278 = g_frameTime;
        g->m_27c = 0;
        if (g->CoordCount() != 0) {
            GruntCoordNode* n = g->CoordHead();
            while (n != 0) {
                GruntCoordNode* next = n->m_next;
                GruntCoord* data = n->m_coord;
                if (data != 0) {
                    void** p = (void**)((char*)data - g_coordPool.m_linkOffset);
                    *p = g_coordPool.m_freeHead;
                    g_coordPool.m_freeHead = p;
                }
                n = next;
            }
            g->m_31c.RemoveAll();
        }
    }
    m_launched = 0;
    return 1;
}

// CBoomerang::SerializeMove @0xe15d0 - vtable slot 1. Bail unless the resource
// manager is loaded (g_gameReg->m_world); round-trip the eight boomerang return-
// trajectory fields (launch pos, dir, origin, phase, launched flag) through the
// archive stream (mode 4 = Write @+0x30, mode 7 = Read @+0x2c), then chain the base
// CProjectile serialize and booleanize its result.
// @early-stop
// ~97%: logic byte-correct; residue is a minor callee-saved register-coloring
// difference in the field round-trip (same regalloc family as CTimeBomb::SerializeMove).
RVA(0x000e15d0, 0x155)
i32 CBoomerang::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    if (mode == 4) {
        ar->Write(&m_launchX, 4);
        ar->Write(&m_launchY, 4);
        ar->Write(&m_dirX, 8);
        ar->Write(&m_dirY, 8);
        ar->Write(&m_originX, 8);
        ar->Write(&m_originY, 8);
        ar->Write(&m_phase, 8);
        ar->Write(&m_launched, 4);
    } else if (mode == 7) {
        ar->Read(&m_launchX, 4);
        ar->Read(&m_launchY, 4);
        ar->Read(&m_dirX, 8);
        ar->Read(&m_dirY, 8);
        ar->Read(&m_originX, 8);
        ar->Read(&m_originY, 8);
        ar->Read(&m_phase, 8);
        ar->Read(&m_launched, 4);
    }
    return CProjectile::SerializeMove(ar, mode, a3, a4) ? 1 : 0;
}

// CBoomerang::MovingSlot16 (slot 16 @0xe08b0, the boomerang motion step) is defined
// out-of-line in Projectile.cpp (interleaved in the CProjectile .text band, and it
// shares g_frameDelta/g_projPhase* with CProjectile::MovingSlot16 there). SIZE + VTBL
// live in <Gruntz/Boomerang.h>.
