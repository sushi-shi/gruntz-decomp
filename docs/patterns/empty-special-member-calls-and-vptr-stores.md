# Empty special members retain distinct call and vptr boundaries

tags: cpp:constructor cpp:destructor cpp:implicit cpp:inline | asm:call asm:mov asm:coff | topic:compiler-artifact topic:source-model

An empty source body is a candidate for implicit generation, not its proof.
Inspect the initializer list, the complete base/member family, ordinary callers,
and EH funclets. Compile omission in the real owner and caller TUs. A standalone
body's equivalence does not establish an equivalent caller boundary.

## Constructor control

At `8c76e8b5f`, nine trigger-logic constructors each occupy eighteen retail bytes:
one base-constructor call, the derived vptr store, and the return sequence. Their
empty authored bodies are out of line in `tileswitchlogic`. Removing both the
declarations and definitions causes VC5 to expand implicit construction in
`tiletriggercontainer`; neither object defines or references any of the nine
constructor symbols. Four fully decoded retail callers contain eighteen calls
through independently resolved linker thunks:

| Retail caller | Constructor callees |
| --- | --- |
| 0x115f60 | 0x111f10, 0x112050, 0x112790, 0x1127c0, 0x1127f0 |
| 0x116610 | 0x112760, 0x112240, 0x112270 |
| 0x116cf0 | 0x112210 |
| 0x117800 | All nine above |

The same control removes the `CFaderMesh`, `SoundStream`, `CGruntzApp`, and
`CGruntzWnd` constructors from all seven actual owner/caller objects. Retail
calls them at 0x17e072, 0x155c34, 0x11c9bb, and 0x809d2 respectively. Keep the
authored boundary in this recovered family. This experiment does not justify
inventing a tag overload or claiming every mixed call/expansion family requires
two source entities.

## Destructor control

Five surface destructors support omission. The four `CDDSurface` derivatives
at 0x142360/0x142820/0x142a40/0x142d40 each retain the exact 83-byte body, including
EH setup, the base vptr, `FreeSurfaces`, and `CPtrArray` destruction. The 25-byte
`CDDrawFrontSurface` destructor at 0x1591b0 retains its base field resets and
`CObject` vptr. All five before/after bodies and their ordered symbolic
relocations agree, and the real compiler emits them naturally. Their manual
declarations and bodies are removed.

Three simultaneous negative controls distinguish the relevant vptr:

| Destructor | Authored / retail | Implicit | First missing operation |
| --- | ---: | ---: | --- |
| `CFaderMesh`, 0x17e990 | 107 bytes | 101 bytes | Derived vptr store before member-array teardown. |
| `CFaderSine`, 0x17fdf0 | 11 bytes | 5 bytes | Derived vptr store before the base tail jump. |
| `SoundStream`, 0x137710 | 11 bytes | 5 bytes | Derived vptr store before the base tail jump. |

Restoring these three authored empty destructors restores the missing layer.
Do not transcribe a vptr store or move base cleanup to retain it. This extends
the [sound-instance control](typed-container-use-replaces-manual-compiler-methods.md).

Surviving source is a separate positive control: the concrete nested
`CRezMgr::CRezItmChunkList` in pinned `845119c` has no destructor declaration.
Removing the reconstruction's empty body lets its authored `CLTBaseList` base
generate the one-byte destructor at 0x13abb0. A source-proven concrete class can
still contain a manually reconstructed compiler method.

Conversely, a record containing only scalars, pointers, POD arrays and trivial
bases has no nontrivial subobject teardown to generate that one-byte function.
An authored inline empty destructor can disappear at ordinary call sites while
remaining an EH funclet target; see the
[WwdRegion/WwdDirtyRect evidence](eh-funclet-names-the-type.md). This is distinct
from declaring the destructor implicit.

## Review and reverse use

The complete queue and per-method fingerprints live in the
[compiler-method audit](../compiler-method-audit.md). Start with the full
initializer and owner family, test the candidate omission, and compare both
ordinary and EH callers. Keep separate dispositions for authored source,
implicit methods, recovered templates, and unresolved owner conflicts.

Removing the six unnecessary declarations also perturbs unrelated VC5 callers.
All thirteen fresh MAX-gate deltas have the same calls and branch/return
skeletons. For example, `CSBI_MenuItem::Render` changes from 69 to 68 bytes,
with 28 instructions, one call, two branches and one return on both sides;
allocation/scheduling starts to differ at +0x8. This is a measured declaration
context effect, not evidence of altered renderer logic. No IL-tap experiment
was performed, so this control does not identify a particular C1/C2 mechanism.
