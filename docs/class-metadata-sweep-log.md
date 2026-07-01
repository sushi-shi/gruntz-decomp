# Class-metadata sweep log

Per-class `SIZE`/`SIZE_UNKNOWN`/`VTBL` annotation sweep (infra: `include/rva.h`,
tests `python -m gruntz.match.class_sizes` + `class_vtables`). Goal: every class
carries a size annotation and every vtabled class a `VTBL` catalog entry —
**per-class verified** because annotating a class in a hot header can reschedule
a neighbour's MSVC5 codegen (measured on the infra proof: `SIZE(CGrunt)` cost
`StepCompassMove` -0.52%). Procedure: annotate in small groups, `gruntz build` +
snapshot-diff every function's fuzzy%; a NEUTRAL group is kept, a REGRESSING
class is reverted and logged here as a hot-header casualty.

This file records the SKIPPED classes (hot-header casualties + un-catalogable
vtables) so the next sweep `@early-stop`s on recognition instead of re-grinding.

---

## Net module — pilot (2026-07-01)

Scope: classes defined in `include/Net/*.h` + `src/Net/*.cpp` (CNetMgr already
had SIZE_UNKNOWN+VTBL from the infra proof; skipped).

Coverage delta (whole-tree counters, Net worklist drained):
- SIZE: 55/3264 → 118/3264 annotated names (**63 Net classes annotated**; Net
  SIZE violators 63 → 0).
- VTBL: Net vtable violators 5 → 3 (2 catalogued, 3 skipped below).

Annotated: **57 SIZE_UNKNOWN + 6 SIZE(exact) + 2 VTBL**.
- Exact `SIZE`: `CNetCmdSlot2`(0x64), `CNetResyncEntry`(0x410),
  `CNetSession2`(0x20bb0), `CNetJoinPacket`(0x28), `CNetConfigBlob`(0x11c),
  `CMsgPacket`(0x38), `CNetCmdSlot`(0x64), `CNetCmdBuf`(0x238),
  `CNetSession`(0x20bb0) — array-element strides, RE'd fixed packets, or a
  byte-matched struct-copy prove these are the *complete* retail object size.
  (Note: `SIZE(T,N)` asserts `sizeof(myStruct)==N`, i.e. it is self-consistent —
  it only encodes a real retail size when the modeled struct is COMPLETE. Every
  partial pad-to-last-touched-field view therefore takes `SIZE_UNKNOWN`.)
- `VTBL`: `CNetPlayerListNode`(0x5f0760), `CNetSessionNode`(0x5f0778) — own
  most-derived vtables, real-polymorphic in NetSessionNode.cpp; those RVAs were
  not yet named in symbol_names.csv, so the catalog rows are new (no collision).

### Hot-header casualties: NONE.
All 63 SIZE + 2 VTBL annotations were matching-NEUTRAL: three build groups
(A = all `src/Net/*.cpp` local classes + LatencyList.h; B1 = `NetMgr.h`
110–347; B2 = `NetMgr.h` 422–962), each snapshot-diffed across all 3394
functions → **0 REGRESS, 0 IMPROVE** every time. No class annotation had to be
reverted. `NetMgr.h` is a genuinely hot header (included by every Net TU plus
several Gruntz TUs — CMulti/Dialogs/NetGameDlg), yet the no-code `char[1]`
typedef the macros lower to under MSVC5 perturbed nothing here.

### VTBL skipped (un-catalogable, NOT casualties — logged for the final sweep):
- `CNetNodeBase` (NetSessionNode.cpp) and `InterfaceObjectBase`
  (InterfaceObject.cpp): base subobjects whose transient construction vtable is
  the SHARED CObject-base dtor vtable **0x5e8cb4**, already catalogued as
  `?g_severusBaseDtorVtbl@@3PAXA`. A `VTBL(...,0x5e8cb4)` would collide on that
  rva (dup-DATA guard) and mis-attribute one shared vtable to a modeling-base
  name. These are the KEEP-HAND-ROLLED / shared-base-vtable exception.
- `CNetPlayerObj` (NetMgr.h): a slot-dispatch modeling view — its virtuals are
  declared-but-undefined so cl emits no vtable in this TU, and its concrete
  retail vtable RVA is not confidently pinned. Left un-catalogued (no wrong RVA
  guessed) pending the matcher that pins its concrete class.

---

## Scale recommendation (pilot → full 3160 SIZE / 313 VTBL worklist)

The per-class-verify sweep is **efficient enough to scale**, with this shape:

1. **SIZE_UNKNOWN is the bulk, scalable default.** ~90% of classes are partial
   pad-to-last-touched-field views whose modeled `sizeof` is NOT the retail size;
   `SIZE_UNKNOWN` (completeness-only) is the correct, zero-risk annotation and
   the only one a bulk fan-out can apply without per-class RE. Exact `SIZE` is an
   opportunistic upgrade the owning matcher adds when the object is provably
   complete (array-element stride, RE'd fixed packet, byte-matched struct-copy) —
   do NOT try to auto-derive exact sizes in a fan-out (wrong number silently
   passes the self-consistent assert).
2. **The neutrality risk is real but rare; batch by header, not by class.** A
   `.cpp`-local class only touches its own TU (do them in one big group). A
   header class touches every includer — do a header in ≤2 line-range groups and
   snapshot-diff. In this pilot NONE regressed, so a per-header (not per-class)
   verify granularity is enough; only split a header further if its group shows a
   REGRESS.
3. **Trust the tooling for the gate.** `gruntz build`'s "regressions vs baseline"
   plus a report.json fuzzy% snapshot-diff (all functions) catches both the
   auto-check's cases and the clangd-fallback-hashed edits. One snapshot-diff per
   build group is the whole verification cost.
4. **Duplicate class NAMES across TUs are common** (a full view in the header +
   a partial same-named view in a `.cpp`). The tests dedupe by name, so annotate
   ONCE at any complete definition; annotating the complete one lets you use
   exact `SIZE`.

Estimated fan-out: one worker per module (or per hot header + its `.cpp` set),
each draining its SIZE + VTBL violators in ~2–3 build groups. Budget per worker
is dominated by reads/edits, not builds (3 builds sufficed for 63+2 here).
