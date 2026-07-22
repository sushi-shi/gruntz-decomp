#ifndef GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H
#define GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic / CUserBase base hierarchy

// Leaf destructors (the /GX leaf-dtor archetype; byte-identical to ~CSimpleAnimation,
// the only per-class difference being the reloc-masked EH funcinfo table).
//
// (L_8860 is DISSOLVED, 2026-07-17: it was ??1CUserLogic's out-of-line COMDAT
// (??_7CUserLogic @0x1e705c slot 0 -> ILT thunk 0x3cfb -> sdd 0x8a10 -> 0x8860).
// The old emitter-blocker is dead: after the CWapX conversion the leaf ctor/dtor
// funclets odr-use ~CUserLogic out-of-line, so the COMDAT is emitted and the body
// is pinned by RVA_COMPGEN in src/Gruntz/ActionArea.cpp.)

// (L_13400 is GONE - the audit's own verdict stood: it IS CUFO (RTTI names 0x1e72b4
// ??_7CUFO). The dtor is CUFO's IMPLICIT compiler-generated one (the elision that
// matches retail's flat CUserBase-only stamp), force-emitted by the RealizeCUFO
// device + RVA_COMPGEN pin in GruntVoice.cpp.)

#endif // GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H
