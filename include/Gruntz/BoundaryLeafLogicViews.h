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

// L_13400 - a REAL, distinct CUserLogic leaf: its own vtable is ??_7L_13400 @0x1e72b4
// (slot 0 -> sdd 0x133d0 -> the dtor 0x13400; slot 1 = 0xb4c40; slot 2 = the GetTypeTag
// at 0x133b0 returning LOGIC_VOICETRIGGER 0x426). @identity-TODO: that type tag says the
// class is CVoiceTrigger, but CVoiceTrigger's own out-of-line dtor is already matched at
// 0x135a0 with vtable 0x1e885c - i.e. ONE of the two bindings is wrong and the conflict
// needs the CUFO/CVoiceTrigger/CPathHazard leaf-vtable family re-audited as a whole (see
// the GruntVoice.cpp note). The vtable rva below is binary-proven either way.
// NOT converted to the CWapX second base (MI1, 2026-07-17), deliberately: this
// class's IDENTITY is unresolved (see the @identity-TODO above), and CWapX is a
// per-CLASS RTTI fact - asserting it on a shell whose real class is unknown would
// be a fabrication. The +0x34..0x3c triple stays flat, exactly as the ex-TILE_LOGIC_TAIL
// spelled it, and the dtor stays an explicit body (nothing constructs an L_13400, so
// no TU emits its vtable/??_G - an implicit dtor would have no emitter to pin).
// Resolve the identity first (the CUFO/CVoiceTrigger/CPathHazard vtable-family audit);
// the CWapX conversion follows for free once the real class is named.
class L_13400 : public CUserLogic {
public:
    CGameObject* m_34;   // +0x34  (ex TILE_LOGIC_TAIL)
    CGameObject* m_38;   // +0x38
    AnimWorkerObj* m_3c; // +0x3c
public:
    virtual ~L_13400() OVERRIDE;
};
SIZE_UNKNOWN(L_13400);
// Its OWN vtable is 0x1e72b4 (slot0 -> sdd 0x133d0 -> the dtor 0x13400) and the audit
// resolves that rva's RTTI name to CUFO - confirming the GruntVoice.cpp note. It stays
// RELOC_VTBL (not VTBL) because binding it would force the CUFO rename + its 9 own slots,
// and the real CUFO:CPathHazard:CUserLogic model is BYTE-PROVEN to crater this dtor to
// 4.7% (MSVC5 /O2 collapses the chain to a flat CUserLogic teardown). @identity-TODO:
// L_13400 == CUFO; the fold needs Ufo.h remodeled as a flat leaf.
RELOC_VTBL(L_13400, 0x001e72b4); // its OWN vtable (== ??_7CUFO; see above)

#endif // GRUNTZ_BOUNDARYLEAFLOGICVIEWS_H
