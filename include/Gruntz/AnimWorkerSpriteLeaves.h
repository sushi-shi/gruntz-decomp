// AnimWorkerSpriteLeaves.h - thin size-views of the grunt-indicator / HUD sprite leaves
// the anim-worker message pump (AnimWorkerHandlers.cpp) `new`s on worker-state 0. Each is
// a real CUserLogic-derived game object whose most-derived 1-arg ctor is matched in
// UserLogic.cpp / GameObjectCtors.cpp; modeled here as a size-view (CUserLogic base + the
// tile-logic tail + an m_body tail) so `new T(owner)` lowers to the exact retail
// `push sizeof(T); call operator new; mov ecx,raw; push owner; call <ctor>` (the ctor
// reloc-masked). The created object is stowed as a plain CUserLogic* and dispatched
// through the inherited 16-slot vtable, so the leaf type is used ONLY for the new-size +
// ctor target.
//
// @identity-TODO THE "CANNOT COEXIST" WALL BELOW IS FALSE - FALSIFIED 2026-07-13, DO NOT
// RE-INHERIT IT. The note used to read: "these live in a dedicated header because the
// canonical class headers (GruntStaminaSprite.h / GruntWingzTimeSprite.h) pull the Grunt.h
// world, which cannot coexist with the canonical UserLogic.h this pump TU already uses (the
// documented CUserLogic true-0x30 vs fat-0x40 two-world ODR split)."
//
// Two independent refutations:
//   1. Grunt.h and UserLogic.h define NO class in common, so no C2011 redefinition can
//      arise between them (a C2011 needs the SAME class defined twice).
//   2. Decisive: a TU that includes <Mfc.h> + <Gruntz/UserLogic.h> +
//      <Gruntz/GruntStaminaSprite.h> + <Gruntz/GruntWingzTimeSprite.h> COMPILES CLEAN under
//      the real MSVC 5.0 (cl /c /O2 /GX produced the .obj, zero diagnostics).
//
// So these six size-views are unnecessary and should be DISSOLVED onto the canonical
// classes: include the real headers in the pump TUs and delete this file. The one thing a
// folder must check is that each canonical class's sizeof still equals the retail
// operator-new immediate recorded here (0x5c / 0x60 / 0x64 / ...) - if it does not, that is
// a REAL layout bug in the canonical class (run gruntz.analysis.stale_walls), not a reason
// to keep the view. Not folded here only for want of budget, NOT because of a wall.
#ifndef GRUNTZ_ANIMWORKERSPRITELEAVES_H
#define GRUNTZ_ANIMWORKERSPRITELEAVES_H

#include <Gruntz/UserLogic.h> // CUserLogic base + TILE_LOGIC_TAIL + real CGameObject

// The leaf ctors take the created object's owning CGameObject* (the retail 1-arg ctor
// signature is ??0<leaf>@@QAE@PAUCGameObject@@@Z); binding to the real struct CGameObject
// (from UserLogic.h) makes the `new T((CGameObject*)owner)` call reloc-tie to the real
// most-derived ctor RVA. Only the sizeof + the ctor target are load-bearing.

// The selected-grunt highlight sprite and the toy-in-hand sprite: real CUserLogic
// leaves whose most-derived 1-arg ctors (0x07e3e0 / 0x07f350) are matched elsewhere;
// modeled here as size-views for the `new T(owner)` size + ctor target only.
struct CGruntSelectedSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntSelectedSprite(CGameObject* obj); // 0x07e3e0
    char m_body[0x5c - 0x40];
}; // sizeof = 0x5c

struct CGruntToySprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntToySprite(CGameObject* obj); // 0x07f350
    char m_body[0x60 - 0x40];
}; // sizeof = 0x60

// The HUD-timer leaves derive CGruntHealthSprite (RTTI); it derives CUserLogic. The
// timer leaves add no data of their own (0x64, same as the base), so they carry no tail.
struct CGruntHealthSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntHealthSprite(CGameObject* obj); // 0x07eb00
    char m_body[0x64 - 0x40];
}; // sizeof = 0x64

struct CGruntStaminaSprite : public CGruntHealthSprite {
    CGruntStaminaSprite(CGameObject* obj); // 0x07fae0
}; // sizeof = 0x64

struct CGruntToyTimeSprite : public CGruntHealthSprite {
    CGruntToyTimeSprite(CGameObject* obj); // 0x07fbd0
}; // sizeof = 0x64

struct CGruntWingzTimeSprite : public CGruntHealthSprite {
    CGruntWingzTimeSprite(CGameObject* obj); // 0x07fcc0
}; // sizeof = 0x64

struct CGruntPowerupSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntPowerupSprite(CGameObject* obj); // 0x07fdb0
    char m_body[0x60 - 0x40];
}; // sizeof = 0x60

#endif // GRUNTZ_ANIMWORKERSPRITELEAVES_H
