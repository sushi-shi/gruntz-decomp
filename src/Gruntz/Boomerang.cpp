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
#include <Gruntz/Projectile.h> // real CProjectile base (: CMovingLogic : CUserLogic)
#include <rva.h>

SIZE_UNKNOWN(CBoomerang);
class CBoomerang : public CProjectile {
public:
    CBoomerang(CGameObject* owner);
    // The five slots CBoomerang overrides over CProjectile's vtable (all declared
    // only; their vftable references reloc-mask). slot 0 = scalar-deleting dtor.
    virtual ~CBoomerang() OVERRIDE; // slot 0  (origin CUserBase)
    virtual i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4)
        OVERRIDE;                          // slot 1 (origin CUserBase)
    virtual i32 UserBaseVfunc2() OVERRIDE; // slot 2  (origin CUserBase)
    virtual void Update() OVERRIDE;        // slot 16 (origin CMovingLogic::Update)
    virtual i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1)
        OVERRIDE; // slot 17 (origin CProjectile)
};

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
    m_sprite->m_08 |= 0x2000002;
}

VTBL(CBoomerang, 0x001e792c);
