#include <rva.h>
// CDroppedObjectShadow.cpp - engine-label stubs for CDroppedObjectShadow.

#include <Stub/CDroppedObjectShadow.h>

// @confidence: high
// @source: decomp-xref
// @stub
// TRACE MIS-ATTRIBUTION (final-sweep re-home target): 0xc62e0 is NOT a member of
// CDroppedObjectShadow (RTTI sizeof 0x54) - it reads this+0x58/0x60/0x68 (doubles),
// this+0x70..0x94 (a dir-pair, a cached tile pair, a flag, and a 64-bit re-arm
// timer {+0x88,+0x8c} vs {+0x90,+0x94}), i.e. a class well past 0x54. It is a
// per-frame scroll/drift Update (not "LoadAttributes"): when the re-arm timer
// (clock = g_645588) expires it re-probes a random destination tile in the wander
// box (mgr->m_68 probe 0x4032ce), spawns/updates a "DroppedObjectShadow" via
// 0x5597b0, and re-reads the "Hazardz/ObjectDropperDelay" bute (g_buteMgr.GetInt,
// default 1000) into the timer duration; then it x87-drifts the double position
// m_60/m_68 by g_645584*m_58 each frame, clamping/wrapping at the world tile
// bounds (mgr->m_30->m_24->m_5c->m_30/0x34) and writing the rounded coords back to
// m_10->m_5c/m_60 via _ftol (0x51f570). Reconstructing it needs its REAL (larger)
// owner class resolved + faithful x87 fcompp/fnstsw compare + 64-bit-timer codegen
// (heavy iteration); left @stub to avoid forcing a divergent partial into the
// wrong 0x54 layout (per matcher instruction). Deferred to the final sweep.
RVA(0x000c62e0, 0x2dd)
void CDroppedObjectShadow::LoadAttributes() {}

// CDroppedObjectShadow(CGameObject*) (0xc7490) reconstructed in
// src/Gruntz/CDroppedObjectShadow.cpp (@early-stop, EH-state wall).
