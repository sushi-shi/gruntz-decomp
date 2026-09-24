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

## Inline leaf and base controls

The follow-up at `d6cddd606` reviews authored header bodies that already carry
`RVA_COMPGEN` labels. A label can identify an emitted copy of an authored inline;
it does not certify that the source declaration is implicit.

| Omitted inline destructor | Retail RVA | Bytes preserved |
| --- | --- | ---: |
| `CAmbientPosSound` | 0xb940 | 15 |
| `CRandomAmbientSound` | 0xbb40 | 15 |
| `CMovingLogic` | 0x13bd0 | 68 |
| `CUniformTileImageSet` | 0x161370 | 7 |
| `CRectTileImageSet` | 0x161460 | 7 |
| `CWwdGridIter` | 0x163a10 | 7 |
| `CWwdGridShell` | 0x1682a0 | 70 |

All seven actual owner objects preserve the complete bodies and ordered
relocations. The final retail audit also resolves EH registration stubs and
vtable targets. In the independent base control, omitting `CUserLogic`'s
destructor loses its vptr store before `zBitVec` cleanup: 68 bytes become 62.
Restore that authored inline. `CUserBase` and `CGruntzCommand` separately require
authored declarations because they introduce their virtual destructor slots.
`CMotionState`'s trivial fields cannot generate its retail one-byte EH destructor;
its authored empty destructor remains.

Nine empty constructor wrappers also add no source initialization and have no
alternate constructor overloads: `CAmbientPosSound`, `CRandomAmbientSound`,
`CDemo`, `CGruntzSingleCommand`, `CGruntzMultiCommand`, `CInputDevBase`, `CUserBase`,
`CWwdGridShell`, and `StreamVoiceFeeder`. Removing them leaves 431 code bodies
identical across their seven actual caller TUs, changes four unrelated bodies,
and adds or removes no emitted symbol. Every construction caller is unchanged;
implicit base/member construction supplies the same initialization. The broad
comparison and before/after objects live in
`build/audits/implicit-boundaries/empty-inline-ctors/`.

Do not apply this to an empty constructor with a parameterized sibling: an
implicit default constructor is then suppressed. The former `TypeKeyRec`
exception is reopened: an empty constructor can supply a
[static-initializer slot](crt-xc-table-is-the-static-initializer-census.md), but
that sufficient spelling does not prove an authored declaration. See canonical
lineage IDs `reassess-dhandler-implicit-startup` and
`reassess-dhandler-startup-owner` before drawing a lifetime conclusion.

These controls correct two older explanations. An authored empty destructor
does **not** invariably retain its own vptr store; the seven leaf controls
above have no such store in either form. The sound bodies are also real
destructors, disproving the former
[manual BaseInit model](vptr-stamp-void-init-not-ctor.md). Use the complete
lifetime and caller family to recover source identity.
