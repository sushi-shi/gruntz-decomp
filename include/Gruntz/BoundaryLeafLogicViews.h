// BoundaryLeafLogicViews.h - the placeholder CUserLogic leaf classes reconstructed in
// BoundaryLeafLogic.cpp (tile-logic leaf destructors + Serialize overrides recovered
// from the engine_boundary backlog).
//
// RTTI cannot attribute these COMDAT-folded leaf methods, so the leaf class names are
// placeholders (L_<rva> / S_<rva>); the recovered FACT is that each is a CUserLogic
// leaf (the shared game-object hierarchy) - only the inheritance chain + offsets are
// load-bearing. Formerly declared inline per-TU; consolidating the class declarations
// into this shared header is pure code motion (matching-neutral). The bodies (each
// carrying its RVA()) stay in BoundaryLeafLogic.cpp.
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
// is pinned by @rva-symbol in src/Gruntz/ActionArea.cpp.)

// (L_13400 is GONE - the audit's own verdict stood: it IS CUFO (RTTI names 0x1e72b4
// ??_7CUFO). The dtor is CUFO's IMPLICIT compiler-generated one (the elision that
// matches retail's flat CUserBase-only stamp), force-emitted by the RealizeCUFO
// device + @rva-symbol pin in GruntVoice.cpp. The last RELOC_VTBL dies with it.)

#endif // GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H
