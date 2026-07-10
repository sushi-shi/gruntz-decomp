// DiscoveredEh.cpp - trace-discovered /GX leaf destructors re-homed from
// src/Stub/Discovered.cpp (engine_discovered is base-profile; these dtors carry a
// C++ exception-handling frame, so they need the eh profile). Only OFFSETS + code
// bytes are load-bearing.
//
// Item-7 owner audit (sema xref):
//  - ~CU55 (0x16460): its only caller is its own deleting-dtor
//    (0x16430, vtable slot); RTTI name unrecovered (Ghidra ClassUnknown_55_016460).
//    It is a byte-identical twin of ~CImgHolder (0x16500, Dialogs.cpp) - a dialog
//    image-holder freeing an embedded CImageList - so it belongs to the dialog
//    cluster, but no real class name is recoverable; kept here with this note.
//  - ~CButeStore (0x174d70): the real class CButeStore has a TU (src/Bute/
//    ButeStoreClear.cpp, CButeStore::ClearRecursive). It is NOT folded there because
//    the two CButeStore models diverge: ButeStoreClear.cpp uses <Bute/ButeMgr.h>'s
//    FLAT CButeStore, while this dtor is multiple-inheritance-modeled (CContainerErr
//    @+0 + CObj50 @+8, two vptr re-stamps + two base dtors). Folding requires
//    reconciling the two models (extend ButeMgr.h's CButeStore to MI) - deferred to
//    a depth pass so it can't regress ButeStoreClear.cpp; kept here meanwhile.
#include <Ints.h>
#include <rva.h>

// (tomalla-55 @0x016460 re-homed to src/Gruntz/Dialogs.cpp as CImgHolder2 - a SECOND
// byte-identical dialog image-holder, dissolved onto that TU's existing CImgHolderBase
// grand-base + shared CImageList shim (twin of the ~CImgHolder @0x16500 already there).)

// (CButeStore @0x174d70 - the multiple-inheritance /GX dtor - re-homed to
// src/Bute/ButeNode.cpp (its RVA neighborhood, next to CButeCfgNode174d @0x174d00),
// keeping its CContainerErr @+0 / CObj50 @+8 MI model - which cannot fold into
// ButeStoreClear.cpp's flat <Bute/ButeMgr.h> CButeStore without reconciling the two
// models; that fold is deferred.)


// --- vtable catalog (reduced-view classes share their base vtable rva) ---
