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

Evidence: adding the retail-backed `CDDSurface::BlitIntoDesc` at `0x0013e2e0` before
source-identical `CDDSurface::ShadeRect` at `0x0013f460` moved the latter's current score
from 68.4455% to 63.6682%. A clean current-header A/B compile, differing only by that
definition, changed two `ShadeRect` color-pack loop regions (mask/shift order, table-load
order, and `ax` versus `di` partial-register destination). This directly falsifies the
former blanket claim that a TU sibling's presence cannot affect another COMDAT's bytes.
