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
// L_8860 - PROVEN to be ??1CUserLogic itself (??_7CUserLogic @0x1e705c slot 0 -> ILT
// thunk 0x3cfb -> sdd 0x8a10 -> this dtor 0x8860). The fold is BLOCKED, not unproven:
// ~CUserLogic is inline in <Gruntz/UserLogic.h> (load-bearing - ~50 leaf dtors fold it),
// so the out-of-line COMDAT can only be pinned by @rva-symbol in a TU whose obj EMITS it,
// and our WorldSoundSet TU does not odr-use CUserLogic yet (retail's did - the whole
// CUserBase/CUserLogic base-COMDAT pool 0x87d0-0x8b50 lands in that obj). Reconstructing
// that TU's CUserLogic user unblocks the fold; until then the placeholder keeps the body
// matched (100%) rather than dropping it. @identity-TODO: L_8860 == CUserLogic.

class L_8860 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    virtual ~L_8860() OVERRIDE;
};
SIZE_UNKNOWN(L_8860);
RELOC_VTBL(L_8860, 0x001e705c); // IS ~CUserLogic (see above) - fold blocked on the emitter

// L_13400 - a REAL, distinct CUserLogic leaf: its own vtable is ??_7L_13400 @0x1e72b4
// (slot 0 -> sdd 0x133d0 -> the dtor 0x13400; slot 1 = 0xb4c40; slot 2 = the GetTypeTag
// at 0x133b0 returning LOGIC_VOICETRIGGER 0x426). @identity-TODO: that type tag says the
// class is CVoiceTrigger, but CVoiceTrigger's own out-of-line dtor is already matched at
// 0x135a0 with vtable 0x1e885c - i.e. ONE of the two bindings is wrong and the conflict
// needs the CUFO/CVoiceTrigger/CPathHazard leaf-vtable family re-audited as a whole (see
// the GruntVoice.cpp note). The vtable rva below is binary-proven either way.
class L_13400 : public CUserLogic {
public:
    TILE_LOGIC_TAIL
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
