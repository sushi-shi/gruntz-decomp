# Adding a preceding function can re-color a later COMDAT through MSVC5 TU state
tags: cpp:method cpp:switch | asm:and asm:shl asm:mov | topic:regalloc topic:scheduling topic:tu-layout topic:codegen-idiom
symptoms: an unchanged later function dips after reconstructing an earlier TU sibling; its
  source fingerprint is unchanged, but masks/shifts/table loads or partial-register
  destinations reorder; removing only the preceding definition restores the old bytes
confidence: 9/10
variants: within-tu-order-vs-field-order.md, header-fwd-decl-count-regalloc-butterfly.md

MSVC 5.0 emits each function in its own COMDAT, but that does **not** make every
function's code generation independent of the translation unit's earlier compiler
state. Adding a substantial preceding definition can change scheduling and register
choices in a later, source-identical function. Simple sibling reorder/removal probes may
stay neutral; they do not prove that every predecessor is neutral.

The diagnostic is a controlled A/B compile under identical headers and flags:

1. Confirm the dipped function's source fingerprint did not change.
2. Compile the current TU once with the newly added predecessor and once with only that
   definition absent.
3. Compare the dipped symbol's raw COFF disassembly and relocations. If its bytes change,
   the effect is real compiler state—not objdiff pairing, link order, or an edit inside
   the victim.

```asm
; CDDSurface::ShadeRect loop without the preceding BlitIntoDesc definition
and  ecx,0x1f
and  edi,0xffffffe0
shl  eax,0x6
mov  di,[edx+edi*2+table2]
or   di,[eax+edx+table1]

; same ShadeRect source after BlitIntoDesc (two dense switches) is defined earlier
and  ecx,0xffffffe0
shl  eax,0x6
and  edi,0x1f
mov  ax,[eax+edx+table1]
shl  edi,0x6
or   ax,[edi+edx+table0]
```

STEERABLE only through authentic TU composition. Use the effect in reverse when an
otherwise-correct later function has a scheduling/register residue: restore missing
retail predecessors and their real control-flow shape, in evidence-backed ownership and
order, then remeasure. `gruntz permute variants ... --state-trials` can test whether the
residue belongs to this family. Never add fake functions, padding, declarations, or
reorder proven owners solely to select a compiler state; preserve MAX-fuzzy while the
real predecessor set lands.

A structurally correct predecessor rehome can therefore make an unchanged later method
worse temporarily. Treat that dip as evidence that some other earlier TU content is
still absent or mis-shaped, not as evidence for restoring the false owner. The useful
reverse-search signature is:

1. the victim's per-function source fingerprint is unchanged;
2. the edited functions precede it in the same translation unit;
3. the victim's diff is a localized register-color/scheduling substitution with
   unchanged control flow, constants, calls, and relocations; and
4. the predecessor change is independently supported by RTTI, ILT/xref, or layout
   evidence.

Keep the victim's MAX score, record the structural correction, and use state trials or
the remaining real predecessor backlog to search for the compensating retail compiler
state.

Evidence: adding the retail-backed `CDDSurface::BlitIntoDesc` at `0x0013e2e0` before
source-identical `CDDSurface::ShadeRect` at `0x0013f460` moved the latter's current score
from 68.4455% to 63.6682%. A clean current-header A/B compile, differing only by that
definition, changed two `ShadeRect` color-pack loop regions (mask/shift order, table-load
order, and `ax` versus `di` partial-register destination). This directly falsifies the
former blanket claim that a TU sibling's presence cannot affect another COMDAT's bytes.

Second instance: rehoming the exact `0x411f0`/`0x412c0` methods from the false sibling
`CWormhole` view to their ILT/RTTI/layout-proven `CTeleporter` owner left
`CTeleporter::Update`'s fingerprint at `9b48a6edc775` but moved it from 100% to
98.9103%. Its only diff is the final lookup/call block choosing `ebx/edx` where the
previous compile chose `ecx/eax`; its constants, branches, calls, and relocations are
unchanged. The ownership proof is independent: RTTI makes CTeleporter and CWormhole
sibling leaves, and the supposedly wormhole-typed candidate test uses ILT `0x4039b3`,
which jumps to `CreateTeleporter`, before calling `0x412c0` on the candidate logic
pointer.

Reverse-use instance: restoring the missing exact
`FontRenderer::SetFont` at `0x179c10` before source-identical
`FontRenderer::DrawGlyphRun` at `0x179e70` improved the latter's current score
from 61.2778% to 61.5787%. The victim was not edited; only the authentic
preceding nine-byte setter landed. This does not recover the whole historical
62.0417% MAX, but it proves that draining real predecessor omissions can move a
dipped successor back toward its retail compiler state.

Grunt movement instance: restoring the retail-backed 527-byte
`CGrunt::IsDropReady` at `0x51510` changed two later, source-identical siblings in
opposite directions. `RectContains` at `0x51850` improved from 55.2258% to
61.6774%, while `RectContainsGated` at `0x51a20` moved from its 63.3543% MAX to
61.9764% current. The latter kept the same source fingerprint
(`95c85ebdac20`) and the same rectangle-test CFG, constants, calls, and
relocations; MSVC recolored its prologue, rectangle temporaries, and query
coordinates. This paired improvement/dip is a particularly strong reverse-use
signature: authentic predecessor recovery can move neighboring functions
toward or away from retail independently, so retain each MAX and continue
restoring the real predecessor sequence.
