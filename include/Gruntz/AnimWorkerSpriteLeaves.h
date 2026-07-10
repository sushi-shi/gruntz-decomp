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
// These live in a dedicated header (not per-.cpp views) because the canonical class
// headers (GruntStaminaSprite.h / GruntWingzTimeSprite.h) pull the Grunt.h world, which
// cannot coexist with the canonical UserLogic.h this pump TU already uses (the documented
// CUserLogic true-0x30 vs fat-0x40 two-world ODR split). Only the sizeof (the retail
// operator-new argument) + the ctor call are load-bearing.
#ifndef GRUNTZ_ANIMWORKERSPRITELEAVES_H
#define GRUNTZ_ANIMWORKERSPRITELEAVES_H

#include <Gruntz/UserLogic.h> // CUserLogic base + TILE_LOGIC_TAIL

// The owning game object handed to each handler (defined in the pump TU); the leaf ctors
// take it by pointer (the ctor is reloc-masked, so the exact arg type is not load-bearing).
struct Owner;

// The selected-grunt highlight sprite and the toy-in-hand sprite: real CUserLogic
// leaves whose most-derived 1-arg ctors (0x07e3e0 / 0x07f350) are matched elsewhere;
// modeled here as size-views for the `new T(owner)` size + ctor target only.
struct CGruntSelectedSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntSelectedSprite(Owner* owner); // 0x07e3e0
    char m_body[0x5c - 0x40];
}; // sizeof = 0x5c

struct CGruntToySprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntToySprite(Owner* owner); // 0x07f350
    char m_body[0x60 - 0x40];
}; // sizeof = 0x60

// The HUD-timer leaves derive CGruntHealthSprite (RTTI); it derives CUserLogic. The
// timer leaves add no data of their own (0x64, same as the base), so they carry no tail.
struct CGruntHealthSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntHealthSprite(Owner* owner); // 0x07eb00
    char m_body[0x64 - 0x40];
}; // sizeof = 0x64

struct CGruntStaminaSprite : public CGruntHealthSprite {
    CGruntStaminaSprite(Owner* owner); // 0x07fae0
}; // sizeof = 0x64

struct CGruntToyTimeSprite : public CGruntHealthSprite {
    CGruntToyTimeSprite(Owner* owner); // 0x07fbd0
}; // sizeof = 0x64

struct CGruntWingzTimeSprite : public CGruntHealthSprite {
    CGruntWingzTimeSprite(Owner* owner); // 0x07fcc0
}; // sizeof = 0x64

struct CGruntPowerupSprite : public CUserLogic {
    TILE_LOGIC_TAIL
    CGruntPowerupSprite(Owner* owner); // 0x07fdb0
    char m_body[0x60 - 0x40];
}; // sizeof = 0x60

#endif // GRUNTZ_ANIMWORKERSPRITELEAVES_H
